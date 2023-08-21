////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtrackernode.cpp
/// @brief Tracker Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "camxmem.h"
#include "camxtrace.h"
#include "camxthreadmanager.h"
#include "camxhwenvironment.h"
#include "camxhwcontext.h"
#include "camximagebuffer.h"
#include "camximageformatutils.h"
#include "camxvendortags.h"
#include "camxpacketbuilder.h"
#include "camxtitan17xcontext.h"
#include "camxtitan17xdefs.h"
#include "titan170_base.h"
#include "camxtrackernode.h"
#include "camxpipeline.h"
#include "camxtranslator.h"
#include "camxstatsdebuginternal.h"
#include "camximagesensormoduledata.h"
#include "camximagesensordata.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// InputVendorTags
///
/// @brief list of vendor tags get from input
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const struct NodeVendorTag InputVendorTags[] =
{
    { "org.quic.camera2.objectTrackingConfig", "Enable" },
    { "org.quic.camera2.objectTrackingConfig", "CmdTrigger" },
    { "org.quic.camera2.objectTrackingConfig", "RegisterROI" },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OutputVendorTags
///
/// @brief list of vendor tags published
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const struct NodeVendorTag OutputVendorTags[] =
{
    { "org.quic.camera2.objectTrackingResults", "TrackerStatus" },
    { "org.quic.camera2.objectTrackingResults", "ResultROI" },
    { "org.quic.camera2.objectTrackingResults", "TrackerScore" },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FrameCropVendorTags
///
/// @brief list of vendor tags from ifecrop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const struct NodeVendorTag FrameCropVendorTags[] =
{
    { "org.quic.camera.ifecropinfo", "AppliedCrop"},
    { "org.quic.camera.ifecropinfo", "ResidualCrop"},
    { "org.quic.camera.ifecropinfo", "SensorIFEAppliedCrop"},
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerBufferDependencyType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum TrackerBufferDependencyType
{
    AddDependencies                 = 0,    ///< First call to tracker Node to ask for tracker dependency
    AddTrackerImageBufDependency    = 1,    ///< Add Tracker img buffer dependency
    TrackerImageBufDependencyMet    = 1,    ///< Tracker img buffer dependency met
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::TrackerNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TrackerNode::TrackerNode()
    : m_trackerFrameStride(0)
    , m_trackerFrameScanline(0)
    , m_pTrackerAlgorithm(0)
{

    FDBaseResolutionType    baseResolution = GetStaticSettings()->FDBaseResolution;

    m_pNodeName         = "Tracker";
    m_isNodeStreamedOff = TRUE;
    m_trackerTriggerCmd = TrackerCMDInvalid;

    m_baseFDHWDimension.width   = static_cast<UINT32>(baseResolution) & 0xFFFF;
    m_baseFDHWDimension.height  = (static_cast<UINT32>(baseResolution) >> 16) & 0xFFFF;

    m_sensorDimension.width         = 0;
    m_sensorDimension.height        = 0;
    m_trackerBufDimension.width     = 0;
    m_trackerBufDimension.height    = 0;
    m_aspectRatioCalibrated         = FALSE;
    m_aspectRatioOffset.left        = 0;
    m_aspectRatioOffset.top         = 0;
    m_aspectRatioOffset.width       = 0;
    m_aspectRatioOffset.height      = 0;
    m_sensorActiveArrayDimension.width  = 0;
    m_sensorActiveArrayDimension.height = 0;
    m_previousIncomingCmd           = TrackerCMDInvalid;
    m_previousRegistrationROI       = { 0 };

    m_staticSettingsTrackingEnable  = GetStaticSettings()->enableTouchToTrack;
    m_trackerImgBufDownscaleRatio   = GetStaticSettings()->touchToTrackDownscaleRatio;
    m_trackerOperationMode          = static_cast<TrackerOperationMode>(GetStaticSettings()->touchToTrackSCVEOperationMode);
    m_trackerPrecisionMode          = static_cast<TrackerPrecisionMode>(GetStaticSettings()->touchToTrackSCVEPrecisionMode);

    CAMX_LOG_VERBOSE(CamxLogGroupTracker, "enable:%d, downscale:%d, FDBasewidth %d, FDBaseheight %d, Opmode %d, Precision %d",
        m_staticSettingsTrackingEnable, m_trackerImgBufDownscaleRatio, m_baseFDHWDimension.width, m_baseFDHWDimension.height,
        m_trackerOperationMode, m_trackerPrecisionMode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::~TrackerNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TrackerNode::~TrackerNode()
{
    TrackerAlgoDestroyParam     algoDestroyParam[TrackerDestroyParamTypeCount];
    TrackerAlgoDestroyParamList algoDestroyParamList;

    // Un-register the job family.
    if ((InvalidJobHandle != m_hThread) && (NULL != m_statsInitializeData.pThreadManager))
    {
        m_statsInitializeData.pThreadManager->UnregisterJobFamily(TrackerThreadCb, "TrackerThread", m_hThread);
    }

    // Destroy Thread data pool
    for (UINT32 index = 0; index < NumOfTrackerNodeThreadData; index++)
    {
        if (NULL != m_pThreadData[index])
        {
            CAMX_FREE(m_pThreadData[index]);
            m_pThreadData[index] = NULL;
        }
    }
    if (NULL != m_pTrackerAlgorithm)
    {
        algoDestroyParamList.paramCount = TrackerDestroyParamTypeCount;
        algoDestroyParamList.pParamList = &algoDestroyParam[0];

        algoDestroyParam[TrackerDestroyParamTypeCameraID].pParam            = &m_cameraInfo.cameraId;
        algoDestroyParam[TrackerDestroyParamTypeCameraID].destroyParamType  = TrackerDestroyParamTypeCameraID;
        algoDestroyParam[TrackerDestroyParamTypeCameraID].sizeOfParam       = sizeof(StatsCameraInfo);
        m_pTrackerAlgorithm->TrackerDestroy(m_pTrackerAlgorithm, &algoDestroyParamList);
        m_pTrackerAlgorithm = NULL;
    }

    if (NULL != m_pLock)
    {
        m_pLock->Destroy();
        m_pLock = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TrackerNode* TrackerNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    TrackerNode*                pTrackerNode        = NULL;
    StatsInitializeCallback*    pInitializecallback = NULL;
    UINT32                      propertyCount       = 0;

    pTrackerNode = CAMX_NEW TrackerNode();

    if (NULL != pTrackerNode && NULL != pCreateInputData && NULL != pCreateInputData->pNodeInfo)
    {
        pTrackerNode->m_skipPattern = 1;
        propertyCount               = pCreateInputData->pNodeInfo->nodePropertyCount;

        for (UINT32 count = 0; count < propertyCount; count++)
        {
            if (NodePropertyStatsSkipPattern == pCreateInputData->pNodeInfo->pNodeProperties[count].id)
            {
                pTrackerNode->m_skipPattern = *static_cast<UINT*>(pCreateInputData->pNodeInfo->pNodeProperties[count].pValue);
                CAMX_LOG_VERBOSE(CamxLogGroupTracker, "Stats skip pattern %d", pTrackerNode->m_skipPattern);
            }
        }
        pInitializecallback                     = &pTrackerNode->m_statsInitializeData.initializecallback;
        pInitializecallback->pTrackerCallback   = pCreateInputData->pTrackerAlgoCallbacks;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupTracker, "TrackerNode::Create failed");
    }
    return pTrackerNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TrackerNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    m_statsInitializeData.pThreadManager     = pFinalizeInitializationData->pThreadManager;
    m_statsInitializeData.pHwContext         = pFinalizeInitializationData->pHwContext;
    m_statsInitializeData.pDebugDataPool     = pFinalizeInitializationData->pDebugDataPool;
    m_statsInitializeData.pPipeline          = GetPipeline();
    m_statsInitializeData.pTuningDataManager = GetPipeline()->GetTuningDataManager();

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::PostPipelineCreate()
{
    CamxResult                      result              = CamxResultSuccess;
    SensorModuleStaticCaps          sensorStaticCaps;
    const SensorMode*               pSensorMode         = NULL;
    const ImageSensorModuleData*    pSensorModuleData   = NULL;

    if (FALSE == m_staticSettingsTrackingEnable)
    {
        return result;
    }

    if (CamxResultSuccess == result)
    {
        result = GetSensorModeData(&pSensorMode);
    }

    if ((CamxResultSuccess == result) && (NULL != pSensorMode))
    {
        m_cameraInfo.cameraId    = GetPipeline()->GetCameraId();
        m_sensorDimension.width  = pSensorMode->resolution.outputWidth;
        m_sensorDimension.height = pSensorMode->resolution.outputHeight;

        pSensorModuleData        = m_statsInitializeData.pHwContext->GetImageSensorModuleData(m_cameraInfo.cameraId);
        pSensorModuleData->GetSensorDataObject()->GetSensorStaticCapability(&sensorStaticCaps, m_cameraInfo.cameraId);

        m_sensorActiveArrayDimension.width  = sensorStaticCaps.activeArraySize.width;
        m_sensorActiveArrayDimension.height = sensorStaticCaps.activeArraySize.height;

        CAMX_LOG_VERBOSE(CamxLogGroupTracker, "CameraID %d, Sensor Dim w: %d (x%d) h: %d (x%d),"
            "Orig active array: %d %d, QCFA active array: %d %d",
            m_cameraInfo.cameraId,
            m_sensorDimension.width, pSensorMode->binningTypeH,
            m_sensorDimension.height, pSensorMode->binningTypeV,
            m_sensorActiveArrayDimension.width, m_sensorActiveArrayDimension.height,
            sensorStaticCaps.QuadCFAActiveArraySize.width, sensorStaticCaps.QuadCFAActiveArraySize.height);
    }

    m_pLock = Mutex::Create("TrackerLock");
    if (NULL == m_pLock)
    {
        CAMX_LOG_ERROR(CamxLogGroupTracker, "Failed in Mutex creation");
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        if (InvalidJobHandle == m_hThread)
        {
            result = m_statsInitializeData.pThreadManager->RegisterJobFamily(
                TrackerThreadCb, "TrackerThread", NULL, JobPriority::Normal, TRUE, &m_hThread);
        }
    }

    if (CamxResultSuccess == result)
    {
        for (UINT32 index = 0; index < NumOfTrackerNodeThreadData; index++)
        {
            if (NULL == m_pThreadData[index])
            {
                m_pThreadData[index] = CAMX_NEW TrackerThreadData;
                if (NULL == m_pThreadData[index])
                {
                    result = CamxResultENoMemory;
                    CAMX_LOG_ERROR(CamxLogGroupTracker, "Failed to create Tracker Thread data. idx: %d", index);
                }
            }
        }
    }

    if (CamxResultSuccess == result &&
        NULL != m_statsInitializeData.initializecallback.pTrackerCallback &&
        NULL != m_statsInitializeData.initializecallback.pTrackerCallback->pfnSetAlgoInterface)
    {
        CHIALGORITHMINTERFACE chiAlgoInterface;

        chiAlgoInterface.pGetVendorTagBase       = ChiStatsSession::GetVendorTagBase;
        chiAlgoInterface.pGetMetadata            = ChiStatsSession::FNGetMetadata;
        chiAlgoInterface.pSetMetaData            = ChiStatsSession::FNSetMetadata;
        chiAlgoInterface.pQueryVendorTagLocation = ChiStatsSession::QueryVendorTagLocation;
        chiAlgoInterface.size                    = sizeof(CHIALGORITHMINTERFACE);

        m_statsInitializeData.initializecallback.pTrackerCallback->pfnSetAlgoInterface(&chiAlgoInterface);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CamxResult      result = CamxResultSuccess;
    CamxDimension   previewDimension = { 0 };
    UINT32          width;
    UINT32          height;

    if (NULL == pBufferNegotiationData)
    {
        CAMX_LOG_ERROR(CamxLogGroupTracker, "pBufferNegotiationData is NULL");
        result = CamxResultEInvalidPointer;
    }
    else
    {
        // only one input port
        BufferRequirement* pInputBufferRequirement = &pBufferNegotiationData->inputBufferOptions[0].bufferRequirement;
        if (NULL == pInputBufferRequirement)
        {
            CAMX_LOG_ERROR(CamxLogGroupTracker, "InputBufferRequirement is NULL");
            result = CamxResultEInvalidPointer;
        }
        else
        {
            width  = m_baseFDHWDimension.width;
            height = m_baseFDHWDimension.height;

            result = GetPreviewDimension(&previewDimension);

            if ((CamxResultSuccess == result) && (0 != previewDimension.width) && (0 != previewDimension.height))
            {
                CamxDimension frameDimension = m_baseFDHWDimension;

                Utils::MatchAspectRatio(&previewDimension, &frameDimension);

                width                        = frameDimension.width;
                height                       = frameDimension.height;
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupTracker, "Failed to get preview dimension, result=%d, width=%d, height=%d",
                    result, previewDimension.width, previewDimension.height);
            }

            width                                  = Utils::EvenFloorUINT32(width);
            height                                 = Utils::EvenFloorUINT32(height);

            CAMX_LOG_VERBOSE(CamxLogGroupTracker,
                "Buffer dimensions T2T: %dx%d, Preview: %dx%d, Request: [%dx%d]",
                m_baseFDHWDimension.width, m_baseFDHWDimension.height,
                previewDimension.width, previewDimension.height,
                width, height);

            pInputBufferRequirement->optimalWidth  = width;
            pInputBufferRequirement->optimalHeight = height;
            pInputBufferRequirement->minWidth      = width;
            pInputBufferRequirement->minHeight     = height;
            pInputBufferRequirement->maxWidth      = width;
            pInputBufferRequirement->maxHeight     = height;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TrackerNode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    const ImageFormat* pImageFormat = NULL;

    CAMX_ASSERT(NULL != pBufferNegotiationData);

    pImageFormat = pBufferNegotiationData->pInputPortNegotiationData[0].pImageFormat;
    CAMX_ASSERT(NULL != pImageFormat);

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];

        BufferProperties* pFinalOutputBufferProperties        = pOutputPortNegotiationData->pFinalOutputBufferProperties;

        for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
        {
            BufferRequirement* pInputPortRequirement =
                &pOutputPortNegotiationData->inputPortRequirement[inputIndex];

            pFinalOutputBufferProperties->imageFormat.width  = pInputPortRequirement->optimalWidth;
            pFinalOutputBufferProperties->imageFormat.height = pInputPortRequirement->optimalHeight;
        }
        pFinalOutputBufferProperties->memFlags |= CSLMemFlagUMDAccess;
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::GetBufferDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::GetBufferDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData
    ) const
{
    CamxResult              result              = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pEnabledPorts       = pExecuteProcessRequestData->pEnabledPortsInfo;

    // Post results when IFE buffer returns
    for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
    {
        UINT* pFenceCount = &pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount;

        PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[i];
        switch (pPerRequestInputPort->portId)
        {
            case TrackerInputPortImage:
                pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[*pFenceCount] = pPerRequestInputPort->phFence;
                pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[*pFenceCount] =
                                                                                      pPerRequestInputPort->pIsFenceSignaled;
                pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency = TRUE;
                pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount++;
                pNodeRequestData->dependencyInfo[0].processSequenceId = static_cast<UINT32>(AddTrackerImageBufDependency);
                break;
            default:
                CAMX_LOG_WARN(CamxLogGroupTracker, "Unhandled input port %d", pPerRequestInputPort->portId);
                break;
        }
    }

    pNodeRequestData->numDependencyLists = 1;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::GetTrackerResultsFromUsecasePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TrackerNode::GetTrackerResultsFromUsecasePool(
    TrackerROIInformation* pROIInfo)
{
    CAMX_ASSERT(NULL != pROIInfo);
    CamxResult        result              = CamxResultSuccess;
    static UINT const TrackerResultTags[] = { PropertyIDUsecaseTrackerResults };
    static UINT const Length              = CAMX_ARRAY_SIZE(TrackerResultTags);
    VOID*             pData[Length]       = { NULL };
    UINT64            dataOffset[Length]  = { 0 };

    result = GetDataList(TrackerResultTags, pData, dataOffset, 1);

    if (CamxResultSuccess == result && NULL != pData[0])
    {
        *pROIInfo = *(static_cast<TrackerROIInformation*>(pData[0]));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::DumpTrackerYUV
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::DumpTrackerYUV(
    UINT64 requestId,
    ImageBuffer* pImageBuffer)
{
    BYTE*   pVirtualAddress1                 = NULL;
    BYTE*   pVirtualAddress2                 = NULL;
    CamxResult result                        = CamxResultSuccess;
    CHAR  inputFilename[FILENAME_MAX];
    FILE*  pInputFileFD;

    if (pImageBuffer != NULL)
    {
        pVirtualAddress1 = pImageBuffer->GetPlaneVirtualAddr(0, 0);
        pVirtualAddress2 = pImageBuffer->GetPlaneVirtualAddr(0, 1);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupTracker, "invalid args. req:%llu, pImageBuffer=%d ",
            requestId,
            pImageBuffer);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        OsUtils::SNPrintF(inputFilename, sizeof(inputFilename),
            "%s%st2t_%d_%s_input_width_%d_height_%d_stride_%d_scanline_%d_req_%llu.yuv",
            FileDumpPath, PathSeparator, InstanceID(), GetPipelineName(),
            m_trackerBufDimension.width, m_trackerBufDimension.height, m_trackerFrameStride, m_trackerFrameScanline, requestId);

        pInputFileFD = OsUtils::FOpen(inputFilename, "wb");

        if (pInputFileFD != NULL && pVirtualAddress1 != NULL && pVirtualAddress2 != NULL)
        {
            OsUtils::FWrite(pVirtualAddress1,
                m_trackerBufDimension.width * m_trackerBufDimension.height,
                1,
                pInputFileFD);

            OsUtils::FWrite(pVirtualAddress2,
                m_trackerBufDimension.width * m_trackerBufDimension.height / 2,
                1,
                pInputFileFD);

            OsUtils::FClose(pInputFileFD);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupTracker, "virtual address, inputfile is null");
            result = CamxResultEInvalidPointer;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupTracker, "DumpTrackerYUV failed. Result: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::PrepareThreadData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::PrepareThreadData(
    ExecuteProcessRequestData* pExecuteProcessRequestData,
    ImageBuffer* pTrackBuffer)
{
    CamxResult              result           = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT64                  requestId        = pNodeRequestData->pCaptureRequest->requestId;
    UINT                    internalIndex    = requestId % NumOfTrackerNodeThreadData;

    if (NULL != m_pThreadData[internalIndex]->pImgBuf)
    {
        CAMX_LOG_WARN(CamxLogGroupTracker, "Potential overflow on thread queue, skip. ReqId %llu", requestId);
        result = CamxResultEBusy;
    }
    else
    {
        m_pThreadData[internalIndex]->pNode         = this;
        m_pThreadData[internalIndex]->requestId     = requestId;
        m_pThreadData[internalIndex]->triggerCmd    = m_trackerTriggerCmd;
        m_pThreadData[internalIndex]->registerROI   = m_trackerRegisterROI;
        m_pThreadData[internalIndex]->pImgBuf       = pTrackBuffer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::GetFrameCropInfo()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::GetFrameCropInfo()
{
    CamxResult      result                   = CamxResultSuccess;
    const UINT      Length                   = CAMX_ARRAY_SIZE(FrameCropVendorTags);
    UINT            configTag[Length]        = { 0, 0, 0 };
    VOID*           pData[Length]            = { NULL, NULL, NULL };
    UINT64          configDataOffset[Length] = { 0 };
    CHIRectangle    appliedCropFullpath      = { 0 };
    CHIRectangle    residualCropFD           = { 0 };
    CHIRectangle    residualCropPreview      = { 0 };

    result = VendorTagManager::QueryVendorTagLocation(FrameCropVendorTags[0].pSectionName,
                                                        FrameCropVendorTags[0].pTagName, &configTag[0]);
    result |= VendorTagManager::QueryVendorTagLocation(FrameCropVendorTags[1].pSectionName,
                                                        FrameCropVendorTags[1].pTagName, &configTag[1]);
    result |= VendorTagManager::QueryVendorTagLocation(FrameCropVendorTags[2].pSectionName,
                                                        FrameCropVendorTags[2].pTagName, &configTag[2]);

    if (CamxResultSuccess == result)
    {
        result = GetDataList(configTag, &pData[0], configDataOffset, Length);

        if (CamxResultSuccess == result && NULL != pData[0])
        {
            m_appliedCropFD         = (reinterpret_cast<IFECropInfo*>(pData[0]))->FDPath;
            m_appliedCropPreview    = (reinterpret_cast<IFECropInfo*>(pData[0]))->displayFullPath;
            appliedCropFullpath     = (reinterpret_cast<IFECropInfo*>(pData[0]))->fullPath;
            if (0 == m_appliedCropPreview.width || 0 == m_appliedCropPreview.height)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupTracker, "DisplayFullpath %d %d %dx%d, Fullpath %d %d %dx%d",
                    m_appliedCropPreview.left, m_appliedCropPreview.top,
                    m_appliedCropPreview.width, m_appliedCropPreview.height,
                    appliedCropFullpath.left, appliedCropFullpath.top,
                    appliedCropFullpath.width, appliedCropFullpath.height);
                m_appliedCropPreview = appliedCropFullpath;
            }


            if (FALSE == m_aspectRatioCalibrated)
            {
                residualCropFD      = (reinterpret_cast<IFECropInfo*>(pData[1]))->FDPath;
                residualCropPreview = (reinterpret_cast<IFECropInfo*>(pData[1]))->displayFullPath;
                if (0 == residualCropPreview.width || 0 == residualCropPreview.height)
                {
                    residualCropPreview = (reinterpret_cast<IFECropInfo*>(pData[1]))->fullPath;
                    CAMX_LOG_INFO(CamxLogGroupTracker, "CamId %d, Residual Crop none for display path", m_cameraInfo.cameraId);

                    if (0 == residualCropPreview.width || 0 == residualCropPreview.height)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupTracker, "CamId %d, residual crop is none", m_cameraInfo.cameraId);
                        result = CamxResultEUnableToLoad;
                    }
                }

                CAMX_LOG_INFO(CamxLogGroupTracker, "CamId %d, FD Applied %d %d %dx%d, residual %d %d %dx%d",
                    m_cameraInfo.cameraId,
                    m_appliedCropFD.left, m_appliedCropFD.top, m_appliedCropFD.width, m_appliedCropFD.height,
                    residualCropFD.left, residualCropFD.top, residualCropFD.width, residualCropFD.height);

                CAMX_LOG_INFO(CamxLogGroupTracker, "CamId %d, Preview Applied crop %d %d %dx%d, residual %d %d %dx%d",
                    m_cameraInfo.cameraId,
                    m_appliedCropPreview.left, m_appliedCropPreview.top,
                    m_appliedCropPreview.width, m_appliedCropPreview.height,
                    residualCropPreview.left, residualCropPreview.top, residualCropPreview.width, residualCropPreview.height);

                m_aspectRatioOffset.left    = m_appliedCropPreview.left;
                m_aspectRatioOffset.top     = m_appliedCropPreview.top;
                m_aspectRatioOffset.width   = m_appliedCropPreview.width;
                m_aspectRatioOffset.height  = m_appliedCropPreview.height;
                m_aspectRatioCalibrated     = TRUE;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupTracker, "Error getting input frame map information");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupTracker, "Error getting IFE CropInfo vendor tag location %d", result);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupTracker, "FD Applied crop (%d %d), %dx%d, CamID %d",
        m_appliedCropFD.left, m_appliedCropFD.top, m_appliedCropFD.width, m_appliedCropFD.height, m_cameraInfo.cameraId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::IsNodeStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TrackerNode::IsNodeStreamOff()
{
    return m_isNodeStreamedOff;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::TrackerThreadCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* TrackerNode::TrackerThreadCb(
    VOID* pArg)
{
    TrackerAlgoInput        trackerAlgoInputs[TrackerInputTypeCount];       ///< algo input array
    TrackerAlgoInputList    trackerAlgoInputList;                           ///< algo input array list
    TrackerAlgoOutput       trackerAlgoOutputs[TrackerOutputTypeCount];     ///< algo output
    TrackerAlgoOutputList   trackerAlgoOutputList;                          ///< algo output list
    CamxResult              result       = CamxResultSuccess;
    TrackerThreadData*      pThreadData  = NULL;
    TrackerNode*            pTrackerNode = NULL;
    UINT64                  requestId;
    UINT32                  cameraId;
    BYTE*                   pPlaneBuffer[2]     = { NULL, NULL };
    UINT                    planeSize[2]        = { 0, 0 };
    BYTE*                   pImageBufferBytes   = NULL;

    trackerAlgoInputList.inputCount       = TrackerInputTypeCount;
    trackerAlgoInputList.pTrackerInputs   = &trackerAlgoInputs[0];
    trackerAlgoOutputList.outputCount     = TrackerOutputTypeCount;
    trackerAlgoOutputList.pTrackerOutputs = &trackerAlgoOutputs[0];

    // profiling time
    CamxTime startTime;
    CamxTime endTime;
    CamX::OsUtils::GetTime(&startTime);

    if (NULL == pArg)
    {
        CAMX_LOG_ERROR(CamxLogGroupTracker, "NULL arg in TrackerThreadCb");
        result = CamxResultEInvalidArg;
    }
    else
    {
        pThreadData  = static_cast<TrackerThreadData*>(pArg);
        pTrackerNode = static_cast<TrackerNode*>(pThreadData->pNode);
        requestId    = pThreadData->requestId;
        cameraId     = pTrackerNode->m_cameraInfo.cameraId;

        if (NULL == pTrackerNode->m_pTrackerAlgorithm->TrackerProcess)
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupTracker, "Null pointer TrackerProcess");
        }
        else if (NULL == pThreadData->pImgBuf)
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupTracker, "Null pointer Image buffer.");
        }
        else if (FALSE == pTrackerNode->IsNodeStreamOff())
        {
            pPlaneBuffer[0] = pThreadData->pImgBuf->GetPlaneVirtualAddr(0, 0);
            pPlaneBuffer[1] = pThreadData->pImgBuf->GetPlaneVirtualAddr(0, 1);
            planeSize[0]    = static_cast<UINT>(pThreadData->pImgBuf->GetPlaneSize(0));
            planeSize[1]    = static_cast<UINT>(pThreadData->pImgBuf->GetPlaneSize(1));

            if (NULL != pPlaneBuffer[0] && NULL != pPlaneBuffer[1])
            {
                if (planeSize[0] != pPlaneBuffer[1] - pPlaneBuffer[0])
                {
                    CAMX_LOG_WARN(CamxLogGroupTracker, "Not contiguous memory planes, need mem allocation");
                    if (MaxTrackerBufferSize < planeSize[0] + planeSize[1])
                    {
                        CAMX_LOG_WARN(CamxLogGroupTracker, "Unbounded image buffer size. Expected max %d, actual %d",
                            MaxTrackerBufferSize, planeSize[0] + planeSize[1]);
                    }
                    pImageBufferBytes = reinterpret_cast<BYTE*>(CAMX_CALLOC(planeSize[0] + planeSize[1]));
                    if (NULL == pImageBufferBytes)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupTracker, "Memory allocation failed");
                        result = CamxResultENoMemory;
                    }
                    else
                    {
                        Utils::Memcpy(pImageBufferBytes, pPlaneBuffer[0], planeSize[0]);
                        Utils::Memcpy(pImageBufferBytes + planeSize[0], pPlaneBuffer[1], planeSize[1]);
                        pPlaneBuffer[0] = pImageBufferBytes;
                    }
                }
            }
            else
            {
                result = CamxResultEInvalidPointer;
                CAMX_LOG_ERROR(CamxLogGroupTracker, "Image buffer plane address NULL [0]=%p, [1]=%p",
                    pPlaneBuffer[0], pPlaneBuffer[1]);
            }
        }

        if (CamxResultSuccess == result && FALSE == pTrackerNode->IsNodeStreamOff())
        {
            // Algorithm output
            trackerAlgoOutputs[TrackerOutputTypeTrackROIResult].pTrackerOutput  = &pTrackerNode->m_trackerResultsFromAlgo;
            trackerAlgoOutputs[TrackerOutputTypeTrackROIResult].outputType      = TrackerOutputTypeTrackROIResult;
            trackerAlgoOutputs[TrackerOutputTypeTrackROIResult].sizeOfTrackerOutput         = sizeof(TrackerResults);
            trackerAlgoOutputs[TrackerOutputTypeTrackROIResult].sizeOfWrittenTrackerOutput  = 0;

            // CameraID
            trackerAlgoInputs[TrackerInputTypeCameraID].inputType          = TrackerInputTypeCameraID;
            trackerAlgoInputs[TrackerInputTypeCameraID].pTrackerInput      = &cameraId;
            trackerAlgoInputs[TrackerInputTypeCameraID].sizeOfTrackerInput = sizeof(UINT32);

            // ReqID
            trackerAlgoInputs[TrackerInputTypeRequestNumber].inputType      = TrackerInputTypeRequestNumber;
            trackerAlgoInputs[TrackerInputTypeRequestNumber].pTrackerInput  = &requestId;
            trackerAlgoInputs[TrackerInputTypeRequestNumber].sizeOfTrackerInput = sizeof(UINT64);

            // CMD
            trackerAlgoInputs[TrackerInputTypeCMDType].inputType          = TrackerInputTypeCMDType;
            trackerAlgoInputs[TrackerInputTypeCMDType].pTrackerInput      = &pThreadData->triggerCmd;
            trackerAlgoInputs[TrackerInputTypeCMDType].sizeOfTrackerInput = sizeof(TrackerCMDType);

            // Register ROI
            trackerAlgoInputs[TrackerInputTypeRegisterROI].inputType          = TrackerInputTypeRegisterROI;
            trackerAlgoInputs[TrackerInputTypeRegisterROI].pTrackerInput      = &pThreadData->registerROI;
            trackerAlgoInputs[TrackerInputTypeRegisterROI].sizeOfTrackerInput = sizeof(CHIRectangle);

            // ImageBuffer
            trackerAlgoInputs[TrackerInputTypeImgBuf].inputType          = TrackerInputTypeImgBuf;
            trackerAlgoInputs[TrackerInputTypeImgBuf].pTrackerInput      = pPlaneBuffer[0];
            trackerAlgoInputs[TrackerInputTypeImgBuf].sizeOfTrackerInput = sizeof(MaxTrackerBufferSize);

            pTrackerNode->m_pLock->Lock();
            result = pTrackerNode->m_pTrackerAlgorithm->TrackerProcess(
                                        pTrackerNode->m_pTrackerAlgorithm, &trackerAlgoInputList, &trackerAlgoOutputList);
            pTrackerNode->m_pLock->Unlock();

            pThreadData->pImgBuf = NULL;
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupTracker, "Error from TrackerJobHandler result: %s", CamxResultStrings[result]);
        }

        if (NULL != pImageBufferBytes)
        {
            CAMX_FREE(pImageBufferBytes);
        }

        pTrackerNode->ProcessRequestIdDone(requestId);  /// Notify request ID done in thread

        CamX::OsUtils::GetTime(&endTime);
        CAMX_LOG_VERBOSE(CamxLogGroupTracker, "Tracker Algorithm Cb camId:%d, reqID:%llu, cmd:%d, state %d, time used: %d ms",
            cameraId, requestId, pThreadData->triggerCmd, pTrackerNode->m_trackerResultsFromAlgo.trackerAlgoState,
            CamX::OsUtils::CamxTimeToMillis(&endTime) - CamX::OsUtils::CamxTimeToMillis(&startTime));
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::CreateTrackerAlgorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::CreateTrackerAlgorithm(
    ExecuteProcessRequestData*  pExecuteProcessRequestData)
{
    CamxResult                  result          = CamxResultSuccess;
    PerRequestInputPortInfo*    pInputPorts     = pExecuteProcessRequestData->pEnabledPortsInfo->pInputPorts;
    TrackerAlgoCreateParamList  createParamList = { 0 };
    ImageBuffer*                pTrackBuffer    = NULL;
    TrackerAlgoCreateParam      createParams[TrackerCreateParamsTypeCount];
    TrackerAlgoSetParamList     setParamList    = { 0 };
    TrackerAlgoSetParam         setParams[TrackerSetParamTypeCount];

    createParamList.createParamsCount = TrackerCreateParamsTypeCount;
    createParamList.pCreateParams     = &createParams[0];
    setParamList.inputParamsCount     = TrackerSetParamTypeCount;
    setParamList.pTrackerSetParams    = &setParams[0];

    UINT32                      numInputPorts = pExecuteProcessRequestData->pEnabledPortsInfo->numInputPorts;
    TrackerBufFormatDimension   createAlgoBufFmtDim;

    for (UINT32 i = 0; i < numInputPorts; i++)
    {
        if (TrackerInputPortImage == pInputPorts[i].portId)
        {
            if (NULL != pInputPorts[i].pImageBuffer)
            {
                pTrackBuffer = pInputPorts[i].pImageBuffer;

                YUVFormat yuvFormat[FormatsMaxPlanes] = { { 0 } };
                if (NULL != pTrackBuffer->GetFormat())
                {
                    yuvFormat[0]                 = pTrackBuffer->GetFormat()->formatParams.yuvFormat[0];
                    m_trackerBufDimension.width  = yuvFormat[0].width;
                    m_trackerBufDimension.height = yuvFormat[0].height;
                    m_trackerFrameStride         = yuvFormat[0].planeStride;
                    m_trackerFrameScanline       = yuvFormat[0].sliceHeight;

                    createAlgoBufFmtDim.imgBufFmt        = static_cast<ChiBufferFormat>(pTrackBuffer->GetFormat()->format);
                    createAlgoBufFmtDim.imgBufDim.width  = yuvFormat[0].width;
                    createAlgoBufFmtDim.imgBufDim.height = yuvFormat[0].height;
                    createAlgoBufFmtDim.planeStride      = yuvFormat[0].planeStride;
                    createAlgoBufFmtDim.scaleDownRatio   = m_trackerImgBufDownscaleRatio;
                    createAlgoBufFmtDim.precisionMode    = m_trackerPrecisionMode;
                    createAlgoBufFmtDim.operationMode    = m_trackerOperationMode;

                    CAMX_LOG_VERBOSE(CamxLogGroupTracker,
                        "Image buffer width %d,  Image buffer height %d, "
                        "FrameStride %d, FrameScanline %d, scaleDown %d, format %d",
                        m_trackerBufDimension.width, m_trackerBufDimension.height,
                        m_trackerFrameStride, m_trackerFrameScanline,
                        createAlgoBufFmtDim.scaleDownRatio, createAlgoBufFmtDim.imgBufFmt);

                    yuvFormat[1] = pTrackBuffer->GetFormat()->formatParams.yuvFormat[1];
                    yuvFormat[2] = pTrackBuffer->GetFormat()->formatParams.yuvFormat[2];
                    CAMX_LOG_VERBOSE(CamxLogGroupTracker, "Plane0 width:%d height:%d stride:%d sliceHeight%d",
                        yuvFormat[0].width, yuvFormat[0].height, yuvFormat[0].planeStride, yuvFormat[0].sliceHeight);
                    CAMX_LOG_VERBOSE(CamxLogGroupTracker, "Plane1 width:%d height:%d stride:%d sliceHeight%d",
                        yuvFormat[1].width, yuvFormat[1].height, yuvFormat[1].planeStride, yuvFormat[1].sliceHeight);
                    CAMX_LOG_VERBOSE(CamxLogGroupTracker, "Plane2 width:%d height:%d stride:%d sliceHeight%d",
                        yuvFormat[2].width, yuvFormat[2].height, yuvFormat[2].planeStride, yuvFormat[2].sliceHeight);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupTracker, "Input Port[%d] Image format is NULL", pInputPorts[i]);
                    result = CamxResultEInvalidPointer;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupTracker, "Input Port[%d] Image buffer is NULL", pInputPorts[i]);
                result = CamxResultEInvalidPointer;
            }
        }
    }
    if (CamxResultSuccess == result)
    {
        createParams[TrackerCreateParamsTypeBufFmtDim].createParamType   = TrackerCreateParamsTypeBufFmtDim;
        createParams[TrackerCreateParamsTypeBufFmtDim].pCreateParam      = &createAlgoBufFmtDim;
        createParams[TrackerCreateParamsTypeBufFmtDim].sizeOfCreateParam = sizeof(TrackerBufFormatDimension);

        createParams[TrackerCreateParamsTypeLogger].createParamType   = TrackerCreateParamsTypeLogger;
        createParams[TrackerCreateParamsTypeLogger].pCreateParam      = reinterpret_cast<VOID*>(&StatsLoggerFunction);
        createParams[TrackerCreateParamsTypeLogger].sizeOfCreateParam = sizeof(StatsLoggingFunction);

        createParams[TrackerCreateParamsTypeCameraInfo].createParamType   = TrackerCreateParamsTypeCameraInfo;
        createParams[TrackerCreateParamsTypeCameraInfo].pCreateParam      = &m_cameraInfo;
        createParams[TrackerCreateParamsTypeCameraInfo].sizeOfCreateParam = sizeof(StatsCameraInfo);

        result = m_statsInitializeData.initializecallback.pTrackerCallback->pfnCreate(
            &createParamList, &m_pTrackerAlgorithm);
    }
    if (CamxResultSuccess == result)
    {
        UINT setParamMinimumConfidence                               =
            static_cast<UINT>(GetStaticSettings()->t2tConfidenceThreshold);
        setParamMinimumConfidence                                    = Utils::MinUINT(setParamMinimumConfidence, 100);
        setParams[TrackerSetParamTypeMinConfidence].setParamType     = TrackerSetParamTypeMinConfidence;
        setParams[TrackerSetParamTypeMinConfidence].sizeOfInputParam = sizeof(UINT);
        setParams[TrackerSetParamTypeMinConfidence].pTrackerSetParam = &setParamMinimumConfidence;

        result = m_pTrackerAlgorithm->TrackerSetParam(m_pTrackerAlgorithm, &setParamList);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::ExecuteProcessRequest(
    ExecuteProcessRequestData*  pExecuteProcessRequestData)
{
    CAMX_ASSERT(NULL != pExecuteProcessRequestData);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pNodeProcessRequestData);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest);

    CamxResult                  result             = CamxResultSuccess;
    NodeProcessRequestData*     pNodeRequestData   = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT64                      requestId          = pNodeRequestData->pCaptureRequest->requestId;
    UINT32                      internalIndex      = requestId % NumOfTrackerNodeThreadData;
    TrackerBufferDependencyType dependencySequence = static_cast<TrackerBufferDependencyType>(
                                                            pNodeRequestData->processSequenceId);

    ImageBuffer*                pTrackBuffer            = NULL;
    CHIRectangle                referenceFrameROI       = { 0 };
    BOOL                        isMaster                = FALSE;
    BOOL                        isMultiCameraUsecase    = FALSE;
    UINT                        numOfCameras            = 0;
    UINT                        multiCamCameraId        = 0;
    BOOL                        bIsThreadSkipped        = FALSE;

    if (NULL != pExecuteProcessRequestData->pEnabledPortsInfo)
    {
        PerRequestInputPortInfo*   pInputPorts   = pExecuteProcessRequestData->pEnabledPortsInfo->pInputPorts;
        if (AddDependencies == dependencySequence)
        {
            result = UpdateT2TFeatureEnable();

            if (FALSE == m_trackingEnable)
            {
               // Skip processing if tracking disabled by static settings
                NotifyJobProcessRequestDone(requestId);
                return CamxResultSuccess;
            }
            else if (TRUE == CanSkipAlgoProcessing(requestId))
            {
                // T2T enabled, but Skip processing based on skip factor
                m_trackerROIInfo.requestId  = requestId;
                result                      = PublishTrackerResultsToVendorTag();
                CAMX_LOG_VERBOSE(CamxLogGroupTracker, "Frame skipped for camera %d req %llu", m_cameraInfo.cameraId, requestId);
                NotifyJobProcessRequestDone(requestId);
                return result;
            }
            else
            {
                result = GetMultiCameraInfo(&isMultiCameraUsecase, &numOfCameras, &multiCamCameraId, &isMaster);
                if (FALSE == isMultiCameraUsecase) ///< Single camera
                {
                    m_cameraInfo.algoRole = StatsAlgoRoleDefault;
                }
                else if (TRUE == isMaster) ///< Multicam master
                {
                    m_cameraInfo.algoRole = StatsAlgoRoleMaster;
                }
                else    ///< Multicam slave
                {
                    m_cameraInfo.algoRole = StatsAlgoRoleSlave;
                }

                if (CamxResultSuccess == result &&
                    (StatsAlgoRoleMaster == m_cameraInfo.algoRole || StatsAlgoRoleDefault == m_cameraInfo.algoRole))
                {
                    ///< Only put dependency if it's master cam or single cam
                    result = GetBufferDependencies(pExecuteProcessRequestData);
                }
                else
                {
                    ///< Slave camera, skip putting dependency
                    CAMX_LOG_VERBOSE(CamxLogGroupTracker, "Frame skipped for camera %d req %llu, algoRole %d,"
                        "multiCam %d isMaster %d",
                        m_cameraInfo.cameraId, requestId, m_cameraInfo.algoRole, isMultiCameraUsecase, isMaster);
                    NotifyJobProcessRequestDone(pNodeRequestData->pCaptureRequest->requestId);
                }
            }
        }
        else if (TrackerImageBufDependencyMet == dependencySequence)
        {
            result =  GetInputFromVendortag();

            if (NULL == m_pTrackerAlgorithm && CamxResultSuccess == result)
            {
                // Creating Algorithm
                result = CreateTrackerAlgorithm(pExecuteProcessRequestData);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupTracker, "Tracker Algorithm creation fail");
                }
            }

            m_trackerROIInfo.requestId         = requestId;
            m_trackerROIInfo.ROICount          = 0;
            m_trackerROIInfo.ROI[0].id         = 0;
            m_trackerROIInfo.ROI[0].confidence = 0;
            m_trackerROIInfo.ROI[0].rect       = { 0, 0, 0, 0 };
            if (CamxResultSuccess == result)
            {
                // get the input img buffer from IFE
                for (UINT i = 0; i < pExecuteProcessRequestData->pEnabledPortsInfo->numInputPorts; i++)
                {
                    if (TrackerInputPortImage == pInputPorts[i].portId)
                    {
                        if ((NULL != pInputPorts[i].pImageBuffer) &&
                                (NULL != pInputPorts[i].pImageBuffer->GetPlaneVirtualAddr(0, 0)))
                        {
                            pTrackBuffer = pInputPorts[i].pImageBuffer;
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupTracker, "InputPort %p portId %d pImageBuffer %p", pInputPorts[i],
                                pInputPorts[i].portId, pInputPorts[i].pImageBuffer);
                            result = CamxResultEInvalidArg;
                        }
                    }
                }
                if (CamxResultSuccess == result)
                {
                    result = GetFrameCropInfo();
                }
                if (CamxResultSuccess == result && NULL != pTrackBuffer)
                {
                    if (NumOfTrackerNodeThreadData > m_statsInitializeData.pThreadManager->GetJobCount(m_hThread) &&
                        CamxResultSuccess ==  PrepareThreadData(pExecuteProcessRequestData, pTrackBuffer))
                    {
                        result = m_statsInitializeData.pThreadManager->PostJob(
                            m_hThread, NULL, reinterpret_cast<VOID**>(&m_pThreadData[internalIndex]), FALSE, FALSE);
                    }
                    else
                    {
                        CAMX_LOG_WARN(CamxLogGroupTracker, "Tracker job queue full, skip, reqId %llu", requestId);
                        bIsThreadSkipped = TRUE;
                    }

                    if (TrackerStateTracking == m_trackerResultsFromAlgo.trackerAlgoState &&
                        TrackerCMDReg != m_trackerTriggerCmd && TrackerCMDCancel != m_trackerTriggerCmd)
                    {
                        // Publishing most recent result
                        referenceFrameROI = Translator::ConvertROIFromCurrentToReference(
                            &m_sensorDimension, &m_trackerBufDimension, &m_appliedCropFD,
                            reinterpret_cast<CHIRectangle*>(&m_trackerResultsFromAlgo.statsRectROI));

                        m_trackerROIInfo.ROICount           = 1;
                        m_trackerROIInfo.ROI[0].id          = 1;
                        m_trackerROIInfo.ROI[0].confidence  = m_trackerResultsFromAlgo.confidence;
                        m_trackerROIInfo.ROI[0].rect.left   = referenceFrameROI.left;
                        m_trackerROIInfo.ROI[0].rect.top    = referenceFrameROI.top;
                        m_trackerROIInfo.ROI[0].rect.width  = referenceFrameROI.width;
                        m_trackerROIInfo.ROI[0].rect.height = referenceFrameROI.height;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupTracker, "Input buffer is NULL");
                    result = CamxResultEFailed;
                }
            }
            result = PublishTrackerResultstoUsecasePool();
            result |= PublishTrackerResultsToVendorTag();
            ProcessMetadataDone(requestId);
            if (TRUE == bIsThreadSkipped)
            {
                // If we did not put thread on queue, we have to notify request done here
                ProcessRequestIdDone(requestId);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupTracker, "Enabled Ports are NULL");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::OnStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::OnStreamOn()
{
    CamxResult result = CamxResultSuccess;
    // Restart JOB after flush
    if ((InvalidJobHandle != m_hThread) &&
        (NULL != m_statsInitializeData.pThreadManager))
    {
        m_statsInitializeData.pThreadManager->ResumeJobFamily(m_hThread);
    }

    if (NULL != m_pLock)
    {
        m_pLock->Lock();
    }

    m_isNodeStreamedOff = FALSE;

    if (NULL != m_pLock)
    {
        m_pLock->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::OnStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::OnStreamOff(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(modeBitmask);

    if (NULL != m_pLock)
    {
        m_pLock->Lock();
    }

    m_isNodeStreamedOff = TRUE;

    if (NULL != m_pLock)
    {
        m_pLock->Unlock();
    }

    // Flush the pending jobs now
    if ((InvalidJobHandle != m_hThread) &&
        (NULL != m_statsInitializeData.pThreadManager))
    {
        m_statsInitializeData.pThreadManager->FlushJobFamily(m_hThread, this, TRUE);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TrackerNode::NotifyJobProcessRequestDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TrackerNode::NotifyJobProcessRequestDone(
    UINT64 requestID)
{
    ProcessMetadataDone(requestID);
    ProcessRequestIdDone(requestID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result   = CamxResultSuccess;
    UINT32     tagCount = 0;
    UINT32     tagID;

    for (UINT32 tagIndex = 0; tagIndex < CAMX_ARRAY_SIZE(OutputVendorTags); ++tagIndex)
    {
        result = VendorTagManager::QueryVendorTagLocation(
            OutputVendorTags[tagIndex].pSectionName,
            OutputVendorTags[tagIndex].pTagName,
            &tagID);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                OutputVendorTags[tagIndex].pSectionName,
                OutputVendorTags[tagIndex].pTagName);
            break;
        }
        else
        {
            pPublistTagList->tagArray[tagCount++] = tagID;
        }
    }

    pPublistTagList->tagCount = tagCount;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::UpdateT2TFeatureEnable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::UpdateT2TFeatureEnable()
{
    CamxResult  result = CamxResultSuccess;
    VOID*       pData  = NULL;

    if (FALSE == m_staticSettingsTrackingEnable)
    {
        // Static settings overrides vendortag enablement
        m_trackingEnable = FALSE;
    }
    else
    {

        result = StatsUtil::ReadVendorTag(this, InputVendorTags[0].pSectionName, InputVendorTags[0].pTagName, &pData);
        if (CamxResultSuccess == result)
        {
            if (NULL != pData)
            {
                m_trackingEnable = *static_cast<BOOL*>(pData);
            }
            else
            {
                m_trackingEnable = FALSE;
                CAMX_LOG_ERROR(CamxLogGroupTracker, "Property data NULL", CamxResultStrings[result]);
                result = CamxResultENoSuch;
            }
        }
        else
        {
            m_trackingEnable = FALSE;
            CAMX_LOG_WARN(CamxLogGroupTracker, "Cannot get vendortag %s", InputVendorTags[0].pTagName);
            result = CamxResultENotImplemented;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::GetInputFromVendortag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::GetInputFromVendortag()
{
    CamxResult              result                      = CamxResultSuccess;
    TrackerCMDType          currentCmd                  = TrackerCMDTrack;
    VOID*                   pData[2]                    = { 0 };
    CHIRectangle            regROI                      = { 0 };

    result = StatsUtil::ReadVendorTag(this, InputVendorTags[1].pSectionName, InputVendorTags[1].pTagName, &pData[0]);
    result |= StatsUtil::ReadVendorTag(this, InputVendorTags[2].pSectionName, InputVendorTags[2].pTagName, &pData[1]);

    if (CamxResultSuccess == result)
    {
        if (NULL != pData[0])
        {
            currentCmd = static_cast<TrackerCMDType>(*static_cast<INT32*>(pData[0]));
        }
        else
        {
            currentCmd = TrackerCMDTrack;
        }
        if (NULL != pData[1])
        {
            regROI = *static_cast<CHIRectangle*>(pData[1]);

            if (TrackerCMDReg == currentCmd)
            {
                CAMX_LOG_INFO(CamxLogGroupTracker, "From vendortag Register ROI %d %d %d %d, cameraId %d",
                    regROI.left, regROI.top, regROI.width, regROI.height, m_cameraInfo.cameraId);
            }

            /// Upper layer is not reporting correct dimension so need compensation
            regROI.left     = regROI.left * m_aspectRatioOffset.width / m_sensorActiveArrayDimension.width
                                + m_aspectRatioOffset.left;
            regROI.top      = regROI.top * m_aspectRatioOffset.height / m_sensorActiveArrayDimension.height
                                + m_aspectRatioOffset.top;
            regROI.width    = regROI.width * m_aspectRatioOffset.width / m_sensorActiveArrayDimension.width;
            regROI.height   = regROI.height * m_aspectRatioOffset.height / m_sensorActiveArrayDimension.height;

            regROI.width    = Utils::MinUINT32(regROI.width, regROI.height);
            regROI.width    = regROI.height;

            if (TrackerCMDReg == currentCmd)
            {
                CAMX_LOG_INFO(CamxLogGroupTracker, "Calibrated Register ROI %d %d %d %d, cameraId %d",
                    regROI.left, regROI.top, regROI.width, regROI.height, m_cameraInfo.cameraId);
            }

            CHIRectangle currentFrameROI = Translator::ConvertROIFromReferenceToCurrent(
            &m_trackerBufDimension, &m_appliedCropFD, &regROI);

            m_trackerRegisterROI.left   = currentFrameROI.left;
            m_trackerRegisterROI.top    = currentFrameROI.top;
            m_trackerRegisterROI.width  = currentFrameROI.width;
            m_trackerRegisterROI.height = currentFrameROI.height;

        }

        if (currentCmd == m_previousIncomingCmd &&
            m_trackerRegisterROI.left   == m_previousRegistrationROI.left &&
            m_trackerRegisterROI.top    == m_previousRegistrationROI.top &&
            m_trackerRegisterROI.width  == m_previousRegistrationROI.width &&
            m_trackerRegisterROI.height == m_previousRegistrationROI.height)
        {
            m_trackerTriggerCmd = TrackerCMDTrack;
        }
        else
        {
            m_trackerTriggerCmd = currentCmd;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupTracker, "Tracking Enable:%d, Tracking Cmd:%d, current Cmd:%d, prev Cmd:%d, camdId %d",
            m_trackingEnable, m_trackerTriggerCmd, currentCmd, m_previousIncomingCmd, m_cameraInfo.cameraId);

        m_previousIncomingCmd               = currentCmd;
        m_previousRegistrationROI.left      = m_trackerRegisterROI.left;
        m_previousRegistrationROI.top       = m_trackerRegisterROI.top;
        m_previousRegistrationROI.width     = m_trackerRegisterROI.width;
        m_previousRegistrationROI.height    = m_trackerRegisterROI.height;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupTracker, "Non-fatal: Cannot get tracking ROI info from application. Result: %s camId %d",
                            CamxResultStrings[result], m_cameraInfo.cameraId);
        result = CamxResultSuccess;     /// CMD and ROI can be missing for certain frames
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::PublishTrackerResultstoUsecasePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::PublishTrackerResultstoUsecasePool()
{
    CamxResult  result          = CamxResultSuccess;
    const UINT  outputTags[]    = { PropertyIDUsecaseTrackerResults };
    const VOID* pOutputData[]   = { &m_trackerROIInfo };
    const UINT  pDataCount[]    = { sizeof(TrackerROIInformation) };

    result = WriteDataList(outputTags, &pOutputData[0], pDataCount, 1);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupTracker, "Falied to publish Tracker results to uscasepool");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::PublishTrackerResultsToVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TrackerNode::PublishTrackerResultsToVendorTag()
{
    CamxResult              result                  = CamxResultSuccess;
    const VOID*             pData[3]                = { 0 };
    UINT                    dataCount[3]            = { 1, 4, 1 };
    TrackerROIInformation   trackerROIInfoVendorTag = { 0 };
    CHIRectangle            calibratedROI           = { 0 };

    trackerROIInfoVendorTag.ROICount            = m_trackerROIInfo.ROICount;;
    trackerROIInfoVendorTag.ROI[0].id           = m_trackerROIInfo.ROI[0].id;
    trackerROIInfoVendorTag.ROI[0].confidence   = m_trackerROIInfo.ROI[0].confidence;

    if (0 != m_trackerROIInfo.ROI[0].rect.width && 0 != m_trackerROIInfo.ROI[0].rect.height)
    {
        calibratedROI.left   =
            static_cast<INT>(m_trackerROIInfo.ROI[0].rect.left) - static_cast<INT>(m_aspectRatioOffset.left);
        calibratedROI.top    =
            static_cast<INT>(m_trackerROIInfo.ROI[0].rect.top) - static_cast<INT>(m_aspectRatioOffset.top);
        calibratedROI.width  = m_trackerROIInfo.ROI[0].rect.width;
        calibratedROI.height = m_trackerROIInfo.ROI[0].rect.height;

        if (0 > calibratedROI.left)
        {
            calibratedROI.width   = Utils::MaxINT32(calibratedROI.width + calibratedROI.left, 0);
            calibratedROI.left    = 0;
        }

        if (calibratedROI.left + calibratedROI.width > static_cast<INT>(m_aspectRatioOffset.width))
        {
            calibratedROI.width = m_aspectRatioOffset.width - calibratedROI.left;
        }

        if (0 > calibratedROI.top)
        {
            calibratedROI.height  = Utils::MaxINT32(calibratedROI.height + calibratedROI.top, 0);
            calibratedROI.top     = 0;
        }

        if (calibratedROI.top + calibratedROI.height > static_cast<INT>(m_aspectRatioOffset.height))
        {
            calibratedROI.height = m_aspectRatioOffset.height - calibratedROI.top;
        }

        trackerROIInfoVendorTag.ROI[0].rect.left    = calibratedROI.left * m_sensorActiveArrayDimension.width
            / m_aspectRatioOffset.width;
        trackerROIInfoVendorTag.ROI[0].rect.width   = calibratedROI.width * m_sensorActiveArrayDimension.width
            / m_aspectRatioOffset.width;
        trackerROIInfoVendorTag.ROI[0].rect.top     = calibratedROI.top * m_sensorActiveArrayDimension.height
            / m_aspectRatioOffset.height;
        trackerROIInfoVendorTag.ROI[0].rect.height  = calibratedROI.height * m_sensorActiveArrayDimension.height
            / m_aspectRatioOffset.height;
    }

    pData[0] = &m_trackerResultsFromAlgo.trackerAlgoState;
    pData[1] = &trackerROIInfoVendorTag.ROI[0].rect;
    pData[2] = &trackerROIInfoVendorTag.ROI[0].confidence;

    result = StatsUtil::WriteVendorTag(this, OutputVendorTags[0].pSectionName, OutputVendorTags[0].pTagName,
                                        pData[0], dataCount[0]);
    result |= StatsUtil::WriteVendorTag(this, OutputVendorTags[1].pSectionName, OutputVendorTags[1].pTagName,
                                        pData[1], dataCount[1]);
    result |= StatsUtil::WriteVendorTag(this, OutputVendorTags[2].pSectionName, OutputVendorTags[2].pTagName,
                                        pData[2], dataCount[2]);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupTracker, "Failed to write T2T ROI to vendor tags. result %d", result);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupTracker, "T2T Roi published to vendortag CamId:%d, roi %d %d %dx%d, conf:%d",
            m_cameraInfo.cameraId, trackerROIInfoVendorTag.ROI[0].rect.left, trackerROIInfoVendorTag.ROI[0].rect.top,
            trackerROIInfoVendorTag.ROI[0].rect.width, trackerROIInfoVendorTag.ROI[0].rect.height,
            trackerROIInfoVendorTag.ROI[0].confidence);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TrackerNode::CanSkipAlgoProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TrackerNode::CanSkipAlgoProcessing(
    UINT64 requestId)
{
    UINT    skipFactor                      = m_skipPattern;
    UINT64  requestIdOffsetFromLastFlush    = GetRequestIdOffsetFromLastFlush(requestId);
    BOOL    skipRequired                    = FALSE;

    skipFactor      = (requestIdOffsetFromLastFlush <= (FirstValidRequestId + GetMaximumPipelineDelay())) ? 1 : skipFactor;
    skipRequired    = requestIdOffsetFromLastFlush % skipFactor != 0;

    return skipRequired;
}
CAMX_NAMESPACE_END
