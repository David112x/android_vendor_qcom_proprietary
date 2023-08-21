////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipetf10.cpp
/// @brief CAMXIPETF10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcdmdefs.h"
#include "camxdefs.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxiqinterface.h"
#include "camxispiqmodule.h"
#include "camxnode.h"
#include "ipe_data.h"
#include "camxipetf10.h"
#include "camxtitan17xcontext.h"
#include "NcLibContext.h"
#include "camxipetf10titan17x.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF10::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPETF10* pModule = CAMX_NEW IPETF10(pCreateData->pNodeIdentifier, pCreateData);

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
            CAMX_ASSERT_ALWAYS_MESSAGE("Memory allocation failed");
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
// IPETF10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF10::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting = NULL;

    switch(static_cast<Titan17xContext*>(pCreateData->initializationData.pHwContext)->GetTitanVersion())
    {
        default:
            m_pHWSetting = CAMX_NEW IPETF10Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        UINT size = m_pHWSetting->GetRegSize();

        m_singlePassCmdLength = (size + (cdm_get_cmd_header_size(CDMCmdRegContinuous) * 4));
        m_cmdLength = m_singlePassCmdLength * PASS_NAME_MAX;

        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);

        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to initilize common library data, no memory");
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, no memory");
        result = CamxResultENoMemory;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF10::SetMaxSupportedPassesForUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPETF10::SetMaxSupportedPassesForUsecase(
    const ISPInputData* pInputData)
{
    if ((pInputData->pTuningData->TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase ==
        ChiModeUsecaseSubModeType::Video) ||
        (pInputData->pTuningData->TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase ==
            ChiModeUsecaseSubModeType::Preview))
    {
        m_dependenceData.maxSupportedPassesForUsecase = PASS_NAME_MAX - 1;
    }
    else if ((pInputData->pTuningData->TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase ==
        ChiModeUsecaseSubModeType::Snapshot) ||
        (pInputData->pTuningData->TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase ==
            ChiModeUsecaseSubModeType::Liveshot) ||
            (pInputData->pTuningData->TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase ==
                ChiModeUsecaseSubModeType::ZSL))
    {
        m_dependenceData.maxSupportedPassesForUsecase = PASS_NAME_MAX;
    }
    else
    {
        m_dependenceData.maxSupportedPassesForUsecase = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF10::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF10::ValidateDependenceParams(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-730) validate dependency parameters
    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IQSettingsPtr [0x%p] ", pInputData->pipelineIPEData.pIPEIQSettings);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPETF10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                 &&
        (NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData) &&
        (NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pHALTagsData))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->TF10Enable;
            isChanged      = (TRUE == m_moduleEnable);
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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_tf10_ipe(
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
                    m_moduleEnable              = m_pChromatix->enable_section.master_en;
                    isChanged                   = (TRUE == m_moduleEnable);
                }

                m_bypassMode = FALSE;
                m_dependenceData.bypassMode = FALSE;
                if ((NoiseReductionModeOff     == pInputData->pHALTagsData->noiseReductionMode) ||
                    (NoiseReductionModeMinimal == pInputData->pHALTagsData->noiseReductionMode) ||
                    ((FALSE == pInputData->pipelineIPEData.isLowResolution) &&
                    (NoiseReductionModeZeroShutterLag == pInputData->pHALTagsData->noiseReductionMode)))
                {
                    m_bypassMode                = TRUE;
                    m_dependenceData.bypassMode = TRUE;
                }

                CAMX_LOG_INFO(CamxLogGroupPProc, "NR mode %d, m_bypassMode %d isLowResolution %d",
                    pInputData->pHALTagsData->noiseReductionMode, m_bypassMode, pInputData->pipelineIPEData.isLowResolution);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get chromatix pointer");
            }
        }

        // Check for trigger update status
        if ((TRUE == m_moduleEnable) &&
            (TRUE == IQInterface::s_interpolationTable.TF10TriggerUpdate(&pInputData->triggerData, &m_dependenceData)))
        {
            if (NULL == pInputData->pOEMIQSetting)
            {
                // Check for module dynamic enable trigger hysterisis
                m_moduleEnable = IQSettingUtils::GetDynamicEnableFlag(
                    m_dependenceData.pChromatix->dynamic_enable_triggers.master_en.enable,
                    m_dependenceData.pChromatix->dynamic_enable_triggers.master_en.hyst_control_var,
                    m_dependenceData.pChromatix->dynamic_enable_triggers.master_en.hyst_mode,
                    &(m_dependenceData.pChromatix->dynamic_enable_triggers.master_en.hyst_trigger),
                    static_cast<VOID*>(&pInputData->triggerData),
                    &m_dependenceData.moduleEnable);

                // Set the module status to avoid calling RunCalculation if it is disabled
                isChanged = (TRUE == m_moduleEnable);
            }
        }

        if (TRUE == m_moduleEnable)
        {
            if ((TRUE != CheckIPEInstanceProperty(pInputData))                                              ||
                (m_dependenceData.fullPassIcaOutputFrameHeight !=
                    pInputData->pipelineIPEData.inputDimension.heightLines)                                 ||
                (m_dependenceData.fullPassIcaOutputFrameWidth !=
                    pInputData->pipelineIPEData.inputDimension.widthPixels)                                 ||
                (m_dependenceData.numOfFrames != pInputData->pipelineIPEData.numOfFrames)                   ||
                (m_dependenceData.mfFrameNum != pInputData->mfFrameNum)                                     ||
                (m_dependenceData.hasTFRefInput != pInputData->pipelineIPEData.hasTFRefInput)               ||
                (m_dependenceData.upscalingFactorMFSR != pInputData->pipelineIPEData.upscalingFactorMFSR)   ||
                (m_dependenceData.isDigitalZoomEnabled != pInputData->pipelineIPEData.isDigitalZoomEnabled) ||
                (m_dependenceData.digitalZoomStartX != pInputData->pipelineIPEData.digitalZoomStartX)       ||
                (m_dependenceData.digitalZoomStartY != pInputData->pipelineIPEData.digitalZoomStartY))
            {
                m_dependenceData.fullPassIcaOutputFrameHeight = pInputData->pipelineIPEData.inputDimension.heightLines;
                m_dependenceData.fullPassIcaOutputFrameWidth  = pInputData->pipelineIPEData.inputDimension.widthPixels;
                m_dependenceData.mfFrameNum                   = pInputData->mfFrameNum;
                m_dependenceData.numOfFrames                  = pInputData->pipelineIPEData.numOfFrames;
                m_dependenceData.upscalingFactorMFSR          = pInputData->pipelineIPEData.upscalingFactorMFSR;
                m_dependenceData.isDigitalZoomEnabled         = pInputData->pipelineIPEData.isDigitalZoomEnabled;
                m_dependenceData.digitalZoomStartX            = pInputData->pipelineIPEData.digitalZoomStartX;
                m_dependenceData.digitalZoomStartY            = pInputData->pipelineIPEData.digitalZoomStartY;
                m_dependenceData.pWarpGeometriesOutput        = pInputData->pipelineIPEData.pWarpGeometryData;
                m_dependenceData.hasTFRefInput                = pInputData->pipelineIPEData.hasTFRefInput;
                SetMaxSupportedPassesForUsecase(pInputData);
                isChanged = TRUE;
            }
            // get address of TF parameters and refinement parameter from firmware data
            m_dependenceData.pRefinementParameters = &m_refinementParams;
            m_dependenceData.pTFParameters         = &m_TFParams;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "TF10 Module is not enabled");
        }
        if (m_moduleEnable != m_dependenceData.moduleEnable)
        {
            isChanged = TRUE;
            m_dependenceData.moduleEnable = m_moduleEnable;
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
// IPETF10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF10::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult     result     = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Number of Passes [0x%d] with MF Config %d",
                  m_dependenceData.maxUsedPasses,
                  pInputData->pipelineIPEData.instanceProperty.processingType);

    if (TRUE == m_enableCommonIQ)
    {

        // running calculation
        result = IQInterface::IPETF10CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE TF10 Calculation Failed.");
        }
    }
    else
    {
        // Default values will be filled in HWSettings
    }

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);

        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
            }

            if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
            {
                result = m_pHWSetting->CreateCmdList(pInputData, NULL);
            }

            if (CamxResultSuccess == result)
            {
                if (CSLCameraTitanVersion::CSLTitan150 == pInputData->titanVersion)
                {
                    // For Talos Refinement should be on ICA2
                    m_refinementParams.icaNum = 1;
                }
                IpeIQSettings*  pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
                Utils::Memcpy(&pIPEIQSettings->refinementParameters, &m_refinementParams,
                    sizeof(pIPEIQSettings->refinementParameters));
                TF10internalData inputData;
                inputData.pISPInputData        = pInputData;
                inputData.pDependenceData      = &m_dependenceData;
                inputData.bValidateTFParams    = m_validateTFParams;
                inputData.bDisableTFRefinement = m_disableTFRefinement;
                inputData.pOffsetPass          = m_offsetPass;
                result = m_pHWSetting->SetupInternalData(&inputData);

                if (CamxResultSuccess == result)
                {
                    result = m_pHWSetting->UpdateTuningMetadata(pInputData);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_WARN(CamxLogGroupPProc, "Update Tuning Metadata failed");
                        result = CamxResultSuccess; // Non-fatal error
                    }
                }
            }

            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Operation failed %d", result);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPETF10::GetModuleData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPETF10::GetModuleData(
    VOID* pModuleData)

{
    CAMX_ASSERT(NULL != pModuleData);
    IPEIQModuleData* pData = reinterpret_cast<IPEIQModuleData*>(pModuleData);

    // data is expected to be filled after execute
    pData->singlePassCmdLength = m_singlePassCmdLength;
    Utils::Memcpy(pData->offsetPass, m_offsetPass, sizeof(pData->offsetPass));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF10::CheckIPEInstanceProperty
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPETF10::CheckIPEInstanceProperty(
    ISPInputData* pInput)
{
    BOOL   isChanged   = FALSE;
    UINT8  useCase     = CONFIG_STILL;
    UINT8  configMF    = MF_CONFIG_NONE;
    UINT32 maxUsedPass = pInput->pipelineIPEData.numPasses;

    UINT32 perspectiveConfidence = 256;

    CAMX_ASSERT(NULL != pInput);

    IPEInstanceProperty* pInstanceProperty = &pInput->pipelineIPEData.instanceProperty;
    if (pInstanceProperty->processingType == IPEProcessingType::IPEMFNRBlend ||
        pInstanceProperty->processingType == IPEProcessingType::IPEMFSRBlend)
    {
        configMF = MF_CONFIG_TEMPORAL;
    }
    else if (pInstanceProperty->processingType == IPEProcessingType::IPEMFNRPrefilter ||
             pInstanceProperty->processingType == IPEProcessingType::IPEMFSRPrefilter)
    {
        configMF    = MF_CONFIG_PREFILT;
    }
    else if (pInstanceProperty->processingType == IPEProcessingType::IPEMFNRPostfilter ||
             pInstanceProperty->processingType == IPEProcessingType::IPEMFSRPostfilter)
    {
        if (pInstanceProperty->profileId == IPEProfileId::IPEProfileIdNPS)
        {
            configMF = MF_CONFIG_TEMPORAL;
        }
        else if (pInstanceProperty->profileId == IPEProfileId::IPEProfileIdDefault)
        {
            configMF = MF_CONFIG_POSTPROCESS;
            maxUsedPass = 1;
        }
    }
    else if (pInstanceProperty->processingType == IPEProcessingType::IPEMFNRScale)
    {
        configMF = MF_CONFIG_NONE;
    }
    else if ((pInstanceProperty->processingType == IPEProcessingType::IPEProcessingTypeDefault) ||
             (pInstanceProperty->processingType == IPEProcessingType::IPEProcessingPreview))
    {
        if (pInput->pipelineIPEData.numOutputRefPorts == 4)
        {
            useCase  = CONFIG_STILL;
            configMF = MF_CONFIG_NONE;
        }
        else if (pInput->pipelineIPEData.numOutputRefPorts == 3)
        {
            useCase     = CONFIG_VIDEO;
            configMF    = MF_CONFIG_NONE;
            maxUsedPass = PASS_NAME_MAX - 1;
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupPProc,
                          "Wrong number of output reference ports = %d",
                           pInput->pipelineIPEData.numOutputRefPorts);
            useCase = CONFIG_STILL;
            configMF = MF_CONFIG_NONE;
            maxUsedPass = 1;
            CAMX_LOG_INFO(CamxLogGroupPProc, "TF Blend on Full pass");
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Wrong processingType %d = ", pInstanceProperty->processingType);
        useCase     = CONFIG_STILL;
        configMF    = MF_CONFIG_NONE;
        maxUsedPass = 1;
    }

    if (0 != (IPEStabilizationType::IPEStabilizationMCTF & pInstanceProperty->stabilizationType))
    {
        perspectiveConfidence = pInput->ICAConfigData.ICAReferenceParams.perspectiveConfidence;
    }
    else
    {
        perspectiveConfidence = 256;
    }

    if (m_dependenceData.useCase != useCase)
    {
        isChanged                = TRUE;
        m_dependenceData.useCase = useCase;
    }

    if (m_dependenceData.configMF != configMF)
    {
        isChanged                 = TRUE;
        m_dependenceData.configMF = configMF;
    }

    if (m_dependenceData.maxUsedPasses != maxUsedPass)
    {
        isChanged                      = TRUE;
        m_dependenceData.maxUsedPasses = maxUsedPass;
    }

    if (m_dependenceData.perspectiveConfidence != perspectiveConfidence)
    {
        isChanged                              = TRUE;
        m_dependenceData.perspectiveConfidence = perspectiveConfidence;
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MCTF case: enable TransformConfidence");
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF10::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;
    TFNcLibOutputData data;

    result = IQInterface::IPETF10GetInitializationData(&data);
    CAMX_ASSERT(CamxResultSuccess == result);

    // +1 for ouput of interpolation library
    UINT interpolationSize = (sizeof(tf_1_0_0::mod_tf10_cct_dataType::cct_dataStruct) *
        (TFMaxNonLeafNode + 1));
    if (NULL == m_dependenceData.pInterpolationData)
    {
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    if ((NULL == m_dependenceData.pNCChromatix) && (CamxResultSuccess == result))
    {
        m_dependenceData.pNCChromatix = CAMX_CALLOC(data.TFChromatixSize);
        if (NULL == m_dependenceData.pNCChromatix)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPETF10::DeallocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }

    if (NULL != m_dependenceData.pNCChromatix)
    {
        CAMX_FREE(m_dependenceData.pNCChromatix);
        m_dependenceData.pNCChromatix = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETF10::IPETF10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPETF10::IPETF10(
    const CHAR*          pNodeIdentifier,
    IPEModuleCreateData* pCreateData)
{
    m_pNodeIdentifier       = pNodeIdentifier;
    m_type                  = ISPIQModuleType::IPETF;
    m_moduleEnable          = TRUE;
    m_32bitDMILength        = 0;
    m_64bitDMILength        = 0;
    m_cmdLength             = 0; // m_IPETFCmdBufferSize * PASS_NAME_MAX;
    m_singlePassCmdLength   = 0; // m_IPETFCmdBufferSize;
    m_bypassMode            = FALSE;
    m_pChromatix            = NULL;

    m_dependenceData.moduleEnable = FALSE;   ///< First frame is always FALSE

    Utils::Memset(&m_refinementParams, 0x0, sizeof(m_refinementParams));
    Utils::Memset(&m_TFParams, 0x0, sizeof(m_TFParams));


    if (TRUE == pCreateData->initializationData.registerBETEn)
    {
        m_enableCommonIQ      = TRUE;
        m_validateTFParams    = FALSE;
        m_disableTFRefinement = FALSE;
    }
    else
    {
        Titan17xContext* pContext = static_cast<Titan17xContext*>(pCreateData->initializationData.pHwContext);
        m_enableCommonIQ          = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableTFCommonIQModule;
        m_validateTFParams        = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->validateTFParams;
        m_disableTFRefinement     = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->disableTFRefinement;
    }

    CAMX_LOG_INFO(CamxLogGroupPProc, "IPE TF m_cmdLength %d ", m_cmdLength);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPETF10::~IPETF10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPETF10::~IPETF10()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }

    DeallocateCommonLibraryData();
    m_pChromatix = NULL;
}


CAMX_NAMESPACE_END
