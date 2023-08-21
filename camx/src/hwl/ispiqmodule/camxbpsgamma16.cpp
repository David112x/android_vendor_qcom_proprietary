////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsgamma16.cpp
/// @brief CAMXBPSGAMMA16 class implementation
///        Gamm LUT to compensate intensity nonlinearity of display
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpsgamma16.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "gamma16setting.h"
#include "camxbpsgamma16titan17x.h"
#include "camxbpsgamma16titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGamma16::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGamma16::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSGamma16* pModule = CAMX_NEW BPSGamma16(pCreateData->pNodeIdentifier);

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
// BPSGamma16::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGamma16::Initialize(
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
            result = BPSGAMMA16Titan480::Create(&m_pHWSetting);
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSGAMMA16Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid hardware version: %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGamma16::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGamma16::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSGamma16");
                pResourceParams->resourceSize                 = BPSGammalLUTBufferSize;
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
// BPSGamma16::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSGamma16::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                &&
        (NULL != pInputData->pHALTagsData)  &&
        (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->GammaEnable;

            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
            CAMX_ASSERT(NULL != pTuningManager);

            m_dependenceData.pLibData = pInputData->pLibInitialData;

            // Search through the tuning data (tree), only when there
            // are changes to the tuning mode data as an optimization
            if ((TRUE == pInputData->tuningModeChanged)    &&
                (TRUE == pTuningManager->IsValidChromatix()))
            {
                CAMX_ASSERT(NULL != pInputData->pTuningData);

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_gamma16_bps(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if ((m_pChromatix != m_dependenceData.pChromatix) ||
                    (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID) ||
                    (m_pChromatix->enable_section.gamma_enable != m_moduleEnable))
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = m_pChromatix->enable_section.gamma_enable;

                    isChanged                   = (TRUE == m_moduleEnable);
                }

                // Check for manual control
                if (TonemapModeContrastCurve == pInputData->pHALTagsData->tonemapCurves.tonemapMode)
                {
                    m_moduleEnable = FALSE;
                    isChanged      = FALSE;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
            }

            if (TRUE == m_moduleEnable)
            {
                if (TRUE ==
                    IQInterface::s_interpolationTable.gamma16TriggerUpdate(&pInputData->triggerData, &m_dependenceData))
                {
                    isChanged = TRUE;
                }
            }
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS,
                           "Invalid Input: pHwContext %p",
                           pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGamma16::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGamma16::FetchDMIBuffer()
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
        result             = CamxResultEUnableToLoad;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGamma16::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGamma16::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            Gamma16OutputData outputData;

            outputData.type = PipelineType::BPS;

            // Update the LUT DMI buffer with Channel G LUT data
            outputData.pGDMIDataPtr =
                reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(BPSGammalLUTBufferSize / sizeof(UINT32)));
            // BET ONLY - InputData is different per module tested
            pInputData->pBetDMIAddr = static_cast<VOID*>(outputData.pGDMIDataPtr);
            CAMX_ASSERT(NULL != outputData.pGDMIDataPtr);

            // Update the LUT DMI buffer with Channel B LUT data
            outputData.pBDMIDataPtr =
                reinterpret_cast<UINT32*>(GammaSizeLUTInBytes + reinterpret_cast<UCHAR*>(outputData.pGDMIDataPtr));
            CAMX_ASSERT(NULL != outputData.pBDMIDataPtr);

            // Update the LUT DMI buffer with Channel R LUT data
            outputData.pRDMIDataPtr =
                reinterpret_cast<UINT32*>(GammaSizeLUTInBytes + reinterpret_cast<UCHAR*>(outputData.pBDMIDataPtr));
            CAMX_ASSERT(NULL != outputData.pRDMIDataPtr);

            result = IQInterface::Gamma16CalculateSetting(&m_dependenceData,
                                                          pInputData->pOEMIQSetting,
                                                          &outputData,
                                                          NULL);

            if (CamxResultSuccess == result)
            {
                m_pGammaG = outputData.pGDMIDataPtr;
                m_pLUTDMICmdBuffer->CommitCommands();

                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pGammaTuning[GammaLUTChannelR] = outputData.pRDMIDataPtr;
                    m_pGammaTuning[GammaLUTChannelG] = outputData.pGDMIDataPtr;
                    m_pGammaTuning[GammaLUTChannelB] = outputData.pBDMIDataPtr;
                }
                if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
                {
                    m_pHWSetting->DumpRegConfig();
                }
            }
            else
            {
                m_pGammaG = NULL;
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Gamma Calculation Failed");
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
// BPSGamma16::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSGamma16::UpdateBPSInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    result = m_pHWSetting->UpdateFirmwareData(pInputData, m_moduleEnable);

    if (CamxResultSuccess == result)
    {
        ISPInternalData* pData = pInputData->pCalculatedData;
        if (NULL != m_pGammaG && NULL != pData)
        {
            for (UINT i = 0; i < NumberOfGammaEntriesPerLUT; i++)
            {
                pData->gammaOutput.gammaG[i] = m_pGammaG[i] & GammaMask;
            }
            pData->gammaOutput.isGammaValid = TRUE;
        }

        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pBPSTuningMetadata)
        {
            result = m_pHWSetting->UpdateTuningMetadata(pInputData);
            if (CamxResultSuccess == result)
            {
                switch (pInputData->titanVersion)
                {
                    case CSLCameraTitanVersion::CSLTitan480:
                        CAMX_STATIC_ASSERT(BPSGammalLUTBufferSize <=
                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.gamma));

                        if (NULL != m_pGammaTuning[GammaLUTChannelR])
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.gamma[GammaLUTChannelR],
                                m_pGammaTuning[GammaLUTChannelR],
                                GammaSizeLUTInBytes);
                        }

                        if (NULL != m_pGammaTuning[GammaLUTChannelG])
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.gamma[GammaLUTChannelG],
                                m_pGammaTuning[GammaLUTChannelG],
                                GammaSizeLUTInBytes);
                        }

                        if (NULL != m_pGammaTuning[GammaLUTChannelB])
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.gamma[GammaLUTChannelB],
                                m_pGammaTuning[GammaLUTChannelB],
                                GammaSizeLUTInBytes);
                        }
                        if (NULL != pInputData->pipelineBPSData.pDebugDataWriter)
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSGammaLUT,
                                DebugDataTagType::TuningGammaCurve,
                                CAMX_ARRAY_SIZE(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.gamma),
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.gamma,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata480.BPSDMIData.gamma));
                        }
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                            result = CamxResultSuccess; // Non-fatal error
                        }
                        break;
                    case CSLCameraTitanVersion::CSLTitan150:
                    case CSLCameraTitanVersion::CSLTitan160:
                    case CSLCameraTitanVersion::CSLTitan170:
                    case CSLCameraTitanVersion::CSLTitan175:
                        CAMX_STATIC_ASSERT(BPSGammalLUTBufferSize <=
                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.gamma));

                        if (NULL != m_pGammaTuning[GammaLUTChannelR])
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.gamma[GammaLUTChannelR],
                                m_pGammaTuning[GammaLUTChannelR],
                                GammaSizeLUTInBytes);
                        }

                        if (NULL != m_pGammaTuning[GammaLUTChannelG])
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.gamma[GammaLUTChannelG],
                                m_pGammaTuning[GammaLUTChannelG],
                                GammaSizeLUTInBytes);
                        }

                        if (NULL != m_pGammaTuning[GammaLUTChannelB])
                        {
                            Utils::Memcpy(
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.gamma[GammaLUTChannelB],
                                m_pGammaTuning[GammaLUTChannelB],
                                GammaSizeLUTInBytes);
                        }
                        if (NULL != pInputData->pipelineBPSData.pDebugDataWriter)
                        {
                            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                                DebugDataTagID::TuningBPSGammaLUT,
                                DebugDataTagType::TuningGammaCurve,
                                CAMX_ARRAY_SIZE(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.gamma),
                                &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.gamma,
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.gamma));
                        }
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
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
// BPSGamma16::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSGamma16::Execute(
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
            m_pLUTDMICmdBuffer = pInputData->p64bitDMIBuffer;
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
// BPSGamma16::BPSGamma16
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGamma16::BPSGamma16(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type              = ISPIQModuleType::BPSGamma;
    m_numLUT            = GammaLUTMax;
    m_cmdLength         = 0;
    m_moduleEnable      = FALSE;
    m_pGammaG           = NULL;
    m_pChromatix        = NULL;
    m_pHWSetting        = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGamma16::~BPSGamma16
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSGamma16::~BPSGamma16()
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

    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
