////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecc12.cpp
/// @brief CAMXIFECC12 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifecc12titan17x.h"
#include "camxifecc12.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC12::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFECC12* pModule = CAMX_NEW IFECC12;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFECC12 object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFECC12 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC12::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFECC12Titan17x;
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
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE static_cast<IFECC12Titan17x*>(m_pHWSetting);
            m_pHWSetting                = NULL;
            m_cmdLength                 = 0;
            m_dependenceData.pHWSetting = NULL;
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
// IFECC12::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC12::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("CC module calculation Failed.");
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
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC12::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(cc_1_2_0::cc12_rgn_dataType) * (CC12MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for cc_1_2_0::cc12_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECC12::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.CCEnable = m_moduleEnable;

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
// IFECC12::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECC12::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged      = FALSE;
    BOOL dynamicEnable  = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFECC));

    CAMX_ASSERT(NULL != pInputData);

    if ((NULL != pInputData->pHALTagsData)   &&
        (NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData))
    {
        ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->CCEnable;

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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_cc12_ife(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);

            if (NULL != m_pChromatix)
            {
                if ((NULL == m_dependenceData.pChromatix)                                       ||
                    (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID) ||
                    (m_moduleEnable              != m_pChromatix->enable_section.cc_enable))
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = m_pChromatix->enable_section.cc_enable;
                    if (TRUE == m_moduleEnable)
                    {
                        isChanged = TRUE;
                    }
                }
            }

            // Check for manual tone map mode, if yes disable 2dLUT
            if ((TRUE                               == m_moduleEnable)                    &&
                (ColorCorrectionModeTransformMatrix == pHALTagsData->colorCorrectionMode) &&
                (((ControlAWBModeOff                == pHALTagsData->controlAWBMode)      &&
                (ControlModeAuto                    == pHALTagsData->controlMode)) ||
                (ControlModeOff                     == pHALTagsData->controlMode)))
            {
                m_moduleEnable = FALSE;
                isChanged      = FALSE;
                CAMX_LOG_INFO(CamxLogGroupISP, "<CCM> manual m_moduleEnable %d", m_moduleEnable);
            }
        }

        m_moduleEnable &= dynamicEnable;
        if ((TRUE == m_moduleEnable) && (m_dynamicEnable != dynamicEnable))
        {
            isChanged = TRUE;
        }
        m_dynamicEnable = dynamicEnable;

        if (TRUE == m_moduleEnable)
        {
            if ((TRUE ==
                IQInterface::s_interpolationTable.IFECC12TriggerUpdate(&pInputData->triggerData, &m_dependenceData)) ||
                (TRUE == pInputData->forceTriggerUpdate))
            {
                isChanged = TRUE;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input: pNewAECUpdate %x pNewAWBUpdate %x  HwContext %x",
            pInputData->pHwContext, pInputData->pAECUpdateData, pInputData->pAWBUpdateData, pInputData->pHwContext);
    }

    return isChanged;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC12::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    result = IQInterface::IFECC12CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "CC12 Calculation Failed.");
    }

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECC12::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12::IFECC12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECC12::IFECC12()
{
    m_type           = ISPIQModuleType::IFECC;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
    m_moduleEnable   = TRUE;
    m_pChromatix     = NULL;

    m_dependenceData.pChromatix       = NULL;
    m_dependenceData.DRCGain          = 0.0f;
    m_dependenceData.exposureTime     = 0.0f;
    m_dependenceData.AECSensitivity   = 0.0f;
    m_dependenceData.luxIndex         = 0.0f;
    m_dependenceData.AECGain          = 0.0f;
    m_dependenceData.colorTemperature = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECC12::~IFECC12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECC12::~IFECC12()
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
