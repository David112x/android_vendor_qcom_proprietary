////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsbpcpdpc30.cpp
/// @brief bpsbpcpdpc30 class implementation
///        BPC: corrects bad pixels and clusters
///        PDPC:corrects phase-differential auto focus pixels
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "pdpc_3_0_0.h"
#include "camxbpsbpcpdpc30.h"
#include "camxbpspdpc30titan480.h"

CAMX_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC30::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC30::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSBPCPDPC30* pModule = CAMX_NEW BPSBPCPDPC30(pCreateData->pNodeIdentifier);

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
// BPSBPCPDPC30::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC30::Initialize(
    BPSModuleCreateData* pCreateData)
{
    ResourceParams cmdBufferConfig = { 0 };
    CamxResult     result          = CamxResultSuccess;
    ISPInputData*  pInputData      = &pCreateData->initializationData;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSPDPC30Titan480::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid hardware version: %d", pCreateData->titanVersion);
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
        else
        {
            m_cmdLength         = m_pHWSetting->GetCommandLength();
            m_32bitDMILength    = m_pHWSetting->Get32bitDMILength();
            m_64bitDMILength    = m_pHWSetting->Get64bitDMILength();
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    // m_pLUTCmdBufferManager will be allocated externally in BPS Node
    m_pLUTCmdBufferManager = NULL;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC30::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC30::FillCmdBufferManagerParams(
    const ISPInputData*     pInputData,
    IQModuleCmdBufferParam* pParam)
{
    CamxResult      result              = CamxResultSuccess;
    ResourceParams* pResourceParams     = NULL;
    CHAR*           pBufferManagerName  = NULL;

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
                    (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSBPCPDPC30");

                pResourceParams->resourceSize           = (m_32bitDMILength * sizeof(UINT32));
                pResourceParams->poolSize               = (pInputData->requestQueueDepth * pResourceParams->resourceSize);
                pResourceParams->alignment              = CamxCommandBufferAlignmentInBytes;

                pResourceParams->cmdParams.type                 = CmdType::CDMDMI;
                pResourceParams->usageFlags.cmdBuffer           = 1;
                pResourceParams->cmdParams.enableAddrPatching   = 0;
                pResourceParams->cmdParams.maxNumNestedAddrs    = 0;
                pResourceParams->memFlags                       = CSLMemFlagUMDAccess;
                pResourceParams->pDeviceIndices                 = pInputData->pipelineBPSData.pDeviceIndex;
                pResourceParams->numDevices                     = 1;

                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].pBufferManagerName  = pBufferManagerName;
                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].pParams             = pResourceParams;
                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].ppCmdBufferManager  = &m_pLUTCmdBufferManager;
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
// BPSBPCPDPC30::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC30::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(pdpc_3_0_0::pdpc30_rgn_dataType) * (PDPC30MaxNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for pdpc_3_0_0::pdpc30_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC30::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSBPCPDPC30::CheckDependenceChange(
    ISPInputData* pInputData)
{
    CamxResult result    = CamxResultSuccess;
    BOOL       isChanged = FALSE;

    if ((NULL != pInputData)                           &&
        (NULL != pInputData->pHwContext)               &&
        (NULL != pInputData->pTuningData))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->PDPCEnable;
            isChanged = (TRUE == m_moduleEnable);
        }
        else
        {
            TuningDataManager*                pTuningManager = pInputData->pTuningDataManager;

            CAMX_ASSERT(NULL != pTuningManager);

            // Search through the tuning data (tree), only when there
            // are changes to the tuning mode data as an optimization
            if ((TRUE == pInputData->tuningModeChanged) &&
                (TRUE == pTuningManager->IsValidChromatix()))
            {
                m_pChromatix = pTuningManager->GetChromatix()->GetModule_pdpc30_bps(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if (m_pChromatix != m_dependenceData.pChromatix)
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable = (TRUE == m_pChromatix->enable_section.enable               &&
                                     ((TRUE == m_pChromatix->enable_section.pdpc_enable         &&
                                      pInputData->sensorData.sensorPDAFInfo.PDAFPixelCount > 0) ||
                                      TRUE == m_pChromatix->enable_section.bpc_enable           ||
                                      TRUE == m_pChromatix->enable_section.gic_enable));

                    isChanged      = (TRUE == m_moduleEnable);
                }

                m_dependenceData.bpcEnable  = m_pChromatix->enable_section.bpc_enable;
                m_dependenceData.pdpcEnable = (TRUE == m_pChromatix->enable_section.pdpc_enable) &&
                                              (pInputData->sensorData.sensorPDAFInfo.PDAFPixelCount > 0);
                m_dependenceData.gicEnable  = m_pChromatix->enable_section.gic_enable;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
            }
        }

        m_dependenceData.moduleEnable = m_moduleEnable;
        if ((TRUE == m_moduleEnable) &&
            (HotPixelModeOff == pInputData->pHALTagsData->hotPixelMode))
        {
            m_dependenceData.bpcEnable            = FALSE;
            m_dependenceData.directionalBPCEnable = FALSE;
        }
        if (TRUE == m_moduleEnable)
        {
            // original common library code
            // m_dependenceData.blackLevelOffset = pInputData->pCalculatedData->blackLevelOffset;
            m_dependenceData.blackLevelOffset = pInputData->pCalculatedMetadata->BLSblackLevelOffset;

            // Check for trigger update status
            if (TRUE == IQInterface::s_interpolationTable.PDPC30TriggerUpdate(&(pInputData->triggerData), &m_dependenceData))
            {
                isChanged = TRUE;
            }
        }

        // PDAF data is fixed, and expected to comes only during the stream start
        if (TRUE == m_moduleEnable)
        {
            /// @todo (CAMX-1207) Sensor need to send PDAF dimensions info.
            /// remove this hardcoding after that.
            result = IQInterface::IFEGetSensorMode(&pInputData->sensorData.format, &m_dependenceData.sensorType);
            if (CamxResultSuccess == result)
            {
                m_dependenceData.imageWidth         = pInputData->sensorData.sensorOut.width;
                m_dependenceData.imageHeight        = pInputData->sensorData.sensorOut.height;
                m_dependenceData.PDAFBlockWidth     = pInputData->sensorData.sensorPDAFInfo.PDAFBlockWidth;
                m_dependenceData.PDAFBlockHeight    = pInputData->sensorData.sensorPDAFInfo.PDAFBlockHeight;
                m_dependenceData.PDAFGlobaloffsetX  =
                    static_cast<UINT16>(pInputData->sensorData.sensorPDAFInfo.PDAFGlobaloffsetX);
                m_dependenceData.PDAFGlobaloffsetY  =
                    static_cast<UINT16>(pInputData->sensorData.sensorPDAFInfo.PDAFGlobaloffsetY);
                m_dependenceData.PDAFPixelCount     =
                    static_cast<UINT16>(pInputData->sensorData.sensorPDAFInfo.PDAFPixelCount);
                m_pixelFormat                       = pInputData->sensorData.format;
                m_dependenceData.ZZHDRPattern       = static_cast<UINT16>(pInputData->sensorData.ZZHDRColorPattern);
                switch(pInputData->sensorData.ZZHDRFirstExposure)
                {
                    case ZZHDRFirstExposurePattern::SHORTEXPOSURE:
                        m_dependenceData.zzHDRFirstRBEXP = 1;
                        break;
                    case ZZHDRFirstExposurePattern::LONGEXPOSURE:
                        m_dependenceData.zzHDRFirstRBEXP = 0;
                        break;
                    default:
                        CAMX_LOG_WARN(CamxLogGroupISP, "UnSupported ZZHDRFirstExposurePattern");
                }

                Utils::Memcpy(m_dependenceData.PDAFPixelCoords,
                    &pInputData->sensorData.sensorPDAFInfo.PDAFPixelCoords,
                    sizeof(PDPixelCoordinates) * m_dependenceData.PDAFPixelCount);

                isChanged = TRUE;
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;

        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS,
                "Invalid Input: pHwContext %p pTuningData %p",
                pInputData->pHwContext,
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
// BPSBPCPDPC30::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC30::FetchDMIBuffer()
{
    CamxResult      result          = CamxResultSuccess;
    PacketResource* pPacketResource = NULL;

    CAMX_ASSERT(NULL != m_pLUTCmdBufferManager);

    // Recycle the last updated 32 bit LUT DMI cmd buffer
    if (NULL != m_pLUTDMICmdBuffer)
    {
        m_pLUTCmdBufferManager->Recycle(m_pLUTDMICmdBuffer);
        m_pLUTDMICmdBuffer = NULL;
    }

    // fetch a fresh 32 bit LUT DMI cmd buffer
    result = m_pLUTCmdBufferManager->GetBuffer(&pPacketResource);
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
// BPSBPCPDPC30::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC30::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            BPSPDPC30OutputData outputData;

            VOID* pDMIBuffer = m_pLUTDMICmdBuffer->BeginCommands(m_32bitDMILength);

            outputData.pDMIDataPtrPDAF   = reinterpret_cast<UINT64*>(pDMIBuffer);
            // BET ONLY - InputData is different per module tested (need revisit)
            pInputData->pBetDMIAddr      = static_cast<VOID*>(outputData.pDMIDataPtrPDAF);
            outputData.pDMIDataPtrPNoise = reinterpret_cast<UINT32*>(pDMIBuffer) + (BPSPDPC30DMILengthPDAF * 2);

            result = IQInterface::BPSPDPC30CalculateSetting(&m_dependenceData,
                                                            pInputData->pOEMIQSetting,
                                                            &outputData,
                                                            m_pixelFormat);
            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();
                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pPDPCMaskLUT  = outputData.pDMIDataPtrPDAF;
                    m_pPDPCNoiseLUT = outputData.pDMIDataPtrPNoise;
                }
                if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
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
/// BPSBPCPDPC30::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSBPCPDPC30::UpdateBPSInternalData(
    ISPInputData* pInputData
    ) const
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
                if (NULL != m_pPDPCMaskLUT)
                {
                    CAMX_STATIC_ASSERT((sizeof(UINT64) * BPSPDPC30DMILengthPDAF) <=
                        sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.PDPCLUT.BPSPDPCMaskLUT));
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.PDPCLUT.BPSPDPCMaskLUT,
                        m_pPDPCMaskLUT,
                        (sizeof(UINT64) * BPSPDPC30DMILengthPDAF));
                }

                if (NULL != m_pPDPCNoiseLUT)
                {
                    CAMX_STATIC_ASSERT((sizeof(UINT32) * BPSPDPC30DMILengthNoise) <=
                        sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.PDPCLUT.BPSPDPCNoiseLUT));
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.PDPCLUT.BPSPDPCNoiseLUT,
                        m_pPDPCNoiseLUT,
                        (sizeof(UINT32) * BPSPDPC30DMILengthNoise));
                }

                if ((TRUE == m_moduleEnable) &&
                    (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                {
                    result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                        DebugDataTagID::TuningBPSBPCPDPC30PackedLUT,
                        DebugDataTagType::TuningBPSBPCPDPC30LUT,
                        1,
                        &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.PDPCLUT,
                        sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.PDPCLUT));

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
/// BPSBPCPDPC30::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC30::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
        }

        pInputData->p32bitDMIBuffer = m_pLUTDMICmdBuffer;
        if ((CamxResultSuccess == result) && ((TRUE == m_moduleEnable) || (TRUE == m_dsBPCEnable)))
        {
            result = m_pHWSetting->CreateCmdList(pInputData, NULL);
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
// BPSBPCPDPC30::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSBPCPDPC30::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSBPCPDPC30::BPSBPCPDPC30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSBPCPDPC30::BPSBPCPDPC30(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type              = ISPIQModuleType::BPSBPCPDPC;
    m_moduleEnable      = TRUE;
    m_dsBPCEnable       = TRUE;
    m_numLUT            = BPSPDPC30MaxLUT;

    m_cmdLength         = 0;
    m_64bitDMILength    = 0;
    m_pPDPCMaskLUT      = NULL;
    m_pChromatix        = NULL;
    m_pHWSetting        = NULL;

    m_dependenceData.moduleEnable         = FALSE;   ///< First frame is always FALSE
    m_dependenceData.directionalBPCEnable = TRUE;    ///< TRUE means to use interpolation data
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSBPCPDPC30::~BPSBPCPDPC30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSBPCPDPC30::~BPSBPCPDPC30()
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

    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }

    m_pChromatix = NULL;
}

CAMX_NAMESPACE_END
