////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdr30.cpp
/// @brief CAMXIFEHDR30 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifehdr30titan480.h"
#include "camxifehdr30.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEHDR30* pModule = CAMX_NEW IFEHDR30;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEHDR30 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEHDR30 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEHDR30Titan480;
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
            CAMX_DELETE static_cast<IFEHDR30Titan480*>(m_pHWSetting);
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
// IFEHDR30::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            if (CamxResultSuccess == RunCalculation(pInputData))
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
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
// IFEHDR30::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize     = (sizeof(hdr_3_0_0::hdr30_rgn_dataType) * (HDR30MaxmiumNonLeafNode + 1));
    UINT interpolationSizeBC10 = (sizeof(bincorr_1_0_0::bincorr10_rgn_dataType) * (BC10MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for hdr_3_0_0::hdr30_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    if ((CamxResultSuccess == result) && (NULL == m_dependenceData.bc10Data.pBC10InterpolationData))
    {
        // Alloc for bincorr_1_0_0::bincorr10_rgn_dataType
        m_dependenceData.bc10Data.pBC10InterpolationData = CAMX_CALLOC(interpolationSizeBC10);
        if (NULL == m_dependenceData.bc10Data.pBC10InterpolationData)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to alloc BC interpolation, no memory");
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDR30::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // MAC EN / REC EN are not present in 30
    pInputData->pCalculatedData->moduleEnable.IQModules.HDREnable = m_moduleEnable;

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
// IFEHDR30::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEHDR30::CheckDependenceChange(
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
            if ((TRUE == pInputData->tuningModeChanged)      &&
                (TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_hdr30_ife(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
                m_pChromatixBC = pTuningManager->GetChromatix()->GetModule_bincorr10_ife(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }

            if (NULL != m_pChromatix)
            {
                if ((m_pChromatix   != m_dependenceData.pChromatix) ||
                    (m_moduleEnable != m_pChromatix->enable_section.hdr_enable))
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = m_pChromatix->enable_section.hdr_enable;
                    isChanged = (TRUE == m_moduleEnable);

                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to get Chromatix");
            }

            if (NULL != m_pChromatixBC)
            {
                if ((m_pChromatixBC != m_dependenceData.bc10Data.pChromatixBC) ||
                    m_moduleEnable  != m_pChromatixBC->enable_section.bincorr_enable)
                {
                    m_dependenceData.bc10Data.pChromatixBC = m_pChromatixBC;
                    m_moduleEnable |= m_pChromatixBC->enable_section.bincorr_enable;
                    isChanged = (TRUE == m_moduleEnable);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to get ChromatixBC");
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

            if ((TRUE == IQInterface::s_interpolationTable.HDR30TriggerUpdate(&(pInputData->triggerData),
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
// IFEHDR30::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        TuningDataManager*              pTuningManager = pInputData->pTuningDataManager;
        hdr_3_0_0::chromatix_hdr30Type* pChromatix     = NULL;

        if ((TRUE == pTuningManager->IsValidChromatix()) &&
            (NULL != pInputData->pTuningData))
        {

            pChromatix = pTuningManager->GetChromatix()->GetModule_hdr30_ife(
                            reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                            pInputData->pTuningData->noOfSelectionParameter);
        }
        if (NULL != pChromatix)
        {
            if (NULL != pInputData->pStripingInput)
            {
                pInputData->pStripingInput->enableBits.HDR           = pChromatix->enable_section.hdr_enable;
                pInputData->pStripingInput->stripingInput.HDREnable  =
                    static_cast<int16_t>(pChromatix->enable_section.hdr_enable);

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
// IFEHDR30::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        HDR30OutputData outputData;

        outputData.type = PipelineType::IFE;

        result = IQInterface::HDR30CalculateSetting(&m_dependenceData, NULL, &outputData);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "HDR30 Calculation Failed. result %d", result);
        }

        if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDR30::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }

    if (NULL != m_dependenceData.bc10Data.pBC10InterpolationData)
    {
        CAMX_FREE(m_dependenceData.bc10Data.pBC10InterpolationData);
        m_dependenceData.bc10Data.pBC10InterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30::IFEHDR30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDR30::IFEHDR30()
{
    m_type           = ISPIQModuleType::IFEHDR;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
    m_moduleEnable   = TRUE;
    m_pChromatix     = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEHDR30::~IFEHDR30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDR30::~IFEHDR30()
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
