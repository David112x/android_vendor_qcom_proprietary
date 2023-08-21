////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeabf40.cpp
/// @brief IFEABF40 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifeabf40titan480.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxiqinterface.h"
#include "camxispiqmodule.h"
#include "camxnode.h"
#include "camxifeabf40.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEABF40* pModule = CAMX_NEW IFEABF40;

        if (NULL != pModule)
        {
            CamX::Utils::Memset(&pCreateData->initializationData.pStripeConfig->stateABF,
                                0,
                                sizeof(pCreateData->initializationData.pStripeConfig->stateABF));

            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Module initialization failed");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEABF40 object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEABF40 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEABF40Titan480;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength         = m_pHWSetting->GetCommandLength();
        m_32bitDMILength    = m_pHWSetting->Get32bitDMILength();
        result              = AllocateCommonLibraryData();
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE static_cast<IFEABF40Titan480*>(m_pHWSetting);
            m_pHWSetting     = NULL;
            m_cmdLength      = 0;
            m_32bitDMILength = 0;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData)                    &&
        (NULL != m_pHWSetting)                  &&
        (NULL != pInputData->pCmdBuffer)        &&
        (NULL != pInputData->p32bitDMIBuffer)   &&
        (NULL != pInputData->p32bitDMIBufferAddr))
    {
        m_pState                              = &pInputData->pStripeConfig->stateABF;
        m_pState->dependence40Data.pHWSetting = static_cast<VOID*>(m_pHWSetting);

        if (TRUE == CheckDependenceChange(pInputData))
        {
            if (TRUE == pInputData->bankUpdate.isValid)
            {
                m_pState->dependence40Data.LUTBankSel = pInputData->bankUpdate.ABFBank;
            }
            result = RunCalculation(pInputData);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
                // Switch LUT Bank select immediately after writing
                m_pState->dependence40Data.LUTBankSel ^= 1;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "ABF module calculation Failed.");
            }
        }

        if (FALSE == m_moduleEnable)
        {
            result = m_pHWSetting->SetupRegisterSetting(&m_moduleEnable);

            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateSubCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
            }
        }

        if (CamxResultSuccess == result)
        {
            UpdateIFEInternalData(pInputData);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input: pInputData %p m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEABF40::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.ABFEnable = m_moduleEnable;
    pInputData->pCalculatedMetadata->BLSblackLevelOffset          = m_blacklevelOffset;
    pInputData->triggerData.blackLevelOffset                      = m_blacklevelOffset;
    pInputData->pCalculatedData->noiseReductionMode               = m_noiseReductionMode;

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIFETuningMetadata)
    {
        if (CamxResultSuccess != m_pHWSetting->UpdateTuningMetadata(pInputData->pIFETuningMetadata))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "UpdateTuningMetadata failed.");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(abf_4_0_0::abf40_rgn_dataType) * (ABF40MaxmiumNoLeafNode + 1));

    if (NULL == m_pInterpolationData)
    {
        // Alloc for abf_4_0_0::abf40_rgn_dataType
        m_pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_pInterpolationData)
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupISP, "No memory for interpolation data");
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEABF40::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged      = FALSE;
    BOOL dynamicEnable  = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFEABF));

    if ((NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->ABFEnable;
            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
            CAMX_ASSERT(NULL != pTuningManager);

            // Search through the tuning data (tree), only when there
            // are changes to the tuning mode data as an optimization
            if ((TRUE == pInputData->tuningModeChanged)      &&
                (TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                m_pChromatix    = pTuningManager->GetChromatix()->GetModule_abf40_ife(
                                      reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                      pInputData->pTuningData->noOfSelectionParameter);
                m_pChromatixBLS = pTuningManager->GetChromatix()->GetModule_bls12_ife(
                                      reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                      pInputData->pTuningData->noOfSelectionParameter);
            }
            if ((NULL != m_pChromatix) && (NULL != m_pChromatixBLS))
            {
                if ((NULL                           == m_pState->dependence40Data.pChromatix)                        ||
                    (m_pChromatix->SymbolTableID    != m_pState->dependence40Data.pChromatix->SymbolTableID)         ||
                    (m_pChromatixBLS->SymbolTableID != m_pState->dependence40Data.BLSData.pChromatix->SymbolTableID) ||
                    (m_moduleEnable                 != (m_bilateralEnable | m_minmaxEnable | m_dirsmthEnable)))
                {
                    m_pState->dependence40Data.pChromatix         = m_pChromatix;
                    m_pState->dependence40Data.BLSData.pChromatix = m_pChromatixBLS;
                    m_pState->dependence40Data.pInterpolationData = m_pInterpolationData;

                    m_crossPlaneEnable  = m_pChromatix->chromatix_abf40_reserve.cross_plane_en;
                    m_actAdjEnable      = m_pChromatix->chromatix_abf40_reserve.act_adj_en;
                    m_bilateralEnable   = m_pChromatix->enable_section.bilateral_en;
                    m_minmaxEnable      = m_pChromatix->enable_section.minmax_en;
                    m_dirsmthEnable     = m_pChromatix->enable_section.dirsmth_en;
                    m_blsEnable         = m_pChromatixBLS->enable_section.bls_enable;
                    m_moduleEnable      = (m_bilateralEnable | m_minmaxEnable | m_dirsmthEnable | m_blsEnable);
                    isChanged           = TRUE;
                }
            }
        }

        if ((TRUE == m_moduleEnable)                                        &&
            (TRUE == IQInterface::s_interpolationTable.ABF40TriggerUpdate(
                     &pInputData->triggerData, &m_pState->dependence40Data) ||
            (TRUE == pInputData->forceTriggerUpdate)))
        {
            if (NULL == pInputData->pOEMIQSetting)
            {
                // Check for module dynamic enable trigger hysterisis
                m_bilateralEnable = IQSettingUtils::GetDynamicEnableFlag(
                    m_pState->dependence40Data.pChromatix->dynamic_enable_triggers.bilateral_en.enable,
                    m_pState->dependence40Data.pChromatix->dynamic_enable_triggers.bilateral_en.hyst_control_var,
                    m_pState->dependence40Data.pChromatix->dynamic_enable_triggers.bilateral_en.hyst_mode,
                    &(m_pState->dependence40Data.pChromatix->dynamic_enable_triggers.bilateral_en.hyst_trigger),
                    static_cast<VOID*>(&(pInputData->triggerData)),
                    &m_pState->dependence40Data.moduleEnable);

                m_moduleEnable = (m_bilateralEnable | m_minmaxEnable | m_dirsmthEnable | m_blsEnable);

                // Set status to isChanged to avoid calling RunCalculation if the module is disabled
                isChanged = (TRUE == m_moduleEnable);
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

        m_moduleEnable &= dynamicEnable;
        if ((TRUE == m_moduleEnable) && (m_dynamicEnable != dynamicEnable))
        {
            isChanged = TRUE;
        }
        m_dynamicEnable = dynamicEnable;

        m_pState->dependence40Data.bilateralEnable          = m_bilateralEnable;
        m_pState->dependence40Data.minmaxEnable             = m_minmaxEnable;
        m_pState->dependence40Data.directionalSmoothEnable  = m_dirsmthEnable;
        m_pState->dependence40Data.BLSEnable                = m_blsEnable;
        m_pState->dependence40Data.moduleEnable             = m_moduleEnable;
        m_pState->dependence40Data.crossPlaneEnable         = m_crossPlaneEnable;
        m_pState->dependence40Data.activityAdjustEnable     = m_actAdjEnable;

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Pointer");
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->ABFEnable;
        }
        else
        {
            TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
            CAMX_ASSERT(pTuningManager != NULL);

            // Search through the tuning data (tree), only when there
            // are changes to the tuning mode data as an optimization
            if ((TRUE == pInputData->tuningModeChanged)      &&
                (TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_abf40_ife(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                m_moduleEnable = m_pChromatix->enable_section.bilateral_en ||
                                 m_pChromatix->enable_section.dirsmth_en   ||
                                 m_pChromatix->enable_section.minmax_en;
            }
        }

        if (NULL != pInputData->pStripingInput)
        {
            pInputData->pStripingInput->enableBits.ABF = m_moduleEnable;
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Data");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF40::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult      result   = CamxResultSuccess;
    UINT32*         pDMIAddr = pInputData->p32bitDMIBufferAddr;
    ABF40OutputData outputData;

    pDMIAddr += m_32bitDMIBufferOffsetDword + (pInputData->pStripeConfig->stripeId * m_32bitDMILength);

    CAMX_ASSERT(NULL != pDMIAddr);

    outputData.type         = PipelineType::IFE;
    outputData.pNoiseLUT    = pDMIAddr;
    outputData.pActivityLUT = reinterpret_cast<UINT32*>((reinterpret_cast<UCHAR*>(outputData.pNoiseLUT) +
                              IFEABF40NoiseLUTSize));
    outputData.pDarkLUT     = reinterpret_cast<UINT32*>((reinterpret_cast<UCHAR*>(outputData.pActivityLUT) +
                              IFEABF40ActivityLUTSize));

    result                  = IQInterface::ABF40CalculateSetting(&m_pState->dependence40Data,
                                                                 pInputData->pOEMIQSetting,
                                                                 &outputData);

    m_blacklevelOffset      = outputData.blackLevelOffset;

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, " ABF Calculation Failed.");
    }

    if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEABF40::DeallocateCommonLibraryData()
{
    if (NULL != m_pInterpolationData)
    {
        CAMX_FREE(m_pInterpolationData);
        m_pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::IFEABF40
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEABF40::IFEABF40()
{
    m_type           = ISPIQModuleType::IFEABF;
    m_64bitDMILength = 0;
    m_moduleEnable   = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF40::~IFEABF40
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEABF40::~IFEABF40()
{
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
