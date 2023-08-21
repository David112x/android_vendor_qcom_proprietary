////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdr22.cpp
/// @brief CAMXIFEHDR22 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifehdr22titan17x.h"
#include "camxifehdr22.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR22::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR22::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEHDR22* pModule = CAMX_NEW IFEHDR22;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEHDR22 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEHDR22 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR22::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR22::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEHDR22Titan17x;
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
            CAMX_DELETE static_cast<IFEHDR22Titan17x*>(m_pHWSetting);
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
// IFEHDR22::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR22::Execute(
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
                CAMX_LOG_ERROR(CamxLogGroupISP, "HDR22 module calculation Failed.");
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
// IFEHDR22::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR22::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(hdr_2_2_0::hdr22_rgn_dataType) * (HDR22MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for hdr_2_0_0::hdr22_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR22::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDR22::UpdateIFEInternalData(
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
// IFEHDR22::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEHDR22::CheckDependenceChange(
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

            if (TRUE == m_moduleEnable)
            {
                m_MACEnable   = TRUE;
                m_RECONEnable = TRUE;
            }
            else
            {
                m_MACEnable   = FALSE;
                m_RECONEnable = FALSE;
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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_hdr22_ife(
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
                    m_MACEnable                 = FALSE;
                    m_RECONEnable               = FALSE;

                    if (TRUE == m_moduleEnable)
                    {
                        m_MACEnable = m_pChromatix->chromatix_hdr22_reserve.hdr_mac_en;
                        m_RECONEnable = m_pChromatix->chromatix_hdr22_reserve.hdr_recon_en;

                        if ( TRUE == m_MACEnable && TRUE == m_RECONEnable )
                        {
                            isChanged     = TRUE;
                        }
                        else
                        {
                            m_moduleEnable = FALSE;
                        }
                    }
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
            CamxResult result = CamxResultSuccess;
            isChanged         = TRUE;

            m_dependenceData.blackLevelOffset =
                static_cast<UINT16>(pInputData->pCalculatedMetadata->BLSblackLevelOffset);
            m_dependenceData.ZZHDRMode        = 1;
            m_dependenceData.ZZHDRPattern     =
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

            result = IQInterface::GetPixelFormat(&pInputData->sensorData.format, &(m_dependenceData.bayerPattern));

            if ((TRUE == IQInterface::s_interpolationTable.HDR22TriggerUpdate(&(pInputData->triggerData),
                                                                                &m_dependenceData)) ||
                (TRUE == pInputData->forceTriggerUpdate))
            {
                isChanged = TRUE;
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
// IFEHDR22::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR22::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            HDR22OutputData outputData;
            HDR22InputData  inputData;

            outputData.type                = PipelineType::IFE;
            Utils::Memcpy(&inputData, &m_dependenceData, sizeof(HDR22InputData));

            result = IQInterface::HDR22CalculateSetting(&inputData, pInputData->pOEMIQSetting, &outputData);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "HDR22 Calculation Failed.");
            }
            else if (NULL != pInputData->pStripingInput)
            {
                pInputData->pStripingInput->enableBits.HDR = m_moduleEnable;
                pInputData->pStripingInput->stripingInput.HDREnable = static_cast<int16_t>(m_moduleEnable);
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
// IFEHDR22::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR22::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    HDR22OutputData outputData;
    outputData.type   = PipelineType::IFE;

    CAMX_UNREFERENCED_PARAM(pInputData);

    result = IQInterface::HDR22CalculateSetting(&m_dependenceData, NULL, &outputData);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "HDR22 Calculation Failed. result %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR22::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDR22::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR22::IFEHDR22
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDR22::IFEHDR22()
{
    m_type           = ISPIQModuleType::IFEHDR;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
    m_moduleEnable   = TRUE;
    m_pChromatix     = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEHDR22::~IFEHDR22
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDR22::~IFEHDR22()
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
