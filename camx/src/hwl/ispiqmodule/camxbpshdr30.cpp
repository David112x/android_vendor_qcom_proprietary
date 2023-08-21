////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpshdr30.cpp
/// @brief CAMXBPSHDR30 class implementation
/// HDR_Recon:  Reconstructs interlaced or zig-zag T1 (long exposure)/T2 (short exposure) HDR fields
///                    to full T1 and T2 images
/// HDR_MAC:    Combines full T1 and T2 images from HDR_Recon according to motion in the image contents
///                    to avoid motion artifacts in output HDR image
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpshdr30.h"
#include "camxiqinterface.h"
#include "camxbpshdr30titan480.h"
#include "camxnode.h"
#include "camxisphwsetting.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "titan480_bps.h"

CAMX_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSHDR30* pModule = CAMX_NEW BPSHDR30(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Module initialization failed");
                CAMX_DELETE(pModule);
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
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
// BPSHDR30::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30::Initialize(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSHDR30Titan480::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unsupported or Invalid Titan Version: %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength = m_pHWSetting->GetCommandLength();
        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSizeHDR30 = (sizeof(hdr_3_0_0::hdr30_rgn_dataType) * (HDR30MaxmiumNonLeafNode + 1));
    UINT interpolationSizeBC10  = (sizeof(bincorr_1_0_0::bincorr10_rgn_dataType) * (BC10MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for hdr_3_0_0::hdr30_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSizeHDR30);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to alloc HDR interpolation, no memory");
            result = CamxResultENoMemory;
        }
    }

    if ((CamxResultSuccess == result) && (NULL == m_dependenceData.bc10Data.pBC10InterpolationData))
    {
        // Alloc for bincorr_1_0_0::bincorr10_rgn_dataType
        m_dependenceData.bc10Data.pBC10InterpolationData = CAMX_CALLOC(interpolationSizeBC10);
        if (NULL == m_dependenceData.bc10Data.pBC10InterpolationData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to alloc BC interpolation, no memory");
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSHDR30::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData) && (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->HDREnable;
            isChanged = (TRUE == m_moduleEnable);
        }
        else
        {
            TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
            CAMX_ASSERT(NULL != pTuningManager);

            // Search through the tuning data (tree), only when there
            // are changes to the tuning mode data as an optimization
            if ((TRUE == pInputData->tuningModeChanged) &&
                (TRUE == pTuningManager->IsValidChromatix()))
            {
                CAMX_ASSERT(NULL != pInputData->pTuningData);

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_hdr30_bps(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);

                m_pChromatixBC = pTuningManager->GetChromatix()->GetModule_bincorr10_bps(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);

            }

            CAMX_ASSERT(NULL != m_pChromatix);
            CAMX_ASSERT(NULL != m_pChromatixBC);
            if (NULL != m_pChromatix)
            {
                if (m_pChromatix != m_dependenceData.pChromatix)
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable = m_pChromatix->enable_section.hdr_enable;

                    isChanged = (TRUE == m_moduleEnable);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get HDR3.0 Chromatix");
            }

            if (NULL != m_pChromatixBC)
            {
                if (m_pChromatixBC != m_dependenceData.bc10Data.pChromatixBC)
                {
                    m_dependenceData.bc10Data.pChromatixBC = m_pChromatixBC;
                    m_dependenceData.bc10Data.moduleEnable = m_pChromatixBC->enable_section.bincorr_enable;
                    m_moduleEnable |= m_pChromatixBC->enable_section.bincorr_enable;
                    isChanged = (TRUE == m_moduleEnable);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get BC10 Chromatix");
            }
        }

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
                    CAMX_LOG_WARN(CamxLogGroupISP, "UnSupported ZZHDRFirstExposurePattern");
            }

            result = IQInterface::GetPixelFormat(&pInputData->sensorData.format, &(m_dependenceData.bayerPattern));
            CAMX_ASSERT(CamxResultSuccess == result);

            // Check for trigger update status
            if ((TRUE == IQInterface::s_interpolationTable.HDR30TriggerUpdate(&(pInputData->triggerData), &m_dependenceData)) ||
                (TRUE == IQInterface::s_interpolationTable.BC10TriggerUpdate(&(pInputData->triggerData),
                    &(m_dependenceData.bc10Data))))
            {
                // Set status to isChanged to avoid calling RunCalculation if the module is disabled
                isChanged = (TRUE == m_moduleEnable);
            }
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS,
                           "Invalid Input: pHwContext:%p",
                            pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult          result = CamxResultSuccess;
    HDR30OutputData     outputData;
    BpsIQSettings*      pBPSIQSettings;

    if (NULL != pInputData)
    {
        pBPSIQSettings = reinterpret_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
        if (NULL != pBPSIQSettings)
        {
            outputData.type                    = PipelineType::BPS;

            result = IQInterface::HDR30CalculateSetting(&m_dependenceData, NULL, &outputData);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "HDR30 Calculation Failed. result %d", result);
            }
            if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
            {
                m_pHWSetting->DumpRegConfig();
            }
        }
        else
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Null pBPSIQSetting pointer");
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
// BPSHDR30::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* BPSHDR30::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHDR30::UpdateBPSInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    result = m_pHWSetting->UpdateFirmwareData(pInputData, m_moduleEnable);

    if (CamxResultSuccess == result)
    {
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
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateFirmwareData failed.");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30::Execute(
    ISPInputData* pInputData)
{
    CamxResult  result       = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
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
            CAMX_LOG_ERROR(CamxLogGroupBPS, "BPSHDR30 module calculation Failed.");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHDR30::DeallocateCommonLibraryData()
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
// BPSHDR30::BPSHDR30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHDR30::BPSHDR30(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier  = pNodeIdentifier;
    m_type             = ISPIQModuleType::BPSHDR;
    m_moduleEnable     = TRUE;
    m_pChromatix       = NULL;
    m_pChromatixBC     = NULL;
    m_pHWSetting       = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30::~BPSHDR30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHDR30::~BPSHDR30()
{
    m_pChromatix   = NULL;
    m_pChromatixBC = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
