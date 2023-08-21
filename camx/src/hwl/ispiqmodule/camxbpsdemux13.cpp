////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsdemux13.cpp
/// @brief CAMXBPSDEMUX13 class implementation
///        Demultiplex Bayer mosaicked pixels into R/G/B channels with channel gain application, or simply demultiplex input
///        stream, e.g., interleaved YUV, to separate channels
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpsdemux13.h"
#include "camxbpsdemux13titan17x.h"
#include "camxbpsdemux13titan480.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemux13::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSDemux13* pModule = CAMX_NEW BPSDemux13(pCreateData->pNodeIdentifier);
        if (NULL == pModule)
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
        }
        else
        {
            result = pModule->Initialize(pCreateData);
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemux13::Initialize(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    // Check Module Hardware Version and Create HW Setting Object

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSDemux13Titan480::Create(&m_pHWSetting);
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSDemux13Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Titan Version: %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength                 = m_pHWSetting->GetCommandLength();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSDemux13::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)             &&
        (NULL != pInputData->pHwContext) &&
        (NULL != pInputData->pHALTagsData))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->DemuxEnable;

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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_demux13_bps(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if ((NULL == m_dependenceData.pChromatixInput) ||
                (m_pChromatix->SymbolTableID != m_dependenceData.pChromatixInput->SymbolTableID))
                {
                    m_dependenceData.pChromatixInput = m_pChromatix;
                    m_moduleEnable                   = m_pChromatix->enable_section.demux_enable;
                    isChanged                        = (TRUE == m_moduleEnable);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
            }
        }

        if ((TRUE == m_moduleEnable))
        {
            ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

            if ((m_pixelFormat != pInputData->sensorData.format) ||
                (FALSE == Utils::FEqual(m_dependenceData.digitalGain, pInputData->sensorData.dGain)))
            {
                m_pixelFormat                = pInputData->sensorData.format;
                m_dependenceData.digitalGain = pInputData->sensorData.dGain;
                isChanged                    = TRUE;
            }
            // If manual mode then override digital gain
            if (NULL != pHALTagsData)
            {
                if (((ControlModeOff == pHALTagsData->controlMode) ||
                    (ControlAEModeOff == pHALTagsData->controlAEMode)) &&
                    (pHALTagsData->controlPostRawSensitivityBoost > 0) &&
                    (NULL == pInputData->pOEMIQSetting))
                {
                    m_dependenceData.digitalGain = pHALTagsData->controlPostRawSensitivityBoost / 100.0f;
                    isChanged                    = TRUE;

                    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "manual mode isp gain %d",
                        pHALTagsData->controlPostRawSensitivityBoost);
                }
            }
        }

        /// @todo (CAMX-561) Hard code Config Period and Config Config for now.
        m_dependenceData.demuxInConfigPeriod = 1;
        m_dependenceData.demuxInConfigConfig = 0;
        m_dependenceData.blackLevelOffset    = pInputData->pCalculatedMetadata->BLSblackLevelOffset;

        // Get from chromatix
        m_dependenceData.digitalGain          =
            (FALSE == Utils::FEqual(m_dependenceData.digitalGain, 0.0f)) ?
            m_dependenceData.digitalGain : 1.0f;
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
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input: pHwContext:%p", pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemux13::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        Demux13OutputData outputData = {};

        outputData.type                  = PipelineType::BPS;

        result = IQInterface::Demux13CalculateSetting(&m_dependenceData,
                                                      pInputData->pOEMIQSetting,
                                                      &outputData,
                                                      m_pixelFormat);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Demux Calculation Failed. result %d", result);
        }
        if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSDemux13::UpdateBPSInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    m_pHWSetting->UpdateFirmwareData(pInputData, m_moduleEnable);

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pBPSTuningMetadata)
    {
        result = m_pHWSetting->UpdateTuningMetadata(pInputData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateTuningMetadata failed.");
            result = CamxResultSuccess; // Non-fatal error
        }
    }

    pInputData->pCalculatedData->controlPostRawSensitivityBoost = static_cast<INT32>(m_dependenceData.digitalGain * 100);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemux13::Execute(
    ISPInputData* pInputData)
{
    CamxResult  result          = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            if (FALSE == pInputData->useHardcodedRegisterValues)
            {
                result = RunCalculation(pInputData);
            }
        }

        if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
        {
            result = m_pHWSetting->CreateCmdList(pInputData, NULL);
        }

        if (CamxResultSuccess == result)
        {
            UpdateBPSInternalData(pInputData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Operation failed %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer %p, m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13::BPSDemux13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSDemux13::BPSDemux13(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier     = pNodeIdentifier;
    m_type                = ISPIQModuleType::BPSDemux;
    m_moduleEnable        = FALSE;
    m_pChromatix          = NULL;
    m_pHWSetting          = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13::~BPSDemux13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSDemux13::~BPSDemux13()
{
    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
