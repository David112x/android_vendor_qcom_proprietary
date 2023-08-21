////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipehnr10.cpp
/// @brief ipehnr10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcdmdefs.h"
#include "camxdefs.h"
#include "camxipehnr10.h"
#include "camxiqinterface.h"
#include "camxtitan17xcontext.h"
#include "camxtuningdatamanager.h"
#include "camxisphwsetting.h"
#include "hnr10setting.h"
#include "parametertuningtypes.h"
#include "camxipehnr10titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEHNR10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEHNR10::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPEHNR10* pModule = CAMX_NEW IPEHNR10(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Module initialization failed !!");
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
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEHNR10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEHNR10::Initialize(
    IPEModuleCreateData* pCreateData)
{
    ResourceParams cmdBufferConfig = { 0 };
    CamxResult     result          = CamxResultSuccess;
    ISPInputData*  pInputData      = &pCreateData->initializationData;

    m_pHWSetting = NULL;

    switch (pCreateData->titanVersion)
    {
        default:
            m_pHWSetting = CAMX_NEW IPEHNR10Titan480;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        UINT regCmdSize = m_pHWSetting->GetRegSize();
        UINT size       = PacketBuilder::RequiredWriteRegRangeSizeInDwords(regCmdSize / RegisterWidthInBytes);

        m_cmdLength = size * sizeof(UINT32);

        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        result = AllocateCommonLibraryData();
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
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
// IPEHNR10::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEHNR10::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "IPEHNR10");
                pResourceParams->resourceSize                 = IPEHNRLutBufferSize;
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
// IPEHNR10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEHNR10::AllocateCommonLibraryData()
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
// IPEHNR10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEHNR10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)           &&
        (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->HNREnable;
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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_hnr10_ipe(
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
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get Chromatix");
            }
        }

        // Disable module if requested by HAL
        m_noiseReductionMode = pInputData->pHALTagsData->noiseReductionMode;
        if (m_noiseReductionMode == NoiseReductionModeOff)
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
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input: pHwContext %p", pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEHNR10::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEHNR10::FetchDMIBuffer()
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
        CAMX_LOG_ERROR(CamxLogGroupPProc, "LUT command Buffer is NULL");
    }

    if (CamxResultSuccess == result)
    {
        m_pLUTDMICmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }
    else
    {
        m_pLUTDMICmdBuffer = NULL;
        result             = CamxResultEUnableToLoad;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEHNR10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEHNR10::RunCalculation(
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
                reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(IPEHNRLutBufferSizeDWORD));

            outputData.pHNRParameters         = &m_HNRParameters;

            outputData.pFNRAndClampDMIBuffer  =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pLNRDMIBuffer) +
                    IPELNRLutBufferSize);
            CAMX_ASSERT(NULL != outputData.pFNRAndClampDMIBuffer);

            outputData.pFNRAcDMIBuffer        =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pFNRAndClampDMIBuffer) +
                    IPEFNRAndGAinLutBufferSize);
            CAMX_ASSERT(NULL != outputData.pFNRAcDMIBuffer);

            outputData.pSNRDMIBuffer          =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pFNRAcDMIBuffer) +
                    IPEFNRAcLutBufferSize);
            CAMX_ASSERT(NULL != outputData.pSNRDMIBuffer);

            outputData.pBlendLNRDMIBuffer     =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pSNRDMIBuffer) +
                    IPESNRLutBufferSize);
            CAMX_ASSERT(NULL != outputData.pBlendLNRDMIBuffer);

            outputData.pBlendSNRDMIBuffer     =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pBlendLNRDMIBuffer) +
                    IPEBlendLNRLutBufferSize);
            CAMX_ASSERT(NULL != outputData.pBlendSNRDMIBuffer);

            m_dependenceData.LUTBankSel      ^= 1;

            result = IQInterface::HNR10CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();

                if (NULL != pInputData->pIPETuningMetadata)
                {
                    m_pLNRDMIBuffer         = outputData.pLNRDMIBuffer;
                    m_pFNRAndClampDMIBuffer = outputData.pFNRAndClampDMIBuffer;
                    m_pFNRAcDMIBuffer       = outputData.pFNRAcDMIBuffer;
                    m_pSNRDMIBuffer         = outputData.pSNRDMIBuffer;
                    m_pBlendLNRDMIBuffer    = outputData.pBlendLNRDMIBuffer;
                    m_pBlendSNRDMIBuffer    = outputData.pBlendSNRDMIBuffer;
                }
                if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
                {
                    m_pHWSetting->DumpRegConfig();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "HNR10 Calculation Failed %d", result);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid input data pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEHNR10::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEHNR10::UpdateIPEInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult     result         = CamxResultSuccess;
    IpeIQSettings* pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    if (NULL != pIPEIQSettings)
    {
        CamX::Utils::Memcpy(&pIPEIQSettings->hnrParameters, &m_HNRParameters, sizeof(HnrParameters));
        pIPEIQSettings->hnrParameters.moduleCfg.EN = (TRUE == pInputData->pipelineIPEData.realtimeFlag) ? 0 : m_moduleEnable;
        pInputData->pCalculatedData->noiseReductionMode = m_noiseReductionMode;

        // Post tuning metadata if module and setting is enabled
        if ((NULL != pInputData->pIPETuningMetadata) &&
            (TRUE == pIPEIQSettings->hnrParameters.moduleCfg.EN))
        {
            CAMX_STATIC_ASSERT(MAX_FACE_NUM <= TuningMaxFaceNumber);
            CAMX_STATIC_ASSERT(IPEHNRLutBufferSize ==
                               sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.HNRLUT));

            result = m_pHWSetting->UpdateTuningMetadata(pInputData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                result = CamxResultSuccess; // Non-fatal error
            }

            if (NULL != m_pLNRDMIBuffer)
            {
                Utils::Memcpy(&pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.HNRLUT.LNRLUT,
                              m_pLNRDMIBuffer,
                              IPELNRLutBufferSize);
            }
            if (NULL != m_pFNRAndClampDMIBuffer)
            {
                Utils::Memcpy(&pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.HNRLUT.FNRAndClampLUT,
                              m_pFNRAndClampDMIBuffer,
                              IPEFNRAndGAinLutBufferSize);
            }
            if (NULL != m_pFNRAcDMIBuffer)
            {
                Utils::Memcpy(&pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.HNRLUT.FNRAcLUT,
                              m_pFNRAcDMIBuffer,
                              IPEFNRAcLutBufferSize);
            }
            if (NULL != m_pSNRDMIBuffer)
            {
                Utils::Memcpy(&pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.HNRLUT.SNRLUT,
                              m_pSNRDMIBuffer,
                              IPESNRLutBufferSize);
            }
            if (NULL != m_pBlendLNRDMIBuffer)
            {
                Utils::Memcpy(&pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.HNRLUT.blendLNRLUT,
                              m_pBlendLNRDMIBuffer,
                              IPEBlendLNRLutBufferSize);
            }
            if (NULL != m_pBlendSNRDMIBuffer)
            {
                Utils::Memcpy(&pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.HNRLUT.blendSNRLUT,
                              m_pBlendSNRDMIBuffer,
                              IPEBlendSNRLutBufferSize);
            }

            TuningFaceData* pHNRFaceDetection = &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEHNRFaceDetection;

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

            // Write tuning-metadata
            DebugDataTagID hnrLUTTagID;
            DebugDataTagID hnrFaceDataTagID;

            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                hnrLUTTagID      = DebugDataTagID::TuningIPEHNR10PackedLUT;
                hnrFaceDataTagID = DebugDataTagID::TuningIPEHNRFace;
            }
            else
            {
                hnrLUTTagID      = DebugDataTagID::TuningIPEHNR10PackedLUTOffline;
                hnrFaceDataTagID = DebugDataTagID::TuningIPEHNRFaceOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                hnrLUTTagID,
                DebugDataTagType::TuningHNR10LUT,
                1,
                &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.HNRLUT,
                sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEDMIData.HNRLUT));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                result = CamxResultSuccess; // Non-fatal error
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                hnrFaceDataTagID,
                DebugDataTagType::TuningFaceData,
                1,
                &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEHNRFaceDetection,
                sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEHNRFaceDetection));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                result = CamxResultSuccess; // Non-fatal error
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEHNR10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEHNR10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
        }

        // Regardless of any update in dependency parameters, command buffers and IQSettings/Metadata shall be updated.
        if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
        {
            // Record offset where CDM DMI header starts for LUTs for the module
            // Patch the LUTs DMI buffer into CDM pack
            CmdBuffer* pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
            m_offsetLUT              = (pDMICmdBuffer->GetResourceUsedDwords() * sizeof(UINT32));

            HNRHwSettingParams moduleHWSettingParams = { 0 };
            moduleHWSettingParams.pLUTDMICmdBuffer   = m_pLUTDMICmdBuffer;

            result = m_pHWSetting->CreateCmdList(pInputData, reinterpret_cast<UINT32*>(&moduleHWSettingParams));
        }

        if (CamxResultSuccess == result)
        {
            UpdateIPEInternalData(pInputData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Operation failed %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid pointer pInputData %p, m_pHWSetting %p",
            pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEHNR10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEHNR10::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEHNR10::IPEHNR10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEHNR10::IPEHNR10(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier  = pNodeIdentifier;
    m_type             = ISPIQModuleType::IPEHNR;
    m_moduleEnable     = TRUE;
    m_numLUT           = HNRMaxLUT;
    m_pChromatix       = NULL;
    m_pLUTDMICmdBuffer = NULL;
    m_cmdLength        = 0;

    m_dependenceData.moduleEnable = FALSE; ///< First frame is always FALSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEHNR10::~IPEHNR10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEHNR10::~IPEHNR10()
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
