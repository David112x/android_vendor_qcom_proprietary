////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpshdr22.cpp
/// @brief CAMXBPSHDR22 class implementation
/// HDR_Recon:  Reconstructs interlaced or zig-zag T1 (long exposure)/T2 (short exposure) HDR fields
///                    to full T1 and T2 images
/// HDR_MAC:    Combines full T1 and T2 images from HDR_Recon according to motion in the image contents
///                    to avoid motion artifacts in output HDR image
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpshdr22.h"
#include "camxiqinterface.h"
#include "camxbpshdr22titan17x.h"
#include "camxnode.h"
#include "camxisphwsetting.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSHDR22* pModule = CAMX_NEW BPSHDR22(pCreateData->pNodeIdentifier);

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
// BPSHDR22::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22::Initialize(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Titan480 uses HDR30");
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSHDR22Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Titan Version: %d", pCreateData->titanVersion);
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
// BPSHDR22::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(hdr_2_2_0::hdr22_rgn_dataType) * (HDR22MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for hdr_2_2_0::hdr22_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSHDR22::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if ((NULL != pInputData)           &&
        (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->HDREnable;
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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_hdr22_bps(
                                   reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                   pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if (m_pChromatix != m_dependenceData.pChromatix)
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = m_pChromatix->enable_section.hdr_enable;

                    if (TRUE == m_moduleEnable)
                    {
                        m_MACEnable   = m_pChromatix->chromatix_hdr22_reserve.hdr_mac_en;
                        m_RECONEnable = m_pChromatix->chromatix_hdr22_reserve.hdr_recon_en;
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
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
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
                    CAMX_LOG_ERROR(CamxLogGroupISP, "UnSupported ZZHDRFirstExposurePattern");
            }

            result = IQInterface::GetPixelFormat(&pInputData->sensorData.format, &(m_dependenceData.bayerPattern));
            CAMX_ASSERT(CamxResultSuccess == result);

            // Check for trigger update status
            if (TRUE == IQInterface::s_interpolationTable.HDR22TriggerUpdate(&(pInputData->triggerData), &m_dependenceData))
            {
                if (NULL == pInputData->pOEMIQSetting)
                {
                    // Check for HDR MAC dynamic enable trigger hysterisis
                    m_MACEnable = IQSettingUtils::GetDynamicEnableFlag(
                        m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_mac_en.enable,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_mac_en.hyst_control_var,
                        m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_mac_en.hyst_mode,
                        &(m_dependenceData.pChromatix->dynamic_enable_triggers.hdr_mac_en.hyst_trigger),
                        static_cast<VOID*>(&(pInputData->triggerData)),
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
// BPSHDR22::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22::RunCalculation(
    const ISPInputData* pInputData)
{
    CamxResult          result = CamxResultSuccess;
    HDR22OutputData     outputData;

    if (NULL != pInputData)
    {
        outputData.type                    = PipelineType::BPS;

        result = IQInterface::HDR22CalculateSetting(&m_dependenceData, NULL, &outputData);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "HDR22 Calculation Failed. result %d", result);
        }
        if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
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
// BPSHDR22::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* BPSHDR22::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHDR22::UpdateBPSInternalData(
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
// BPSHDR22::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22::Execute(
    ISPInputData* pInputData)
{
    CamxResult  result       = CamxResultSuccess;

    if (NULL != pInputData)
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
            CAMX_LOG_ERROR(CamxLogGroupBPS, "BPSHDR22 module calculation Failed.");
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
// BPSHDR22::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHDR22::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22::BPSHDR22
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHDR22::BPSHDR22(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier     = pNodeIdentifier;
    m_type                = ISPIQModuleType::BPSHDR;
    m_moduleEnable        = FALSE;
    m_pChromatix          = NULL;
    m_pHWSetting          = NULL;

    m_cmdLength           = 0;

    m_dependenceData.moduleMacEnable   = FALSE; ///< First frame is always FALSE
    m_dependenceData.moduleReconEnable = FALSE; ///< First frame is always FALSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22::~BPSHDR22
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHDR22::~BPSHDR22()
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
