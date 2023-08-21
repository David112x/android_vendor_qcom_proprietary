////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipelenr10.cpp
/// @brief ipelenr10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcdmdefs.h"
#include "camxdefs.h"
#include "camxipelenr10.h"
#include "camxiqinterface.h"
#include "camxtitan17xcontext.h"
#include "camxtuningdatamanager.h"
#include "camxisphwsetting.h"
#include "lenr10setting.h"
#include "parametertuningtypes.h"
#include "camxipelenr10titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELENR10::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPELENR10* pModule = CAMX_NEW IPELENR10(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_ASSERT_ALWAYS_MESSAGE("Memory allocation failed");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELENR10::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult     result          = CamxResultSuccess;
    ISPInputData*  pInputData      = &pCreateData->initializationData;
    m_pHWSetting                   = NULL;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IPELENR10Titan480;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid version, module not supprted");
            break;
    }

    if (NULL != m_pHWSetting)
    {
        UINT regCmdSize = m_pHWSetting->GetRegSize();
        UINT size       = PacketBuilder::RequiredWriteRegRangeSizeInDwords(regCmdSize / RegisterWidthInBytes);

        m_cmdLength = size * sizeof(UINT32);

        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        result = AllocateCommonLibraryData();
        if (CamxResultSuccess != result)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Unable to create HW Setting Class, no memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELENR10::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(lenr_1_0_0::lenr10_rgn_dataType) * (LENR10MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for lenr_1_0_0::lenr10_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPELENR10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if (NULL != pInputData)
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->LENREnable;
            isChanged      = (TRUE == m_moduleEnable);

            /// @todo (CAMX-2012) Face detection parameters need to be consumed from FD.
            m_dependenceData.pFDData = &pInputData->fDData;
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

                m_pChromatix = pTuningManager->GetChromatix()->GetModule_lenr10_ipe(
                        reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                        pInputData->pTuningData->noOfSelectionParameter);
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                // m_dependenceData.totalScaleRatio = 1.0f;

                m_dependenceData.pFDData = &pInputData->fDData;

                if (m_pChromatix != m_dependenceData.pChromatix)
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = (m_pChromatix->enable_section.enable       &&
                                                  (m_pChromatix->enable_section.lenr_bltr_en ||
                                                   m_pChromatix->enable_section.lenr_lce_en));

                    isChanged                   = (TRUE == m_moduleEnable);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to get Chromatix");
            }
        }

        // Disable module if requested by HAL
        m_noiseReductionMode = pInputData->pHALTagsData->noiseReductionMode;
        if ((m_noiseReductionMode == NoiseReductionModeOff) ||
            (pInputData->pipelineIPEData.numPasses <= 1) ||
            (TRUE == pInputData->pipelineIPEData.realtimeFlag))
        {

            m_moduleEnable  = FALSE;
            isChanged       = FALSE;
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " noise reduction off  m_moduleEnable %d, realtimeFlag %d, numPasses %d",
                             m_moduleEnable,
                             pInputData->pipelineIPEData.numPasses,
                             pInputData->pipelineIPEData.realtimeFlag);
        }

        // Check for trigger update status
        if ((TRUE == m_moduleEnable) &&
            (TRUE == IQInterface::s_interpolationTable.LENR10TriggerUpdate(&pInputData->triggerData, &m_dependenceData)))
        {
            m_dependenceData.cropFrameBoundaryPixAlignRight = 0;
            if (NULL == pInputData->pOEMIQSetting)
            {
                // Check for module dynamic enable trigger hysterisis
                m_moduleEnable = IQSettingUtils::GetDynamicEnableFlag(
                    m_dependenceData.pChromatix->dynamic_enable_triggers.lenr_nr_enable.enable,
                    m_dependenceData.pChromatix->dynamic_enable_triggers.lenr_nr_enable.hyst_control_var,
                    m_dependenceData.pChromatix->dynamic_enable_triggers.lenr_nr_enable.hyst_mode,
                    &(m_dependenceData.pChromatix->dynamic_enable_triggers.lenr_nr_enable.hyst_trigger),
                    static_cast<VOID*>(&pInputData->triggerData),
                    &m_dependenceData.moduleEnable);

                // Set status to isChanged to avoid calling RunCalculation if the module is disabled
                isChanged = (TRUE == m_moduleEnable);
            }
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input: pHwContext %p", pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Pointer");
        }
    }

    return isChanged;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELENR10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        LENR10OutputData outputData;

        outputData.pLENRParams = &m_LENRParameters;

        result = IQInterface::LENR10CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

        if (CamxResultSuccess == result)
        {
            if (NULL != pInputData->pIPETuningMetadata)
            {
                // Add support for tuning  metadata
            }
            if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
            {
                m_pHWSetting->DumpRegConfig();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "LENR10 Calculation Failed %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid input data pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPELENR10::UpdateIPEInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult     result         = CamxResultSuccess;
    IpeIQSettings* pIPEIQSettings = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    if (NULL != pIPEIQSettings)
    {
        CamX::Utils::Memcpy(&pIPEIQSettings->lenrParameters, &m_LENRParameters, sizeof(LenrParameters));
        if ((pInputData->pipelineIPEData.numPasses > 1) ||
           (FALSE == pInputData->pipelineIPEData.realtimeFlag))
        {
            pIPEIQSettings->lenrParameters.moduleCfg.EN = m_moduleEnable;
        }
        else
        {
            pIPEIQSettings->lenrParameters.moduleCfg.EN = 0;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "4clamp %d, 8clamp %d, 16 clamp %d, BLTR_EN %d, EN %d, FD_SNR_EN %d"
                         "FNR_EN %d, LAYER_1_ONLY %d, LCE_EN %d, RNR_EN %d, SNR_EN %d, POST_CROP_EN %d",
                         pIPEIQSettings->lenrParameters.moduleCfg.DN4_BLTR_CLAMP_EN,
                         pIPEIQSettings->lenrParameters.moduleCfg.DN8_BLTR_CLAMP_EN,
                         pIPEIQSettings->lenrParameters.moduleCfg.DN16_BLTR_CLAMP_EN,
                         pIPEIQSettings->lenrParameters.moduleCfg.BLTR_EN,
                         pIPEIQSettings->lenrParameters.moduleCfg.EN,
                         pIPEIQSettings->lenrParameters.moduleCfg.FD_SNR_EN,
                         pIPEIQSettings->lenrParameters.moduleCfg.FNR_EN,
                         pIPEIQSettings->lenrParameters.moduleCfg.LAYER_1_ONLY,
                         pIPEIQSettings->lenrParameters.moduleCfg.LCE_EN,
                         pIPEIQSettings->lenrParameters.moduleCfg.RNR_EN,
                         pIPEIQSettings->lenrParameters.moduleCfg.SNR_EN,
                         pIPEIQSettings->lenrParameters.moduleCfg.POST_CROP_EN);
    }

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIPETuningMetadata)
    {
        result = m_pHWSetting->UpdateTuningMetadata(pInputData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Update Tuning Metadata failed");
            result = CamxResultSuccess; // Non-fatal error
        }

        CAMX_STATIC_ASSERT(MAX_FACE_NUM <= TuningMaxFaceNumber);

        TuningFaceData* pLENRFaceDetection = &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPELENRFaceDetection;

        if (NULL != m_dependenceData.pFDData)
        {
            pLENRFaceDetection->numberOfFace = (m_dependenceData.pFDData->numberOfFace > LENR_V10_MAX_FACE_NUM) ?
                LENR_V10_MAX_FACE_NUM : m_dependenceData.pFDData->numberOfFace;
            for (UINT32 count = 0; count < m_dependenceData.pFDData->numberOfFace; count++)
            {
                pLENRFaceDetection->faceRadius[count]  = m_dependenceData.pFDData->faceRadius[count];
                pLENRFaceDetection->faceCenterX[count] = m_dependenceData.pFDData->faceCenterX[count];
                pLENRFaceDetection->faceCenterY[count] = m_dependenceData.pFDData->faceCenterY[count];
            }
        }

        if ((NULL != pIPEIQSettings) && (TRUE == pIPEIQSettings->lenrParameters.moduleCfg.EN))
        {
            DebugDataTagID faceDataTagID;
            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                faceDataTagID = DebugDataTagID::TuningIPELENRFace;
            }
            else
            {
                faceDataTagID = DebugDataTagID::TuningIPELENRFaceOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                faceDataTagID,
                DebugDataTagType::TuningFaceData,
                1,
                pLENRFaceDetection,
                sizeof(TuningFaceData));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                result = CamxResultSuccess; // Non-fatal error
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELENR10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

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
            UpdateIPEInternalData(pInputData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Operation failed %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid pointer pInputData %p, m_pHWSetting %p",
                       pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPELENR10::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10::IPELENR10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPELENR10::IPELENR10(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier  = pNodeIdentifier;
    m_type             = ISPIQModuleType::IPELENR;
    m_moduleEnable     = TRUE;
    m_numLUT           = 0;
    m_pChromatix       = NULL;
    m_cmdLength        = 0;

    m_dependenceData.moduleEnable = FALSE; ///< First frame is always FALSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10::~IPELENR10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPELENR10::~IPELENR10()
{
    DeallocateCommonLibraryData();


    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
