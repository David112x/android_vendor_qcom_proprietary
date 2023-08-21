////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsbpcpdpc20.cpp
/// @brief bpsbpcpdpc20 class implementation
///        BPC: corrects bad pixels and clusters
///        PDPC:corrects phase-differential auto focus pixels
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "pdpc_2_0_0.h"
#include "camxbpsbpcpdpc20titan17x.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSBPCPDPC20* pModule = CAMX_NEW BPSBPCPDPC20(pCreateData->pNodeIdentifier);

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
// BPSBPCPDPC20::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20::Initialize(
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
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Titan480 uses BPCPDPC30");
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSBPCPDPC20Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid hardware version: %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength = m_pHWSetting->GetCommandLength();

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
// BPSBPCPDPC20::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSBPCPDPC20");
                pResourceParams->resourceSize                 = (BPSPDPC20DMILengthDword * sizeof(UINT32));
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
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input Error %p", pParam);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(pdpc_2_0_0::pdpc20_rgn_dataType) * (PDPC20MaxNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for pdpc_2_0_0::pdpc20_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSBPCPDPC20::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)             &&
        (NULL != pInputData->pHwContext) &&
        (NULL != pInputData->pHALTagsData))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->PDPCEnable;
            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            TuningDataManager* pTuningManager = pInputData->pTuningDataManager;

            // Search through the tuning data (tree), only when there
            // are changes to the tuning mode data as an optimization
            if ((TRUE == pInputData->tuningModeChanged)      &&
                (NULL != pTuningManager)                     &&
                (TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_pdpc20_bps(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
            }

            if (NULL != m_pChromatix)
            {
                if (m_pChromatix != m_dependenceData.pChromatix)
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = (m_pChromatix->enable_section.pdpc_enable &&
                                                  (pInputData->sensorData.sensorPDAFInfo.PDAFPixelCount > 0));
                    m_dsBPCEnable               = m_pChromatix->enable_section.dsbpc_enable;

                    isChanged                   = ((TRUE == m_moduleEnable) || (TRUE == m_dsBPCEnable));
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
            }
        }

        if ((TRUE == m_moduleEnable) || (TRUE == m_dsBPCEnable))
        {
            m_dependenceData.blackLevelOffset = pInputData->pCalculatedMetadata->BLSblackLevelOffset;

            // Check for trigger update status
            if (TRUE == IQInterface::s_interpolationTable.BPSPDPC20TriggerUpdate(&(pInputData->triggerData), &m_dependenceData))
            {
                if (NULL == pInputData->pOEMIQSetting)
                {
                    // Check for dsBPC module dynamic enable trigger hysterisis
                    m_dsBPCEnable = IQSettingUtils::GetDynamicEnableFlag(
                        m_dependenceData.pChromatix->dynamic_enable_triggers.dsbpc_enable.enable,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.dsbpc_enable.hyst_control_var,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.dsbpc_enable.hyst_mode,
                        &(m_dependenceData.pChromatix->dynamic_enable_triggers.dsbpc_enable.hyst_trigger),
                        static_cast<VOID*>(&(pInputData->triggerData)),
                        &m_dependenceData.moduleEnable);

                    // Set status to isChanged to avoid calling RunCalculation if the module is disabled
                    isChanged = ((TRUE == m_dsBPCEnable) || (TRUE == m_moduleEnable));
                }
            }
        }

        // PDAF data is fixed, and expected to comes only during the stream start
        if (TRUE == isChanged)
        {
            /// @todo (CAMX-1207) Sensor need to send PDAF dimensions info.
            /// remove this hardcoding after that.
            CamxResult result = IQInterface::IFEGetSensorMode(&pInputData->sensorData.format, &m_dependenceData.sensorType);

            if (CamxResultSuccess == result)
            {
                m_dependenceData.imageWidth        = pInputData->sensorData.sensorOut.width;
                m_dependenceData.imageHeight       = pInputData->sensorData.sensorOut.height;
                m_dependenceData.PDAFBlockWidth    = pInputData->sensorData.sensorPDAFInfo.PDAFBlockWidth;
                m_dependenceData.PDAFBlockHeight   = pInputData->sensorData.sensorPDAFInfo.PDAFBlockHeight;
                m_dependenceData.PDAFGlobaloffsetX =
                    static_cast<UINT16>(pInputData->sensorData.sensorPDAFInfo.PDAFGlobaloffsetX);
                m_dependenceData.PDAFGlobaloffsetY =
                    static_cast<UINT16>(pInputData->sensorData.sensorPDAFInfo.PDAFGlobaloffsetY);
                m_dependenceData.PDAFPixelCount    =
                    static_cast<UINT16>(pInputData->sensorData.sensorPDAFInfo.PDAFPixelCount);
                m_pixelFormat                      = pInputData->sensorData.format;
                m_dependenceData.ZZHDRPattern      = static_cast<UINT16>(pInputData->sensorData.ZZHDRColorPattern);
                switch(pInputData->sensorData.ZZHDRFirstExposure)
                {
                    case ZZHDRFirstExposurePattern::SHORTEXPOSURE:
                        m_dependenceData.zzHDRFirstRBEXP = 1;
                        break;
                    case ZZHDRFirstExposurePattern::LONGEXPOSURE:
                        m_dependenceData.zzHDRFirstRBEXP = 0;
                        break;
                    default:
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "UnSupported ZZHDRFirstExposurePattern");
                }

                Utils::Memcpy(m_dependenceData.PDAFPixelCoords,
                              &pInputData->sensorData.sensorPDAFInfo.PDAFPixelCoords,
                              sizeof(PDPixelCoordinates) * m_dependenceData.PDAFPixelCount);
            }
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS,
                "Invalid Input: pHwContext %p pHALTagsData %p pTuningData %p",
                pInputData->pHwContext,
                pInputData->pHALTagsData,
                pInputData->pTuningData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20::FetchDMIBuffer()
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
        CAMX_LOG_ERROR(CamxLogGroupBPS, "LUT Cmd Buffer is NULL");
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
// BPSBPCPDPC20::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            BPSBPCPDPC20OutputData outputData;

            // Update the LUT DMI buffer with Red LUT data
            outputData.pDMIDataPtr = reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(BPSPDPC20DMILengthDword));
            // BET ONLY - InputData is different per module tested
            pInputData->pBetDMIAddr = static_cast<VOID*>(outputData.pDMIDataPtr);
            CAMX_ASSERT(NULL != outputData.pDMIDataPtr);

            result = IQInterface::BPSBPCPDPC20CalculateSetting(&m_dependenceData,
                                                               pInputData->pOEMIQSetting,
                                                               &outputData,
                                                               m_pixelFormat);
            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();
                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pPDPCMaskLUT = outputData.pDMIDataPtr;
                }
                if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
                {
                    m_pHWSetting->DumpRegConfig();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "BPCPDPC Calculation Failed. result %d", result);
            }
        }
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "BPCPDPC Calculation Failed. result %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSBPCPDPC20::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSBPCPDPC20::UpdateBPSInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;
    VOID*      pSettingData = static_cast<VOID*>(pInputData);

    result = m_pHWSetting->UpdateFirmwareData(pInputData, m_moduleEnable);

    if (CamxResultSuccess == result)
    {
        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pBPSTuningMetadata)
        {
            result = m_pHWSetting->UpdateTuningMetadata(pInputData);
            if (CamxResultSuccess == result)
            {
                CAMX_STATIC_ASSERT((BPSPDPC20DMILengthDword * sizeof(UINT32)) <=
                    sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.PDPCLUT));

                if (NULL != m_pPDPCMaskLUT)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.PDPCLUT,
                                m_pPDPCMaskLUT,
                                (BPSPDPC20DMILengthDword * sizeof(UINT32)));
                }
                if ((TRUE == m_moduleEnable) && (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                {
                    result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                        DebugDataTagID::TuningBPSBPCPDPC20PackedLUT,
                        DebugDataTagType::UInt32,
                        CAMX_ARRAY_SIZE(pInputData->pBPSTuningMetadata->
                                        BPSTuningMetadata17x.BPSDMIData.PDPCLUT.BPSPDPCMaskLUT),
                        &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.PDPCLUT.BPSPDPCMaskLUT,
                        sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.PDPCLUT.BPSPDPCMaskLUT));

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                        result = CamxResultSuccess; // Non-fatal error
                    }
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
/// BPSBPCPDPC20::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20::Execute(
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

        if ((CamxResultSuccess == result) &&
            ((TRUE == m_moduleEnable)     || (TRUE == m_dsBPCEnable)))
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
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer %p, m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSBPCPDPC20::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSBPCPDPC20::BPSBPCPDPC20
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSBPCPDPC20::BPSBPCPDPC20(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier     = pNodeIdentifier;
    m_type                = ISPIQModuleType::BPSBPCPDPC;
    m_moduleEnable        = FALSE;
    m_dsBPCEnable         = FALSE;
    m_numLUT              = PDPCMaxLUT;

    m_cmdLength           = 0;

    m_pPDPCMaskLUT = NULL;
    m_pChromatix   = NULL;
    m_pHWSetting   = NULL;

    m_dependenceData.moduleEnable = FALSE;   ///< First frame is always FALSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSBPCPDPC20::~BPSBPCPDPC20
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSBPCPDPC20::~BPSBPCPDPC20()
{
    DeallocateCommonLibraryData();

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

    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
