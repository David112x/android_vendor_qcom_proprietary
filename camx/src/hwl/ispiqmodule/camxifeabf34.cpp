////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeabf34.cpp
/// @brief IFEABF34 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifeabf34titan17x.h"
#include "camxifeabf34.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF34::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEABF34* pModule = CAMX_NEW IFEABF34;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEABF34 object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEABF34 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF34::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEABF34Titan17x;
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
            CAMX_DELETE static_cast<IFEABF34Titan17x*> (m_pHWSetting);
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
// IFEABF34::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF34::Execute(
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
        m_pState = &pInputData->pStripeConfig->stateABF;
        m_pState->dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);

        if (TRUE == CheckDependenceChange(pInputData))
        {
            if (TRUE == pInputData->bankUpdate.isValid)
            {
                m_pState->dependenceData.LUTBankSel = pInputData->bankUpdate.ABFBank;
            }
            result = RunCalculation(pInputData);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
                // Switch LUT Bank select immediately after writing
                m_pState->dependenceData.LUTBankSel ^= 1;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "ABF module calculation Failed.");
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
// IFEABF34::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEABF34::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.ABFEnable = m_moduleEnable;
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
// IFEABF34::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF34::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(abf_3_4_0::abf34_rgn_dataType) * (ABF34MaxmiumNonLeafNode + 1));

    if (NULL == m_pInterpolationData)
    {
        // Alloc for abf_3_4_0::abf34_rgn_dataType
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
// IFEABF34::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEABF34::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged     = FALSE;
    BOOL dynamicEnable = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFEABF));

    if ((NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData) &&
        (NULL != pInputData->pHALTagsData))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->ABFEnable;

            if (TRUE == m_moduleEnable)
            {
                isChanged = TRUE;
            }
            m_noiseReductionMode = NoiseReductionModeFast;
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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_abf34_ife(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if ((NULL                        == m_pState->dependenceData.pChromatix)                ||
                    (m_pChromatix->SymbolTableID != m_pState->dependenceData.pChromatix->SymbolTableID) ||
                    (m_moduleEnable              != m_pChromatix->enable_section.abf_enable))
                {
                    m_pState->dependenceData.pChromatix         = m_pChromatix;
                    m_pState->dependenceData.pInterpolationData = m_pInterpolationData;
                    m_moduleSBPCEnable                          = m_pChromatix->enable_section.sbpc_enable;
                    m_moduleEnable                              = m_pChromatix->enable_section.abf_enable;
                }
            }
        }

        m_noiseReductionMode = pInputData->pHALTagsData->noiseReductionMode;

        if (m_noiseReductionMode == NoiseReductionModeOff)
        {
            m_moduleEnable = FALSE;
            isChanged      = FALSE;
        }

        m_moduleEnable &= dynamicEnable;
        if ((TRUE == m_moduleEnable) && (m_dynamicEnable != dynamicEnable))
        {
            isChanged = TRUE;
        }
        m_dynamicEnable = dynamicEnable;

        // Check for trigger update status
        if ((TRUE == m_moduleEnable || TRUE == m_moduleSBPCEnable) &&
           ((TRUE == pInputData->forceTriggerUpdate)              ||
            (TRUE ==
             IQInterface::s_interpolationTable.IFEABF34TriggerUpdate(&pInputData->triggerData, &m_pState->dependenceData))))
        {
            if (NULL == pInputData->pOEMIQSetting)
            {
                // Check for module dynamic enable trigger hysterisis
                m_moduleEnable = IQSettingUtils::GetDynamicEnableFlag(
                    m_pState->dependenceData.pChromatix->dynamic_enable_triggers.abf_enable.enable,
                    m_pState->dependenceData.pChromatix->dynamic_enable_triggers.abf_enable.hyst_control_var,
                    m_pState->dependenceData.pChromatix->dynamic_enable_triggers.abf_enable.hyst_mode,
                    &(m_pState->dependenceData.pChromatix->dynamic_enable_triggers.abf_enable.hyst_trigger),
                    static_cast<VOID*>(&pInputData->triggerData),
                    &m_pState->dependenceData.moduleEnable);

                // Set status to isChanged to avoid calling RunCalculation if the module is disabled
                isChanged = (TRUE == m_moduleEnable) || (TRUE == m_moduleSBPCEnable);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Pointer");
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF34::PrepareStripingParameters(
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
            if ((TRUE == pInputData->tuningModeChanged)    &&
                (TRUE == pTuningManager->IsValidChromatix()))
            {
                CAMX_ASSERT(NULL != pInputData->pTuningData);

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_abf34_ife(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                m_moduleEnable = m_pChromatix->enable_section.abf_enable ||
                                 m_pChromatix->enable_section.sbpc_enable;
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
// IFEABF34::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEABF34::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult      result   = CamxResultSuccess;
    UINT32*         pDMIAddr = pInputData->p32bitDMIBufferAddr;
    ABF34OutputData outputData;

    pDMIAddr += m_32bitDMIBufferOffsetDword + (pInputData->pStripeConfig->stripeId * m_32bitDMILength);
    CAMX_ASSERT(NULL != pDMIAddr);

    outputData.pDMIData = pDMIAddr;

    result = IQInterface::IFEABF34CalculateSetting(&m_pState->dependenceData, pInputData->pOEMIQSetting, &outputData);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, " ABF Calculation Failed.");
    }

    if (0 != (pInputData->dumpRegConfig &
        (1 << (static_cast<UINT32>(m_type) - OffsetOfIFEIQModuleIndex))))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEABF34::DeallocateCommonLibraryData()
{
    if (NULL != m_pInterpolationData)
    {
        CAMX_FREE(m_pInterpolationData);
        m_pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEABF34::IFEABF34
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEABF34::IFEABF34()
{
    m_type               = ISPIQModuleType::IFEABF;
    m_64bitDMILength     = 0;
    m_moduleEnable       = TRUE;
    m_moduleSBPCEnable   = TRUE;

    m_noiseReductionMode = 0;
    m_pState             = NULL;
    m_pChromatix         = NULL;
    m_pInterpolationData = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEABF34::~IFEABF34
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEABF34::~IFEABF34()
{
    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
