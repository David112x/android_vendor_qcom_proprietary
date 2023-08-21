////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsgtm10.cpp
/// @brief BPSGTM10 class implementation
///        Global tone mapping to normalize intensities of the image content
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpsgtm10.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "gtm10setting.h"
#include "camxbpsgtm10titan17x.h"
#include "camxbpsgtm10titan480.h"
#include "camxisphwsetting.h"

// Common library includes
#include "iqcommondefs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSGTM10* pModule = CAMX_NEW BPSGTM10(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10::Initialize(
    BPSModuleCreateData* pCreateData)
{
    ResourceParams cmdBufferConfig = { 0 };
    CamxResult     result          = CamxResultSuccess;
    ISPInputData*  pInputData      = &pCreateData->initializationData;

    // 32 bit LUT buffer manager is created externally by BPS node
    m_pLUTCmdBufferManager = NULL;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSGTM10Titan480::Create(&m_pHWSetting);
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSGTM10Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Titan Version: %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSGTM10");
                pResourceParams->resourceSize                 = BPSGTM10DMILengthInBytes;
                pResourceParams->poolSize                     = (pInputData->requestQueueDepth * pResourceParams->resourceSize);
                pResourceParams->usageFlags.cmdBuffer         = 1;
                pResourceParams->cmdParams.type               = CmdType::CDMDMI;
                pResourceParams->alignment                    = CamxCommandBufferAlignmentInBytes;
                pResourceParams->cmdParams.enableAddrPatching = 0;
                pResourceParams->cmdParams.maxNumNestedAddrs  = 0;
                pResourceParams->memFlags                     = CSLMemFlagUMDAccess;
                pResourceParams->pDeviceIndices               = pInputData->pipelineBPSData.pDeviceIndex;
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
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Out Of Memory");
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Out Of Memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(gtm_1_0_0::gtm10_rgn_dataType) * (GTM10MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSGTM10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL    isChanged   = FALSE;
    BOOL    manualMode  = FALSE;

    if ((NULL != pInputData)           &&
        (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->GTMEnable;

            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            if (NULL != pInputData->pTuningData)
            {
                TuningDataManager* pTuningManager = pInputData->pTuningDataManager;

                if (NULL != pTuningManager)
                {
                    // Search through the tuning data (tree), only when there
                    // are changes to the tuning mode data as an optimization
                    if ((TRUE == pInputData->tuningModeChanged)    &&
                        (TRUE == pTuningManager->IsValidChromatix()))
                    {
                        m_pChromatix    = pTuningManager->GetChromatix()->GetModule_gtm10_bps(
                                            reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                            pInputData->pTuningData->noOfSelectionParameter);
                        m_pTMCChromatix = pTuningManager->GetChromatix()->GetModule_tmc10_sw(
                                            reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                            pInputData->pTuningData->noOfSelectionParameter);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "NULL Tuning data pointer");
                }

                if (NULL != m_pChromatix)
                {
                    if (m_pChromatix != m_dependenceData.pChromatix)
                    {
                        m_dependenceData.pChromatix = m_pChromatix;

                        if (NULL != m_pTMCChromatix)
                        {
                            m_pTMCInput.pChromatix  = m_pTMCChromatix;
                        }
                        else
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Failed to get TMC 10 Chromatix, m_pTMCChromatix is NULL");
                        }

                        m_moduleEnable              = m_pChromatix->enable_section.gtm_enable;

                        isChanged                   = (TRUE == m_moduleEnable);
                    }

                    // Check for manual control
                    if ((TonemapModeContrastCurve   == pInputData->pHALTagsData->tonemapCurves.tonemapMode) &&
                        (0                          != pInputData->pHALTagsData->tonemapCurves.curvePoints))
                    {
                        m_moduleEnable  = FALSE;
                        isChanged       = FALSE;
                        manualMode      = TRUE;
                    }

                    if (TRUE == m_moduleEnable)
                    {
                        if (TRUE == IQInterface::s_interpolationTable.GTM10TriggerUpdate(&pInputData->triggerData,
                                                                                        &m_dependenceData))
                        {
                            isChanged = TRUE;
                        }
                    }

                    m_pTMCInput.adrcGTMEnable = FALSE;

                    if ((NULL != m_pTMCChromatix)                                         &&
                        (TRUE == m_pTMCChromatix->enable_section.adrc_isp_enable)         &&
                        (TRUE == m_pTMCChromatix->chromatix_tmc10_reserve.use_gtm)        &&
                        (SWTMCVersion::TMC10 == pInputData->triggerData.enabledTMCversion) &&
                        (FALSE == manualMode))
                    {
                        isChanged = TRUE;
                        m_pTMCInput.adrcGTMEnable = TRUE;
                        IQInterface::s_interpolationTable.TMC10TriggerUpdate(&pInputData->triggerData, &m_pTMCInput);

                        // Assign memory for parsing ADRC Specific data.
                        if (NULL == m_pADRCData)
                        {
                            m_pADRCData = CAMX_NEW ADRCData;
                            if (NULL != m_pADRCData)
                            {
                                Utils::Memset(m_pADRCData, 0, sizeof(ADRCData));
                            }
                            else
                            {
                                isChanged = FALSE;
                                CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed for ADRC Specific data");
                            }
                        }

                        pInputData->triggerData.pADRCData = m_pADRCData;
                        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "BPS GTM : using TMC 10");
                    }
                    else
                    {
                        if (NULL != pInputData->triggerData.pADRCData)
                        {
                            m_pTMCInput.adrcGTMEnable = pInputData->triggerData.pADRCData->gtmEnable;
                            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "BPS GTM : using TMC 11 or 12, version = %u, gtmEnable = %u",
                                            pInputData->triggerData.enabledTMCversion,
                                            m_pTMCInput.adrcGTMEnable);
                        }
                    }

                    m_pTMCInput.pAdrcOutputData = pInputData->triggerData.pADRCData;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "NULL Tuning Pointer");
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS,
                       "Invalid Input: pInputData %p", pInputData);
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10::FetchDMIBuffer()
{
    CamxResult      result          = CamxResultSuccess;
    PacketResource* pPacketResource = NULL;

    if (NULL != m_pLUTCmdBufferManager)
    {
        // Recycle the last updated LUT DMI cmd buffer
        if (NULL != m_pLUTDMICmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTDMICmdBuffer);
        }

        // fetch a fresh LUT DMI cmd buffer
        result = m_pLUTCmdBufferManager->GetBuffer(&pPacketResource);
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "LUT Command Buffer Manager is NULL");
    }

    if (CamxResultSuccess == result)
    {
        m_pLUTDMICmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }
    else
    {
        m_pLUTDMICmdBuffer = NULL;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            GTM10OutputData outputData;

            outputData.regCmd.BPS.pDMIDataPtr =
                reinterpret_cast<UINT64*>(m_pLUTDMICmdBuffer->BeginCommands(BPSGTM10DMILengthDword));
            CAMX_ASSERT(NULL != outputData.regCmd.BPS.pDMIDataPtr);

            outputData.type = PipelineType::BPS;
            outputData.registerBETEn = pInputData->registerBETEn;

            result =
                IQInterface::GTM10CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData, &m_pTMCInput);

            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();
                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pGTMLUTPtr = outputData.regCmd.BPS.pDMIDataPtr;
                }
                if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
                {
                    m_pHWSetting->DumpRegConfig();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "GTM10 Calculation Failed. result %d", result);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSGTM10::UpdateBPSInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    result = m_pHWSetting->UpdateFirmwareData(pInputData, m_moduleEnable);

    if (CamxResultSuccess == result)
    {
        pInputData->pCalculatedData->percentageOfGTM =
            (NULL == m_pTMCInput.pAdrcOutputData) ? 0 : m_pTMCInput.pAdrcOutputData->gtmPercentage;

        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pBPSTuningMetadata)
        {
            result = m_pHWSetting->UpdateTuningMetadata(pInputData);
            if (CamxResultSuccess == result)
            {
                switch (pInputData->titanVersion)
                {
                    case CSLCameraTitanVersion::CSLTitan480:
                        CAMX_STATIC_ASSERT(BPSGTM10DMILengthInBytes ==
                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.GTM.LUT));

                        if (NULL != m_pGTMLUTPtr)
                        {
                            Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.GTM.LUT,
                                          m_pGTMLUTPtr,
                                          BPSGTM10DMILengthInBytes);
                        }

                        if ((TRUE == m_moduleEnable) && (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSGTMPackedLUT,
                                DebugDataTagType::UInt64,
                                CAMX_ARRAY_SIZE(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.GTM.LUT),
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.GTM.LUT,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.GTM.LUT));

                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                                result = CamxResultSuccess; // Non-fatal error
                            }
                        }
                        break;
                    case CSLCameraTitanVersion::CSLTitan150:
                    case CSLCameraTitanVersion::CSLTitan160:
                    case CSLCameraTitanVersion::CSLTitan170:
                    case CSLCameraTitanVersion::CSLTitan175:
                        CAMX_STATIC_ASSERT(BPSGTM10DMILengthInBytes ==
                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.GTM.LUT));

                        if (NULL != m_pGTMLUTPtr)
                        {
                            Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.GTM.LUT,
                                        m_pGTMLUTPtr,
                                        BPSGTM10DMILengthInBytes);
                        }
                        if ((TRUE == m_moduleEnable) && (pInputData->pipelineBPSData.pDebugDataWriter != NULL))
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSGTMPackedLUT,
                                DebugDataTagType::UInt64,
                                CAMX_ARRAY_SIZE(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.GTM.LUT),
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.GTM.LUT,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.GTM.LUT));

                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                                result = CamxResultSuccess; // Non-fatal error
                            }
                        }
                        break;
                    default:
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Titan Version: %d", pInputData->titanVersion);
                        break;
                }
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateTuningMetadata failed.");
                result = CamxResultSuccess; // Non-fatal error
            }
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateFirmwareData failed.");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGTM10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result           = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        /// @todo (CAMX-1768) Add Setting to perform interpolation regardless of dependence change
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
        }

        pInputData->p64bitDMIBuffer = m_pLUTDMICmdBuffer;

        if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
        {
            result = m_pHWSetting->CreateCmdList(pInputData, NULL);
            // Switch Banks after update
            m_dependenceData.LUTBankSel ^= 1;
        }

        if (CamxResultSuccess == result)
        {
            UpdateBPSInternalData(pInputData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Operation failed %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer %p hWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSGTM10::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::BPSGTM10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGTM10::BPSGTM10(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type          = ISPIQModuleType::BPSGTM;
    m_moduleEnable  = FALSE;
    m_numLUT        = MaxGTM10LUT;
    m_cmdLength     = 0;
    m_pGTMLUTPtr    = NULL;
    m_pChromatix    = NULL;
    m_pTMCChromatix = NULL;
    m_pADRCData     = NULL;
    m_pHWSetting    = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGTM10::~BPSGTM10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGTM10::~BPSGTM10()
{
    DeallocateCommonLibraryData();

    if (NULL != m_pADRCData)
    {
        CAMX_DELETE m_pADRCData;
        m_pADRCData = NULL;
    }

    if (NULL != m_pLUTCmdBufferManager)
    {
        if (NULL != m_pLUTDMICmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTDMICmdBuffer);
            m_pLUTDMICmdBuffer = NULL;
        }

        m_pLUTCmdBufferManager->Uninitialize();
        CAMX_DELETE m_pLUTCmdBufferManager;
        m_pLUTCmdBufferManager = NULL;
    }

    m_pChromatix    = NULL;
    m_pTMCChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
