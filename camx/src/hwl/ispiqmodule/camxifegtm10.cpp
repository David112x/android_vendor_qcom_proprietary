////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifegtm10.cpp
/// @brief CAMXIFEGTM10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifegtm10titan17x.h"
#include "camxifegtm10titan480.h"
#include "camxifegtm10.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEGTM10* pModule = CAMX_NEW IFEGTM10;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEGTM10 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEGTM10 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEGTM10Titan480;
            m_b32bitDMI  = TRUE;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEGTM10Titan17x;
            m_b32bitDMI  = FALSE;
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
        m_32bitDMILength            = m_pHWSetting->Get32bitDMILength();
        m_64bitDMILength            = m_pHWSetting->Get64bitDMILength();
        result                      = AllocateCommonLibraryData();
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
            m_cmdLength      = 0;
            m_32bitDMILength = 0;
            m_64bitDMILength = 0;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class");
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(gtm_1_0_0::gtm10_rgn_dataType) * (GTM10MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for gtm_1_0_0::gtm10_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10::Execute(
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
                result = m_pHWSetting->CreateCmdList(pSettingData,
                            (m_b32bitDMI == TRUE) ? &m_32bitDMIBufferOffsetDword : &m_64bitDMIBufferOffsetDword);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "GTM10 module calculation Failed.");
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
// IFEGTM10::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEGTM10::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.GTMEnable = m_moduleEnable;
    pInputData->pCalculatedData->percentageOfGTM =
        (NULL == m_pTMCInput.pAdrcOutputData) ? 0 : m_pTMCInput.pAdrcOutputData->gtmPercentage;

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
// IFEGTM10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEGTM10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL                            isChanged     = FALSE;
    tmc_1_0_0::chromatix_tmc10Type* ptmcChromatix = NULL;

    BOOL dynamicEnable = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFEGTM));

    if ((NULL != pInputData->pHwContext)     &&
        (NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->GTMEnable;

            if (TRUE == m_moduleEnable)
            {
                isChanged = TRUE;
            }
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

                            m_pChromatix  = pTuningManager->GetChromatix()->GetModule_gtm10_ife(
                                                reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                                pInputData->pTuningData->noOfSelectionParameter);
                        }
                        ptmcChromatix = pTuningManager->GetChromatix()->GetModule_tmc10_sw(
                                            reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                            pInputData->pTuningData->noOfSelectionParameter);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Chromatix tuning data, pTuningManager is NULL");
                    }

                }

                if (NULL != m_pChromatix)
                {
                    if ((NULL                           == m_dependenceData.pChromatix)                 ||
                        (m_pChromatix->SymbolTableID    != m_dependenceData.pChromatix->SymbolTableID)  ||
                        (m_moduleEnable                 != m_pChromatix->enable_section.gtm_enable))
                    {
                        m_dependenceData.pChromatix = m_pChromatix;
                        if (NULL != ptmcChromatix)
                        {
                            m_pTMCInput.pChromatix  = ptmcChromatix;
                        }
                        else
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Failed to get TMC 10 Chromatix, ptmcChromatix is NULL");
                        }

                        m_moduleEnable = m_pChromatix->enable_section.gtm_enable;
                        if (TRUE == m_moduleEnable)
                        {
                            isChanged = TRUE;
                        }
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to get Chromatix, m_pChromatix is NULL");
                }
            }
        }

        // Check for manual control
        if ((TonemapModeContrastCurve == pInputData->pHALTagsData->tonemapCurves.tonemapMode) &&
           (0 != pInputData->pHALTagsData->tonemapCurves.curvePoints))
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

        if (TRUE == m_moduleEnable)
        {
            if ((TRUE ==
                IQInterface::s_interpolationTable.GTM10TriggerUpdate(&pInputData->triggerData, &m_dependenceData)) ||
                (TRUE == pInputData->forceTriggerUpdate))
            {
                isChanged = TRUE;
            }

            m_pTMCInput.adrcGTMEnable = FALSE;

            if ((NULL != ptmcChromatix)                                  &&
                (TRUE == ptmcChromatix->enable_section.adrc_isp_enable)  &&
                (TRUE == ptmcChromatix->chromatix_tmc10_reserve.use_gtm) &&
                (SWTMCVersion::TMC10 == pInputData->triggerData.enabledTMCversion))
            {
                isChanged = TRUE;
                m_pTMCInput.adrcGTMEnable = TRUE;
                IQInterface::s_interpolationTable.TMC10TriggerUpdate(&pInputData->triggerData, &m_pTMCInput);

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
                        CAMX_LOG_ERROR(CamxLogGroupISP, "Memory allocation failed for ADRC Specific data");
                    }
                }
                pInputData->triggerData.pADRCData = m_pADRCData;
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE GTM : using TMC 10");
            }
            else
            {
                if (NULL != pInputData->triggerData.pADRCData)
                {
                    m_pTMCInput.adrcGTMEnable = pInputData->triggerData.pADRCData->gtmEnable;
                    CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE GTM : using TMC 11 or 12, version = %u, gtmEnable = %u",
                                        pInputData->triggerData.enabledTMCversion,
                                        m_pTMCInput.adrcGTMEnable);
                }
            }

            m_pTMCInput.pAdrcOutputData = pInputData->triggerData.pADRCData;
        }

        if ((ControlAWBLockOn == pInputData->pHALTagsData->controlAWBLock) &&
            (ControlAELockOn  == pInputData->pHALTagsData->controlAECLock) &&
            (FALSE == pInputData->forceTriggerUpdate))
        {
            isChanged = FALSE;
        }

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Pointer");
    }

    return isChanged;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult      result      = CamxResultSuccess;
    VOID*           pDMIAddr    = NULL;
    GTM10OutputData outputData;

    if (NULL != pInputData)
    {
        outputData.type                   = PipelineType::IFE;
        outputData.registerBETEn          = pInputData->registerBETEn;
        outputData.bIsBankUpdateValid     = pInputData->bankUpdate.isValid;
        outputData.bankSelect             = pInputData->bankUpdate.GTMBank;

        if ((TRUE == m_b32bitDMI) && (NULL != pInputData->p32bitDMIBufferAddr))
        {
            pDMIAddr = static_cast<VOID*>(pInputData->p32bitDMIBufferAddr + m_32bitDMIBufferOffsetDword);
            outputData.regCmd.IFE.pDMIDataPtr = reinterpret_cast<UINT64*>(pDMIAddr);
        }
        else if ((FALSE == m_b32bitDMI) && (NULL != pInputData->p64bitDMIBufferAddr))
        {
            pDMIAddr = reinterpret_cast<VOID*>(pInputData->p64bitDMIBufferAddr + m_64bitDMIBufferOffsetDword);
            outputData.regCmd.IFE.pDMIDataPtr = reinterpret_cast<UINT64*>(pDMIAddr);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "GTM10 DMI buffer is NULL");
            result = CamxResultEInvalidArg;
        }

        if (CamxResultSuccess == result)
        {
            result = IQInterface::GTM10CalculateSetting(&m_dependenceData,
                                                        pInputData->pOEMIQSetting,
                                                        &outputData,
                                                        &m_pTMCInput);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "GTM10 Calculation Failed. result %d", result);
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
// IFEGTM10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEGTM10::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10::IFEGTM10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGTM10::IFEGTM10()
{
    m_type           = ISPIQModuleType::IFEGTM;
    m_moduleEnable   = TRUE;
    m_pChromatix     = NULL;
    m_pADRCData      = NULL;
    m_b32bitDMI      = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEGTM10::~IFEGTM10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGTM10::~IFEGTM10()
{
    if (NULL != m_pADRCData)
    {
        CAMX_DELETE m_pADRCData;
        m_pADRCData = NULL;
    }

    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
