////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebpcbcc50.cpp
/// @brief CAMXIFEBPCBCC50 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifebpcbcc50titan17x.h"
#include "camxifebpcbcc50.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBPCBCC50::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEBPCBCC50* pModule = CAMX_NEW IFEBPCBCC50;

        if (NULL != pModule)
        {
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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEBPCBCC50 object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEBPCBCC50 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBPCBCC50::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEBPCBCC50Titan17x;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength                 = m_pHWSetting->GetCommandLength();
        result                      = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_DELETE static_cast<IFEBPCBCC50Titan17x*>(m_pHWSetting);
            m_pHWSetting                = NULL;
            m_dependenceData.pHWSetting = NULL;
            m_cmdLength                 = 0;
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
// IFEBPCBCC50::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBPCBCC50::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(bpcbcc_5_0_0::bpcbcc50_rgn_dataType) * (BPCBCC50MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for bpcbcc_5_0_0::bpcbcc50_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBPCBCC50::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if (NULL != pInputData && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
            }
        }

        if (CamxResultSuccess == result)
        {
            UpdateIFEInternalData(pInputData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "BPC module calculation failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input: pInputData %p, m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBPCBCC50::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.BPCEnable     = m_moduleEnable;
    pInputData->pCalculatedMetadata->hotPixelMode                     = m_hotPixelMode;

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
// IFEBPCBCC50::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEBPCBCC50::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                 &&
        (NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData) &&
        (NULL != pInputData->pHALTagsData))
    {

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->BPCBCCEnable;

            if (TRUE == m_moduleEnable)
            {
                isChanged = TRUE;
            }
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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_bpcbcc50_ife(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if ((NULL == m_dependenceData.pChromatix)                                       ||
                    (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID) ||
                    (m_moduleEnable              != m_pChromatix->enable_section.bpcbcc_enable))
                {
                    m_moduleEnable                   = m_pChromatix->enable_section.bpcbcc_enable;
                    m_dependenceData.pChromatix      = m_pChromatix;
                    m_dependenceData.symbolIDChanged = TRUE;
                    if (TRUE == m_moduleEnable)
                    {
                        isChanged = TRUE;
                    }
                }

                m_hotPixelMode = pInputData->pHALTagsData->hotPixelMode;
                if ((TRUE == m_moduleEnable) &&
                    (HotPixelModeOff == pInputData->pHALTagsData->hotPixelMode))
                {
                    m_moduleEnable = FALSE;
                    isChanged      = FALSE;
                }
            }
        }

        // Check for trigger update status
        if ((TRUE == m_moduleEnable) &&
            ((TRUE == IQInterface::s_interpolationTable.IFEBPCBCC50TriggerUpdate(&pInputData->triggerData,
                                                                                 &m_dependenceData))       ||
            (TRUE == pInputData->forceTriggerUpdate)))
        {
            if (NULL == pInputData->pOEMIQSetting)
            {
                // Check for module dynamic enable trigger hysterisis
                m_moduleEnable = IQSettingUtils::GetDynamicEnableFlag(
                    m_dependenceData.pChromatix->dynamic_enable_triggers.bpcbcc_enable.enable,
                    m_dependenceData.pChromatix->dynamic_enable_triggers.bpcbcc_enable.hyst_control_var,
                    m_dependenceData.pChromatix->dynamic_enable_triggers.bpcbcc_enable.hyst_mode,
                    &(m_dependenceData.pChromatix->dynamic_enable_triggers.bpcbcc_enable.hyst_trigger),
                    static_cast<VOID*>(&pInputData->triggerData),
                    &m_dependenceData.moduleEnable);

                // Set status to isChanged to avoid calling RunCalculation if the module is disabled
                isChanged = (TRUE == m_moduleEnable);
            }

            m_dependenceData.nonHdrMultFactor = 1.0f;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "pInputData is NULL ");
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBPCBCC50::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult                            result               = CamxResultSuccess;
    TuningDataManager*                    pTuningManager       = NULL;
    bpcbcc_5_0_0::chromatix_bpcbcc50Type* pBPCBCCChromatixData = NULL;

    if (NULL != pInputData)
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->BPCBCCEnable;
        }
        else
        {
            pTuningManager  = pInputData->pTuningDataManager;

            CAMX_ASSERT(NULL != pTuningManager);

            if ((TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                pBPCBCCChromatixData = pTuningManager->GetChromatix()->GetModule_bpcbcc50_ife(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }


            if (NULL != pBPCBCCChromatixData)
            {
                if ((NULL == m_dependenceData.pChromatix) ||
                    (m_dependenceData.pChromatix->SymbolTableID != pBPCBCCChromatixData->SymbolTableID))
                {
                    m_moduleEnable = pBPCBCCChromatixData->enable_section.bpcbcc_enable;
                }
            }
        }
        if (NULL != pInputData->pStripingInput)
        {
            pInputData->pStripingInput->enableBits.BPC = m_moduleEnable;
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
// IFEBPCBCC50::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBPCBCC50::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult         result     = CamxResultSuccess;

    result = IQInterface::IFEBPCBCC50CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "BPCBCC Calculation Failed.");
    }

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBPCBCC50::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50::IFEBPCBCC50
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBPCBCC50::IFEBPCBCC50()
{
    m_type                        = ISPIQModuleType::IFEBPCBCC;
    m_pChromatix                  = NULL;
    m_dependenceData.moduleEnable = FALSE; ///< First frame is always FALSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEBPCBCC50::~IFEBPCBCC50
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBPCBCC50::~IFEBPCBCC50()
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
