////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpshnr10.cpp
/// @brief bpshnr10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpshnr10.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxbpshnr10titan17x.h"
#include "camxtuningdatamanager.h"
#include "camxisphwsetting.h"
#include "hnr10setting.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSHNR10* pModule = CAMX_NEW BPSHNR10(pCreateData->pNodeIdentifier);

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
// BPSHNR10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10::Initialize(
    BPSModuleCreateData* pCreateData)
{
    ResourceParams cmdBufferConfig = { 0 };
    CamxResult     result          = CamxResultSuccess;
    ISPInputData*  pInputData      = &pCreateData->initializationData;

    // 32 bit LUT buffer manager is created externally by BPS node
    m_pLUTCmdBufferManager = NULL;

    if (CamxResultSuccess == result)
    {
        switch (pCreateData->titanVersion)
        {
            case CSLCameraTitanVersion::CSLTitan150:
            case CSLCameraTitanVersion::CSLTitan160:
            case CSLCameraTitanVersion::CSLTitan170:
            case CSLCameraTitanVersion::CSLTitan175:
                result = BPSHNR10Titan17x::Create(&m_pHWSetting);
                break;
            default:
                result = CamxResultEInvalidState;
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Unsupported or Invalid Titan version: %d", pCreateData->titanVersion);
                break;
        }
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
// BPSHNR10::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSHNR10");
                pResourceParams->resourceSize                 = BPSHNRLutBufferSize;
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
// BPSHNR10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(hnr_1_0_0::hnr10_rgn_dataType) * (HNR10MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for hnr_1_0_0::hnr10_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSHNR10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)           &&
        (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->HNREnable;
            isChanged      = (TRUE == m_moduleEnable);

            /// @todo (CAMX-2012) Face detection parameters need to be consumed from FD.
            m_dependenceData.pFDData = &pInputData->fDData;
        }
        else
        {
            TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
            CAMX_ASSERT(NULL != pTuningManager);

            // Search through the tuning data (tree), only when there
            // are changes to the tuning mode data as an optimization
            if ((TRUE == pInputData->tuningModeChanged)    &&
                (TRUE == pTuningManager->IsValidChromatix()))
            {
                CAMX_ASSERT(NULL != pInputData->pTuningData);

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_hnr10_bps(
                        reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                        pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                /// @todo (CAMX-2109) hardcode prescale ratio to 1, need to calculate based on video or snapshot case selection
                // m_dependenceData.totalScaleRatio = 1.0f;

                /// @todo (CAMX-2012) Face detection parameters need to be consumed from FD.
                m_dependenceData.pFDData = &pInputData->fDData;

                if (m_pChromatix != m_dependenceData.pChromatix)
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = m_pChromatix->enable_section.hnr_nr_enable;

                    isChanged                   = (TRUE == m_moduleEnable);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
            }
        }

        // Disable module if requested by HAL
        if (NoiseReductionModeOff == pInputData->pHALTagsData->noiseReductionMode)
        {
            m_moduleEnable  = FALSE;
            isChanged       = FALSE;
        }

        // Check for trigger update status
        if ((TRUE == m_moduleEnable) &&
            (TRUE == IQInterface::s_interpolationTable.HNR10TriggerUpdate(&pInputData->triggerData, &m_dependenceData)))
        {
            if (NULL == pInputData->pOEMIQSetting)
            {
                // Check for module dynamic enable trigger hysterisis
                m_moduleEnable = IQSettingUtils::GetDynamicEnableFlag(
                    m_dependenceData.pChromatix->dynamic_enable_triggers.hnr_nr_enable.enable,
                    m_dependenceData.pChromatix->dynamic_enable_triggers.hnr_nr_enable.hyst_control_var,
                    m_dependenceData.pChromatix->dynamic_enable_triggers.hnr_nr_enable.hyst_mode,
                    &(m_dependenceData.pChromatix->dynamic_enable_triggers.hnr_nr_enable.hyst_trigger),
                    static_cast<VOID*>(&pInputData->triggerData),
                    &m_dependenceData.moduleEnable);

                // Set status to isChanged to avoid calling RunCalculation if the module is disabled
                isChanged = (TRUE == m_moduleEnable);
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
// BPSHNR10::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10::FetchDMIBuffer()
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
        CAMX_LOG_ERROR(CamxLogGroupBPS, "LUT command Buffer is NULL");
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
// BPSHNR10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            HNR10OutputData outputData;

            outputData.pLNRDMIBuffer          =
                reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(BPSHNRLutBufferSizeDWORD));
            // BET ONLY - InputData is different per module tested
            pInputData->pBetDMIAddr = static_cast<VOID*>(outputData.pLNRDMIBuffer);
            CAMX_ASSERT(NULL != outputData.pLNRDMIBuffer);

            outputData.pFNRAndClampDMIBuffer  =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pLNRDMIBuffer) +
                    BPSLNRLutBufferSize);
            CAMX_ASSERT(NULL != outputData.pFNRAndClampDMIBuffer);

            outputData.pFNRAcDMIBuffer        =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pFNRAndClampDMIBuffer) +
                    BPSFNRAndGAinLutBufferSize);
            CAMX_ASSERT(NULL != outputData.pFNRAcDMIBuffer);

            outputData.pSNRDMIBuffer          =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pFNRAcDMIBuffer) +
                    BPSFNRAcLutBufferSize);
            CAMX_ASSERT(NULL != outputData.pSNRDMIBuffer);

            outputData.pBlendLNRDMIBuffer     =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pSNRDMIBuffer) +
                    BPSSNRLutBufferSize);
            CAMX_ASSERT(NULL != outputData.pBlendLNRDMIBuffer);

            outputData.pBlendSNRDMIBuffer     =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pBlendLNRDMIBuffer) +
                    BPSBlendLNRLutBufferSize);
            CAMX_ASSERT(NULL != outputData.pBlendSNRDMIBuffer);

            m_dependenceData.LUTBankSel      ^= 1;

            result = IQInterface::HNR10CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();

                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pLNRDMIBuffer         = outputData.pLNRDMIBuffer;
                    m_pFNRAndClampDMIBuffer = outputData.pFNRAndClampDMIBuffer;
                    m_pFNRAcDMIBuffer       = outputData.pFNRAcDMIBuffer;
                    m_pSNRDMIBuffer         = outputData.pSNRDMIBuffer;
                    m_pBlendLNRDMIBuffer    = outputData.pBlendLNRDMIBuffer;
                    m_pBlendSNRDMIBuffer    = outputData.pBlendSNRDMIBuffer;
                }
                if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
                {
                    m_pHWSetting->DumpRegConfig();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "HNR10 Calculation Failed %d", result);
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
// BPSHNR10::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHNR10::UpdateBPSInternalData(
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
                CAMX_STATIC_ASSERT(MAX_FACE_NUM <= TuningMaxFaceNumber);
                CAMX_STATIC_ASSERT(BPSHNRLutBufferSize <=
                                   sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.HNRLUT));

                if (NULL != m_pLNRDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.HNRLUT.LNRLUT,
                                  m_pLNRDMIBuffer,
                                  BPSLNRLutBufferSize);
                }
                if (NULL != m_pFNRAndClampDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.HNRLUT.FNRAndClampLUT,
                                  m_pFNRAndClampDMIBuffer,
                                  BPSFNRAndGAinLutBufferSize);
                }
                if (NULL != m_pFNRAcDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.HNRLUT.FNRAcLUT,
                                  m_pFNRAcDMIBuffer,
                                  BPSFNRAcLutBufferSize);
                }
                if (NULL != m_pSNRDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.HNRLUT.SNRLUT,
                                  m_pSNRDMIBuffer,
                                  BPSSNRLutBufferSize);
                }
                if (NULL != m_pBlendLNRDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.HNRLUT.blendLNRLUT,
                                  m_pBlendLNRDMIBuffer,
                                  BPSBlendLNRLutBufferSize);
                }
                if (NULL != m_pBlendSNRDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.HNRLUT.blendSNRLUT,
                                  m_pBlendSNRDMIBuffer,
                                  BPSBlendSNRLutBufferSize);
                }

                TuningFaceData* pHNRFaceDetection = &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSHNRFaceDetection;

                if (NULL != m_dependenceData.pFDData)
                {
                    pHNRFaceDetection->numberOfFace = (m_dependenceData.pFDData->numberOfFace > HNR_V10_MAX_FACE_NUM) ?
                        HNR_V10_MAX_FACE_NUM : m_dependenceData.pFDData->numberOfFace;
                    for (UINT32 count = 0; count < m_dependenceData.pFDData->numberOfFace; count++)
                    {
                        pHNRFaceDetection->faceRadius[count]  = m_dependenceData.pFDData->faceRadius[count];
                        pHNRFaceDetection->faceCenterX[count] = m_dependenceData.pFDData->faceCenterX[count];
                        pHNRFaceDetection->faceCenterY[count] = m_dependenceData.pFDData->faceCenterY[count];
                    }
                }
                if (TRUE == m_moduleEnable)
                {
                    result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                        DebugDataTagID::TuningBPSHNR10PackedLUT,
                        DebugDataTagType::TuningHNR10LUT,
                        1,
                        &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.HNRLUT,
                        sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.HNRLUT));

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                        result = CamxResultSuccess; // Non-fatal error
                    }

                    result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                        DebugDataTagID::TuningBPSHNRFace,
                        DebugDataTagType::TuningFaceData,
                        1,
                        &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSHNRFaceDetection,
                        sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSHNRFaceDetection));

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
// BPSHNR10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result           = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
        }

        pInputData->p64bitDMIBuffer = m_pLUTDMICmdBuffer;
        VOID*      pSettingData     = static_cast<VOID*>(pInputData);

        if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
        {
            result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
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
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }
    CAMX_LOG_INFO(CamxLogGroupPProc, "HNR10 Calculation completed %d", result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHNR10::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10::BPSHNR10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHNR10::BPSHNR10(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type         = ISPIQModuleType::BPSHNR;
    m_moduleEnable = FALSE;
    m_numLUT       = HNRMaxLUT;
    m_pChromatix   = NULL;
    m_pHWSetting   = NULL;
    m_cmdLength    = 0;

    m_dependenceData.moduleEnable = FALSE; ///< First frame is always FALSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10::~BPSHNR10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHNR10::~BPSHNR10()
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
