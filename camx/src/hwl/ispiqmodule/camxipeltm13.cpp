////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeltm13.cpp
/// @brief IPELTM class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "camxdefs.h"
#include "camxipeltm13.h"
#include "camxiqinterface.h"
#include "camxispiqmodule.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "ipe_data.h"
#include "parametertuningtypes.h"
#include "camxtitan17xcontext.h"
#include "camxipeltm13titan17x.h"

CAMX_NAMESPACE_BEGIN

static UINT32 gamma64Table[] =
{
    0,    64, 108, 144, 176, 205, 232, 258, 282, 304, 326, 347, 367, 386, 405, 423, 441,
    458, 475, 491, 507, 523, 538, 553, 568, 583, 597, 611, 625, 638, 651, 665, 678, 690,
    703, 715, 728, 740, 752, 764, 775, 787, 798, 810, 821, 832, 843, 854, 865, 875, 886,
    896, 907, 917, 927, 937, 947, 957, 967, 977, 987, 996, 1006, 1015, 1023,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPELTM13* pModule = CAMX_NEW IPELTM13(pCreateData->pNodeIdentifier, pCreateData);

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
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    // Check Module Hardware Version and Create HW Setting Object
    switch (pCreateData->moduleVersion.hwLTMVersion & ISPVersionMask)
    {
        default:
            m_pHWSetting = CAMX_NEW IPELTM13Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_dependenceData.pHWSetting     = static_cast<VOID*>(m_pHWSetting);

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

    if (CamxResultSuccess == result)
    {
        // Precompute Offset of Look up table with LUT command buffer for ease of patching

        m_offsetLUTCmdBuffer[LTMIndexWeight]      = 0;
        m_offsetLUTCmdBuffer[LTMIndexLA0]         = (IPELTMLUTNumEntries[LTMIndexWeight]) * sizeof(UINT32);
        m_offsetLUTCmdBuffer[LTMIndexLA1]         =
            IPELTMLUTNumEntries[LTMIndexLA0] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexLA0];
        m_offsetLUTCmdBuffer[LTMIndexCurve]       =
            IPELTMLUTNumEntries[LTMIndexLA1] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexLA1];

        m_offsetLUTCmdBuffer[LTMIndexScale]       =
            IPELTMLUTNumEntries[LTMIndexCurve] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexCurve];
        m_offsetLUTCmdBuffer[LTMIndexMask]        =
            IPELTMLUTNumEntries[LTMIndexScale] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexScale];

        m_offsetLUTCmdBuffer[LTMIndexLCEPositive] =
            IPELTMLUTNumEntries[LTMIndexMask] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexMask];
        m_offsetLUTCmdBuffer[LTMIndexLCENegative] =
            IPELTMLUTNumEntries[LTMIndexLCEPositive] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexLCEPositive];
        m_offsetLUTCmdBuffer[LTMIndexRGamma0]     =
            IPELTMLUTNumEntries[LTMIndexLCENegative] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexLCENegative];

        m_offsetLUTCmdBuffer[LTMIndexRGamma1]     =
            IPELTMLUTNumEntries[LTMIndexRGamma0] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexRGamma0];
        m_offsetLUTCmdBuffer[LTMIndexRGamma2]     =
            IPELTMLUTNumEntries[LTMIndexRGamma1] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexRGamma1];
        m_offsetLUTCmdBuffer[LTMIndexRGamma3]     =
            IPELTMLUTNumEntries[LTMIndexRGamma2] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexRGamma2];
        m_offsetLUTCmdBuffer[LTMIndexRGamma4]     =
            IPELTMLUTNumEntries[LTMIndexRGamma3] * sizeof(UINT32) + m_offsetLUTCmdBuffer[LTMIndexRGamma3];

        m_dependenceData.pGammaPrev  = m_gammaPrev;
        m_dependenceData.pIGammaPrev = m_igammaPrev;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13::FillCmdBufferManagerParams(
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
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "IPELTM13");
                pResourceParams->resourceSize                 = IPELTM13LUTBufferSizeInDwords * sizeof(UINT32);
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
// IPELTM13::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(ltm_1_3_0::ltm13_rgn_dataType) * (LTM13MaxNoLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for ltm_1_3_0::ltm13_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13::UpdateIPEInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult      result          = CamxResultSuccess;
    IpeIQSettings*  pIPEIQSettings  = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    // set LTM module enable based on ANR status or ipe configIO topology type
    pIPEIQSettings->ltmParameters.moduleCfg.EN = GetLTMEnableStatus(pInputData, pIPEIQSettings);

    m_pHWSetting->SetupInternalData(static_cast<VOID*>(pIPEIQSettings));

    // hardcode to 0 to disable Reverse Gamma. Will set to igamma_en when metadata works for IFE/BPS Gamma Output
    if (TRUE == m_ignoreChromatixRGammaFlag)
    {
        pIPEIQSettings->ltmParameters.moduleCfg.RGAMMA_EN = 0;
    }

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIPETuningMetadata)
    {
        CAMX_STATIC_ASSERT(IPELTM13LUTBufferSize ==
                           sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.LTMLUT));

        if (NULL != m_pLTMLUTs)
        {
            Utils::Memcpy(&pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.LTMLUT,
                          m_pLTMLUTs,
                          IPELTM13LUTBufferSize);
        }

        pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPELTMExposureData.exposureIndex[IPELTMExposureIndexPrevious] =
            m_dependenceData.prevExposureIndex;
        pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPELTMExposureData.exposureIndex[IPELTMExposureIndexCurrent] =
            m_dependenceData.exposureIndex;

        if (TRUE == pIPEIQSettings->ltmParameters.moduleCfg.EN)
        {
            DebugDataTagID ltmLUTTagID;
            DebugDataTagID exposureDataTagID;

            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                ltmLUTTagID       = DebugDataTagID::TuningIPELTM13PackedLUT;
                exposureDataTagID = DebugDataTagID::TuningIPELTMExposureIndex;
            }
            else
            {
                ltmLUTTagID       = DebugDataTagID::TuningIPELTM13PackedLUTOffline;
                exposureDataTagID = DebugDataTagID::TuningIPELTMExposureIndexOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                ltmLUTTagID,
                DebugDataTagType::TuningLTM13LUT,
                1,
                &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.LTMLUT,
                sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEDMIData.LTMLUT));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                result = CamxResultSuccess; // Non-fatal error
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                exposureDataTagID,
                DebugDataTagType::Float,
                CAMX_ARRAY_SIZE(pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPELTMExposureData.exposureIndex),
                &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPELTMExposureData.exposureIndex,
                sizeof(pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPELTMExposureData.exposureIndex));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                result = CamxResultSuccess; // Non-fatal error
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPELTM13::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pHALTagsData)    &&
        (NULL != pInputData->pAECUpdateData)  &&
        (NULL != pInputData->pCalculatedData) &&
        (NULL != pInputData->pHwContext))
    {
        ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->LTMEnable;
            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            if (NULL != pInputData->pTuningData)
            {
                TuningDataManager* pTuningManager = pInputData->pTuningDataManager;

                if (NULL != pTuningManager)
                {
                    if (TRUE == pTuningManager->IsValidChromatix())
                    {
                        // Search through the tuning data (tree), only when there
                        // are changes to the tuning mode data as an optimization
                        if (TRUE == pInputData->tuningModeChanged)
                        {
                            m_pChromatix = pTuningManager->GetChromatix()->GetModule_ltm13_ipe(
                                reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                pInputData->pTuningData->noOfSelectionParameter);

                            m_ptmcChromatix = pTuningManager->GetChromatix()->GetModule_tmc10_sw(
                                reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                pInputData->pTuningData->noOfSelectionParameter);
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Chromatix tuning data");
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "NULL Tuning data pointer");
                }

                m_dependenceData.ltmLceStrength = pInputData->pHALTagsData->ltmContrastStrength.ltmDynamicContrastStrength;
                m_dependenceData.ltmDarkBoostStrength = pInputData->pHALTagsData->ltmContrastStrength.ltmDarkBoostStrength;
                m_dependenceData.ltmBrightSupressStrength =
                    pInputData->pHALTagsData->ltmContrastStrength.ltmBrightSupressStrength;

                if (NULL != m_pChromatix)
                {
                    if ((m_pChromatix != m_dependenceData.pChromatix) ||
                        (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID) ||
                        (m_pChromatix->enable_section.ltm_enable != m_moduleEnable))
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "updating chromatix pointer");
                        m_dependenceData.pChromatix = m_pChromatix;
                        if (NULL != m_ptmcChromatix)
                        {
                            m_pTMCInput.pChromatix      = m_ptmcChromatix;
                        }
                        else
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Failed to get TMC 10 Chromatix, m_ptmcChromatix is NULL");
                        }

                        m_moduleEnable              = m_pChromatix->enable_section.ltm_enable;

                        if (TRUE == m_moduleEnable)
                        {
                            isChanged = TRUE;
                        }
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get Chromatix");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "NULL Tuning Pointer");
            }

            if ((TonemapModeContrastCurve == pHALTagsData->tonemapCurves.tonemapMode))
            {
                m_moduleEnable = FALSE;
                isChanged = FALSE;
            }
        }

        if (TRUE == m_moduleEnable)
        {
            UINT32* pGammaOutput = NULL;

            if ((TRUE == m_useHardcodedGamma) ||
                (FALSE == pInputData->pCalculatedData->gammaOutput.isGammaValid))
            {
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "use hardcodedGamma for iGamma");
                pGammaOutput = &gamma64Table[0];
            }
            else
            {
                pGammaOutput = &pInputData->pCalculatedData->gammaOutput.gammaG[0];
            }

            if (TRUE == IQInterface::s_interpolationTable.LTM13TriggerUpdate(&pInputData->triggerData, &m_dependenceData))
            {
                isChanged = TRUE;
            }

            if (pInputData->parentNodeID == IFE)
            {
                m_dependenceData.prevExposureIndex = m_dependenceData.exposureIndex;
                m_dependenceData.exposureIndex     = m_dependenceData.luxIndex;
            }

            if (TRUE != pInputData->registerBETEn)
            {
                if ((pInputData->frameNum <= (pInputData->maximumPipelineDelay + 1)) || (BPS == pInputData->parentNodeID))
                {
                    // For first valid request the previous exposre index is 0 so assign the current exposre index
                    m_dependenceData.prevExposureIndex = m_dependenceData.exposureIndex;
                }
            }
            else
            {
                // Skip the attenuation for the first valid 3A Input Data
                if ((4 >= pInputData->frameNum) || (BPS == pInputData->parentNodeID))
                {
                    // For first valid request the previous exposre index is 0 so assign the current exposre index
                    m_dependenceData.prevExposureIndex = m_dependenceData.exposureIndex;
                }
            }

            for (INT i = 0; i < 65; i++)
            {
                m_dependenceData.gammaOutput[i] = static_cast<FLOAT>(pGammaOutput[i]);
            }

            if ((m_dependenceData.imageWidth != pInputData->pipelineIPEData.inputDimension.widthPixels) ||
                (m_dependenceData.imageHeight != pInputData->pipelineIPEData.inputDimension.heightLines))
            {
                m_dependenceData.imageWidth  = pInputData->pipelineIPEData.inputDimension.widthPixels;
                m_dependenceData.imageHeight = pInputData->pipelineIPEData.inputDimension.heightLines;
                isChanged = TRUE;
            }

            m_pTMCInput.adrcLTMEnable = FALSE;

            if ((NULL  != m_ptmcChromatix)                                  &&
                (TRUE  == m_ptmcChromatix->enable_section.adrc_isp_enable)  &&
                (TRUE  == m_ptmcChromatix->chromatix_tmc10_reserve.use_ltm) &&
                (SWTMCVersion::TMC10 == pInputData->triggerData.enabledTMCversion))
            {
                isChanged                 = TRUE;
                m_pTMCInput.adrcLTMEnable = TRUE;
                IQInterface::s_interpolationTable.TMC10TriggerUpdate(&pInputData->triggerData, &m_pTMCInput);

                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ADRC_ENABLED DRC gain = %f, DRCGainDark %f",
                    pInputData->triggerData.DRCGain, pInputData->triggerData.DRCGainDark);

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
                        isChanged                 = FALSE;
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Memory allocation failed for ADRC Specific data");
                    }
                }
                pInputData->triggerData.pADRCData = m_pADRCData;
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE LTM : using TMC 10");
            }
            else
            {
                if (NULL != pInputData->triggerData.pADRCData)
                {
                    m_pTMCInput.adrcLTMEnable = pInputData->triggerData.pADRCData->ltmEnable;
                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE LTM : using TMC 11 or 12, version = %u, ltmEnable= %u",
                                    pInputData->triggerData.enabledTMCversion,
                                    m_pTMCInput.adrcLTMEnable);
                }
            }

            m_pTMCInput.pAdrcOutputData = pInputData->triggerData.pADRCData;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
    }

    CAMX_LOG_INFO(CamxLogGroupPProc, "isChanged = %d", isChanged);

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13::FetchDMIBuffer()
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
        CAMX_LOG_ERROR(CamxLogGroupPProc, "LUT Command Buffer manager is NULL");
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
// IPELTM13::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer();

        if (CamxResultSuccess == result)
        {
            LTM13OutputData outputData;

            outputData.pDMIDataPtr = reinterpret_cast<UINT32*>(m_pLUTCmdBuffer->BeginCommands(IPELTM13LUTBufferSizeInDwords));
            CAMX_ASSERT(NULL != outputData.pDMIDataPtr);

            result =
                IQInterface::IPELTM13CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData, &m_pTMCInput);

            if (CamxResultSuccess == result)
            {
                result = m_pLUTCmdBuffer->CommitCommands();

                if (NULL != pInputData->pIPETuningMetadata)
                {
                    m_pLTMLUTs  = outputData.pDMIDataPtr;
                }
            }

            if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
            {
                m_pHWSetting->DumpRegConfig();
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "LTM Calculation Failed.");

                m_pLUTCmdBufferManager->Recycle(m_pLUTCmdBuffer);
                m_pLUTCmdBuffer = NULL;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Cannot get buffer from CmdBufferManager");
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13::Execute(
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
            // This offset holds CDM header of LTM LUTs, store this offset and node will patch it in top level payload
            CmdBuffer*  pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
            m_offsetLUT               = pDMICmdBuffer->GetResourceUsedDwords() * sizeof(UINT32);

            LTMHWSettingParams moduleHWSettingParams = { 0 };

            moduleHWSettingParams.pLUTCmdBuffer       = m_pLUTCmdBuffer;
            moduleHWSettingParams.pOffsetLUTCmdBuffer = &m_offsetLUTCmdBuffer[0];

            result = m_pHWSetting->CreateCmdList(pInputData, reinterpret_cast<UINT32*>(&moduleHWSettingParams));
        }

        if (CamxResultSuccess == result)
        {
            result = UpdateIPEInternalData(pInputData);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Operation failed %d", result);
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
// IPELTM13::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPELTM13::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13::IPELTM13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPELTM13::IPELTM13(
    const CHAR*          pNodeIdentifier,
    IPEModuleCreateData* pCreateData)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type              = ISPIQModuleType::IPELTM;
    m_moduleEnable      = TRUE;
    m_cmdLength         = 0;
    m_numLUT            = LTMIndexMax;
    m_pLUTCmdBuffer     = NULL;

    m_pLTMLUTs          = NULL;
    m_pChromatix        = NULL;
    m_ptmcChromatix     = NULL;
    m_pADRCData         = NULL;

    if (TRUE == pCreateData->initializationData.registerBETEn)
    {
        m_ignoreChromatixRGammaFlag = FALSE;
        m_useHardcodedGamma         = FALSE;
    }
    else
    {
        Titan17xContext*               pContext        = NULL;
        const Titan17xSettingsManager* pSettingManager = NULL;

        pContext                    = static_cast<Titan17xContext*>(pCreateData->initializationData.pHwContext);
        pSettingManager             = pContext->GetTitan17xSettingsManager();
        m_ignoreChromatixRGammaFlag = pSettingManager->GetTitan17xStaticSettings()->ignoreChromatixRGammaFlag;
        m_useHardcodedGamma         = pSettingManager->GetTitan17xStaticSettings()->useHardcodedGamma;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                     "IPE Local Tone Map m_numLUT %d, m_ignoreChromatixRGammaFlag %d, m_useHardcodedGamma %d",
                     m_numLUT,
                     m_ignoreChromatixRGammaFlag,
                     m_useHardcodedGamma);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPELTM13::~IPELTM13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPELTM13::~IPELTM13()
{
    if (NULL != m_pADRCData)
    {
        CAMX_DELETE m_pADRCData;
        m_pADRCData = NULL;
    }

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

    m_pChromatix    = NULL;
    m_ptmcChromatix = NULL;
    DeallocateCommonLibraryData();

    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPELTM13::DumpRegConfig(
    UINT32* pDMIDataPtr
    ) const
{
    CHAR   dumpFilename[256];
    FILE*  pFile = NULL;
    UINT32 LTMWeightCurve[LTM_WEIGHT_LUT_SIZE]   = {};
    UINT32 tempLTMWeightCurveTable[12];
    UINT32 offset               = 0;
    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/IPE_regdump.txt", ConfigFileDirectory);
    pFile = CamX::OsUtils::FOpen(dumpFilename, "a+");

    if (NULL != pFile)
    {
        CamX::OsUtils::FPrintF(pFile, "******** IPE LTM13 ********\n");

        if (TRUE == m_moduleEnable)
        {
            CamX::OsUtils::FPrintF(pFile, "* ltm13Data.wt[2][24] = \n");
            for (UINT index = 0; index < LTM_WEIGHT_LUT_SIZE; index++)
            {
                LTMWeightCurve[index] = static_cast<UINT32>(*(pDMIDataPtr + index));
            }
            for (UINT index = 0; index < LTM_WEIGHT_LUT_SIZE/2; index++)
            {
                if (index % 2 == 0)
                {
                    tempLTMWeightCurveTable[index*2+3] = (LTMWeightCurve[index] & 0xff00) >> 8;
                    tempLTMWeightCurveTable[index*2+1] = (LTMWeightCurve[index] & 0x00ff) >> 0;
                    CAMX_LOG_VERBOSE(CamxLogGroupApp, "tempLTMWeightCurveTable[%d] = %d, LTMWeightCurve[%d] = %d",
                        index*2+3, tempLTMWeightCurveTable[index*2+3], index, LTMWeightCurve[index]);
                    CAMX_LOG_VERBOSE(CamxLogGroupApp, "tempLTMWeightCurveTable[%d] = %d, LTMWeightCurve[%d] = %d",
                        index*2+1, tempLTMWeightCurveTable[index*2+1], index, LTMWeightCurve[index]);
                }
                else
                {
                    tempLTMWeightCurveTable[index*2+0] = (LTMWeightCurve[index] & 0xff00) >> 8;
                    tempLTMWeightCurveTable[index*2-2] = (LTMWeightCurve[index] & 0x00ff) >> 0;
                    CAMX_LOG_VERBOSE(CamxLogGroupApp, "tempLTMWeightCurveTable[%d] = %d, LTMWeightCurve[%d] = %d",
                        index*2+0, tempLTMWeightCurveTable[index*2+0], index, LTMWeightCurve[index]);
                    CAMX_LOG_VERBOSE(CamxLogGroupApp, "tempLTMWeightCurveTable[%d] = %d, LTMWeightCurve[%d] = %d",
                        index*2-2, tempLTMWeightCurveTable[index*2-2], index, LTMWeightCurve[index]);
                }
            }
            for (UINT index = 0; index < LTM_WEIGHT_LUT_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", tempLTMWeightCurveTable[index]);
            }

            CamX::OsUtils::FPrintF(pFile, "\n* ltm13Data.la_curve[2][64] = \n");
            offset += IPELTMLUTNumEntries[LTMIndexWeight];
            for (UINT index = 0; index < LTM_CURVE_LUT_SIZE - 1; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", (*(pDMIDataPtr + offset + index)));
            }

            CamX::OsUtils::FPrintF(pFile, "\n* ltm13Data.ltm_curve[2][64] = \n");
            offset += IPELTMLUTNumEntries[LTMIndexLA0] + IPELTMLUTNumEntries[LTMIndexLA1];
            for (UINT index = 0; index < LTM_CURVE_LUT_SIZE - 1; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", (*(pDMIDataPtr + offset + index)));
            }

            CamX::OsUtils::FPrintF(pFile, "\n* ltm13Data.ltm_scale[2][64] = \n");
            offset += IPELTMLUTNumEntries[LTMIndexCurve];
            for (UINT index = 0; index < LTM_SCALE_LUT_SIZE - 1; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", (*(pDMIDataPtr + offset + index)));
            }

            CamX::OsUtils::FPrintF(pFile, "\n* ltm13Data.mask_rect_curve[2][64] = \n");
            offset += IPELTMLUTNumEntries[LTMIndexScale];
            for (UINT index = 0; index < LTM_SCALE_LUT_SIZE - 1; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", (*(pDMIDataPtr + offset + index)));
            }

            CamX::OsUtils::FPrintF(pFile, "\n* ltm13Data.lce_scale_pos[2][16] = \n");
            offset += IPELTMLUTNumEntries[LTMIndexMask];
            for (UINT index = 0; index < LCE_SCALE_LUT_SIZE - 1; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", (*(pDMIDataPtr + offset + index)));
            }

            CamX::OsUtils::FPrintF(pFile, "* ltm13Data.lce_scale_neg[2][16] = \n");
            offset += IPELTMLUTNumEntries[LTMIndexLCEPositive];
            for (UINT index = 0; index < LCE_SCALE_LUT_SIZE - 1; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", (*(pDMIDataPtr + offset + index)));
            }

            CamX::OsUtils::FPrintF(pFile, "\n* ltm13Data.igamma64[2][64] = \n");
            offset += IPELTMLUTNumEntries[LTMIndexLCENegative];
            for (UINT index = 0; index < LTM_GAMMA_LUT_SIZE; index++)
            {
                CamX::OsUtils::FPrintF(pFile, "           %d", (*(pDMIDataPtr + offset + index)));
            }
        }
        else
        {
            CamX::OsUtils::FPrintF(pFile, "NOT ENABLED");
        }

        CamX::OsUtils::FPrintF(pFile, "\n\n");
        CamX::OsUtils::FClose(pFile);
    }
}

CAMX_NAMESPACE_END
