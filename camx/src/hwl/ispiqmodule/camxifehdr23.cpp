////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdr23.cpp
/// @brief CAMXIFEHDR23 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifehdr23titan17x.h"
#include "camxifehdr23.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR23::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR23::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEHDR23* pModule = CAMX_NEW IFEHDR23;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEHDR23 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEHDR23 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR23::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR23::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEHDR23Titan17x;
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
            CAMX_DELETE static_cast<IFEHDR23Titan17x*>(m_pHWSetting);
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
// IFEHDR23::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR23::Execute(
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
                CAMX_LOG_ERROR(CamxLogGroupISP, "HDR23 module calculation Failed.");
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
// IFEHDR23::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR23::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(hdr_2_3_0::hdr23_rgn_dataType) * (HDR23MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for hdr_2_0_0::hdr23_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR23::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDR23::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.HDRMACEnable    = m_MACEnable;
    pInputData->pCalculatedData->moduleEnable.IQModules.HDRReconEnable  = m_RECONEnable;

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
// IFEHDR23::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEHDR23::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged      = FALSE;
    BOOL dynamicEnable  = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFEHDR));

    if ((NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->HDREnable;
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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_hdr23_ife(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if ((NULL                        == m_dependenceData.pChromatix)                ||
                    (m_pChromatix->SymbolTableID != m_dependenceData.pChromatix->SymbolTableID) ||
                    (m_moduleEnable              != m_pChromatix->enable_section.hdr_enable))
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = m_pChromatix->enable_section.hdr_enable;

                    if (TRUE == m_moduleEnable)
                    {
                        m_MACEnable   = m_pChromatix->chromatix_hdr23_reserve.hdr_mac_en;
                        m_RECONEnable = m_pChromatix->chromatix_hdr23_reserve.hdr_recon_en;
                    }

                    m_moduleEnable = ((TRUE == m_MACEnable) && (TRUE == m_RECONEnable));

                    if (FALSE == m_moduleEnable)
                    {
                        m_MACEnable   = FALSE;
                        m_RECONEnable = FALSE;
                    }

                    isChanged = (TRUE == m_moduleEnable);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to get Chromatix");
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
            m_dependenceData.ZZHDRMode         = 1;
            m_dependenceData.ZZHDRPattern      =
                static_cast<UINT16>(pInputData->sensorData.ZZHDRColorPattern);

            switch(pInputData->sensorData.ZZHDRFirstExposure)
            {
                case ZZHDRFirstExposurePattern::SHORTEXPOSURE:
                    m_dependenceData.ZZHDRFirstRBEXP = 1;
                    m_dependenceData.RECONFirstField = 1;
                    break;
                case ZZHDRFirstExposurePattern::LONGEXPOSURE:
                    m_dependenceData.ZZHDRFirstRBEXP = 0;
                    m_dependenceData.RECONFirstField = 0;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP, "UnSupported ZZHDRFirstExposurePattern");
            }
            m_dependenceData.HDRZigzagReconSel = 1;

            // Check for trigger update status
            if ((TRUE == IQInterface::s_interpolationTable.IFEHDR23TriggerUpdate(&pInputData->triggerData,
                                                                                 &m_dependenceData))       ||
                (TRUE == pInputData->forceTriggerUpdate))
            {
                if (NULL == pInputData->pOEMIQSetting)
                {
                    // Check for HDR MAC dynamic enable trigger hysterisis
                    m_MACEnable = IQSettingUtils::GetDynamicEnableFlag(
                        m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_mac_en.enable,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_mac_en.hyst_control_var,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_mac_en.hyst_mode,
                        &(m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_mac_en.hyst_trigger),
                        static_cast<VOID*>(&pInputData->triggerData),
                        &m_dependenceData.moduleMacEnable);

                    // Check for HDR RECON dynamic enable trigger hysterisis
                    m_RECONEnable = IQSettingUtils::GetDynamicEnableFlag(
                        m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_recon_en.enable,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_recon_en.hyst_control_var,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_recon_en.hyst_mode,
                        &(m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_recon_en.hyst_trigger),
                        static_cast<VOID*>(&pInputData->triggerData),
                        &m_dependenceData.moduleReconEnable);

                    m_moduleEnable = ((TRUE == m_MACEnable) && (TRUE == m_RECONEnable));

                    if (FALSE == m_moduleEnable)
                    {
                        m_MACEnable   = FALSE;
                        m_RECONEnable = FALSE;
                    }

                    // Set status to isChanged to avoid calling RunCalculation if the module is disabled
                    isChanged = (TRUE == m_moduleEnable);
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP,
                       "Invalid Input: pNewAECUpdate %x pNewAWBUpdate %x  HwContext %x",
                       pInputData->pAECUpdateData,
                       pInputData->pAWBUpdateData,
                       pInputData->pHwContext);
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR23::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR23::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            HDR23OutputData outputData;
            HDR23InputData  inputData;

            Utils::Memcpy(&inputData, &m_dependenceData, sizeof(HDR23InputData));

            result = IQInterface::HDR23CalculateSetting(&inputData, pInputData->pOEMIQSetting, &outputData);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "HDR23 Calculation Failed.");
            }
            else if (NULL != pInputData->pStripingInput)
            {
                pInputData->pStripingInput->enableBits.HDR           = m_moduleEnable;
                pInputData->pStripingInput->stripingInput.HDREnable = static_cast<int16_t>(m_moduleEnable);
                pInputData->pStripingInput->stripingInput.HDRInput.HDRZrecFirstRBExp =
                    outputData.hdr23State.hdr_zrec_first_rb_exp;
            }

            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("HDR module calculation Failed.");
            }
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
// IFEHDR23::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR23::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult        result = CamxResultEFailed;
    HDR23OutputData   outputData;

    CAMX_UNREFERENCED_PARAM(pInputData);

    result = IQInterface::HDR23CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "HDR23 Calculation Failed.");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR23::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDR23::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR23::IFEHDR23
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDR23::IFEHDR23()
{
    m_type           = ISPIQModuleType::IFEHDR;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
    m_pChromatix     = NULL;

    m_dependenceData.moduleMacEnable   = FALSE; ///< First frame is always FALSE
    m_dependenceData.moduleReconEnable = FALSE; ///< First frame is always FALSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEHDR23::~IFEHDR23
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDR23::~IFEHDR23()
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
