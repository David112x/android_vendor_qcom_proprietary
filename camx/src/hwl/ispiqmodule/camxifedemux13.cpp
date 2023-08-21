////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedemux13.cpp
/// @brief CAMXIFEDEMUX13 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxifedemux13titan17x.h"
#include "camxifedemux13titan480.h"
#include "camxifedemux13.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEDemux13* pModule = CAMX_NEW IFEDemux13;

        if (NULL == pModule)
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create IFEDemux13 object.");
        }
        else
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Module initialization failed");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEDemux13 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEDemux13Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEDemux13Titan17x;
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
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class");
        result      = CamxResultENoMemory;
        m_cmdLength = 0;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
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
                    result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
                }
            }
        }
        else
        {
            result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
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
// IFEDemux13::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDemux13::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->moduleEnable.IQModules.demuxEnable = m_moduleEnable;
    pInputData->pCalculatedMetadata->demuxEnable                    = m_moduleEnable;
    pInputData->pCalculatedData->controlPostRawSensitivityBoost     = static_cast<INT32>(m_dependenceData.digitalGain * 100);

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
// IFEDemux13::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEDemux13::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged      = FALSE;
    BOOL dynamicEnable  = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFEDemux));

    if (NULL != pInputData->pOEMIQSetting)
    {
        m_moduleEnable  = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->DemuxEnable;
        isChanged      |= (TRUE == m_moduleEnable);
    }
    else
    {
        ISPHALTagsData*    pHALTagsData   = pInputData->pHALTagsData;
        TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
        CAMX_ASSERT(NULL != pTuningManager);

        // Search through the tuning data (tree), only when there
        // are changes to the tuning mode data as an optimization
        if ((TRUE == pInputData->tuningModeChanged)    &&
            (TRUE == pTuningManager->IsValidChromatix()))
        {
            CAMX_ASSERT(NULL != pInputData->pTuningData);

            m_pChromatix = pTuningManager->GetChromatix()->GetModule_demux13_ife(
                               reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                               pInputData->pTuningData->noOfSelectionParameter);
        }

        CAMX_ASSERT(NULL != m_pChromatix);
        if (NULL != m_pChromatix)
        {
            if ((NULL                           == m_dependenceData.pChromatixInput)                ||
                (m_pChromatix->SymbolTableID    != m_dependenceData.pChromatixInput->SymbolTableID) ||
                (m_moduleEnable                 != m_pChromatix->enable_section.demux_enable))
            {
                m_dependenceData.pChromatixInput  = m_pChromatix;
                m_moduleEnable                    = m_pChromatix->enable_section.demux_enable;
                isChanged                        |= (TRUE == m_moduleEnable);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to get Chromatix");
        }

            // If manual mode then override digital gain
        if (((ControlModeOff   == pHALTagsData->controlMode) ||
             (ControlAEModeOff == pHALTagsData->controlAEMode)) &&
             (pHALTagsData->controlPostRawSensitivityBoost > 0) &&
             (TRUE == m_moduleEnable))
        {
            m_dependenceData.digitalGain = static_cast<FLOAT>(pHALTagsData->controlPostRawSensitivityBoost) / 100.0f;
            isChanged = TRUE;
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "manual mode isp gain %f", pHALTagsData->controlPostRawSensitivityBoost);
        }
        else if ((TRUE == m_moduleEnable) &&
            ((m_pixelFormat != pInputData->sensorData.format) ||
             (FALSE == Utils::FEqual(m_dependenceData.digitalGain, pInputData->sensorData.dGain))))
        {
            m_pixelFormat                 = pInputData->sensorData.format;
            m_dependenceData.digitalGain  = pInputData->sensorData.dGain;
            isChanged                     = TRUE;
        }
    }

    if (TRUE == pInputData->forceTriggerUpdate)
    {
        isChanged = TRUE;
    }

    /// @todo (CAMX-561) Hard code Config Period and Config Config for now.
    m_dependenceData.demuxInConfigPeriod  = 1;
    m_dependenceData.demuxInConfigConfig  = 0;

    m_dependenceData.blackLevelOffset = pInputData->pCalculatedMetadata->BLSblackLevelOffset;

    // Override with computed values in Linearization IQ module
    m_dependenceData.stretchGainRed       =
        (FALSE == Utils::FEqual(pInputData->pCalculatedMetadata->stretchGainRed, 0.0f)) ?
        pInputData->pCalculatedMetadata->stretchGainRed : 1.0f;
    m_dependenceData.stretchGainGreenEven =
        (FALSE == Utils::FEqual(pInputData->pCalculatedMetadata->stretchGainGreenEven, 0.0f)) ?
        pInputData->pCalculatedMetadata->stretchGainGreenEven : 1.0f;
    m_dependenceData.stretchGainGreenOdd  =
        (FALSE == Utils::FEqual(pInputData->pCalculatedMetadata->stretchGainGreenOdd, 0.0f)) ?
        pInputData->pCalculatedMetadata->stretchGainGreenOdd : 1.0f;
    m_dependenceData.stretchGainBlue      =
        (FALSE == Utils::FEqual(pInputData->pCalculatedMetadata->stretchGainBlue, 0.0f)) ?
        pInputData->pCalculatedMetadata->stretchGainBlue : 1.0f;

    // Update bayer pattern from sensor format
    if (CamxResultSuccess != IQInterface::GetPixelFormat(&pInputData->sensorData.format, &(m_dependenceData.bayerPattern)))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported sensor format : %d", pInputData->sensorData.format);
        // Disable demux block if sensor format is not supported
        isChanged = FALSE;
    }
    else if (NULL == pInputData->pOEMIQSetting)
    {
        m_moduleEnable  = TRUE;
        isChanged      |= TRUE;
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
// IFEDemux13::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    Demux13OutputData outputData;

    outputData.type                  = PipelineType::IFE;

    result = IQInterface::Demux13CalculateSetting(&m_dependenceData,
                                                  pInputData->pOEMIQSetting,
                                                  &outputData,
                                                  m_pixelFormat);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Demux Calculation Failed.");
    }

    if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
    {
        m_pHWSetting->DumpRegConfig();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13::IFEDemux13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemux13::IFEDemux13()
{
    m_type           = ISPIQModuleType::IFEDemux;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
    m_moduleEnable   = TRUE;
    m_pChromatix     = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEDemux13::~IFEDemux13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemux13::~IFEDemux13()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
