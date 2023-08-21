////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsabf40.cpp
/// @brief BPSABF40 class implementation
///        ABF: Individual or cross channel Bayer domain denoise filter.  Also provide radial noise reduction (RNR) capability.
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpsabf40.h"
#include "camxbpsabf40titan17x.h"
#include "camxbpsabf40titan480.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSABF40* pModule = CAMX_NEW BPSABF40(pCreateData->pNodeIdentifier);

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
// BPSABF40::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40::Initialize(
    BPSModuleCreateData* pCreateData)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = &pCreateData->initializationData;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSABF40Titan480::Create(&m_pHWSetting);
            m_numLUT = MaxABFLUT - 1;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSABF40Titan17x::Create(&m_pHWSetting);
            m_numLUT = MaxABFLUT;
            break;

        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid hardware version: %d", pCreateData->titanVersion);
            break;
    }

    if (result == CamxResultSuccess)
    {
        m_dependenceData.pHWSetting     = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength                     = m_pHWSetting->GetCommandLength();
        m_32bitDMILength                = m_pHWSetting->Get32bitDMILength();

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
// BPSABF40::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSABF40");
                pResourceParams->resourceSize                 = m_32bitDMILength * sizeof(UINT32);
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
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(abf_4_0_0::abf40_rgn_dataType) * (ABF40MaxmiumNoLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for abf_4_0_0::abf40_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSABF40::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;
    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "#### pHALTagsData %p ", pInputData->pHALTagsData);

    if ((NULL != pInputData)              &&
        (NULL != pInputData->pHwContext)  &&
        (NULL != pInputData->pTuningData) &&
        (NULL != pInputData->pHALTagsData))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable       = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->ABFEnable;
            isChanged            = (TRUE == m_moduleEnable);
            m_minmaxEnable       = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->ABFEnableSection.minmax_en;
            m_dirsmthEnable      = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->ABFEnableSection.dirsmth_en;
        }
        else
        {
            if (NULL != pInputData->pTuningData)
            {
                TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
                CAMX_ASSERT(NULL != pTuningManager);

                // Search through the tuning data (tree), only when there
                // are changes to the tuning mode data as an optimization
                if ((TRUE == pInputData->tuningModeChanged)   &&
                    (TRUE == pTuningManager->IsValidChromatix()))
                {
                    m_pChromatix    = pTuningManager->GetChromatix()->GetModule_abf40_bps(
                                          reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                          pInputData->pTuningData->noOfSelectionParameter);
                    m_pChromatixBLS = pTuningManager->GetChromatix()->GetModule_bls12_bps(
                                          reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                          pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                CAMX_ASSERT(NULL != m_pChromatixBLS);

                if ((NULL != m_pChromatix)  &&
                    (NULL != m_pChromatixBLS))
                {
                    if ((m_pChromatix           != m_dependenceData.pChromatix)                   ||
                        (m_pChromatixBLS        != m_dependenceData.BLSData.pChromatix)           ||
                        (m_bilateralEnable      != m_pChromatix->enable_section.bilateral_en)     ||
                        (m_minmaxEnable         != m_pChromatix->enable_section.minmax_en)        ||
                        (m_dirsmthEnable        != m_pChromatix->enable_section.dirsmth_en)       ||
                        (m_noiseReductionMode   != pInputData->pHALTagsData->noiseReductionMode))
                    {
                        m_dependenceData.pChromatix         = m_pChromatix;
                        m_dependenceData.BLSData.pChromatix = m_pChromatixBLS;

                        m_crossPlaneEnable  = m_pChromatix->chromatix_abf40_reserve.cross_plane_en;
                        m_actEnable         = m_pChromatix->chromatix_abf40_reserve.act_adj_en;
                        m_bilateralEnable   = m_pChromatix->enable_section.bilateral_en;
                        m_minmaxEnable      = m_pChromatix->enable_section.minmax_en;
                        m_dirsmthEnable     = m_pChromatix->enable_section.dirsmth_en;
                        m_blsEnable         = m_pChromatixBLS->enable_section.bls_enable;
                        m_moduleEnable      = (m_bilateralEnable | m_minmaxEnable | m_dirsmthEnable | m_blsEnable);
                        isChanged           = (TRUE == m_moduleEnable);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Null TuningData Pointer");
            }
        }

        if (TRUE == m_moduleEnable)
        {
            CamxResult result = IQInterface::GetPixelFormat(&pInputData->sensorData.format,
                                                            &(m_dependenceData.BLSData.bayerPattern));
            CAMX_ASSERT(CamxResultSuccess == result);

            // Check for trigger update status
            if (TRUE == IQInterface::s_interpolationTable.ABF40TriggerUpdate(&(pInputData->triggerData), &m_dependenceData))
            {
                if (NULL == pInputData->pOEMIQSetting)
                {
                    // Check for module dynamic enable trigger hysterisis, only for bilateral filtering in abf40
                    m_bilateralEnable = IQSettingUtils::GetDynamicEnableFlag(
                        m_dependenceData.pChromatix->dynamic_enable_triggers.bilateral_en.enable,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.bilateral_en.hyst_control_var,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.bilateral_en.hyst_mode,
                        &(m_dependenceData.pChromatix->dynamic_enable_triggers.bilateral_en.hyst_trigger),
                        static_cast<VOID*>(&(pInputData->triggerData)),
                        &m_dependenceData.moduleEnable);

                    m_moduleEnable = (m_bilateralEnable | m_minmaxEnable | m_dirsmthEnable | m_blsEnable);

                    // Set the module status to avoid calling RunCalculation if it is disabled
                    isChanged = (TRUE == m_moduleEnable);
                }
            }
        }

        m_noiseReductionMode = pInputData->pHALTagsData->noiseReductionMode;

        if (NoiseReductionModeOff == m_noiseReductionMode)
        {
            m_bilateralEnable   = FALSE;
            m_minmaxEnable      = FALSE;
            m_dirsmthEnable     = FALSE;
            m_moduleEnable      = (m_bilateralEnable | m_minmaxEnable | m_dirsmthEnable | m_blsEnable);
            isChanged           = (TRUE == m_moduleEnable);
        }

        m_dependenceData.bilateralEnable            = m_bilateralEnable;
        m_dependenceData.minmaxEnable               = m_minmaxEnable;
        m_dependenceData.directionalSmoothEnable    = m_dirsmthEnable;
        m_dependenceData.BLSEnable                  = m_blsEnable;
        m_dependenceData.moduleEnable               = m_moduleEnable;
        m_dependenceData.crossPlaneEnable           = m_crossPlaneEnable;
        m_dependenceData.activityAdjustEnable       = m_actEnable;
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input: pHwContext %p", pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40::FetchDMIBuffer()
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
        CAMX_LOG_ERROR(CamxLogGroupBPS, "LUT Command Buffer Failed");
    }

    if (CamxResultSuccess == result)
    {
        m_pLUTDMICmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }
    else
    {
        m_pLUTDMICmdBuffer = NULL;
        result             = CamxResultEUnableToLoad;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            BpsIQSettings*  pBPSIQSettings = reinterpret_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
            CAMX_ASSERT(NULL != pBPSIQSettings);

            ABF40OutputData outputData;
            outputData.type          = PipelineType::BPS;
            outputData.pNoiseLUT     = NULL;
            outputData.pNoiseLUT1    = NULL;
            outputData.pActivityLUT  = NULL;
            outputData.pDarkLUT      = NULL;

            // Update the LUT DMI buffer with Noise 0 LUT data
            outputData.pNoiseLUT = reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(m_32bitDMILength));
            CAMX_ASSERT(NULL != outputData.pNoiseLUT);
            // BET ONLY - InputData is different per module tested
            pInputData->pBetDMIAddr = static_cast<VOID*>(outputData.pNoiseLUT);

            if (pInputData->titanVersion == CSLCameraTitanVersion::CSLTitan480)
            {
                // Update the LUT DMI buffer with Activity LUT data
                outputData.pActivityLUT = (outputData.pNoiseLUT) + BPSABF40NoiseLUTSizeDword;
            }
            else
            {
                // Update the LUT DMI buffer with Noise 0 LUT1 data
                outputData.pNoiseLUT1 = (outputData.pNoiseLUT) + BPSABF40NoiseLUTSizeDword;
                CAMX_ASSERT(NULL != outputData.pNoiseLUT1);

                // Update the LUT DMI buffer with Activity LUT data
                outputData.pActivityLUT = (outputData.pNoiseLUT1) + BPSABF40NoiseLUTSizeDword;
            }

            // Update the LUT DMI buffer with Dark LUT data
            outputData.pDarkLUT = (outputData.pActivityLUT) + BPSABF40ActivityLUTSizeDword;

            result = IQInterface::ABF40CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();

                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pABFNoiseLUT      = outputData.pNoiseLUT;
                    m_pABFNoiseLUT1     = outputData.pNoiseLUT1;
                    m_pABFActivityLUT   = outputData.pActivityLUT;
                    m_pABFDarkLUT       = outputData.pDarkLUT;
                }
                if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
                {
                    m_pHWSetting->DumpRegConfig();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "BPSABF40 Calculation Failed.");
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
// BPSABF40::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSABF40::UpdateBPSInternalData(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    result = m_pHWSetting->UpdateFirmwareData(pInputData, m_moduleEnable);

    if (CamxResultSuccess == result)
    {
        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pBPSTuningMetadata)
        {
            result = m_pHWSetting->UpdateTuningMetadata(pInputData);
            if (CamxResultSuccess == result)
            {
                switch (pInputData->titanVersion)
                {
                    case CSLCameraTitanVersion::CSLTitan480:
                        if ((m_32bitDMILength * sizeof(UINT32)) >
                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.ABFLUT))
                        {
                            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid ABF Total LUT Buffer Size");
                        }

                        if (NULL != m_pABFNoiseLUT)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.ABFLUT.noiseLUT,
                                m_pABFNoiseLUT,
                                BPSABF40NoiseLUTSize);
                        }
                        if (NULL != m_pABFActivityLUT)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.ABFLUT.activityLUT,
                                m_pABFActivityLUT,
                                BPSABF40ActivityLUTSize);
                        }
                        if (NULL != m_pABFDarkLUT)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.ABFLUT.darkLUT,
                                m_pABFDarkLUT,
                                BPSABF40DarkLUTSize);
                        }
                        if ((TRUE == m_moduleEnable) && (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSABFPackedLUT,
                                DebugDataTagType::TuningABFLUT,
                                1,
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.ABFLUT,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.ABFLUT));

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
                        if ((m_32bitDMILength * sizeof(UINT32)) >
                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.ABFLUT))
                        {
                            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid ABF Total LUT Buffer Size");
                        }

                        if (NULL != m_pABFNoiseLUT)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.ABFLUT.noiseLUT,
                                m_pABFNoiseLUT,
                                BPSABF40NoiseLUTSize);
                        }
                        if (NULL != m_pABFNoiseLUT1)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.ABFLUT.noise1LUT,
                                m_pABFNoiseLUT1,
                                BPSABF40NoiseLUTSize);
                        }
                        if (NULL != m_pABFActivityLUT)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.ABFLUT.activityLUT,
                                m_pABFActivityLUT,
                                BPSABF40ActivityLUTSize);
                        }
                        if (NULL != m_pABFDarkLUT)
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.ABFLUT.darkLUT,
                                m_pABFDarkLUT,
                                BPSABF40DarkLUTSize);
                        }
                        if ((TRUE == m_moduleEnable) && (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSABFPackedLUT,
                                DebugDataTagType::TuningABFLUT,
                                1,
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.ABFLUT,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.ABFLUT));

                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                                result = CamxResultSuccess; // Non-fatal error
                            }
                        }
                        break;
                    default:
                        CAMX_LOG_WARN(CamxLogGroupBPS, "Invalid hardware version: %d", pInputData->titanVersion);
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
// BPSABF40::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
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
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input: pInputData %p, m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSABF40::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40::BPSABF40
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSABF40::BPSABF40(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier                = pNodeIdentifier;
    m_type                           = ISPIQModuleType::BPSABF;
    m_moduleEnable                   = FALSE;
    m_numLUT                         = 0;
    m_dependenceData.moduleEnable    = FALSE;   ///< First frame is always FALSE
    m_pABFNoiseLUT                   = NULL;
    m_pABFActivityLUT                = NULL;
    m_pABFDarkLUT                    = NULL;
    m_pChromatix                     = NULL;
    m_pChromatixBLS                  = NULL;
    m_pHWSetting                     = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40::~BPSABF40
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSABF40::~BPSABF40()
{
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
    m_pChromatixBLS = NULL;

    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
