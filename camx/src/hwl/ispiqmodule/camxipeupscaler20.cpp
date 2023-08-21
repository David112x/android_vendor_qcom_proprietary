////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeupscaler20.cpp
/// @brief CAMXIPEUpscaler class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxipeupscaler20.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "ipe_data.h"
#include "parametertuningtypes.h"
#include "camxtitan17xcontext.h"
#include "camxipeupscaler20titan17x.h"
#include "camxipeupscaler20titan480.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 IPEUpscalerLUTBufferSizeInDWord =
    IPEUpscaleLUTNumEntries[DMI_LUT_A] +
    IPEUpscaleLUTNumEntries[DMI_LUT_B] +
    IPEUpscaleLUTNumEntries[DMI_LUT_C] +
    IPEUpscaleLUTNumEntries[DMI_LUT_D];

static const UINT32 IPEUpscalerLUTBufferSize        =
    IPEUpscaleLUTSizes[DMI_LUT_A] +
    IPEUpscaleLUTSizes[DMI_LUT_B] +
    IPEUpscaleLUTSizes[DMI_LUT_C] +
    IPEUpscaleLUTSizes[DMI_LUT_D];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPEUpscaler20* pModule = CAMX_NEW IPEUpscaler20(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Module initialization failed");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Memory allocation failed");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null input pointer");
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    // Check Module Hardware Version and Create HW Setting Object
    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IPEUpscaler20Titan480;
            break;
        default:
            m_pHWSetting = CAMX_NEW IPEUpscaler20Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to create HW Setting Class, no memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20::FillCmdBufferManagerParams(
    const ISPInputData*     pInputData,
    IQModuleCmdBufferParam* pParam)
{
    CamxResult result                  = CamxResultSuccess;
    ResourceParams* pResourceParams    = NULL;
    CHAR*           pBufferManagerName = NULL;

    if ((NULL != pParam) && (NULL != pParam->pCmdBufManagerParam) && (NULL != pInputData))
    {
        // The Resource Params and Buffer Manager Name will be freed by caller Node
        pResourceParams = static_cast<ResourceParams*>(CAMX_CALLOC(sizeof(ResourceParams)));
        if (NULL != pResourceParams)
        {
            pBufferManagerName = static_cast<CHAR*>(CAMX_CALLOC((sizeof(CHAR) * MaxStringLength256)));
            if (NULL != pBufferManagerName)
            {
                OsUtils::SNPrintF(pBufferManagerName, (sizeof(CHAR) * MaxStringLength256), "CBM_%s_%s",
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "IPEUpscaler20");
                pResourceParams->resourceSize                 = IPEUpscalerLUTBufferSize;
                pResourceParams->poolSize                     = (pInputData->requestQueueDepth * pResourceParams->resourceSize);
                pResourceParams->usageFlags.cmdBuffer         = 1;
                pResourceParams->cmdParams.type               = CmdType::CDMDMI;
                pResourceParams->alignment                    = CamxCommandBufferAlignmentInBytes;
                pResourceParams->cmdParams.enableAddrPatching = 0;
                pResourceParams->cmdParams.maxNumNestedAddrs  = 0;
                pResourceParams->memFlags                     = CSLMemFlagUMDAccess;
                pResourceParams->pDeviceIndices               = pInputData->pipelineIPEData.pDeviceIndex;
                pResourceParams->numDevices                   = 1;

                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].pBufferManagerName = pBufferManagerName;
                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].pParams            = pResourceParams;
                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].ppCmdBufferManager = &m_pLUTCmdBufferManager;
                pParam->numberOfCmdBufManagers++;

            }
            else
            {
                result = CamxResultENoMemory;
                CAMX_FREE(pResourceParams);
                pResourceParams = NULL;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Out Of Memory");
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Out Of Memory");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input Error %p", pParam);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(upscale_2_0_0::upscale20_rgn_dataType) * (Upscale20MaxNoLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for upscale_2_0_0::upscale20_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEUpscaler20::UpdateIPEInternalData(
    ISPInputData* pInputData)
{
    CamxResult                result         = CamxResultSuccess;
    IpeIQSettings*            pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
    UpScalerParameters*       pUpscalerParam = &pIPEIQSettings->upscalerParameters;
    ChromaUpScalerParameters* pChromaUpParam = &pIPEIQSettings->chromaUpscalerParameters;

    if (NULL != m_pHWSetting)
    {
        m_pHWSetting->SetupInternalData(pUpscalerParam);
    }
    pChromaUpParam->chromaUpCosited = m_dependenceData.cosited;
    pChromaUpParam->chromaUpEven    = m_dependenceData.evenOdd;

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != m_pHWSetting))
    {
        IPEUpscaler20LUT* pUpscalerLUT = NULL;
        DebugDataTagID    dataTagID;

        result = m_pHWSetting->UpdateTuningMetadata(pInputData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Update Tuning Metadata failed.");
            result = CamxResultSuccess; // Non-fatal error
        }

        switch (pInputData->titanVersion)
        {
            case CSLCameraTitanVersion::CSLTitan480:
                pUpscalerLUT = &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.upscalerLUT;
                break;
            case CSLCameraTitanVersion::CSLTitan150:
            case CSLCameraTitanVersion::CSLTitan160:
            case CSLCameraTitanVersion::CSLTitan170:
            case CSLCameraTitanVersion::CSLTitan175:
                pUpscalerLUT = &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.upscalerLUT;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid hardware version: %d", pInputData->titanVersion);
                break;
        }

        if ((NULL != pUpscalerLUT) && (IPEUpscalerLUTBufferSize == sizeof(IPEUpscaler20LUT)))
        {
            if (NULL != m_pUpscalerLUTs)
            {
                Utils::Memcpy(pUpscalerLUT, m_pUpscalerLUTs, IPEUpscalerLUTBufferSize);
            }

            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPEUpscaler20PackedLUT;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPEUpscaler20PackedLUTOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::TuningUpscaler20LUT,
                1,
                pUpscalerLUT,
                sizeof(IPEUpscaler20LUT));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                result = CamxResultSuccess; // Non-fatal error
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Tuning data, incorrect LUT buffer size");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20::ValidateDependenceParams(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-730) validate dependency parameters
    if ((NULL == pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader]) ||
        (NULL == pInputData->pipelineIPEData.pIPEIQSettings))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEUpscaler20::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pAECUpdateData)  &&
        (NULL != pInputData->pAWBUpdateData)  &&
        (NULL != pInputData->pHwContext))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->US20Enable;

            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            if ((NULL != pInputData->pTuningData))
            {
                TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
                CAMX_ASSERT(NULL != pTuningManager);

                // Search through the tuning data (tree), only when there
                // are changes to the tuning mode data as an optimization
                if ((TRUE == pInputData->tuningModeChanged)    &&
                    (TRUE == pTuningManager->IsValidChromatix()))
                {
                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_upscale20_ipe(
                                       reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                       pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if (m_pChromatix != m_dependenceData.pChromatix)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "updating chromatix pointer");

                        m_dependenceData.pChromatix = m_pChromatix;
                        // Firmware has the control. As of now, we need to set it to TRUE in order to program DMI LUT
                        m_moduleEnable              = TRUE;
                        isChanged                   = (TRUE == m_moduleEnable);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get Chromatix");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Tuning Pointer");
            }
        }

        if (TRUE == m_moduleEnable)
        {
            AECFrameControl* pNewAECUpdate = pInputData->pAECUpdateData;

            // This is hardcoded for now based on discussion with FW team
            m_dependenceData.cosited = 0;
            m_dependenceData.evenOdd = 0;

            if ((FALSE == Utils::FEqual(m_dependenceData.luxIndex, pNewAECUpdate->luxIndex)) ||
                (FALSE == Utils::FEqual(m_dependenceData.AECGain,
                              pNewAECUpdate->exposureInfo[ExposureIndexSafe].linearGain)))
            {
                m_dependenceData.luxIndex = pNewAECUpdate->luxIndex;
                m_dependenceData.AECGain  = pNewAECUpdate->exposureInfo[ExposureIndexSafe].linearGain;
                isChanged                 = TRUE;
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "UpScalar20 Module is not enabled");
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc,
                           "Invalid Input: pNewAECUpdate %p pNewAWBUpdate %p pHwContext %p",
                           pInputData->pAECUpdateData,
                           pInputData->pAWBUpdateData,
                           pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Lux Index [0x%f] ", pInputData->pAECUpdateData->luxIndex);

    result = FetchDMIBuffer();
    if (CamxResultSuccess == result)
    {
        Upscale20OutputData outputData;
        UINT32* pLUT = reinterpret_cast<UINT32*>(m_pLUTCmdBuffer->BeginCommands(IPEUpscalerLUTBufferSizeInDWord));
        CAMX_ASSERT(NULL != pLUT);

        outputData.pDMIPtr        = pLUT;

        result = IQInterface::IPEUpscale20CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);
        if (CamxResultSuccess == result)
        {
            result = m_pLUTCmdBuffer->CommitCommands();

            if (NULL != pInputData->pIPETuningMetadata)
            {
                m_pUpscalerLUTs = pLUT;
            }
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE Upscale20 / ChromaUp20 Calculation Failed.");
            m_pLUTCmdBufferManager->Recycle(m_pLUTCmdBuffer);
            m_pLUTCmdBuffer = NULL;
        }

        if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
        {
            if (NULL != m_pHWSetting)
            {
                m_pHWSetting->DumpRegConfig();
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Cannot get buffer from CmdBufferManager");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20::FetchDMIBuffer()
{
    CamxResult      result          = CamxResultSuccess;
    PacketResource* pPacketResource = NULL;

    if (NULL != m_pLUTCmdBufferManager)
    {
        // Recycle the last updated LUT DMI cmd buffer
        if (NULL != m_pLUTCmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTCmdBuffer);
        }

        // fetch a fresh LUT DMI cmd buffer
        result = m_pLUTCmdBufferManager->GetBuffer(&pPacketResource);
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "LUT Command Buffer Is NULL");
    }

    if (CamxResultSuccess == result)
    {
        m_pLUTCmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }
    else
    {
        m_pLUTCmdBuffer = NULL;
        result          = CamxResultEUnableToLoad;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEUpscaler20::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        // Check if dependency is published and valid
        result = ValidateDependenceParams(pInputData);
        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
            }

            // Regardless of any update in dependency parameters, command buffers and IQSettings/Metadata shall be updated.
            if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
            {
                // Record the offset within DMI Header buffer,
                // it is the offset at which this module has written a CDM DMI header.
                CmdBuffer*  pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
                m_offsetLUT               = (pDMICmdBuffer->GetResourceUsedDwords() * sizeof(UINT32));

                UpscalerHWSettingParams moduleHWSettingParams = { 0 };
                moduleHWSettingParams.pLUTCmdBuffer           = m_pLUTCmdBuffer;

                result = m_pHWSetting->CreateCmdList(pInputData, reinterpret_cast<UINT32*>(&moduleHWSettingParams));
            }

            if (CamxResultSuccess == result)
            {
                UpdateIPEInternalData(pInputData);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEUpscaler20::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEUpscaler20::IPEUpscaler20
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEUpscaler20::IPEUpscaler20(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type              = ISPIQModuleType::IPEUpscaler;
    m_moduleEnable      = TRUE;
    m_cmdLength         = 0;
    m_numLUT            = MaxLUTNum;
    m_pLUTCmdBuffer     = NULL;
    m_pChromatix        = NULL;
    m_pUpscalerLUTs     = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE Upscaler m_numLUT %d ", m_numLUT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEUpscaler20::~IPEUpscaler20
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEUpscaler20::~IPEUpscaler20()
{
    if (NULL != m_pLUTCmdBufferManager)
    {
        if (NULL != m_pLUTCmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTCmdBuffer);
            m_pLUTCmdBuffer = NULL;
        }

        m_pLUTCmdBufferManager->Uninitialize();
        CAMX_DELETE m_pLUTCmdBufferManager;
        m_pLUTCmdBufferManager = NULL;
    }

    m_pChromatix = NULL;
    DeallocateCommonLibraryData();

    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
