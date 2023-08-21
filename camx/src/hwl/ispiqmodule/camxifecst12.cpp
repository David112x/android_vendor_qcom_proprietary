////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecst12.cpp
/// @brief CAMXIFECST12 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifecst12titan17x.h"
#include "camxifecst12titan480.h"
#include "camxifecst12.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFECST12* pModule = CAMX_NEW IFECST12;
        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFECST12 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFECST12 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFECST12Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFECST12Titan17x;
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
        if (result != CamxResultSuccess)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
            m_cmdLength  = 0;
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
// IFECST12::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (FALSE == pInputData->useHardcodedRegisterValues)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
                if (CamxResultSuccess == result)
                {
                    result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
                }
            }
        }
        else
        {
            result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
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
// IFECST12::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECST12::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged      = FALSE;
    BOOL dynamicEnable  = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFECST));

    if (NULL != pInputData->pOEMIQSetting)
    {
        m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->CSTEnable;
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
        if ((TRUE == pInputData->tuningModeChanged)    &&
            (TRUE == pTuningManager->IsValidChromatix()))
        {
            CAMX_ASSERT(NULL != pInputData->pTuningData);

            m_pChromatix = pTuningManager->GetChromatix()->GetModule_cst12_ife(
                               reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                               pInputData->pTuningData->noOfSelectionParameter);
        }

        CAMX_ASSERT(NULL != m_pChromatix);
        if (NULL != m_pChromatix)
        {
            if ((NULL                           == m_dependenceData.pChromatix)                ||
                (m_pChromatix->SymbolTableID    != m_dependenceData.pChromatix->SymbolTableID) ||
                (m_moduleEnable                 != m_pChromatix->enable_section.cst_enable))

            {
                m_dependenceData.pChromatix = m_pChromatix;
                m_moduleEnable              = m_pChromatix->enable_section.cst_enable;
                if (TRUE == m_moduleEnable)
                {
                    isChanged = TRUE;
                }
            }
            if (TRUE == pInputData->forceTriggerUpdate)
            {
                isChanged = TRUE;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to get chromatix pointer");
        }
    }

    m_moduleEnable &= dynamicEnable;
    if ((TRUE == m_moduleEnable) && (m_dynamicEnable != dynamicEnable))
    {
        isChanged = TRUE;
    }
    m_dynamicEnable = dynamicEnable;

    return isChanged;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult      result = CamxResultSuccess;
    CST12OutputData outputData;

    outputData.type = PipelineType::IFE;

    result = IQInterface::CST12CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "CST12 Calculation Failed. result %d", result);
    }

    if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECST12::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.CSTEnable = m_moduleEnable;

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
// IFECST12::IFECST12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECST12::IFECST12()
{
    m_type                      = ISPIQModuleType::IFECST;
    m_32bitDMILength            = 0;
    m_64bitDMILength            = 0;
    m_moduleEnable              = TRUE;
    m_dependenceData.pChromatix = NULL;
    m_pChromatix                = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECST12::~IFECST12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECST12::~IFECST12()
{
    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
