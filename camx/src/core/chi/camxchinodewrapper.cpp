////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodewrapper.cpp
/// @brief Chi node wrapper class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if !_WINDOWS
#include <cutils/properties.h>
#endif // _WINDOWS

#include "camxchi.h"
#include "camxchinodewrapper.h"
#include "camxcmdbuffermanager.h"
#include "camxsettingsmanager.h"
#include "camxhal3metadatautil.h"
#include "camxhwenvironment.h"
#include "camxpipeline.h"
#include "inttypes.h"
#include "camximagebuffer.h"
#include "camxthreadmanager.h"
#include "camxtuningdatamanager.h"
#include "chincsdefs.h"
#include "camxtitan17xcontext.h"
#include "camxtitan17xdefs.h"
#include "camxcsl.h"

// NOWHINE FILE CP036a: const_cast<> is prohibited

CAMX_NAMESPACE_BEGIN

CAMX_STATIC_ASSERT(ChiNodeFormatsMaxPlanes == FormatsMaxPlanes);
CAMX_STATIC_ASSERT(ChiNodeMaxProperties <= MaxProperties);

static const INT    FenceCompleteProcessSequenceId   = -2;                    ///< Fence complete sequence id
static const UINT32 MaximumChiNodeWrapperInputPorts  = 32;                    ///< The maximum input ports for Chi node wrapper
static const UINT32 MaximumChiNodeWrapperOutputPorts = 32;                    ///< The maximum output ports for Chi node wrapper
static const CHAR*  pMemcpyNodeName                  = "com.qti.node.memcpy"; ///< memcpy node name string

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiNodeWrapper* ChiNodeWrapper::Create(
    const NodeCreateInputData*   pCreateInputData,
    NodeCreateOutputData*        pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);
    UINT32 enableDummynode = 0;

#if !_WINDOWS
    CHAR   prop[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.camera.enabledummynode", prop, "0");
    enableDummynode = atoi(prop);
#endif // _WINDOWS

    CamxResult      result       = CamxResultSuccess;
    ChiNodeWrapper* pChiNode     = CAMX_NEW ChiNodeWrapper();
    if (NULL != pChiNode)
    {
        ExternalComponentInfo externalComponentInfo[1] = {{0}};

        if (1 == enableDummynode)
        {
            // NOWHINE CP036a: Const cast allowed for conversion to interfaces
            externalComponentInfo[0].pComponentName = const_cast<CHAR*>(pMemcpyNodeName);
        }
        else
        {
            externalComponentInfo[0].pComponentName = static_cast<CHAR*>(pCreateInputData->pNodeInfo->pNodeProperties->pValue);
        }

        result = HwEnvironment::GetInstance()->SearchExternalComponent(&externalComponentInfo[0], 1);
        if (CamxResultSuccess == result)
        {
            memcpy(&pChiNode->m_nodeCallbacks, &externalComponentInfo[0].nodeCallbacks, sizeof(ChiNodeCallbacks));

            pChiNode->m_numInputPort  = pCreateInputData->pNodeInfo->inputPorts.numPorts;
            pChiNode->m_numOutputPort = pCreateInputData->pNodeInfo->outputPorts.numPorts;

            CAMX_ASSERT(0 < pChiNode->m_numInputPort);
            CAMX_ASSERT(0 < pChiNode->m_numOutputPort);

            pChiNode->m_phInputBuffer   = pChiNode->AllocateChiBufferHandlePool(
                                            pChiNode->m_numInputPort * MaxRequestQueueDepth);
            pChiNode->m_phOutputBuffer  = pChiNode->AllocateChiBufferHandlePool(pChiNode->m_numOutputPort * MaxOutputBuffers);

            UINT32 size = (sizeof(CHINODEBYPASSINFO) * pChiNode->m_numOutputPort);
            pChiNode->m_pBypassData = static_cast<CHINODEBYPASSINFO*>(CAMX_CALLOC(size));
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Failed to load Chi interface for %s", externalComponentInfo[0].pComponentName);
        }

        pChiNode->m_pChiContext      = pCreateInputData->pChiContext;
        pChiNode->m_pRequestLock     = Mutex::Create("RequestLock");
        pChiNode->m_pRequestProgress = Condition::Create("RequestLockProgress");
        pChiNode->m_flushInProgress  = FALSE;

        for (UINT32 requestIndex = 0; requestIndex < MaxRequestQueueDepth; requestIndex++)
        {
            pChiNode->m_requestInProgress[requestIndex] = FALSE;
        }

        if ((NULL == pChiNode->m_phInputBuffer) || (NULL == pChiNode->m_phOutputBuffer))
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Failed to allocate memory");
            CAMX_DELETE pChiNode;
            pChiNode = NULL;
        }

    }

    return pChiNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNCacheOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNCacheOps(
    CHINODEBUFFERHANDLE hChiBuffer,
    BOOL                invalidata,
    BOOL                clean)
{
    CDKResult     result        = CDKResultEFailed;
    ChiImageList* pImageBuffers = static_cast<ChiImageList*>(hChiBuffer);
    INT32         hLastHandle   = 0;

    for (UINT i = 0; i < pImageBuffers->numberOfPlanes; i++)
    {
        if ((0 != pImageBuffers->handles[i]) && (hLastHandle != pImageBuffers->handles[i]))
        {
            CSLMemHandle hCSLHandle = static_cast<CSLMemHandle>(pImageBuffers->handles[i]);

            result = CSLBufferCacheOp(hCSLHandle, invalidata, clean);
            if ( result !=  CDKResultSuccess)
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Buffer cache op failed %s", Utils::CamxResultToString(result));
                break;
            }
            hLastHandle = pImageBuffers->handles[i];
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNGetMetadata(
    CHIMETADATAINFO* pMetadataInfo)
{
    CAMX_ASSERT(NULL != pMetadataInfo);
    CAMX_ASSERT(NULL != pMetadataInfo->chiSession);

    CDKResult result = CDKResultSuccess;
    if (pMetadataInfo->size <= sizeof(CHIMETADATAINFO))
    {
        ChiNodeWrapper* pNode   = static_cast<ChiNodeWrapper*>(pMetadataInfo->chiSession);
        result                  = pNode->GetMetadata(pMetadataInfo);
    }
    else
    {
        result = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupChi, "MetadataInfo size mismatch");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetMultiCamDynamicMetaByCamId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNGetMultiCamDynamicMetaByCamId(
    CHIMETADATAINFO* pMetadataInfo,
    UINT32           cameraId)
{
    CAMX_ASSERT(NULL != pMetadataInfo);
    CAMX_ASSERT(NULL != pMetadataInfo->chiSession);

    CDKResult result = CDKResultSuccess;
    if (pMetadataInfo->size <= sizeof(CHIMETADATAINFO))
    {
        ChiNodeWrapper* pNode   = static_cast<ChiNodeWrapper*>(pMetadataInfo->chiSession);
        result                  = pNode->GetMulticamDynamicMetaByCamId(pMetadataInfo, cameraId);
    }
    else
    {
        result = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupChi, "MedatadataInfo size mismatch");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetStaticMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNGetStaticMetadata(
    CHIMETADATAINFO* pMetadataInfo,
    UINT32           cameraId)
{
    CAMX_ASSERT(NULL != pMetadataInfo);
    CAMX_ASSERT(NULL != pMetadataInfo->chiSession);

    CDKResult result = CDKResultSuccess;
    if (pMetadataInfo->size <= sizeof(CHIMETADATAINFO))
    {
        ChiNodeWrapper* pNode = static_cast<ChiNodeWrapper*>(pMetadataInfo->chiSession);
        result = pNode->GetStaticMetadata(pMetadataInfo, cameraId);
    }
    else
    {
        result = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupChi, "MedatadataInfo size mismatch");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetSupportedPSMetadataList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNGetSupportedPSMetadataList(
    CHIMETADATAIDARRAY* pMetadataIdArray)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pMetadataIdArray)
    {
        if (pMetadataIdArray->size <= sizeof(CHIMETADATAIDARRAY))
        {
            if (NULL != pMetadataIdArray->chiSession)
            {
                ChiNodeWrapper* pNode   = static_cast<ChiNodeWrapper*>(pMetadataIdArray->chiSession);
                result                  = pNode->QuerySupportedPSMetadataList(pMetadataIdArray);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "NULL chiSession pointer");
                result = CDKResultEInvalidPointer;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "MetadataInfo size mismatch");
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "NULL pMetadataIdArray");
        result = CDKResultEInvalidPointer;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetPSMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNGetPSMetadata(
    CHIPSMETADATA* pMetadataInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pMetadataInfo)
    {
        if (pMetadataInfo->size <= sizeof(CHIMETADATAIDARRAY))
        {
            if (NULL != pMetadataInfo->chiSession)
            {
                ChiNodeWrapper* pNode   = static_cast<ChiNodeWrapper*>(pMetadataInfo->chiSession);
                result                  = pNode->GetPortMetadata(pMetadataInfo);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "NULL chiSession pointer");
                result = CDKResultEInvalidPointer;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "pMetadataInfo size mismatch");
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "NULL pMetadataInfo");
        result = CDKResultEInvalidPointer;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNSetPSMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNSetPSMetadata(
    CHIPSMETADATA* pMetadataInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pMetadataInfo)
    {
        if (pMetadataInfo->size <= sizeof(CHIMETADATAIDARRAY))
        {
            if (NULL != pMetadataInfo->chiSession)
            {
                ChiNodeWrapper* pNode   = static_cast<ChiNodeWrapper*>(pMetadataInfo->chiSession);
                result                  = pNode->SetPortMetadata(pMetadataInfo);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "NULL chiSession pointer");
                result = CDKResultEInvalidPointer;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "pMetadataInfo size mismatch");
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "NULL pMetadataInfo");
        result = CDKResultEInvalidPointer;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNPublishPSMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNPublishPSMetadata(
    UINT32                   metadataId,
    CHIPSMETADATABYPASSINFO* pBypassInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pBypassInfo)
    {
        if (pBypassInfo->size <= sizeof(CHIPSMETADATABYPASSINFO))
        {
            if (NULL != pBypassInfo->chiSession)
            {
                ChiNodeWrapper* pNode   = static_cast<ChiNodeWrapper*>(pBypassInfo->chiSession);
                result = pNode->PublishPortMetadata(metadataId, pBypassInfo);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "NULL chiSession pointer");
                result = CDKResultEInvalidPointer;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "pBypassInfo size mismatch actual %u expected %u",
                pBypassInfo->size, sizeof(CHIPSMETADATABYPASSINFO));
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Need Valid ByPassInfo to get chi handle");
        result = CDKResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNIsPSMetadataPublished
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiNodeWrapper::FNIsPSMetadataPublished(
    CHIPSTAGINFO* pPSTagInfo)
{
    BOOL bIsPSMetadataPublished = FALSE;

    if (NULL != pPSTagInfo)
    {
        ChiNodeWrapper* pNode  = static_cast<ChiNodeWrapper*>(pPSTagInfo->chiSession);
        bIsPSMetadataPublished = pNode->GetPipeline()->IsPSMPublished(pPSTagInfo->tag);
    }

    return bIsPSMetadataPublished;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNSetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNSetMetadata(
    CHIMETADATAINFO* pMetadataInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pMetadataInfo)
    {
        if (pMetadataInfo->size <= sizeof(CHIMETADATAIDARRAY))
        {
            if (NULL != pMetadataInfo->chiSession)
            {
                ChiNodeWrapper* pNode   = static_cast<ChiNodeWrapper*>(pMetadataInfo->chiSession);
                result                  = pNode->SetMetadata(pMetadataInfo);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "NULL chiSession pointer");
                result = CDKResultEInvalidPointer;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "pMetadataInfo size mismatch");
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "NULL pMetadataInfo");
        result = CDKResultEInvalidPointer;
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetTuningmanager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiNodeWrapper::FNGetTuningmanager(
    CHIHANDLE       hChiSession,
    CHIDATAREQUEST* pDataRequest)
{
    CAMX_ASSERT(NULL != hChiSession);

    TuningDataManager* pTuningManager     = NULL;
    TuningSetManager*  pTuningDataManager = NULL;

    ChiNodeWrapper* pNode = static_cast<ChiNodeWrapper*>(hChiSession);
    if (NULL != pNode)
    {
        if (NULL != pDataRequest)
        {
            // dataRequestIndex in the camera ID
            pTuningManager = pNode->GetTuningDataManagerWithCameraId(pDataRequest->index);
        }
        else
        {
            pTuningManager = pNode->GetTuningDataManager();
        }

        if ((NULL != pTuningManager) &&
            (TRUE == pTuningManager->IsValidChromatix()))
        {
            pTuningDataManager = static_cast<TuningSetManager*>(pTuningManager->GetChromatix());
        }
    }
    return pTuningDataManager;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetVendorTagBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNGetVendorTagBase(
    CHIVENDORTAGBASEINFO* pVendorTagBaseInfo)
{
    CAMX_ASSERT(NULL != pVendorTagBaseInfo);
    CDKResult result = CDKResultSuccess;
    if (pVendorTagBaseInfo->size <= sizeof(CHIVENDORTAGBASEINFO))
    {
        UINT32 sectionBase      = 0;
        CamxResult resultCamx   = CamxResultSuccess;
        if (NULL == pVendorTagBaseInfo->pTagName)
        {
            resultCamx = VendorTagManager::QueryVendorTagSectionBase(pVendorTagBaseInfo->pComponentName, &sectionBase);
        }
        else
        {
            resultCamx = VendorTagManager::QueryVendorTagLocation(pVendorTagBaseInfo->pComponentName,
                                                                  pVendorTagBaseInfo->pTagName, &sectionBase);
        }

        if (CamxResultSuccess != resultCamx)
        {
            result = CDKResultEFailed;
        }
        else
        {
            pVendorTagBaseInfo->vendorTagBase = sectionBase;
        }
    }
    else
    {
        result = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupChi, "CHIVENDORTAGBASEINFO size mismatch");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNProcRequestDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNProcRequestDone(
    CHINODEPROCESSREQUESTDONEINFO* pInfo)
{
    CAMX_ASSERT(NULL != pInfo);
    CAMX_ASSERT(NULL != pInfo->hChiSession);

    CDKResult result = CDKResultSuccess;
    if (pInfo->size <= sizeof(CHINODEPROCESSREQUESTDONEINFO))
    {
        ChiNodeWrapper* pNode   = static_cast<ChiNodeWrapper*>(pInfo->hChiSession);
        result                  = pNode->ProcRequestDone(pInfo);
    }
    else
    {
        result = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupChi, "RequestDone Info size mismatch");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNProcMetadataDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNProcMetadataDone(
    CHINODEPROCESSMETADATADONEINFO* pInfo)
{
    CAMX_ASSERT(NULL != pInfo);
    CAMX_ASSERT(NULL != pInfo->hChiSession);

    CDKResult result = CDKResultSuccess;
    if (pInfo->size <= sizeof(CHINODEPROCESSMETADATADONEINFO))
    {
        ChiNodeWrapper* pNode = static_cast<ChiNodeWrapper*>(pInfo->hChiSession);
        result                = pNode->ProcMetadataDone(pInfo);
    }
    else
    {
        result = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupChi, "MetadataDone Info size mismatch");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNCreateFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNCreateFence(
    CHIHANDLE               hChiSession,
    CHIFENCECREATEPARAMS*   pInfo,
    CHIFENCEHANDLE*         phChiFence)
{
    CAMX_ASSERT(NULL != pInfo);
    CAMX_ASSERT(NULL != hChiSession);

    CDKResult result = CDKResultSuccess;
    if (NULL == phChiFence)
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "phChiFence is NULL");
        result = CDKResultEInvalidArg;
    }
    else if (pInfo->size > sizeof(CHIFENCECREATEPARAMS))
    {
        result = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupChi, "CreateFence Info size mismatch");
    }
    else
    {
        ChiNodeWrapper* pNode   = static_cast<ChiNodeWrapper*>(hChiSession);
        // do we need to convert result?
        result = pNode->GetChiContext()->CreateChiFence(pInfo, phChiFence);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNReleaseFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNReleaseFence(
    CHIHANDLE       hChiSession,
    CHIFENCEHANDLE  hChiFence)
{
    CAMX_ASSERT(NULL != hChiSession);

    CDKResult   result  = CDKResultSuccess;
    ChiFence*   pChiFence   = static_cast<ChiFence*>(hChiFence);
    if (NULL != pChiFence)
    {
        ChiNodeWrapper* pNode   = static_cast<ChiNodeWrapper*>(hChiSession);
        result = pNode->GetChiContext()->ReleaseChiFence(hChiFence);
    }
    else
    {
        result = CDKResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupChi, "hChiFence is Invalid");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetDataSource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNGetDataSource(
    CHIHANDLE            hChiSession,
    CHIDATASOURCE*       pDataSource,
    CHIDATASOURCECONFIG* pDataSourceConfig)
{
    CAMX_UNREFERENCED_PARAM(hChiSession);

    CamxResult              result               = CDKResultSuccess;
    CHINCSDATASOURCECONFIG* pNCSDataSourceConfig = NULL;
    NCSSensorConfig         lNCSConfig           = { 0 };
    BOOL                    isSensorValid        = TRUE;

    if ((NULL != pDataSource) && (NULL != pDataSourceConfig))
    {
        switch (pDataSourceConfig->sourceType)
        {
            case ChiDataGyro:
                pNCSDataSourceConfig = reinterpret_cast<CHINCSDATASOURCECONFIG*>(pDataSourceConfig->pConfig);
                lNCSConfig.sensorType = NCSGyroType;
                break;
            case ChiDataAccel:
                pNCSDataSourceConfig = reinterpret_cast<CHINCSDATASOURCECONFIG*>(pDataSourceConfig->pConfig);
                lNCSConfig.sensorType = NCSAccelerometerType;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupCore, "Unsupported data source");
                result = CDKResultEUnsupported;
                lNCSConfig.sensorType = NCSMaxType;
                isSensorValid = FALSE;
                break;
        }

        if (TRUE == isSensorValid)
        {
            NCSSensor* pSensorObject = NULL;

            CAMX_ASSERT_MESSAGE(pNCSDataSourceConfig->size == sizeof(CHINCSDATASOURCECONFIG),
                                "Data Source config structure of invalid size !!");

            lNCSConfig.operationMode = pNCSDataSourceConfig->operationMode;
            lNCSConfig.reportRate    = pNCSDataSourceConfig->reportRate;
            lNCSConfig.samplingRate  = pNCSDataSourceConfig->samplingRate;

            pSensorObject = SetupNCSLink(&lNCSConfig);
            if (NULL != pSensorObject)
            {
                pDataSource->dataSourceType = pDataSourceConfig->sourceType;
                pDataSource->pHandle        = static_cast<VOID*>(pSensorObject);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Setup NCS link failed");
                result               = CDKResultEFailed;
                pSensorObject        = NULL;
                pDataSource->pHandle = NULL;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid pointer to the data source pointer!!");
        result               = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiNodeWrapper::FNGetData(
    CHIHANDLE            hChiSession,
    CHIDATASOURCEHANDLE  hDataSourceHandle,
    CHIDATAREQUEST*      pDataRequest,
    UINT*                pSize)
{
    VOID* pData = NULL;

    CAMX_ASSERT_MESSAGE((NULL != hChiSession) && (NULL != hDataSourceHandle) && (NULL != pDataRequest),
                        "Invalid input params");

    switch (hDataSourceHandle->dataSourceType)
    {
        case ChiDataGyro:
        case ChiDataAccel:
            pData = FNGetDataNCS(hChiSession, hDataSourceHandle, pDataRequest, pSize);
            break;
        case ChiTuningManager:
            pData = FNGetTuningmanager(hChiSession, pDataRequest);
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupChi, "Unsupported data source type");
            break;
    }

    return pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNPutData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNPutData(
    CHIHANDLE            hChiSession,
    CHIDATASOURCEHANDLE  hDataSourceHandle,
    CHIDATAHANDLE        hData)
{
    CamxResult result        = CamxResultSuccess;
    NCSSensor* pSensorObject = NULL;

    CAMX_ASSERT_MESSAGE((NULL != hChiSession) && (NULL != hDataSourceHandle),
                        "Invalid input params");

    if ((NULL != hDataSourceHandle->pHandle) && (NULL != hData))
    {
        switch (hDataSourceHandle->dataSourceType)
        {
            case ChiDataGyro:
            case ChiDataAccel:
                pSensorObject = static_cast<NCSSensor*>(hDataSourceHandle->pHandle);
                result = pSensorObject->PutBackDataObj(
                    static_cast<NCSSensorData*>(hData));
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupChi, "Unsupported data source type");
                result = CamxResultEUnsupported;
                break;
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid accessor object");
    }

    return CamxResultToCDKResult(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetDataNCS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiNodeWrapper::FNGetDataNCS(
    CHIHANDLE            hChiSession,
    CHIDATASOURCEHANDLE  hDataSourceHandle,
    CHIDATAREQUEST*      pDataRequest,
    UINT*                pSize)
{
    NCSSensor*      pSensorObject      = NULL;
    NCSSensorData*  pSensorDataObject  = NULL;
    ChiNodeWrapper* pNode              = NULL;
    VOID*           pData              = NULL;
    ChiFence*       pChiFence          = NULL;
    CamxResult      result             = CamxResultSuccess;
    ChiNCSDataRequest* pNCSDataRequest = NULL;

    pNode = static_cast<ChiNodeWrapper*>(hChiSession);

    if (NULL != pDataRequest)
    {
        pNCSDataRequest = reinterpret_cast<ChiNCSDataRequest*>(pDataRequest->hRequestPd);

        switch (pDataRequest->requestType)
        {
            case ChiFetchData:
                pSensorObject = static_cast<NCSSensor*>(hDataSourceHandle->pHandle);
                if ((NULL != pNCSDataRequest) && (pNCSDataRequest->size == sizeof(ChiNCSDataRequest)))
                {
                    if (NULL != pSensorObject)
                    {
                        /// Just get N Recent samples
                        if ( 0 < pNCSDataRequest->numSamples)
                        {
                            pData = pSensorObject->GetLastNSamples(pNCSDataRequest->numSamples);
                        }
                        /// ASynchrounous windowed request
                        else if (NULL != pNCSDataRequest->hChiFence)
                        {
                            pChiFence = static_cast<ChiFence*>(pNCSDataRequest->hChiFence);
                            result = pSensorObject->GetDataAsync(
                                pNCSDataRequest->windowRequest.tStart,
                                pNCSDataRequest->windowRequest.tEnd,
                                pChiFence);
                            pNCSDataRequest->result = CamxResultToCDKResult(result);
                        }
                        /// Synchrounous windowed request
                        else
                        {
                            pData = pSensorObject->GetDataSync(
                                pNCSDataRequest->windowRequest.tStart,
                                pNCSDataRequest->windowRequest.tEnd);
                        }

                        // Populate size if pData is valid
                        if ((NULL != pSize) && (NULL != pData))
                        {
                            *pSize = static_cast<NCSSensorData*>(pData)->GetNumSamples();
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid sensor object");
                        pData = NULL;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid request payload");
                    pData = NULL;
                }
                break;

            case ChiIterateData:
                pSensorDataObject = static_cast<NCSSensorData*>(hDataSourceHandle->pHandle);
                if (NULL != pSensorDataObject)
                {
                    if (0 <= pDataRequest->index)
                    {
                        pSensorDataObject->SetIndex(pDataRequest->index);
                        pData = pSensorDataObject->GetCurrent();
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid index to iterate");
                        pData = NULL;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Sensor object is NULL");
                    pData = NULL;
                }
                break;
            default:
                break;
        }

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid input request pointer");
    }

    return pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNPutDataSource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNPutDataSource(
    CHIHANDLE           hChiSession,
    CHIDATASOURCEHANDLE hDataSourceHandle)
{
    CDKResult          result     = CDKResultSuccess;

    CAMX_ASSERT_MESSAGE((NULL != hChiSession) || (NULL != hDataSourceHandle),
                        "Invalid input params");

    CAMX_LOG_VERBOSE(CamxLogGroupChi, "Unregister");

    switch (hDataSourceHandle->dataSourceType)
    {
        case ChiDataGyro:
        case ChiDataAccel:
            result = CamxResultToCDKResult(DestroyNCSLink(hDataSourceHandle->pHandle));
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupChi, "Unsupported data source type");
            result = CDKResultSuccess;
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNWaitFenceAsync
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNWaitFenceAsync(
    CHIHANDLE               hChiSession,
    PFNCHIFENCECALLBACK     pCallbackFn,
    CHIFENCEHANDLE          hChiFence,
    VOID*                   pData)
{
    CamxResult result = CamxResultEFailed;

    CAMX_ASSERT((NULL != hChiSession) && (NULL != pData) && (NULL != hChiFence) && (NULL != pCallbackFn));

    ChiNodeWrapper* pNode = static_cast<ChiNodeWrapper*>(hChiSession);

    result = pNode->GetChiContext()->WaitChiFence(hChiFence, pCallbackFn, pData, UINT64_MAX);
    if (CamxResultSuccess != result)
    {
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNSignalFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNSignalFence(
    CHIHANDLE               hChiSession,
    CHIFENCEHANDLE          hChiFence,
    CDKResult               statusResult)
{
    CamxResult result = CamxResultEFailed;

    CAMX_ASSERT((NULL != hChiSession) && (NULL != hChiFence));

    ChiNodeWrapper* pNode = static_cast<ChiNodeWrapper*>(hChiSession);

    result = pNode->GetChiContext()->SignalChiFence(hChiFence,
                                                    CDKResultToCamxResult(statusResult));
    if (CamxResultSuccess != result)
    {
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetFenceStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNGetFenceStatus(
    CHIHANDLE               hChiSession,
    CHIFENCEHANDLE          hChiFence,
    CDKResult*              pFenceResult)
{
    CDKResult     result     = CDKResultEFailed;
    CDKResult     lResult;


    CAMX_ASSERT((NULL != hChiSession) && (NULL != hChiFence) && (NULL != pFenceResult));

    ChiNodeWrapper* pNode = static_cast<ChiNodeWrapper*>(hChiSession);

    result = pNode->GetChiContext()->GetChiFenceResult(hChiFence, &lResult);
    if (CamxResultSuccess != result)
    {
        result        = CDKResultEFailed;
        *pFenceResult = CDKResultEInvalidState;
    }
    else
    {
        if (CDKResultSuccess == lResult)
        {
            *pFenceResult = CDKResultSuccess;
        }
        else if (CDKResultEFailed == lResult)
        {
            *pFenceResult = CDKResultEFailed;
        }
        else
        {
            *pFenceResult = CDKResultEInvalidState;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::SetCSLHwInfoAndAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::SetCSLHwInfoAndAcquire(
    CHICSLHWINFO* pCSLInfo)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCSLInfo)
    {
        m_CSLHwInfo.requireCSLAccess     = pCSLInfo->requireCSLAccess;
        m_CSLHwInfo.requireOutputBuffers = pCSLInfo->requireOutputBuffers;
        m_CSLHwInfo.requireScratchBuffer = pCSLInfo->requireScratchBuffer;

        if (TRUE == m_CSLHwInfo.requireCSLAccess)
        {
            INT32          deviceIndex           = -1;
            UINT           indicesLengthRequired = 0;
            HwEnvironment* pHwEnvironment        = HwEnvironment::GetInstance();

            m_CSLHwInfo.CSLHwResourceID = pCSLInfo->CSLHwResourceID;

            result = pHwEnvironment->GetDeviceIndices(CSLDeviceTypeCustom,
                                                      &deviceIndex,
                                                      1,
                                                      &indicesLengthRequired);

            if ((CamxResultSuccess == result) && (-1 != deviceIndex))
            {
                AddDeviceIndex(deviceIndex);
                result = CreateCmdAndScratchBuffers(pCSLInfo);

                if (CamxResultSuccess == result)
                {
                    result = AcquireDevice(pCSLInfo);

                    if (CamxResultSuccess == result)
                    {
                        result = SetupAndCommitHWData(NULL, FirstValidRequestId, ChiCSLHwPacketInit);

                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Error: %s in Processing Init Packet",
                                NodeIdentifierString(), Utils::CamxResultToString(result));
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s CSL Acquire Fail result: %s",
                            NodeIdentifierString(), Utils::CamxResultToString(result));
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Create Buffer managers Fails result: %s",
                        NodeIdentifierString(), Utils::CamxResultToString(result));
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNBufferManagerCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIBUFFERMANAGERHANDLE ChiNodeWrapper::FNBufferManagerCreate(
    const CHAR*                     pBufferManagerName,
    CHINodeBufferManagerCreateData* pCreateData)
{

    CHIHALSTREAM               dummyHalStream;
    CHIBufferManagerCreateData createData;
    CHIBUFFERMANAGERHANDLE     hBufferManager = NULL;
    ChiStream                  dummyStream    = {};

    dummyStream.dataspace         = DataspaceUnknown;
    dummyStream.rotation          = ChiStreamRotation::StreamRotationCCW0;
    dummyStream.streamType        = ChiStreamTypeBidirectional;
    dummyStream.format            = static_cast<CHISTREAMFORMAT>(pCreateData->format);
    dummyStream.grallocUsage      = pCreateData->consumerFlags | pCreateData->producerFlags;
    dummyStream.width             = pCreateData->width;
    dummyStream.height            = pCreateData->height;
    dummyStream.maxNumBuffers     = pCreateData->maxBufferCount;
    dummyStream.pHalStream        = &dummyHalStream;

    dummyHalStream.overrideFormat = dummyStream.format;
    dummyHalStream.maxBuffers     = dummyStream.maxNumBuffers;
    dummyHalStream.consumerUsage  = pCreateData->consumerFlags;
    dummyHalStream.producerUsage  = pCreateData->producerFlags;


    createData.width                 = pCreateData->width;
    createData.height                = pCreateData->height;
    createData.format                = pCreateData->format;
    createData.producerFlags         = pCreateData->producerFlags;
    createData.consumerFlags         = pCreateData->consumerFlags;
    createData.bufferStride          = pCreateData->bufferStride;
    createData.maxBufferCount        = pCreateData->maxBufferCount;
    createData.immediateBufferCount  = pCreateData->immediateBufferCount;
    createData.bEnableLateBinding    = pCreateData->bEnableLateBinding;
    createData.bufferHeap            = pCreateData->bufferHeap;
    createData.pChiStream            = &dummyStream;

    hBufferManager = ChiBufferManagerCreate(pBufferManagerName, &createData);

    return hBufferManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNBufferManagerDestroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeWrapper::FNBufferManagerDestroy(
    CHIBUFFERMANAGERHANDLE hBufferManager)
{
    if (NULL != hBufferManager)
    {
        ImageBufferManager* pBufferManager = static_cast<ImageBufferManager*>(hBufferManager);
        pBufferManager->Destroy();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! hBufferManager is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNBufferManagerGetImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHINODEBUFFERHANDLE ChiNodeWrapper::FNBufferManagerGetImageBuffer(
    CHIBUFFERMANAGERHANDLE hBufferManager)
{
    ChiNodeBufferHandleWrapper* pBufferHandleWrapper = NULL;

    if (NULL != hBufferManager)
    {
        ImageBufferManager* pBufferManager = static_cast<ImageBufferManager*>(hBufferManager);

        pBufferHandleWrapper = reinterpret_cast<ChiNodeBufferHandleWrapper*>(CAMX_CALLOC(sizeof(ChiNodeBufferHandleWrapper)));

        if (NULL != pBufferHandleWrapper)
        {
            pBufferHandleWrapper->pImageBuffer = pBufferManager->GetImageBuffer();
            pBufferHandleWrapper->canary       = CANARY;
            if (pBufferHandleWrapper->pImageBuffer != NULL)
            {
                ImageBufferToChiBuffer(pBufferHandleWrapper->pImageBuffer, &pBufferHandleWrapper->hBuffer, TRUE);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "pImageBuffer is Null");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Out of memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input! hBufferManager is NULL");
    }

    return static_cast<CHINODEBUFFERHANDLE>(&pBufferHandleWrapper->hBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNBufferManagerReleaseReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNBufferManagerReleaseReference(
    CHIBUFFERMANAGERHANDLE hBufferManager,
    CHINODEBUFFERHANDLE    hNodeBufferHandle)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == hBufferManager) || (NULL == hNodeBufferHandle))
    {
        result = CDKResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid input! hBufferManager or hNodeBufferHandle is NULL");
    }
    else
    {
        ImageBufferManager*         pBufferManager       = static_cast<ImageBufferManager*>(hBufferManager);
        ChiNodeBufferHandleWrapper* pBufferHandleWrapper = reinterpret_cast<ChiNodeBufferHandleWrapper*>(hNodeBufferHandle);
        CAMX_ASSERT(CANARY == pBufferHandleWrapper->canary);

        if (0 == pBufferManager->ReleaseReference(pBufferHandleWrapper->pImageBuffer))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupChi, "Freeing wrapper");
            CAMX_FREE(pBufferHandleWrapper);
        }

    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FNGetFlushInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::FNGetFlushInfo(
    CHIHANDLE     hChiSession,
    CHIFLUSHINFO* pFlushInfo)
{
    CDKResult result = CDKResultEFailed;

    if ((NULL != hChiSession) && (NULL != pFlushInfo))
    {
        ChiNodeWrapper* pNode = static_cast<ChiNodeWrapper*>(hChiSession);
        FlushInfo flushInfo;

        pNode->GetFlushInfo(flushInfo);

        pFlushInfo->lastFlushedRequestId    = flushInfo.lastFlushRequestId;
        pFlushInfo->flushInProgress         = pNode->GetPipeline()->GetFlushStatus();
        result                              = CDKResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::GetMulticamDynamicMetaByCamId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::GetMulticamDynamicMetaByCamId(
    CHIMETADATAINFO* pMetadataInfo,
    UINT32           cameraId)
{
    return GetDynamicMetadata(pMetadataInfo, cameraId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::GetDynamicMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::GetDynamicMetadata(
    CHIMETADATAINFO* pMetadataInfo,
    UINT32           cameraId)
{
    CDKResult result = CDKResultSuccess;
    CAMX_ASSERT(NULL != pMetadataInfo);

    if (pMetadataInfo->metadataType == ChiMetadataDynamic)
    {
        UINT64* pOffset = static_cast<UINT64 *>(CAMX_CALLOC(
                sizeof(UINT64) * pMetadataInfo->tagNum));
        VOID** ppData = static_cast<VOID **>(CAMX_CALLOC(
                sizeof(VOID *) * pMetadataInfo->tagNum));
        BOOL* pNegate = static_cast<BOOL *>(CAMX_CALLOC(
                sizeof(BOOL) * pMetadataInfo->tagNum));

        if (NULL == pOffset || NULL == ppData || NULL == pNegate)
        {
            result = CDKResultENoMemory;
        }

        if (CDKResultSuccess == result)
        {
            for (UINT32 i = 0; i < pMetadataInfo->tagNum; i++)
            {
                INT64 requestTemp =
                        static_cast<INT64>(GetCurrentRequest())
                                - static_cast<INT64>(pMetadataInfo->pTagData[i].requestId);

                if (pMetadataInfo->pTagData[i].requestId
                        <= GetCurrentRequest())
                {
                    pOffset[i] = static_cast<UINT64>(requestTemp);
                    pNegate[i] = FALSE;
                }
                else
                {
                    pOffset[i] = static_cast<UINT64>(-1 * requestTemp);
                    pNegate[i] = TRUE;
                }
            }

            GetDataListFromPipelineByCameraId(pMetadataInfo->pTagList, ppData, pOffset,
                    pMetadataInfo->tagNum, pNegate,
                    GetPipeline()->GetPipelineId(),
                    cameraId);

            for (UINT32 i = 0; i < pMetadataInfo->tagNum; i++)
            {
                pMetadataInfo->pTagData[i].pData = ppData[i];
            }
        }

        if (NULL != ppData)
        {
            CAMX_FREE(ppData);
            ppData = NULL;
        }

        if (NULL != pOffset)
        {
            CAMX_FREE(pOffset);
            pOffset = NULL;
        }

        if (NULL != pNegate)
        {
            CAMX_FREE(pNegate);
            pNegate = NULL;
        }
    } else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Unsupported metadatatype %d",
            NodeIdentifierString(), pMetadataInfo->metadataType);

        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::GetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::GetMetadata(
    CHIMETADATAINFO* pMetadataInfo)
{
    CDKResult result  = CDKResultSuccess;
    if (NULL != pMetadataInfo)
    {
        if (pMetadataInfo->metadataType == ChiMetadataDynamic)
        {
            result = GetDynamicMetadata(pMetadataInfo, InvalidCameraId);
        }
        else if (pMetadataInfo->metadataType == ChiMetadataStatic)
        {
            UINT32  tagNum = pMetadataInfo->tagNum;
            for (UINT32 idx = 0; idx < tagNum; idx++)
            {
                CHITAGDATA* pTagData = &pMetadataInfo->pTagData[idx];
                HAL3MetadataUtil::GetMetadata(const_cast<Metadata *>(m_pStaticMetadata),
                                              pMetadataInfo->pTagList[idx],
                                              &pTagData->pData);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Unsupported metadatatype %d", pMetadataInfo->metadataType);
            result = CDKResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "NULL pMetadataInfo");
        result = CDKResultEInvalidPointer;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::GetStaticMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::GetStaticMetadata(
    CHIMETADATAINFO* pMetadataInfo,
    UINT32           cameraId)
{
    CDKResult result = CDKResultSuccess;
    CAMX_ASSERT(NULL != pMetadataInfo);

    if (CDKResultSuccess == result)
    {
        MetadataSlot* pSlot = NULL;
        pSlot = m_pChiContext->GetStaticMetadataPool(cameraId)->GetSlot(0);

        UINT32  tagNum = pMetadataInfo->tagNum;
        for (UINT32 i = 0; i < tagNum; i++)
        {
            CHITAGDATA* pTagData = &pMetadataInfo->pTagData[i];
            pTagData->pData = pSlot->GetMetadataByTag(pMetadataInfo->pTagList[i] & ~DriverInternalGroupMask,
                                                      NodeIdentifierString());
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::SetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::SetMetadata(
    CHIMETADATAINFO* pMetadataInfo)
{
    UINT32 tagNum       = pMetadataInfo->tagNum;
    CDKResult result    = CDKResultSuccess;

    UINT*         pDataSize = static_cast<UINT *>(CAMX_CALLOC(sizeof(UINT) * tagNum));
    const VOID**  ppData    = static_cast<const VOID **>(CAMX_CALLOC(sizeof(VOID *) * tagNum));

    if (NULL == pDataSize|| NULL == ppData)
    {
        result = CDKResultENoMemory;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < pMetadataInfo->tagNum; i++)
        {
            ppData[i]    = pMetadataInfo->pTagData[i].pData;
            pDataSize[i] = static_cast<UINT>(pMetadataInfo->pTagData[i].dataSize);
        }

        WriteDataList(pMetadataInfo->pTagList, ppData, pDataSize, tagNum);
    }

    if (NULL != ppData)
    {
        CAMX_FREE(ppData);
        ppData = NULL;
    }

    if (NULL != pDataSize)
    {
        CAMX_FREE(pDataSize);
        pDataSize = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::GetPortMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::GetPortMetadata(
    CHIPSMETADATA* pMetadataInfo)
{
    CDKResult result  = CDKResultSuccess;

    UINT32  tagNum = pMetadataInfo->tagCount;

    for (UINT32 idx = 0; idx < tagNum; idx++)
    {
        CHITAGDATA* pTagData = &pMetadataInfo->pTagData[idx];

        result = GetPSMetadata(pMetadataInfo->portId,
                               pMetadataInfo->pTagList[idx],
                               &pTagData->pData,
                               pTagData->offset);

        if (CDKResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s GetPSMetadata Failed", NodeIdentifierString());
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::SetPortMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::SetPortMetadata(
    CHIPSMETADATA* pMetadataInfo)
{
    UINT32 tagNum       = pMetadataInfo->tagCount;
    CDKResult result    = CDKResultSuccess;

    UINT*         pDataSize = static_cast<UINT *>(CAMX_CALLOC(sizeof(UINT) * tagNum));
    const VOID**  ppData    = static_cast<const VOID **>(CAMX_CALLOC(sizeof(VOID *) * tagNum));

    if (NULL == pDataSize|| NULL == ppData)
    {
        result = CDKResultENoMemory;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 idx = 0; idx < tagNum; idx++)
        {
            ppData[idx]    = pMetadataInfo->pTagData[idx].pData;
            pDataSize[idx] = static_cast<UINT>(pMetadataInfo->pTagData[idx].dataSize);
        }

        result = WritePSDataList(pMetadataInfo->portId,
                                 pMetadataInfo->pTagList,
                                 ppData,
                                 pDataSize,
                                 tagNum);

        if (CDKResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s SetPSMetadata Failed", NodeIdentifierString());
        }
    }

    if (NULL != ppData)
    {
        CAMX_FREE(ppData);
        ppData = NULL;
    }

    if (NULL != pDataSize)
    {
        CAMX_FREE(pDataSize);
        pDataSize = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::PublishPortMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::PublishPortMetadata(
    UINT32                   tagId,
    CHIPSMETADATABYPASSINFO* pBypassInfo)
{
    CDKResult result;

    result = PublishPSData(tagId, pBypassInfo);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::GetVendorTagBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::GetVendorTagBase(
    CHIVENDORTAGBASEINFO* pVendorTagBaseInfo)
{
    CAMX_UNREFERENCED_PARAM(pVendorTagBaseInfo);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ProcRequestDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::ProcRequestDone(
    CHINODEPROCESSREQUESTDONEINFO* pInfo)
{
    CAMX_UNREFERENCED_PARAM(pInfo);

    UINT32 requestIdIndex = (pInfo->frameNum % MaxRequestQueueDepth);

    for (UINT i = 0; i < m_perRequestData[requestIdIndex].numFences; i++)
    {
        if (NULL != m_perRequestData[requestIdIndex].phFence[i])
        {
            CSLFenceSignal(*m_perRequestData[requestIdIndex].phFence[i], CSLFenceResultSuccess);
        }
    }

    if (FALSE == pInfo->isEarlyMetadataDone)
    {
        ProcessPartialMetadataDone(pInfo->frameNum);
        ProcessMetadataDone(pInfo->frameNum);
    }

    if (TRUE == IsSinkNoBufferNode())
    {
        ProcessRequestIdDone(pInfo->frameNum);
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ProcMetadataDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::ProcMetadataDone(
    CHINODEPROCESSMETADATADONEINFO* pInfo)
{
    ProcessPartialMetadataDone(pInfo->frameNum);
    ProcessMetadataDone(pInfo->frameNum);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData*  pFinalizeInitializationData)
{
    CAMX_UNREFERENCED_PARAM(pFinalizeInitializationData);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::GetChiNodeCapsMask
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiNodeWrapper::GetChiNodeCapsMask(
    const NodeCreateInputData*  pCreateInputData,
    CHINODECREATEINFO*          pNodeCreateInfo)
{
    UINT32 propertyCount = pCreateInputData->pNodeInfo->nodePropertyCount;
    CDKResult result    = CDKResultSuccess;

    if (0 == OsUtils::StrCmp(m_pComponentName, "com.qti.node.gpu"))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s nodePropertyCount %d",
            NodeIdentifierString(), pCreateInputData->pNodeInfo->nodePropertyCount);

        for (UINT32 count = 0; count < propertyCount; count++)
        {
            if (NodePropertyGPUCapsMaskType == pCreateInputData->pNodeInfo->pNodeProperties[count].id)
            {
                if (NULL != pCreateInputData->pNodeInfo->pNodeProperties[count].pValue)
                {
                    m_instancePropertyCapsMask = static_cast<UINT>(
                                                 atoi(static_cast<const CHAR*>(
                                                 pCreateInputData->pNodeInfo->pNodeProperties[count].pValue)));

                    CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s m_instancePropertyCapsMask 0x%x",
                        NodeIdentifierString(), m_instancePropertyCapsMask);

                    switch (m_instancePropertyCapsMask)
                    {
                        case ChiNodeCapsScale:
                        case ChiNodeCapsGpuMemcpy:
                        case ChiNodeCapsGPUGrayscale:
                        case ChiNodeCapsGPURotate:
                        case ChiNodeCapsGPUDownscale:
                        case ChiNodeCapsGPUFlip:
                        case ChiNodeCapsGPUSkipProcessing:
                        case ChiNodeCapsGPUSkipProcessing + ChiNodeCapsGPUEnableMapping + ChiNodeCapsGPUDownscale:
                        case ChiNodeCapsGPUEnableMapping:
                            pNodeCreateInfo->nodeCaps.nodeCapsMask += m_instancePropertyCapsMask;
                            break;
                        default:
                            pNodeCreateInfo->nodeCaps.nodeCapsMask += ChiNodeCapsGpuMemcpy;
                            break;
                    }
                }
                else
                {
                    result = CamxResultEInvalidPointer;
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s NodeProperties pValue is NUll", NodeIdentifierString());
                }
            }
        }
        CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s GPU node capsMask is 0x%x",
            NodeIdentifierString(), pNodeCreateInfo->nodeCaps.nodeCapsMask);
    }
    if (0 == OsUtils::StrCmp(m_pComponentName, "com.qti.node.dewarp"))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s nodePropertyCount %d",
            NodeIdentifierString(), pCreateInputData->pNodeInfo->nodePropertyCount);

        for (UINT32 count = 0; count < propertyCount; count++)
        {
            if (NodePropertyGPUCapsMaskType == pCreateInputData->pNodeInfo->pNodeProperties[count].id)
            {
                if (NULL != pCreateInputData->pNodeInfo->pNodeProperties[count].pValue)
                {
                    m_instancePropertyCapsMask = static_cast<UINT>(
                        atoi(static_cast<const CHAR*>(
                            pCreateInputData->pNodeInfo->pNodeProperties[count].pValue)));
                }
                else
                {
                    result = CamxResultEInvalidPointer;
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s NodeProperties pValue is NUll", NodeIdentifierString());
                }
            }
        }

        switch (m_instancePropertyCapsMask)
        {
            case ChiNodeCapsDewarpEISV2:
            case ChiNodeCapsDewarpEISV3:
                pNodeCreateInfo->nodeCaps.nodeCapsMask = m_instancePropertyCapsMask;
                break;
            default:
                pNodeCreateInfo->nodeCaps.nodeCapsMask = ChiNodeCapsDewarpEISV2;
                break;
        }
        CAMX_LOG_INFO(CamxLogGroupCore, "Node::%s Dewarp node capsMask is 0x%x",
            NodeIdentifierString(), pNodeCreateInfo->nodeCaps.nodeCapsMask);
    }

    if (0 == OsUtils::StrCmp(m_pComponentName, "com.qti.node.fcv"))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s nodePropertyCount %d",
            NodeIdentifierString(), pCreateInputData->pNodeInfo->nodePropertyCount);

        for (UINT32 count = 0; count < propertyCount; count++)
        {
            if (NodePropertyGPUCapsMaskType == pCreateInputData->pNodeInfo->pNodeProperties[count].id)
            {
                if (NULL != pCreateInputData->pNodeInfo->pNodeProperties[count].pValue)
                {
                    m_instancePropertyCapsMask = static_cast<UINT>(
                        atoi(static_cast<const CHAR*>(
                            pCreateInputData->pNodeInfo->pNodeProperties[count].pValue)));
                }
                else
                {
                    result = CamxResultEInvalidPointer;
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s NodeProperties pValue is NUll", NodeIdentifierString());
                }
            }
        }

        pNodeCreateInfo->nodeCaps.nodeCapsMask = m_instancePropertyCapsMask;
        CAMX_LOG_INFO(CamxLogGroupCore, "Node::%s FCV node capsMask is 0x%x",
            NodeIdentifierString(), pNodeCreateInfo->nodeCaps.nodeCapsMask);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::ProcessingNodeInitialize(
    const NodeCreateInputData*   pCreateInputData,
    NodeCreateOutputData*        pCreateOutputData)
{
    CamxResult          result         = CamxResultSuccess;
    CHINODECREATEINFO   nodeCreateInfo = {0};

    ChiCameraInfo cameraInfo           = {0};
    CameraInfo    legacyCameraInfo;

    cameraInfo.pLegacy  = static_cast<VOID*>(&legacyCameraInfo);
    result              = m_pChiContext->GetCameraInfo(GetPipeline()->GetCameraId(), &cameraInfo);
    if (CamxResultSuccess == result)
    {
        m_pStaticMetadata = legacyCameraInfo.pStaticCameraInfo;
        CAMX_LOG_INFO(CamxLogGroupCore,
                      "Node::%s device version %d static info %p",
                      NodeIdentifierString(),
                      legacyCameraInfo.deviceVersion,
                      legacyCameraInfo.pStaticCameraInfo);
        CAMX_ASSERT(NULL != m_pStaticMetadata);
    }

    if (pCreateInputData->pNodeInfo->nodePropertyCount > 0)
    {
        // Expecting the component name to always be the first property with an id equal to 1.
        const CHAR* pComponentName = static_cast<CHAR*>(pCreateInputData->pNodeInfo->pNodeProperties[0].pValue);
        CAMX_ASSERT(pCreateInputData->pNodeInfo->pNodeProperties[0].id == NodePropertyCustomLib);

        SIZE_T componentNameLen = OsUtils::StrLen(pComponentName) + 1;
        m_pComponentName = static_cast<CHAR*>(CAMX_CALLOC(componentNameLen));

        if (NULL != m_pComponentName)
        {
            OsUtils::StrLCpy(m_pComponentName, pComponentName, componentNameLen);
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_ASSERT_ALWAYS();
        }
    }

    for (UINT32 index = 0; index < pCreateInputData->pNodeInfo->nodePropertyCount; index++)
    {
        if (NodePropertySkipUpdatingBufferProperty == pCreateInputData->pNodeInfo->pNodeProperties->id)
        {
            SetBufferSkipProperties(*static_cast<BOOL*>(pCreateInputData->pNodeInfo->pNodeProperties->pValue));
        }
    }


    if (CamxResultSuccess == result)
    {
        ChiNodeInterface nodeInterface;

        nodeInterface.size                           = sizeof(ChiNodeInterface);
        nodeInterface.majorVersion                   = 0;
        nodeInterface.minorVersion                   = 0;
        nodeInterface.pGetMetadata                   = FNGetMetadata;
        nodeInterface.pGetMultiCamDynamicMetaByCamId = FNGetMultiCamDynamicMetaByCamId;
        nodeInterface.pSetMetadata                   = FNSetMetadata;
        nodeInterface.pGetVendorTagBase              = FNGetVendorTagBase;
        nodeInterface.pProcessRequestDone            = FNProcRequestDone;
        nodeInterface.pCreateFence                   = FNCreateFence;
        nodeInterface.pReleaseFence                  = FNReleaseFence;
        nodeInterface.pWaitFenceAsync                = FNWaitFenceAsync;
        nodeInterface.pSignalFence                   = FNSignalFence;
        nodeInterface.pGetFenceStatus                = FNGetFenceStatus;
        nodeInterface.pGetDataSource                 = FNGetDataSource;
        nodeInterface.pPutDataSource                 = FNPutDataSource;
        nodeInterface.pGetData                       = FNGetData;
        nodeInterface.pPutData                       = FNPutData;
        nodeInterface.pCacheOps                      = FNCacheOps;
        nodeInterface.pProcessMetadataDone           = FNProcMetadataDone;
        nodeInterface.pGetFlushInfo                  = FNGetFlushInfo;
        nodeInterface.pCreateBufferManager           = FNBufferManagerCreate;
        nodeInterface.pDestroyBufferManager          = FNBufferManagerDestroy;
        nodeInterface.pBufferManagerGetImageBuffer   = FNBufferManagerGetImageBuffer;
        nodeInterface.pBufferManagerReleaseReference = FNBufferManagerReleaseReference;
        nodeInterface.pGetStaticMetadata             = FNGetStaticMetadata;
        nodeInterface.pGetSupportedPSMetadataList    = FNGetSupportedPSMetadataList;
        nodeInterface.pGetPSMetadata                 = FNGetPSMetadata;
        nodeInterface.pSetPSMetadata                 = FNSetPSMetadata;
        nodeInterface.pPublishPSMetadata             = FNPublishPSMetadata;
        nodeInterface.pIsPSMetadataPublished         = FNIsPSMetadataPublished;

        m_nodeCallbacks.pChiNodeSetNodeInterface(&nodeInterface);
    }

    if ((NULL != m_nodeCallbacks.pCreate) && (CamxResultSuccess == result))
    {
        nodeCreateInfo.size                   = sizeof(nodeCreateInfo);
        nodeCreateInfo.hChiSession            = static_cast<CHIHANDLE>(this);
        nodeCreateInfo.phNodeSession          = NULL;
        nodeCreateInfo.nodeId                 = pCreateInputData->pNodeInfo->nodeId;
        nodeCreateInfo.nodeInstanceId         = pCreateInputData->pNodeInfo->instanceId;
        nodeCreateInfo.nodeFlags.isRealTime   = IsRealTime();
        nodeCreateInfo.nodeFlags.isBypassable = IsBypassableNode();
        nodeCreateInfo.nodeFlags.isInplace    = IsInplace();
        nodeCreateInfo.nodeFlags.isSecureMode = IsSecureMode();
        nodeCreateInfo.pipelineDelay          = GetMaximumPipelineDelay();
        /// @todo (CAMX-1806) fill the node caps
        /// nodeCreateInfo.nodeCaps = xx;

        GetChiNodeCapsMask(pCreateInputData, &nodeCreateInfo);

        switch (static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion())
        {
            case CSLCameraTitanVersion::CSLTitan170:
                nodeCreateInfo.chiICAVersion = ChiICAVersion::ChiICA10;
                break;
            case CSLCameraTitanVersion::CSLTitan175:
                nodeCreateInfo.chiICAVersion = ChiICAVersion::ChiICA20;
                break;
            case CSLCameraTitanVersion::CSLTitan160:
                nodeCreateInfo.chiICAVersion = ChiICAVersion::ChiICA20;
                break;
            case CSLCameraTitanVersion::CSLTitan150:
                // As Talos ICAVersion is 10. However, ICA is not used for Dewarping. Hence, setting at max/invalid.
                nodeCreateInfo.chiICAVersion = ChiICAVersion::ChiICAMax;
                break;
            case CSLCameraTitanVersion::CSLTitan480:
                nodeCreateInfo.chiICAVersion = ChiICAVersion::ChiICA30;
                break;
            default:
                nodeCreateInfo.chiICAVersion = ChiICAVersion::ChiICAMax;
                break;
        }

        // Pass node properties to chi node
        CAMX_STATIC_ASSERT(sizeof(PerNodeProperty) == sizeof(CHINODEPROPERTY));
        nodeCreateInfo.nodePropertyCount = pCreateInputData->pNodeInfo->nodePropertyCount;
        nodeCreateInfo.pNodeProperties   = reinterpret_cast<CHINODEPROPERTY*>(pCreateInputData->pNodeInfo->pNodeProperties);

        CDKResult cdkResult = m_nodeCallbacks.pCreate(&nodeCreateInfo);
        result              = CDKResultToCamxResult(cdkResult);
    }

    if (CamxResultSuccess == result)
    {
        pCreateOutputData->createFlags.canDRQPreemptOnStopRecording = nodeCreateInfo.nodeFlags.canDRQPreemptOnStopRecording;
        pCreateOutputData->createFlags.hasDelayedNotification       = nodeCreateInfo.nodeFlags.hasDelayedNotification;
        m_canNodeSetBufferDependency                                = nodeCreateInfo.nodeFlags.canSetInputBufferDependency;
        m_hNodeSession                                              = nodeCreateInfo.phNodeSession;

        if (NULL != m_nodeCallbacks.pGetCapabilities)
        {
            m_nodeCapsMask.size = sizeof(CHINODECAPSINFO);
            m_nodeCallbacks.pGetCapabilities(&m_nodeCapsMask);
            if ( (0 == OsUtils::StrCmp(m_pComponentName, "com.qti.node.gpu")) ||
                (0 == OsUtils::StrCmp(m_pComponentName, "com.qti.node.dewarp")))
            {
                m_nodeCapsMask.nodeCapsMask = nodeCreateInfo.nodeCaps.nodeCapsMask;
            }
            if (m_nodeCapsMask.nodeCapsMask & ChiNodeCapsParallelReq)
            {
                m_parallelProcessRequests = TRUE;
                CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s supports parallel process request\n", NodeIdentifierString());
            }
        }
    }

    UINT32  groupID         = 1;
    UINT    numOutputPorts  = 0;
    UINT    outputPortId[MaxBufferComposite];

    GetAllOutputPortIds(&numOutputPorts, &outputPortId[0]);

    for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
    {
        pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] = groupID++;
    }

    pCreateOutputData->bufferComposite.hasCompositeMask = FALSE;

    pCreateOutputData->maxOutputPorts = MaximumChiNodeWrapperOutputPorts;
    pCreateOutputData->maxInputPorts  = MaximumChiNodeWrapperInputPorts;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CAMX_ASSERT(NULL != pBufferNegotiationData);

    CamxResult                   result                                                  = CamxResultSuccess;
    OutputPortNegotiationData*   pOutputPortNegotiationData                              = NULL;
    ChiNodeQueryBufferInfo       chiNodeQueryBufferInfo                                  = {0};
    UINT32                       totalInputPorts                                         = 0;
    UINT32                       totalOutputPorts                                        = 0;
    UINT32                       inputPortId[MaximumChiNodeWrapperInputPorts]            = {};
    ChiInputPortQueryBufferInfo  nodeInputPortOptions[MaximumChiNodeWrapperInputPorts]   = {};
    ChiOutputPortQueryBufferInfo outputPortInfo[MaximumChiNodeWrapperOutputPorts]        = {};
    ChiNodeBufferRequirement     outputPortRequirement[MaximumChiNodeWrapperOutputPorts][MaximumChiNodeWrapperInputPorts] = {};
    AlignmentInfo                alignmentLCM[FormatsMaxPlanes]                          = { {0} };
    const ImageFormat*           pFormat                                                 = NULL;

    // Get Input Port List
    GetAllInputPortIds(&totalInputPorts, &inputPortId[0]);
    totalOutputPorts = pBufferNegotiationData->numOutputPortsNotified;

    chiNodeQueryBufferInfo.size                 = sizeof(CHINODEQUERYBUFFERINFO);
    chiNodeQueryBufferInfo.hNodeSession         = m_hNodeSession;
    chiNodeQueryBufferInfo.numOutputPorts       = totalOutputPorts;
    chiNodeQueryBufferInfo.numInputPorts        = totalInputPorts;
    chiNodeQueryBufferInfo.pInputOptions        = &nodeInputPortOptions[0];
    chiNodeQueryBufferInfo.pOutputPortQueryInfo = &outputPortInfo[0];

    for (UINT outputIndex = 0; outputIndex < totalOutputPorts && outputIndex < MaximumChiNodeWrapperOutputPorts; outputIndex++)
    {
        pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[outputIndex];

        chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].outputPortId                   =
            GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);
        chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].numConnectedInputPorts         =
            pOutputPortNegotiationData->numInputPortsNotification;
        chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].pBufferRequirement             =
            &outputPortRequirement[outputIndex][0];
        chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].outputPortflags.isSinkNoBuffer =
            IsSinkPortNoBuffer(GetOutputPortId(pOutputPortNegotiationData->outputPortIndex));
        chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].outputPortflags.isSinkBuffer   =
            IsSinkPortWithBuffer(GetOutputPortId(pOutputPortNegotiationData->outputPortIndex));

        pFormat = GetOutputPortImageFormat(outputIndex);
        if (NULL != pFormat)
        {
            chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].outputBufferOption.format =
                static_cast<ChiFormat>(pFormat->format);
        }
        else
        {
            chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].outputBufferOption.format =
                ChiFormat::YUV420NV21; ///< NV21 as default format
        }

        for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
        {
            chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].pBufferRequirement[inputIndex].minW     =
                pOutputPortNegotiationData->inputPortRequirement[inputIndex].minWidth;
            chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].pBufferRequirement[inputIndex].minH     =
                pOutputPortNegotiationData->inputPortRequirement[inputIndex].minHeight;
            chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].pBufferRequirement[inputIndex].maxW     =
                pOutputPortNegotiationData->inputPortRequirement[inputIndex].maxWidth;
            chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].pBufferRequirement[inputIndex].maxH     =
                pOutputPortNegotiationData->inputPortRequirement[inputIndex].maxHeight;
            chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].pBufferRequirement[inputIndex].optimalW =
                pOutputPortNegotiationData->inputPortRequirement[inputIndex].optimalWidth;
            chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].pBufferRequirement[inputIndex].optimalH =
                pOutputPortNegotiationData->inputPortRequirement[inputIndex].optimalHeight;

            for (UINT planeIdx = 0; planeIdx < FormatsMaxPlanes; planeIdx++)
            {
                alignmentLCM[planeIdx].strideAlignment   =
                    Utils::CalculateAlignment(alignmentLCM[planeIdx].strideAlignment,
                                        pOutputPortNegotiationData->inputPortRequirement[inputIndex]
                                                           .planeAlignment[planeIdx].strideAlignment);
                alignmentLCM[planeIdx].scanlineAlignment =
                    Utils::CalculateAlignment(alignmentLCM[planeIdx].scanlineAlignment,
                                        pOutputPortNegotiationData->inputPortRequirement[inputIndex]
                                                           .planeAlignment[planeIdx].scanlineAlignment);
            }
        }
    }

    for (UINT inputIndex = 0; inputIndex < chiNodeQueryBufferInfo.numInputPorts; inputIndex++)
    {
        ChiInputPortQueryBufferInfo* pInputOptions = &chiNodeQueryBufferInfo.pInputOptions[inputIndex];

        pInputOptions->inputPortId = inputPortId[inputIndex];

        pFormat = GetInputPortImageFormat(inputIndex);
        if (NULL != pFormat)
        {
            pInputOptions->inputBufferOption.format = static_cast<ChiFormat>(pFormat->format);
        }
        else
        {
            pInputOptions->inputBufferOption.format = ChiFormat::YUV420NV21; ///< NV21 as default format
        }
    }

    // call into the chi node using the QueryBufferInfo to query for the requirement of input buffer
    m_nodeCallbacks.pQueryBufferInfo(&chiNodeQueryBufferInfo);

    // Sink no buffer node doesn't have any buffer to output, so skip the logic
    if (FALSE == IsSinkNoBufferNode())
    {
        for (UINT outputIndex = 0; outputIndex < pBufferNegotiationData->numOutputPortsNotified; outputIndex++)
        {
            ChiNodeBufferRequirement* pOutputOption =
                &chiNodeQueryBufferInfo.pOutputPortQueryInfo[outputIndex].outputBufferOption;

            pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[outputIndex];

            // Store the buffer requirements for this output port which will be reused to set, during forward walk.
            // The values stored here could be final output dimensions unless it is overridden by forward walk.
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth  = pOutputOption->optimalW;
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight = pOutputOption->optimalH;
            pOutputPortNegotiationData->outputBufferRequirementOptions.minWidth      = pOutputOption->minW;
            pOutputPortNegotiationData->outputBufferRequirementOptions.minHeight     = pOutputOption->minH;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxWidth      = pOutputOption->maxW;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxHeight     = pOutputOption->maxH;

            for (UINT planeIdx = 0; planeIdx < FormatsMaxPlanes; planeIdx++)
            {
                alignmentLCM[planeIdx].strideAlignment   =
                Utils::CalculateAlignment(alignmentLCM[planeIdx].strideAlignment,
                                    pOutputOption->planeAlignment[planeIdx].strideAlignment);
                alignmentLCM[planeIdx].scanlineAlignment =
                Utils::CalculateAlignment(alignmentLCM[planeIdx].scanlineAlignment,
                                    pOutputOption->planeAlignment[planeIdx].scanlineAlignment);
            }

            CAMX_STATIC_ASSERT(sizeof(AlignmentInfo) == sizeof(ChiAlignmentInfo));
            Utils::Memcpy(&pOutputPortNegotiationData->outputBufferRequirementOptions.planeAlignment[0],
                          &alignmentLCM[0],
                          sizeof(AlignmentInfo) * FormatsMaxPlanes);

            if ((pOutputOption->optimalW == 0) ||
                (pOutputOption->optimalH == 0) ||
                (pOutputOption->minW     == 0) ||
                (pOutputOption->minH     == 0) ||
                (pOutputOption->maxW     == 0) ||
                (pOutputOption->maxH     == 0))
            {
                result = CamxResultEFailed;
                break;
            }
        }
    }

    if (result == CamxResultEFailed)
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s ERROR: Buffer Negotiation Failed", NodeIdentifierString());
    }
    else
    {
        pBufferNegotiationData->numInputPorts = totalInputPorts;

        for (UINT input = 0; input < totalInputPorts; input++)
        {
            ChiInputPortQueryBufferInfo* pInput = &chiNodeQueryBufferInfo.pInputOptions[input];

            pBufferNegotiationData->inputBufferOptions[input].nodeId     = Type();
            pBufferNegotiationData->inputBufferOptions[input].instanceId = InstanceID();
            pBufferNegotiationData->inputBufferOptions[input].portId     = pInput->inputPortId;

            BufferRequirement* pInputBufferRequirement = &pBufferNegotiationData->inputBufferOptions[input].bufferRequirement;

            pInputBufferRequirement->optimalWidth  = pInput->inputBufferOption.optimalW;
            pInputBufferRequirement->optimalHeight = pInput->inputBufferOption.optimalH;
            pInputBufferRequirement->minWidth      = pInput->inputBufferOption.minW;
            pInputBufferRequirement->minHeight     = pInput->inputBufferOption.minH;
            pInputBufferRequirement->maxWidth      = pInput->inputBufferOption.maxW;
            pInputBufferRequirement->maxHeight     = pInput->inputBufferOption.maxH;

            for (UINT planeIdx = 0; planeIdx < FormatsMaxPlanes; planeIdx++)
            {
                alignmentLCM[planeIdx].strideAlignment   =
                Utils::CalculateAlignment(alignmentLCM[planeIdx].strideAlignment,
                                    pInput->inputBufferOption.planeAlignment[planeIdx].strideAlignment);
                alignmentLCM[planeIdx].scanlineAlignment =
                Utils::CalculateAlignment(alignmentLCM[planeIdx].scanlineAlignment,
                                    pInput->inputBufferOption.planeAlignment[planeIdx].scanlineAlignment);
            }

            CAMX_STATIC_ASSERT(sizeof(AlignmentInfo) == sizeof(ChiAlignmentInfo));
            Utils::Memcpy(&pInputBufferRequirement->planeAlignment[0],
                          &alignmentLCM[0],
                          sizeof(AlignmentInfo) * FormatsMaxPlanes);

            CAMX_LOG_INFO(CamxLogGroupChi,
                          "Node::%s Buffer Negotiation dims, Port %d Optimal %d x %d, Min %d x %d, Max %d x %d\n",
                          NodeIdentifierString(),
                          pInput->inputPortId,
                          pInputBufferRequirement->optimalWidth,
                          pInputBufferRequirement->optimalHeight,
                          pInputBufferRequirement->minWidth,
                          pInputBufferRequirement->minHeight,
                          pInputBufferRequirement->maxWidth,
                          pInputBufferRequirement->maxHeight);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeWrapper::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    UINT32                      width                       = 0;
    UINT32                      height                      = 0;

    CAMX_ASSERT(NULL != pBufferNegotiationData);

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData   = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        InputPortNegotiationData*  pInputPortNegotiationData    = &pBufferNegotiationData->pInputPortNegotiationData[0];
        BufferProperties*          pFinalOutputBufferProperties = pOutputPortNegotiationData->pFinalOutputBufferProperties;
        CHINODESETBUFFERPROPERTIESINFO  bufferInfo      = {0};
        CHINODEIMAGEFORMAT              bufferFormat    = {0};

        // Option-1: Check the node's capability in ChiNodeWrapper and decide based on the capability whether it can scale.
        // Option-2: Pass all of the parameters from BufferNegotiationData to the ChiNode and let it decide the output.
        // Option-2 needs interface changes, so Option-1 can be considered for Phase-1 and Option-2 is Phase-2.

        CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s nodeCapsMask : %d, ChiNodeCapsScale:%d",
            NodeIdentifierString(), m_nodeCapsMask.nodeCapsMask, ChiNodeCapsScale);

        if (m_nodeCapsMask.nodeCapsMask & ChiNodeCapsScale)
        {
            UINT32 outWidth  = pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth;
            UINT32 outHeight = pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight;
            // Ensure output width/height is not greater than input width/height
            width  = (pInputPortNegotiationData->pImageFormat->width < outWidth) ?
                      pInputPortNegotiationData->pImageFormat->width : outWidth;
            height = (pInputPortNegotiationData->pImageFormat->height < outHeight) ?
                      pInputPortNegotiationData->pImageFormat->height : outHeight;
        }
        else if ( (m_nodeCapsMask.nodeCapsMask & ChiNodeCapsGPURotate)      |
                    (m_nodeCapsMask.nodeCapsMask & ChiNodeCapsDewarpEISV3)  |
                    (m_nodeCapsMask.nodeCapsMask & ChiNodeCapsDewarpEISV2)  )
        {
            INT  parentOutputPortId = GetParentNodeOutputPortId(pInputPortNegotiationData->inputPortId);
            UINT divideFactor       = 1;

            CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s ComponentName %s InputPortId %d output %d optimal wxh:%dx%d",
                NodeIdentifierString(),
                m_pComponentName,
                parentOutputPortId,
                GetOutputPortId(pOutputPortNegotiationData->outputPortIndex),
                pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth,
                pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight);

            switch (parentOutputPortId)
            {
                case IFEOutputPortDS4:
                case IFEOutputPortDisplayDS4:
                case IPEOutputPortDS4Ref:
                    divideFactor = 4;
                    break;
                case IFEOutputPortDS16:
                case IFEOutputPortDisplayDS16:
                case IPEOutputPortDS16Ref :
                    divideFactor = 16;
                    break;
                default:
                    divideFactor = 1;
                    break;
            }
            width  = Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
                         pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth, divideFactor)/divideFactor);
            height = Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
                         pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight, divideFactor)/divideFactor);
        }
        else
        {
            width  = pInputPortNegotiationData->pImageFormat->width;
            height = pInputPortNegotiationData->pImageFormat->height;
        }

        BOOL shouldNegotiate        = FALSE;

        if ((FALSE == IsSinkNoBufferNode()) &&
            (FALSE == IsSinkPortWithBuffer(pOutputPortNegotiationData->outputPortIndex)) &&
            (FALSE == IsNonSinkHALBufferOutput(pOutputPortNegotiationData->outputPortIndex)))
        {
            shouldNegotiate = TRUE;
        }

        if (TRUE == IsSinkPortWithBuffer(pOutputPortNegotiationData->outputPortIndex))
        {
            auto* pOptions  = &pOutputPortNegotiationData->outputBufferRequirementOptions;
            shouldNegotiate = (TRUE == ((pOptions->maxWidth != pOptions->optimalWidth) ||
                                        (pOptions->minWidth != pOptions->optimalWidth)));
        }

        if (TRUE == shouldNegotiate)
        {
            UINT outputPortId = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);

            switch (outputPortId)
            {
                case ChiNodeOutputFull:
                    pFinalOutputBufferProperties->imageFormat.width  = width;
                    pFinalOutputBufferProperties->imageFormat.height = height;
                    break;

                // DS port is used for normal downscale where output buffers should be same as made in buffer negotiation
                case ChiNodeOutputDSb:
                    pFinalOutputBufferProperties->imageFormat.width =
                        pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth;
                    pFinalOutputBufferProperties->imageFormat.height =
                        pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight;
                    break;

                case ChiNodeOutputDS:
                    pFinalOutputBufferProperties->imageFormat.width =
                        pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth;
                    pFinalOutputBufferProperties->imageFormat.height =
                        pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight;
                    break;

                case ChiNodeOutputDS4:
                    pFinalOutputBufferProperties->imageFormat.width  =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(width, 4) / 4);
                    pFinalOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(height, 4) / 4);
                    break;

                case ChiNodeOutputDS16:
                    pFinalOutputBufferProperties->imageFormat.width  =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(width, 16) / 16);
                    pFinalOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(height, 16) / 16);
                    break;

                case ChiNodeOutputDS64:
                    pFinalOutputBufferProperties->imageFormat.width  =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(width, 64) / 64);
                    pFinalOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(height, 64) / 64);
                    break;

                default:
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Unsupported port %d", NodeIdentifierString(), outputPortId);
                    break;
            }

            width  = pFinalOutputBufferProperties->imageFormat.width;
            height = pFinalOutputBufferProperties->imageFormat.height;

            Utils::Memcpy(&pFinalOutputBufferProperties->imageFormat.planeAlignment[0],
                          &pOutputPortNegotiationData->outputBufferRequirementOptions.planeAlignment[0],
                          sizeof(AlignmentInfo) * FormatsMaxPlanes);
        }

        bufferInfo.hNodeSession = m_hNodeSession;
        bufferInfo.size         = sizeof(CHINODESETBUFFERPROPERTIESINFO);
        bufferInfo.portId       = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);
        bufferInfo.pFormat      = &bufferFormat;

        bufferFormat.width      = width;
        bufferFormat.height     = height;

        CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s ComponentName %s index %u portId %u bufferInfo %u x %u",
                      NodeIdentifierString(), m_pComponentName, index, bufferInfo.portId,
                      bufferFormat.width, bufferFormat.height);

        if (NULL != m_nodeCallbacks.pSetBufferInfo)
        {
            m_nodeCallbacks.pSetBufferInfo(&bufferInfo);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::PostPipelineCreate()
{
    CDKResult cdkResult = CDKResultSuccess;

    if (NULL != m_nodeCallbacks.pPostPipelineCreate)
    {
        cdkResult = m_nodeCallbacks.pPostPipelineCreate(m_hNodeSession);
    }
    else if (NULL != m_nodeCallbacks.pPipelineCreated)
    {
        // This is to maintain backward compatibility for now.
        // pPipelineCreated is deprecated and will be removed soon
        CAMX_LOG_WARN(CamxLogGroupChi, "Node::%s pPipelineCreated is deprecated, use pPostPipelineCreate",
                      NodeIdentifierString());

        m_nodeCallbacks.pPipelineCreated(m_hNodeSession);
    }

    return CDKResultToCamxResult(cdkResult);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::ExecuteProcessRequest(
    ExecuteProcessRequestData*   pExecuteProcessRequestData)
{

    CAMX_ASSERT(NULL != pExecuteProcessRequestData);
    CAMX_ASSERT(NULL != m_hNodeSession);

    CamxResult                result              = CamxResultSuccess;
    PerRequestActivePorts*    pPerRequestPorts    = pExecuteProcessRequestData->pEnabledPortsInfo;
    CHINODEPROCESSREQUESTINFO info                = {0};
    PerRequestOutputPortInfo* pOutputPort         = NULL;
    NodeProcessRequestData*   pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    CHIDEPENDENCYINFO         dependency          = {0};
    UINT32                    requestIdIndex      = 0;
    BOOL                      hasBufferDependency = FALSE;

    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "ExecuteProcessRequest: %s Req.ID: %d Seq.ID: %" PRIu64 "",
                                               m_pComponentName, pNodeRequestData->pCaptureRequest->requestId,
                                               pNodeRequestData->processSequenceId);

    CSLFence hDependentFence[MaxBufferComposite] = {CSLInvalidHandle};

    info.size           = sizeof(info);
    info.hNodeSession   = m_hNodeSession;
    info.frameNum       = pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId;
    info.inputNum       = pPerRequestPorts->numInputPorts;
    info.phInputBuffer  = m_phInputBuffer;
    info.outputNum      = pPerRequestPorts->numOutputPorts;
    info.phOutputBuffer = m_phOutputBuffer;
    info.pBypassData    = m_pBypassData;
    info.pDependency    = &dependency;

    dependency.size                 = sizeof(CHIDEPENDENCYINFO);
    dependency.processSequenceId    = pNodeRequestData->processSequenceId;
    dependency.hNodeSession         = m_hNodeSession;

    UINT32 inputIndex  = (info.frameNum *  pPerRequestPorts->numInputPorts) %
                         (MaxRequestQueueDepth * pPerRequestPorts->numInputPorts);
    UINT32 outputIndex = (info.frameNum *  pPerRequestPorts->numOutputPorts) %
                         (MaxOutputBuffers * pPerRequestPorts->numOutputPorts);

    CAMX_ASSERT(m_numInputPort >= pPerRequestPorts->numInputPorts);
    CAMX_ASSERT(m_numOutputPort >= pPerRequestPorts->numOutputPorts);

    m_pRequestLock->Lock();
    m_requestInProgress[info.frameNum % MaxRequestQueueDepth] = TRUE;
    m_pRequestLock->Unlock();

    requestIdIndex = (pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId % MaxRequestQueueDepth);

    if (pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask)
    {
        hasBufferDependency = TRUE;
    }

    // If LateBinding is not enabled, we will have buffer information always, populate buffer information as well to avoid
    // unnecessary dependencies
    if (FALSE == HwEnvironment::GetInstance()->GetStaticSettings()->enableImageBufferLateBinding)
    {
        pNodeRequestData->bindIOBuffers = TRUE;
    }

    if ((0 == dependency.processSequenceId) && (FALSE == hasBufferDependency))
    {
        m_perRequestData[requestIdIndex].numFences = 0;

        if (FALSE == IsSinkNoBufferNode())
        {
            for (UINT i = 0; i < pPerRequestPorts->numOutputPorts; i++)
            {
                pOutputPort = &pPerRequestPorts->pOutputPorts[i];

                if ((NULL != pOutputPort) && (NULL != pOutputPort->ppImageBuffer[0]))
                {
                    m_perRequestData[requestIdIndex].phFence[i] = pOutputPort->phFence;
                    m_perRequestData[requestIdIndex].numFences++;

                    ImageBufferToChiBuffer(pOutputPort->ppImageBuffer[0],
                                           m_phOutputBuffer[outputIndex + i],
                                           pNodeRequestData->bindIOBuffers);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s i=%d, Output Port/Image Buffer is Null ",
                                   NodeIdentifierString(), i);
                    result = CamxResultEInvalidArg;
                    break;
                }
            }

            info.phOutputBuffer = &m_phOutputBuffer[outputIndex];
        }

        if (CamxResultSuccess == result)
        {
            info.phInputBuffer = &m_phInputBuffer[inputIndex];

            // First request always waits for input fences
            UINT i = 0;
            for (i = 0; i < pPerRequestPorts->numInputPorts; i++)
            {
                PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];

                if ((NULL != pInputPort) && (NULL != pInputPort->pImageBuffer))
                {
                    ImageBufferToChiBuffer(pInputPort->pImageBuffer,
                                           m_phInputBuffer[inputIndex + i],
                                           pNodeRequestData->bindIOBuffers);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s i=%d Input Port/Image Buffer is Null ",
                                   NodeIdentifierString(), i);
                    result = CamxResultEInvalidArg;
                    break;
                }

                // Irrespective of SourceBuffer input port or not, its better to add input fence dependencies always.
                // For exa, if chi creates fences and attaches to input buffers, there is no gaurantee that input fences are
                // signalled by the time Chi submits the request to this pipeline.
                // Also, with late binding, we anyway have to go back to DRQ to make sure input, output buffers are
                // allocated/bound to corresponding ImageBuffers
                // if (FALSE == IsSourceBufferInputPort(i))
                {
                    if (TRUE == m_canNodeSetBufferDependency)
                    {
                        if (CSLInvalidFence != *pInputPort->phFence)
                        {
                            m_phInputBuffer[inputIndex + i]->pfenceHandle     = pInputPort->phFence;
                            m_phInputBuffer[inputIndex + i]->pIsFenceSignaled = pInputPort->pIsFenceSignaled;
                        }
                        else
                        {
                            m_phInputBuffer[inputIndex + i]->pfenceHandle     = NULL;
                            m_phInputBuffer[inputIndex + i]->pIsFenceSignaled = NULL;
                        }

                        CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s Req[%llu] [%d] Fence=%p(%d) Signalled=%p(%d)",
                                         NodeIdentifierString(), info.frameNum, i,
                                         m_phInputBuffer[inputIndex + i]->pfenceHandle, *pInputPort->phFence,
                                         m_phInputBuffer[inputIndex + i]->pIsFenceSignaled, *pInputPort->pIsFenceSignaled);
                    }
                    else
                    {
                        // Looking at index 0, because we don't have more than one dependency list. Need to update here,
                        // if we plan to change to more than one dependency list.
                        UINT fenceCount = pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount;

                        pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[fenceCount] =
                            pInputPort->phFence;
                        pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[fenceCount] =
                            pInputPort->pIsFenceSignaled;

                        // Iterate to check if a particular fence is already added as dependency in case of composite,
                        // and skip adding double dependency
                        for (UINT fenceIndex = 0; fenceIndex <= i; fenceIndex++)
                        {
                            if (hDependentFence[fenceIndex] == *pInputPort->phFence)
                            {
                                CAMX_LOG_VERBOSE(CamxLogGroupStats,
                                                 "Node::%s Fence handle %d, already added dependency portIndex %d",
                                                 NodeIdentifierString(),
                                                 *pInputPort->phFence,
                                                 i);
                                break;
                            }
                            else if (hDependentFence[fenceIndex] == CSLInvalidHandle)
                            {
                                hDependentFence[fenceIndex] = *pInputPort->phFence;
                                pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount++;

                                CAMX_LOG_VERBOSE(CamxLogGroupStats, "Node::%s Add dependency for Fence handle %d, portIndex %d",
                                                 NodeIdentifierString(),
                                                 *pInputPort->phFence,
                                                 i);

                                break;
                            }
                        }
                    }
                }
            }

            // reset all the temp fence bookkeeper to invalid
            for (UINT fenceIndex = 0; fenceIndex <= i; fenceIndex++)
            {
                hDependentFence[fenceIndex] = CSLInvalidHandle;
            }
        }

        if (pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount > 0)
        {
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency    = TRUE;
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
            pNodeRequestData->dependencyInfo[0].processSequenceId                                 =
                FenceCompleteProcessSequenceId;
            pNodeRequestData->numDependencyLists = 1;

            CAMX_LOG_VERBOSE(CamxLogGroupChi,
                             "Node:%s Added buffer dependencies. reqId = %llu Number of fences = %d",
                             NodeIdentifierString(),
                             pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId,
                             pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount);
        }
    }
    else if (FenceCompleteProcessSequenceId == pNodeRequestData->processSequenceId)
    {
        // Internal callback should result in the CHI node getting 0, as the first time it will be invoked
        // Other (than 0 and -2) sequenceIds are passed through
        dependency.processSequenceId = 0;
    }

    if ((0 == pNodeRequestData->numDependencyLists) && (FALSE == hasBufferDependency))
    {
        // If this node requested IOBufferAvailability, base node would have bound buffers to ImageBuffers by now.
        // Populate the Image buffer information to ChiImageList now.
        if (TRUE == pNodeRequestData->bindIOBuffers)
        {
            for (UINT i = 0; i < pPerRequestPorts->numOutputPorts; i++)
            {
                pOutputPort = &pPerRequestPorts->pOutputPorts[i];

                if ((NULL != pOutputPort) && (NULL != pOutputPort->ppImageBuffer[0]))
                {
                    ImageBufferToChiBuffer(pOutputPort->ppImageBuffer[0],
                                           m_phOutputBuffer[outputIndex + i],
                                           pNodeRequestData->bindIOBuffers);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s i=%d, Output Port/Image Buffer is Null ",
                                   NodeIdentifierString(), i);
                    result = CamxResultEInvalidArg;
                    break;
                }
            }

            for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
            {
                PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];

                if ((NULL != pInputPort) && (NULL != pInputPort->pImageBuffer))
                {
                    ImageBufferToChiBuffer(pInputPort->pImageBuffer,
                                           m_phInputBuffer[inputIndex + i],
                                           pNodeRequestData->bindIOBuffers);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s i=%d Input Port/Image Buffer is Null ",
                                   NodeIdentifierString(), i);
                    result = CamxResultEInvalidArg;
                    break;
                }
            }
        }

        info.phOutputBuffer = &m_phOutputBuffer[outputIndex];
        info.phInputBuffer  = &m_phInputBuffer[inputIndex];

        for (UINT i = 0; i < pPerRequestPorts->numOutputPorts; i++)
        {
            m_phOutputBuffer[outputIndex + i]->portId = pPerRequestPorts->pOutputPorts[i].portId;
        }
        for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
        {
            m_phInputBuffer[inputIndex + i]->portId = pPerRequestPorts->pInputPorts[i].portId;
        }

        info.requireCmdBuffers = FALSE;

        CDKResult cdkResult = m_nodeCallbacks.pProcessRequest(&info);
        result              = CDKResultToCamxResult(cdkResult);

        CAMX_LOG_VERBOSE(CamxLogGroupChi,
                         "Node::%s result %d info.requireCmdBuffers %d info.frameNum %llu"
                         "m_CSLHwInfo.requireCSLAccess %d m_nodeCallbacks.pFillHwdata %p",
                         NodeIdentifierString(), result, info.requireCmdBuffers, info.frameNum,
                         m_CSLHwInfo.requireCSLAccess, m_nodeCallbacks.pFillHwdata);

        if ((CamxResultSuccess == result)           && (TRUE == info.requireCmdBuffers) &&
            (TRUE == m_CSLHwInfo.requireCSLAccess)  && (NULL != m_nodeCallbacks.pFillHwdata))
        {
            if (FALSE == IsPipelineStreamedOn())
            {
                result = SetupAndCommitHWData(&info, info.frameNum, ChiCSLHwPacketInit);
            }
            else
            {
                result = SetupAndCommitHWData(&info, info.frameNum, ChiCSLHwPacketPerRequest);
            }
        }

        m_perRequestData[requestIdIndex].isDelayedRequestDone = info.isDelayedRequestDone;

        CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s Req[%llu] Dependency count for Properties=%d , for Buffers=%u",
                         NodeIdentifierString(), info.frameNum,
                         info.pDependency->count, info.pDependency->inputBufferFenceCount);

        CAMX_ASSERT_MESSAGE(-1 != info.pDependency->processSequenceId, "-1 is a reserved ProcessSequenceId");
        CAMX_ASSERT_MESSAGE(-2 != info.pDependency->processSequenceId, "-2 is a reserved ProcessSequenceId");

        // Satisfy sequential execution dependency if chi node requests for it,
        // else dependency will be satisfied on request done
        if (TRUE == info.pDependency->satisfySequentialExecutionDependency)
        {
            UINT        tag     = GetNodeCompleteProperty();
            const UINT  one     = 1;
            const VOID* pOne[1] = { &one };
            WriteDataList(&tag, pOne, &one, 1);
        }

        // Set a dependency on the completion of the previous ExecuteProcessRequest() call
        // so that we can guarantee serialization of all ExecuteProcessRequest() calls for this node.
        // Do this when Chi node requests for sequential execution
        if (TRUE == info.pDependency->sequentialExecutionNeeded)
        {
            if (FirstValidRequestId < GetRequestIdOffsetFromLastFlush(info.frameNum))
            {
                info.pDependency->properties[info.pDependency->count] = GetNodeCompleteProperty();
                info.pDependency->offsets[info.pDependency->count]    = 1;
                info.pDependency->count++;
            }
        }

        if (0 < info.pDependency->count)
        {
            UINT& rCount = pNodeRequestData->dependencyInfo[0].propertyDependency.count;

            for (UINT i = 0; i < info.pDependency->count; i++)
            {
                UINT32 tag              = info.pDependency->properties[i];
                UINT32 multiCameraTagId = 0;

                // check if the dependency's tag is for internal result pool
                if ((0 != (tag & DriverInternalGroupMask)) && (tag != GetNodeCompleteProperty()))
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Only support depending on tag in result pool %x",
                                   NodeIdentifierString(), tag);
                    continue;
                }
                result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicameraoutputmetadata",
                                                                  "OutputMetadataOpticalZoom",
                                                                  &multiCameraTagId);
                // Add multi-camera dependency
                if (multiCameraTagId == tag)
                {
                    SetMultiCameraMasterDependency(pNodeRequestData);
                }
                else
                {
                    UINT32 propertyID = VendorTagManager::GetMappedPropertyID(tag);

                    if (FALSE == GetPipeline()->IsPSMPublished(tag))
                    {
                        // if the dependency maps to property, then use the propertyID, otherwise
                        // just use the tag id
                        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount]    =
                            propertyID > 0 ? propertyID : tag;
                        pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[rCount]       =
                            info.pDependency->offsets[i];
                        pNodeRequestData->dependencyInfo[0].propertyDependency.negate[rCount]        =
                            info.pDependency->negate[i];
                        CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s vendor tag value %x, mapped property value %x, offset %d",
                                         NodeIdentifierString(), tag,
                                         pNodeRequestData->dependencyInfo[0].propertyDependency.properties[i],
                                         info.pDependency->offsets[i]);
                        rCount++;
                    }
                    else
                    {
                        SetPSMetadataDependency(tag,
                            info.pDependency->inputPortId[i],
                            info.pDependency->offsets[i],
                            pNodeRequestData);
                    }
                }
            }

            if (0 < rCount)
            {
                // Update dependency request data for topology to consume
                pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
                pNodeRequestData->numDependencyLists                                      = 1;
                pNodeRequestData->dependencyInfo[0].processSequenceId                     = info.pDependency->processSequenceId;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s All dependencies are invalid", NodeIdentifierString());
            }
        }

        if (0 < info.pDependency->inputBufferFenceCount)
        {
            UINT i = 0;
            for (i = 0; i < info.pDependency->inputBufferFenceCount; i++)
            {
                if ((NULL != info.pDependency->pInputBufferFence[i])            &&
                    (NULL != info.pDependency->pInputBufferFenceIsSignaled[i]))
                {
                    UINT fenceCount = pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount;

                    pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[fenceCount] =
                        static_cast<CSLFence*>(info.pDependency->pInputBufferFence[i]);
                    pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[fenceCount] =
                        static_cast<UINT*>(info.pDependency->pInputBufferFenceIsSignaled[i]);

                    CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s Req[%llu] [%d] Fence=%p(%d) Signalled=%p(%d)",
                                     NodeIdentifierString(), info.frameNum, i,
                                     pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[fenceCount],
                                     *(pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[fenceCount]),
                                     pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[fenceCount],
                                     *(pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[fenceCount]));

                    // Iterate to check if a particular fence is already added as dependency in case of composite,
                    // and skip adding double dependency
                    for (UINT fenceIndex = 0; fenceIndex <= i; fenceIndex++)
                    {
                        if (hDependentFence[fenceIndex] == *static_cast<CSLFence*>(info.pDependency->pInputBufferFence[i]))
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupStats,
                                             "Node::%s Fence handle %d, already added dependency portIndex %d",
                                             NodeIdentifierString(),
                                             *static_cast<CSLFence*>(info.pDependency->pInputBufferFence[i]),
                                             i);
                            break;
                        }
                        else if (hDependentFence[fenceIndex] == CSLInvalidHandle)
                        {
                            hDependentFence[fenceIndex] = *static_cast<CSLFence*>(info.pDependency->pInputBufferFence[i]);
                            pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount++;

                            CAMX_LOG_VERBOSE(CamxLogGroupStats, "Node::%s Add dependency for Fence handle %d, portIndex %d",
                                             NodeIdentifierString(),
                                             *static_cast<CSLFence*>(info.pDependency->pInputBufferFence[i]),
                                             i);

                            break;
                        }
                    }
                }
            }

            // reset all the temp fence bookkeeper to invalid
            for (UINT fenceIndex = 0; fenceIndex <= i; fenceIndex++)
            {
                hDependentFence[fenceIndex] = CSLInvalidHandle;
            }

            if (pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount > 0)
            {
                pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency = TRUE;
                pNodeRequestData->dependencyInfo[0].processSequenceId = info.pDependency->processSequenceId;
                pNodeRequestData->numDependencyLists = 1;

                CAMX_LOG_VERBOSE(CamxLogGroupChi,
                                 "Node::%s Req[%llu] Added buffer dependencies. reqId = %llu Number of fences = %d",
                                 NodeIdentifierString(), info.frameNum,
                                 pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId,
                                 pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount);
            }
        }

        if (0 < info.pDependency->chiFenceCount)
        {
            UINT  fenceCount      = 0;
            VOID* pFenceUserdata  = NULL;

            for (UINT i = 0; i < info.pDependency->chiFenceCount; i++)
            {
                CHIFENCEHANDLE hChiFence = info.pDependency->pChiFences[i];

                // check if the dependency's chi fence is valid
                if (NULL == hChiFence)
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Only support valid chi fences for dependency",
                                   NodeIdentifierString());
                    continue;
                }

                pNodeRequestData->dependencyInfo[0].chiFenceDependency.pChiFences[i] = static_cast<ChiFence*>(hChiFence);
                // not iterating to check if a particular fence is already added or not, as chi fences are not composited
                fenceCount++;
            }

            if (0 < fenceCount)
            {
                CHIFENCECALLBACKINFO* pFenceCallbackInfo =
                    static_cast<CHIFENCECALLBACKINFO*>(CAMX_CALLOC(sizeof(CHIFENCECALLBACKINFO)));

                if (NULL != pFenceCallbackInfo)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupChi,
                                     "Node::%s depending on chi fence. fenceCount=%d, frameNum=%llu, processSequenceId=%d",
                                     NodeIdentifierString(), fenceCount, info.frameNum, info.pDependency->processSequenceId);

                    pFenceCallbackInfo->frameNum          = info.frameNum;
                    pFenceCallbackInfo->processSequenceId = info.pDependency->processSequenceId;
                    pFenceCallbackInfo->hChiSession       = static_cast<CHIHANDLE>(this);
                    pFenceCallbackInfo->size              = sizeof(CHIFENCECALLBACKINFO);
                    pFenceCallbackInfo->pUserData         = GetChiContext();
                    pFenceUserdata                        = pFenceCallbackInfo;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Out of memory", NodeIdentifierString());
                }

                pNodeRequestData->dependencyInfo[0].dependencyFlags.hasFenceDependency   = TRUE;
                pNodeRequestData->numDependencyLists                                     = 1;
                pNodeRequestData->dependencyInfo[0].chiFenceDependency.chiFenceCount     = fenceCount;
                pNodeRequestData->dependencyInfo[0].chiFenceDependency.pUserData         = pFenceUserdata;
                pNodeRequestData->dependencyInfo[0].chiFenceDependency.pChiFenceCallback = ChiFenceDependencyCallback;
                pNodeRequestData->dependencyInfo[0].processSequenceId                    = info.pDependency->processSequenceId;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s All dependencies are invalid!!!", NodeIdentifierString());
            }
        }

        if (TRUE == info.pDependency->hasIOBufferAvailabilityDependency)
        {
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
            pNodeRequestData->dependencyInfo[0].processSequenceId                                   =
                info.pDependency->processSequenceId;
            pNodeRequestData->numDependencyLists                                                    = 1;
        }

        if ((0 == info.pDependency->count) &&
            (0 == info.pDependency->chiFenceCount) &&
            (0 == info.pDependency->inputBufferFenceCount))
        {
            if (FALSE == info.isDelayedRequestDone)
            {
                if (FALSE == info.isEarlyMetadataDone)
                {
                    ProcessPartialMetadataDone(info.frameNum);
                    ProcessMetadataDone(info.frameNum);
                }

                if (NULL != info.doneFence)
                {
                    CHIFENCECALLBACKINFO* pFenceCallbackInfo =
                        static_cast<CHIFENCECALLBACKINFO*>(CAMX_CALLOC(sizeof(CHIFENCECALLBACKINFO)));

                    if (NULL != pFenceCallbackInfo)
                    {
                        pFenceCallbackInfo->frameNum    = info.frameNum;
                        pFenceCallbackInfo->hChiSession = static_cast<CHIHANDLE>(this);
                        pFenceCallbackInfo->pUserData   = pPerRequestPorts;
                        pFenceCallbackInfo->size        = sizeof(CHIFENCECALLBACKINFO);
                        GetChiContext()->WaitChiFence(info.doneFence, ChiFenceCallback, pFenceCallbackInfo, UINT64_MAX);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Unable to allocate structure, out of memory",
                                       NodeIdentifierString());
                        result = CamxResultENoMemory;
                    }
                }
                else
                {
                    // For Sink nodes signal that request is done.
                    if (TRUE == IsSinkNoBufferNode())
                    {
                        ProcessRequestIdDone(info.frameNum);
                    }

                    for (UINT i = 0; i < pPerRequestPorts->numOutputPorts; i++)
                    {
                        pOutputPort = &pPerRequestPorts->pOutputPorts[i];
                        ImageBuffer*    pImageBuffersToRelease[MaximumChiNodeWrapperInputPorts];
                        UINT            imageBufferReleaseCount[CAMX_ARRAY_SIZE(pImageBuffersToRelease)];
                        UINT            numBuffersToRelease = 0;
                        CSLFenceResult  fenceResult         = CSLFenceResultSuccess;

                        if ((NULL != pOutputPort) && (TRUE == pOutputPort->flags.isDelayedBuffer))
                        {
                            OutputPort*              pOutputPortDesc    = GetOutputPort(i);
                            DelayedOutputBufferInfo* pDelayedBufferData = pOutputPort->pDelayedOutputBufferData;
                            BOOL                     isBypassNeeded     = (TRUE == info.pBypassData[i].isBypassNeeded);
                            UINT                     bypassIndex        = InvalidIndex;

                            if (TRUE == isBypassNeeded)
                            {
                                bypassIndex = info.pBypassData[i].selectedInputPortIndex;

                                if (bypassIndex < pPerRequestPorts->numInputPorts)
                                {
                                    // Select input port data
                                    const PerRequestInputPortInfo* pInputInfo   = &pPerRequestPorts->pInputPorts[bypassIndex];
                                    ImageBuffer* const             pImageBuffer = pOutputPort->ppImageBuffer[0];

                                    pDelayedBufferData->hFence                     = *pInputInfo->phFence;
                                    pDelayedBufferData->ppImageBuffer[0]           = pInputInfo->pImageBuffer;
                                    pDelayedBufferData->isParentInputBuffer        = TRUE;
                                    imageBufferReleaseCount[numBuffersToRelease]   = (pImageBuffer->GetReferenceCount() - 1);
                                    pImageBuffersToRelease[numBuffersToRelease++]  = pImageBuffer;
                                    CAMX_LOG_VERBOSE(CamxLogGroupChi,
                                                     "%s Request %03llu - Releasing img buffer: %p",
                                                     NodeIdentifierString(),
                                                     info.frameNum,
                                                     pOutputPort->ppImageBuffer[0]);
                                }
                                else
                                {
                                    isBypassNeeded = FALSE; // Fall back on non-bypass output. Most likely a green frame
                                    result         = CamxResultEOutOfBounds;
                                    fenceResult    = CSLFenceResultFailed;
                                    CAMX_LOG_ERROR(CamxLogGroupChi,
                                                   "Node::%s Cannot bypass request: %llu - Input Idx: %u OOB | NumInputs: %u"
                                                   "Output Port Id: %u",
                                                   NodeIdentifierString(), info.frameNum, bypassIndex,
                                                   pPerRequestPorts->numInputPorts, pOutputPort->portId);
                                }
                            }
                            if (FALSE == isBypassNeeded)
                            {
                                // Select output port data
                                if (NULL != pOutputPort->phFence)
                                {
                                    pOutputPort->pDelayedOutputBufferData->hFence = *pOutputPort->phFence;
                                }
                                else
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s pOutputPort->phFence is NULL",
                                                   NodeIdentifierString());
                                }
                                pOutputPort->pDelayedOutputBufferData->ppImageBuffer[0]    = pOutputPort->ppImageBuffer[0];
                                pOutputPort->pDelayedOutputBufferData->isParentInputBuffer = FALSE;
                            }



                            UINT numChildRefcount = GetNumChildImageBufferReferences(pOutputPortDesc);

                            UINT numDisabledLinks = (pOutputPort->numPerReqInputPortsDisabled +
                                 pOutputPortDesc->numInputPortsDisabled);

                            if (numChildRefcount > numDisabledLinks)
                            {
                                numChildRefcount -= numDisabledLinks;
                            }
                            else
                            {
                                CAMX_LOG_WARN(CamxLogGroupChi,
                                              "Node: %d PortId: %u Incorrect Ref count!! "
                                              "numDisabledLinks: %u >= numChildRefCount: %u",
                                               NodeIdentifierString(), pOutputPort->portId,
                                               numDisabledLinks, numChildRefcount);
                            }

                            for (UINT inputIdx = 0; inputIdx < pPerRequestPorts->numInputPorts; inputIdx++)
                            {
                                if (inputIdx == bypassIndex)
                                {
                                    continue; // We can't release the bypass port
                                }
                                const PerRequestInputPortInfo* pInputInfo = &pPerRequestPorts->pInputPorts[inputIdx];
                                for (UINT j = 0; j < pOutputPortDesc->numSourcePortsMapped; j++)
                                {
                                    if (pInputInfo->portId == pOutputPortDesc->pMappedSourcePortIds[j])
                                    {
                                        CAMX_LOG_VERBOSE(CamxLogGroupChi,
                                                         "%s Request %03llu - Releasing bypass port %u img buffer: %p",
                                                         NodeIdentifierString(),
                                                         info.frameNum,
                                                         pInputInfo->portId,
                                                         pInputInfo->pImageBuffer);
                                        imageBufferReleaseCount[numBuffersToRelease]  = numChildRefcount;
                                        pImageBuffersToRelease[numBuffersToRelease++] = pInputInfo->pImageBuffer;
                                        break;
                                    }
                                }
                            }

                            CAMX_LOG_VERBOSE(CamxLogGroupChi,
                                "Node::%s Signaled Fence %08x(%08x) Image Buffer %x for request= %llu"
                                " parent buffer = %d input index = %d",
                                NodeIdentifierString(),
                                pOutputPort->phDelayedBufferFence,
                                *pOutputPort->phDelayedBufferFence,
                                pOutputPort->pDelayedOutputBufferData->ppImageBuffer[0],
                                pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId,
                                pOutputPort->pDelayedOutputBufferData->isParentInputBuffer,
                                info.pBypassData[i].selectedInputPortIndex);

                        }

                        CAMX_LOG_VERBOSE(CamxLogGroupChi,
                                         "%03llu] Need to release %u buffers",
                                         info.frameNum,
                                         numBuffersToRelease);
                        for (UINT idx = 0; idx < numBuffersToRelease; idx++)
                        {
                            ImageBuffer* pImageBuffer = pImageBuffersToRelease[idx];
                            CAMX_LOG_VERBOSE(CamxLogGroupChi,
                                             "%03llu] Releasing %u / %u - %p",
                                             info.frameNum,
                                             idx + 1,
                                             numBuffersToRelease,
                                             pImageBuffer);
                            for (UINT j = 0; j < imageBufferReleaseCount[idx]; j++)
                            {
                                if (pImageBuffer->GetReferenceCount() > 1)
                                {
                                    pImageBuffer->ReleaseImageReference();
                                }
                                else
                                {
                                    CAMX_LOG_WARN(CamxLogGroupChi, "Node: %s Cannot release last image buffer reference here!",
                                         NodeIdentifierString());
                                }
                            }
                        }

                        if ((NULL != pOutputPort)                           &&
                            (TRUE == pOutputPort->flags.isDelayedBuffer)    &&
                            ((CamxResultSuccess      == result) ||
                             (CamxResultEOutOfBounds == result)))
                        {
                            CSLFenceSignal(*pOutputPort->phDelayedBufferFence, fenceResult);
                        }

                        if ((NULL != pOutputPort)           &&
                            (NULL != pOutputPort->phFence)  &&
                            (CamxResultSuccess == result))
                        {
                            CSLFenceSignal(*pOutputPort->phFence, CSLFenceResultSuccess);
                        }
                    }
                }
            }
        }
    }

    m_pRequestLock->Lock();
    m_requestInProgress[info.frameNum % MaxRequestQueueDepth] = FALSE;
    if (TRUE == m_flushInProgress)
    {
        m_pRequestProgress->Broadcast();
    }
    m_pRequestLock->Unlock();

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ChiNodeWrapper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiNodeWrapper::ChiNodeWrapper()
{
    m_pNodeName = "ChiNodeWrapper";
    m_derivedNodeHandlesMetaDone = TRUE;
    m_canNodeSetBufferDependency = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::~ChiNodeWrapper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiNodeWrapper::~ChiNodeWrapper()
{
    if (NULL != m_phInputBuffer)
    {
        ReleaseChiBufferHandlePool(m_phInputBuffer);
        m_phInputBuffer = NULL;
        m_numInputPort  = 0;
    }

    if (NULL != m_phOutputBuffer)
    {
        ReleaseChiBufferHandlePool(m_phOutputBuffer);
        m_phOutputBuffer = NULL;
        m_numOutputPort  = 0;
    }

    if (NULL != m_pBypassData)
    {
        CAMX_FREE(m_pBypassData);
        m_pBypassData = NULL;
    }

    if (NULL != m_hNodeSession)
    {
        CHINODEDESTROYINFO info;
        info.size           = sizeof(info);
        info.hNodeSession   = m_hNodeSession;
        m_nodeCallbacks.pDestroy(&info);

        m_hNodeSession = NULL;
    }

    if (NULL != m_pRequestLock)
    {
        m_pRequestLock->Destroy();
        m_pRequestLock = NULL;
    }

    if (NULL != m_pRequestProgress)
    {
        m_pRequestProgress->Destroy();
        m_pRequestProgress = NULL;
    }

    if (NULL != m_pComponentName)
    {
        CAMX_FREE(m_pComponentName);
    }

    if (NULL != m_CSLHwInfo.pScratchMemoryBuffer)
    {
        if (CSLInvalidHandle != m_CSLHwInfo.pScratchMemoryBuffer->hHandle)
        {
            CSLReleaseBuffer(m_CSLHwInfo.pScratchMemoryBuffer->hHandle);
        }

        CAMX_FREE(m_CSLHwInfo.pScratchMemoryBuffer);
        m_CSLHwInfo.pScratchMemoryBuffer = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::AllocateChiBufferHandlePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHINODEBUFFERHANDLE* ChiNodeWrapper::AllocateChiBufferHandlePool(
    INT32 size)
{
    ChiImageList*        pImageList      = static_cast<ChiImageList*>(CAMX_CALLOC(sizeof(ChiImageList) * size));
    CHINODEBUFFERHANDLE* phBufferHandle  = static_cast<CHINODEBUFFERHANDLE*>(CAMX_CALLOC(sizeof(CHINODEBUFFERHANDLE) * size));

    if (NULL != pImageList && NULL != phBufferHandle)
    {
        for (INT32 i = 0; i < size; i++)
        {
            phBufferHandle[i]   = &pImageList[i];
        }
    }
    else
    {
        // one of the memory allocation is failed
        if (NULL != pImageList)
        {
            CAMX_FREE(pImageList);
            pImageList = NULL;
        }

        if (NULL != phBufferHandle)
        {
            CAMX_FREE(phBufferHandle);
            phBufferHandle = NULL;
        }
    }

    return phBufferHandle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ReleaseChiBufferHandlePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeWrapper::ReleaseChiBufferHandlePool(
    CHINODEBUFFERHANDLE* phBufferHandle)
{
    if (NULL != phBufferHandle)
    {
        ChiImageList* pImageList = static_cast<ChiImageList*>(phBufferHandle[0]);
        if (NULL != pImageList)
        {
            CAMX_FREE(pImageList);
            pImageList = NULL;
        }
        CAMX_FREE(phBufferHandle);
        phBufferHandle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ImageFormatToChiFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeWrapper::ImageFormatToChiFormat(
    const ImageFormat*  pFormat,
    CHIIMAGEFORMAT*     pChiFormat)
{
    if (NULL != pFormat)
    {
        pChiFormat->alignment                           = pFormat->alignment;
        pChiFormat->colorSpace                          = static_cast<ChiColorSpace>(pFormat->colorSpace);
        pChiFormat->height                              = pFormat->height;
        pChiFormat->planeAlignment[0].scanlineAlignment = pFormat->planeAlignment->scanlineAlignment;
        pChiFormat->planeAlignment[0].strideAlignment   = pFormat->planeAlignment->strideAlignment;
        pChiFormat->rotation                            = static_cast<ChiRotation>(pFormat->rotation);
        pChiFormat->width                               = pFormat->width;
        pChiFormat->format                              = static_cast<ChiFormat>(pFormat->format);

        for (UINT32 index = 0; index < ChiNodeFormatsMaxPlanes; index++)
        {
            CHIYUVFORMAT*       pChiYUVFormat   = &pChiFormat->formatParams.yuvFormat[index];
            const YUVFormat*    pCamxYUVFormat  = &pFormat->formatParams.yuvFormat[index];

            pChiYUVFormat->width            = pCamxYUVFormat->width;
            pChiYUVFormat->height           = pCamxYUVFormat->height;
            pChiYUVFormat->planeStride      = pCamxYUVFormat->planeStride;
            pChiYUVFormat->sliceHeight      = pCamxYUVFormat->sliceHeight;
            pChiYUVFormat->metadataStride   = pCamxYUVFormat->metadataStride;
            pChiYUVFormat->metadataHeight   = pCamxYUVFormat->metadataHeight;
            pChiYUVFormat->metadataSize     = pCamxYUVFormat->metadataSize;
            pChiYUVFormat->pixelPlaneSize   = pCamxYUVFormat->pixelPlaneSize;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "NULL Image format pointer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ImageBufferToChiBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeWrapper::ImageBufferToChiBuffer(
    ImageBuffer*        pImageBuffer,
    ChiImageList*       pChilImageList,
    BOOL                bPopulateBufferHandles)
{
    CAMX_ASSERT(pImageBuffer != NULL);
    // DO NOT MODIFY pImageBuffer
    // @todo (CAMX-1806) check if Chi could use the Camx::ImageFormat directly
    // packing mismatch !!!! Utils::Memcpy(&hHandle->format, pFormat, sizeof(ImageFormat));
    ImageFormatToChiFormat(pImageBuffer->GetFormat(), &pChilImageList->format);

    pChilImageList->imageCount     = pImageBuffer->GetNumFramesInBatch();
    pChilImageList->numberOfPlanes = pImageBuffer->GetNumberOfPlanes();

    CAMX_ASSERT(pChilImageList->numberOfPlanes <= ChiNodeFormatsMaxPlanes);

    for (UINT i = 0; i < pChilImageList->numberOfPlanes; i++)
    {
        CSLMemHandle     hMemHandle     = 0;
        SIZE_T           offset         = 0;
        SIZE_T           metadataSize   = 0;

        if (TRUE == bPopulateBufferHandles)
        {
            // This may return error if this ImageBuffer is not backed up with buffers yet, we can ignore the error as we want
            // to populate with default 0s in that case.
            pImageBuffer->GetPlaneCSLMemHandle(0, i, &hMemHandle, &offset, &metadataSize);
        }

        pChilImageList->metadataSize[i] = metadataSize;
        pChilImageList->handles[i]      = hMemHandle;

        pChilImageList->planeSize[i]    = pImageBuffer->GetPlaneSize(i);
    }

    for (UINT i = 0; i < pChilImageList->imageCount; i++)
    {
        ChiImage* pImage = &pChilImageList->pImageList[i];

        Utils::Memset(pImage, 0x0, sizeof(ChiImage));

        pImage->size = sizeof(ChiImage);

        if (TRUE == bPopulateBufferHandles)
        {
            pImage->pNativeHandle   = pImageBuffer->GetNativeBufferHandle();

            for (UINT j = 0; j < pChilImageList->numberOfPlanes; j++)
            {
                pImage->fd[j]       = pImageBuffer->GetFileDescriptor();
                pImage->pAddr[j]    = pImageBuffer->GetPlaneVirtualAddr(i, j);
            }
        }
    }
    pChilImageList->imageHandle = static_cast<VOID*>(pImageBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::CDKResultToCamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE CamxResult ChiNodeWrapper::CDKResultToCamxResult(
    CDKResult cdkResult)
{
    CamxResult result = CamxResultSuccess;
    if (CDKResultSuccess != cdkResult)
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::CamxResultToCDKResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE CDKResult ChiNodeWrapper::CamxResultToCDKResult(
    CamxResult camxResult)
{
    CDKResult result = CDKResultSuccess;
    if (CamxResultSuccess != camxResult)
    {
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ChiFenceCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeWrapper::ChiFenceCallback(
    CHIFENCEHANDLE  hChiFence,
    VOID*           pUserData)
{
    CAMX_UNREFERENCED_PARAM(hChiFence);

    PerRequestOutputPortInfo* pOutputPort      = NULL;
    CHIFENCECALLBACKINFO*     pCallbackInfo    = reinterpret_cast<CHIFENCECALLBACKINFO*>(pUserData);

    CAMX_ASSERT(NULL != pCallbackInfo);
    CAMX_ASSERT(sizeof(CHIFENCECALLBACKINFO) == (pCallbackInfo->size));

    PerRequestActivePorts*    pPerRequestPorts = reinterpret_cast<PerRequestActivePorts*>(pCallbackInfo->pUserData);
    ChiNodeWrapper*           pNode            = static_cast<ChiNodeWrapper*>(pCallbackInfo->hChiSession);

    pNode->GetChiContext()->ReleaseChiFence(hChiFence);
    hChiFence = NULL;

    CAMX_ASSERT(NULL != pPerRequestPorts);

    // For Sink nodes signal that request is done.
    if (TRUE == pNode->IsSinkNoBufferNode())
    {
        pNode->ProcessRequestIdDone(pCallbackInfo->frameNum);
    }

    for (UINT i = 0; i < pPerRequestPorts->numOutputPorts; i++)
    {
        pOutputPort = &pPerRequestPorts->pOutputPorts[i];
        if ((NULL != pOutputPort) && (NULL != pOutputPort->phFence))
        {
            CSLFenceSignal(*pOutputPort->phFence, CSLFenceResultSuccess);
        }
    }

    CAMX_FREE(pCallbackInfo);
    pCallbackInfo = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::ChiFenceDependencyCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiNodeWrapper::ChiFenceDependencyCallback(
    CHIFENCEHANDLE  hChiFence,
    VOID*           pUserData)
{
    CHIFENCECALLBACKINFO* pCallbackInfo = reinterpret_cast<CHIFENCECALLBACKINFO*>(pUserData);

    CAMX_ASSERT(NULL != pCallbackInfo);
    CAMX_ASSERT(sizeof(CHIFENCECALLBACKINFO) == (pCallbackInfo->size));

    ChiContext* pContext = static_cast<ChiContext*>(pCallbackInfo->pUserData);

    pContext->ReleaseChiFence(hChiFence);
    hChiFence = NULL;

    CAMX_FREE(pCallbackInfo);
    pCallbackInfo = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::SetupNCSLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NCSSensor* ChiNodeWrapper::SetupNCSLink(
    NCSSensorConfig* pSensorConfig)
{
    CamxResult      result            = CamxResultEFailed;
    NCSService*     pNCSServiceObject = NULL;
    NCSSensorConfig sensorConfig      = { 0 };
    NCSSensorCaps   sensorCapsGyro    = {};
    NCSSensor*      pSensorObject     = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupChi, "Setting up an NCS link");
    if (NULL != pSensorConfig)
    {
        // Get the NCS service object handle
        pNCSServiceObject = reinterpret_cast<NCSService *>(HwEnvironment::GetInstance()->GetNCSObject());
        if (NULL != pNCSServiceObject)
        {
            result = pNCSServiceObject->QueryCapabilites(&sensorCapsGyro, pSensorConfig->sensorType);
            if (result == CamxResultSuccess)
            {
                // Clients responsibility to set it to that config which is supported
                sensorConfig.samplingRate  = pSensorConfig->samplingRate;   // in Hertz
                sensorConfig.reportRate    = pSensorConfig->reportRate;     // in micorsecs
                sensorConfig.operationMode = pSensorConfig->operationMode;  // Batched reporting mode
                sensorConfig.sensorType    = pSensorConfig->sensorType;
                pSensorObject = pNCSServiceObject->RegisterService(pSensorConfig->sensorType, &sensorConfig);
                if (NULL == pSensorObject)
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Unable to register with the NCS !!");
                    result = CamxResultEFailed;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Unable to Query caps error %s", Utils::CamxResultToString(result));
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "NULL ncs input config pointer");
        result = CamxResultEInvalidPointer;
    }

    return pSensorObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::DestroyNCSLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::DestroyNCSLink(
    VOID* pSensorObj)
{
    CamxResult      result            = CamxResultEFailed;
    NCSSensor*      pSensorObject     = NULL;
    NCSService*     pNCSServiceObject = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupChi, "Setting up an NCS link");

    if (NULL != pSensorObj)
    {
        pSensorObject = static_cast<NCSSensor*>(pSensorObj);

        // Get the NCS service object handle
        pNCSServiceObject = reinterpret_cast<NCSService *>(HwEnvironment::GetInstance()->GetNCSObject());
        if (NULL != pNCSServiceObject)
        {
            result = pNCSServiceObject->UnregisterService(pSensorObject);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Unregister %s", Utils::CamxResultToString(result));
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Invalid service object %s", Utils::CamxResultToString(result));
        }

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "NULL Sensor object handle");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeWrapper::PrepareStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::PrepareStreamOn()
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s Comp::%s Prepare stream on", NodeIdentifierString(), ComponentName());

    if (NULL != m_nodeCallbacks.pPrepareStreamOn)
    {
        CHINODEPREPARESTREAMONINFO info = { 0 };
        info.size                       = sizeof(info);
        info.hNodeSession               = m_hNodeSession;

        GetAllInputPortIds(&info.portInfo.numInputPorts, &info.portInfo.inputPortId[0]);

        GetAllOutputPortIds(&info.portInfo.numOutputPorts, &info.portInfo.outputPortId[0]);

        result = CDKResultToCamxResult(m_nodeCallbacks.pPrepareStreamOn(&info));

        if (CamxResultSuccess == result)
        {
            result = SetCSLHwInfoAndAcquire(&info.CSLHwInfo);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Error Acquiring %s",
                    NodeIdentifierString(), Utils::CamxResultToString(result));
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeWrapper::SetupAndCommitHWData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::SetupAndCommitHWData(
    CHINODEPROCESSREQUESTINFO* pInfo,
    UINT64                     requestID,
    ChiCSLHwPacketContextType  hwPacketType)
{
    CHINODEFILLHWDATA hwInfo = { 0 };
    CamxResult        result = CamxResultEFailed;

    m_CSLHwInfo.pPacket = GetPacketForRequest(requestID, m_CSLHwInfo.pPacketManager);

    if (NULL != m_CSLHwInfo.pPacket)
    {
        hwInfo.size                 = sizeof(CHINODEFILLHWDATA);
        hwInfo.hNodeSession         = m_hNodeSession;
        hwInfo.frameNum             = requestID;
        hwInfo.packetContext        = hwPacketType;
        hwInfo.numCmdBufferAddr     = m_CSLHwInfo.numCommandBufferManager;
        hwInfo.scratchBufferAddress = m_CSLHwInfo.pScratchMemoryBuffer->pVirtualAddr;

        for (UINT32 i = 0; i < m_CSLHwInfo.numCommandBufferManager; i++)
        {
            m_CSLHwInfo.pCmdBuffer[i] = GetCmdBufferForRequest(requestID, m_CSLHwInfo.pCommandBufferManager[i]);
            if (NULL != m_CSLHwInfo.pCmdBuffer[i])
            {
                m_CSLHwInfo.pCmdBuffer[i]->BeginCommands(m_CSLHwInfo.sizeCmdBufferData[i]);
                hwInfo.pCmdMemoryAdd[i] = static_cast<UINT32*>(m_CSLHwInfo.pCmdBuffer[i]->GetCurrentWriteAddr());
            }
        }

        CDKResult cdkResult = m_nodeCallbacks.pFillHwdata(&hwInfo, pInfo);

        result = CDKResultToCamxResult(cdkResult);

        if (CamxResultSuccess == result)
        {
            for (UINT32 index = 0; index < hwInfo.numHWIOConfig; index++)
            {
                FrameSubsampleConfig currentConfig = {0};

                currentConfig.frameDropPattern = hwInfo.hwIOConfig[index].frameDropPattern;
                currentConfig.frameDropPeriod  = hwInfo.hwIOConfig[index].frameDropPeriod;
                currentConfig.subsamplePattern = hwInfo.hwIOConfig[index].subsamplePattern;
                currentConfig.subsamplePeriod  = hwInfo.hwIOConfig[index].subsamplePeriod;

                result = m_CSLHwInfo.pPacket->AddIOConfig(
                           static_cast<ImageBuffer*>(hwInfo.hwIOConfig[index].imageBuffer),
                           hwInfo.hwIOConfig[index].channelID,
                           ((hwInfo.hwIOConfig[index].bufferDirection == CHICUSTOMHWBUFFERDIRECTION::Input) ?
                               CSLIODirection::CSLIODirectionInput : CSLIODirection::CSLIODirectionOutput),
                           static_cast<CSLFence*>(hwInfo.hwIOConfig[index].pfenceHandle),
                           1,
                           NULL,
                           &currentConfig,
                           0);
            }
        }

        CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s requestID %llu  hwPacketType %d result %d OpCode to KMD %d",
            NodeIdentifierString(), requestID, hwPacketType, result, hwInfo.opCode);
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Packet from GetPacketForRequest is NULL", NodeIdentifierString());
    }

    if (CamxResultSuccess == result)
    {
        AddCmdBufferReference(requestID, hwInfo.opCode);
        CommitAndSubmitPacket();

        if (FALSE == m_CSLHwInfo.requireOutputBuffers)
        {
            // If no output buffer then recycle command buffers immediately
            ProcessRequestIdDone(requestID);
            ProcessMetadataDone(requestID);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Failed to get per request hardware data error: %s",
            NodeIdentifierString(), Utils::CamxResultToString(result));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeWrapper::ReleaseDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::ReleaseDevice()
{
    CamxResult result = CamxResultSuccess;

    if (TRUE == IsDeviceAcquired())
    {
        result = CSLReleaseDevice(GetCSLSession(), m_CSLHwInfo.hDevice);

        if (CamxResultSuccess == result)
        {
            SetDeviceAcquired(FALSE);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeWrapper::AcquireDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::AcquireDevice(
    CHICSLHWINFO* pCSLInfo)
{
    CamxResult result = CamxResultSuccess;

    if ((0 == m_CSLHwInfo.CSLHwResourceID) || (NULL == pCSLInfo))
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Invalid resource ID %u",
            NodeIdentifierString(), m_CSLHwInfo.CSLHwResourceID);
    }
    else
    {
        if (FALSE == IsDeviceAcquired())
        {
            CSLDeviceResource deviceResource;

            deviceResource.pDeviceResourceParam    = pCSLInfo->hwInitData;
            deviceResource.deviceResourceParamSize = pCSLInfo->sizeInitData;
            deviceResource.resourceID              = m_CSLHwInfo.CSLHwResourceID;

            result = CSLAcquireDevice(GetCSLSession(),
                                      &m_CSLHwInfo.hDevice,
                                      DeviceIndices()[0],
                                      &deviceResource,
                                      pCSLInfo->numResources,
                                      NULL,
                                      0,
                                      NodeIdentifierString());

            if (CamxResultSuccess == result)
            {
                SetDeviceAcquired(TRUE);
                AddCSLDeviceHandle(m_CSLHwInfo.hDevice);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Acquire for resource %u Device Failed",
                    NodeIdentifierString(), m_CSLHwInfo.CSLHwResourceID);
            }
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s CSL Wrapper is in Acquire state", NodeIdentifierString());
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeWrapper::OnStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::OnStreamOn()
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s Comp::%s On stream on", NodeIdentifierString(), ComponentName());

    if ((NULL != m_nodeCallbacks.pOnStreamOn) &&
        (CamxResultSuccess == result))
    {
        CHINODEONSTREAMONINFO info = { 0 };
        info.size                  = sizeof(info);
        info.hNodeSession          = m_hNodeSession;

        result = CDKResultToCamxResult(m_nodeCallbacks.pOnStreamOn(&info));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeWrapper::OnStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::OnStreamOff(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(modeBitmask);

    CAMX_LOG_VERBOSE(CamxLogGroupChi, "Node::%s Comp::%s On stream off", NodeIdentifierString(), ComponentName());

    if (TRUE == m_CSLHwInfo.requireCSLAccess)
    {
        result = ReleaseDevice();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s CSL HW is not in Acquire state", NodeIdentifierString());
            result = CamxResultEInvalidState;
        }
    }

    if ((NULL != m_nodeCallbacks.pOnStreamOff) &&
        (CamxResultSuccess == result))
    {
        CHINODEONSTREAMOFFINFO info = { 0 };
        info.size                   = sizeof(info);
        info.hNodeSession           = m_hNodeSession;

        result = CDKResultToCamxResult(m_nodeCallbacks.pOnStreamOff(&info));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeWrapper::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result = CamxResultSuccess;
    if (NULL == pPublistTagList)
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Node::%s pPublistTagList NULL", NodeIdentifierString());
        result = CamxResultEInvalidArg;
    }
    else if (NULL == m_nodeCallbacks.pQueryMetadataPublishList)
    {
        pPublistTagList->tagCount = 0;
    }
    else
    {
        ChiNodeMetadataList chiMetadataList;

        chiMetadataList.hNodeSession = m_hNodeSession;
        chiMetadataList.size         = sizeof(ChiNodeMetadataList);

        result = m_nodeCallbacks.pQueryMetadataPublishList(&chiMetadataList);

        if ((CamxResultSuccess == result) && (0 < chiMetadataList.tagCount))
        {
            pPublistTagList->tagCount        = chiMetadataList.tagCount;
            pPublistTagList->partialTagCount = chiMetadataList.partialTagCount;
            Utils::Memcpy(pPublistTagList->tagArray, chiMetadataList.tagArray,
                          chiMetadataList.tagCount * sizeof(UINT32));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::CancelRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::CancelRequest(
    UINT64 requestId)
{
    CamxResult result = CamxResultEFailed;

    if (NULL != m_nodeCallbacks.pFlushRequest)
    {
        CHINODEFLUSHREQUESTINFO requestInfo;

        requestInfo.size         = sizeof(CHINODEFLUSHREQUESTINFO);
        requestInfo.hNodeSession = m_hNodeSession;
        requestInfo.frameNum     = requestId;
        result                   = m_nodeCallbacks.pFlushRequest(&requestInfo);
    }

    m_pRequestLock->Lock();
    if (TRUE == m_requestInProgress[requestId % MaxRequestQueueDepth])
    {
        m_flushInProgress = TRUE;
        CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s Cancel Request waiting request= %llu", NodeIdentifierString(), requestId);
        result = m_pRequestProgress->TimedWait(m_pRequestLock->GetNativeHandle(), RequestWaitTime);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: Node::%s Cancel request= %llu timed out at %u ms with error %s!",
                NodeIdentifierString(), requestId, RequestWaitTime, Utils::CamxResultToString(result));
            if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->raiserecoverysigabrt)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: Raise SigAbort to debug"
                                             " the root cause of taking more time by CHI Node::%s",
                                             NodeIdentifierString());
                OsUtils::RaiseSignalAbort();
            }
        }

        m_flushInProgress = FALSE;
        CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s Cancel Request Done request= %llu", NodeIdentifierString(), requestId);
    }
    m_pRequestLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeWrapper::GetFlushResponseTimeInMs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 ChiNodeWrapper::GetFlushResponseTimeInMs()
{
    UINT64 responseTimeInMillisec = 0;

    if (NULL != m_nodeCallbacks.pGetFlushResponse)
    {
        CHINODERESPONSEINFO nodeResponseInfo;

        nodeResponseInfo.hChiSession = m_hNodeSession;
        nodeResponseInfo.size        = sizeof(CHINODERESPONSEINFO);

        m_nodeCallbacks.pGetFlushResponse(&nodeResponseInfo);
        responseTimeInMillisec = nodeResponseInfo.responseTimeInMillisec;
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupChi, "Node::%s Get Flush Response Time not implemented!", NodeIdentifierString());
    }

    return responseTimeInMillisec;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeWrapper::CreateCmdAndScratchBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::CreateCmdAndScratchBuffers(
    CHICSLHWINFO* pCSLInfo)
{
    CamxResult     result         = CamxResultSuccess;
    ResourceParams resourceParams = { 0 };
    UINT32         memFlags       = 0;

    if ((NULL == pCSLInfo) || (0 == pCSLInfo->numCmdBuffers) || (ChiNodeMaxCSLCmdBuffer < pCSLInfo->numCmdBuffers))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupChi,
                       "Node::%s Invalid Arg, input param pCSLInfo NULL or num_cmd_buffers is 0 or larger than max.",
                       NodeIdentifierString());
    }
    else
    {
        // Initializate Command Buffer, + 1 indicates Packet Manager comand buffer manager
        result = InitializeCmdBufferManagerList(pCSLInfo->numCmdBuffers + 1);

        if (CamxResultSuccess == result)
        {
            resourceParams.usageFlags.packet               = 1;
            resourceParams.packetParams.maxNumCmdBuffers   = pCSLInfo->numCmdBuffers;
            resourceParams.packetParams.maxNumIOConfigs    = 24;
            // The max below depends on the number of embeddings of DMI/indirect buffers in the packet. Ideally this
            // value should be calculated based on the design in this node. But an upper bound is fine too.
            resourceParams.packetParams.maxNumPatches      = 24;
            resourceParams.packetParams.enableAddrPatching = 1;
            resourceParams.resourceSize                    = Packet::CalculatePacketSize(&resourceParams.packetParams);

            // Same number as cmd buffers
            resourceParams.poolSize       = pCSLInfo->numberOfBuffer * resourceParams.resourceSize;
            resourceParams.alignment      = CamxPacketAlignmentInBytes;
            resourceParams.pDeviceIndices = NULL;
            resourceParams.numDevices     = 0;
            resourceParams.memFlags       = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

            result = CreateCmdBufferManager("PacketManager", &resourceParams, &(m_CSLHwInfo.pPacketManager));
        }

        for (UINT32 i = 0; i < pCSLInfo->numCmdBuffers; i++)
        {
            if (CamxResultSuccess == result)
            {
                ResourceParams params       = { 0 };
                CHAR bufferManagerName[256] = { 0 };

                m_CSLHwInfo.sizeCmdBufferData[i]    = pCSLInfo->sizeOfCommandBuffer[i];
                // Need to reserve a buffer in the Cmd Buffer for the KMD to do the patching
                params.resourceSize                 = pCSLInfo->sizeOfCommandBuffer[i] * sizeof(UINT32);
                params.poolSize                     = pCSLInfo->numberOfBuffer * params.resourceSize;
                params.usageFlags.cmdBuffer         = 1;
                params.cmdParams.type               = CmdType::CDMIndirect;
                params.alignment                    = CamxCommandBufferAlignmentInBytes;
                params.cmdParams.enableAddrPatching = 0;
                // The max below depends on the number of embeddings of DMI/indirect buffers in the command buffer.
                // Ideally this value should be calculated based on the design in this node. But an upper bound is fine too.
                params.cmdParams.maxNumNestedAddrs  = 0;
                params.pDeviceIndices               = DeviceIndices();
                params.numDevices                   = DeviceIndexCount();
                params.memFlags                     = CSLMemFlagUMDAccess | CSLMemFlagKMDAccess;

                OsUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "CmdBufferManager_%u", i);
                result = CreateCmdBufferManager(bufferManagerName, &params, &m_CSLHwInfo.pCommandBufferManager[i]);

                if (CamxResultSuccess == result)
                {
                    m_CSLHwInfo.numCommandBufferManager++;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Error creating cmd buffer manager %s",
                        NodeIdentifierString(), Utils::CamxResultToString(result));
                }
            }
        }

        if (TRUE == pCSLInfo->requireScratchBuffer)
        {
            // Allocate one scratch Buffer
            m_CSLHwInfo.pScratchMemoryBuffer = static_cast<CSLBufferInfo*>(CAMX_CALLOC(sizeof(CSLBufferInfo)));

            if (NULL != m_CSLHwInfo.pScratchMemoryBuffer)
            {
                if (TRUE == IsSecureMode())
                {
                    memFlags = (CSLMemFlagProtected | CSLMemFlagHw);
                }
                else
                {
                    memFlags = (CSLMemFlagUMDAccess | CSLMemFlagHw | CSLMemFlagKMDAccess);
                }

                result = CSLAlloc(NodeIdentifierString(),
                                  m_CSLHwInfo.pScratchMemoryBuffer,
                                  pCSLInfo->sizeOfScratchBuffer,
                                  CamxCommandBufferAlignmentInBytes,
                                  memFlags,
                                  &DeviceIndices()[0],
                                  1);

                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupChi,
                                     "Node::%s CSLAlloc returned scratchBuffer for fd=%d",
                                     NodeIdentifierString(), m_CSLHwInfo.pScratchMemoryBuffer->fd);
                }
                else
                {
                    result = CamxResultENoMemory;
                    CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Failed to allocate scratch Buffer: "
                                                    "sizeOfScratchBuffer: %u, CmdBufferAlignInBytes: %u, "
                                                    "memFlags: %x, Device Index: %d, Device Pointer: %p",
                                                    NodeIdentifierString(), pCSLInfo->sizeOfScratchBuffer,
                                                    CamxCommandBufferAlignmentInBytes, memFlags, 0, &DeviceIndices()[0]);
                }
            }
            else
            {
                result = CamxResultENoMemory;
                CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s Failed to allocate CSL scratch Buffer", NodeIdentifierString());
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeWrapper::AddCmdBufferReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::AddCmdBufferReference(
    UINT64 requestId,
    UINT32 opcode)
{
    CamxResult result         = CamxResultSuccess;
    UINT32     cmdBufferIndex = 0;

    m_CSLHwInfo.pPacket->SetRequestId(GetCSLSyncId(requestId));

    for (UINT32 i = 0; i < m_CSLHwInfo.numCommandBufferManager; i++)
    {
        if (NULL != m_CSLHwInfo.pCmdBuffer[i])
        {
            /// @todo (CAMX-656) Need to define the Metadata of the packet
            m_CSLHwInfo.pCmdBuffer[i]->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdCommon));
            result = m_CSLHwInfo.pPacket->AddCmdBufferReference(m_CSLHwInfo.pCmdBuffer[i], &cmdBufferIndex);

            if (CamxResultSuccess == result)
            {
                m_CSLHwInfo.pPacket->SetOpcode(CSLDeviceType::CSLDeviceTypeCustom, opcode);

                result = m_CSLHwInfo.pPacket->SetKMDCmdBufferIndex(cmdBufferIndex,
                    (m_CSLHwInfo.pCmdBuffer[i]->GetResourceUsedDwords() * sizeof(UINT32)));
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiNodeWrapper::CommitAndSubmitPacket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ChiNodeWrapper::CommitAndSubmitPacket()
{
    CamxResult result = CamxResultSuccess;

    for (UINT32 i = 0; i < m_CSLHwInfo.numCommandBufferManager; i++)
    {
        if (NULL != m_CSLHwInfo.pCmdBuffer[i])
        {
            m_CSLHwInfo.pCmdBuffer[i]->CommitCommands();
        }
    }

    if (CamxResultSuccess == result)
    {
        result = (m_CSLHwInfo.pPacket)->CommitPacket();
    }

    if (CamxResultSuccess == result)
    {
        result = GetHwContext()->Submit(GetCSLSession(), m_CSLHwInfo.hDevice, m_CSLHwInfo.pPacket);
    }

    if (CamxResultSuccess != result)
    {
        if (CamxResultECancelledRequest == result)
        {
            CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s CSL Submit Failed: %s due to Ongoing flush",
                NodeIdentifierString(), Utils::CamxResultToString(result));
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s CSL Submit Failed: %s",
                NodeIdentifierString(), Utils::CamxResultToString(result));
        }
    }

    return result;
}

CAMX_NAMESPACE_END
