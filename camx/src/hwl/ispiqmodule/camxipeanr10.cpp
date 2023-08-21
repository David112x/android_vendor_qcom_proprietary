////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeanr10.cpp
/// @brief CAMXIPEANR10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "camxdefs.h"
#include "camxipeanr10.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtitan17xcontext.h"
#include "camxtuningdatamanager.h"
#include "ipe_data.h"
#include "parametertuningtypes.h"
#include "camxipeanr10titan17x.h"
#include "camxipeanr10titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10::Create(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        IPEANR10* pModule = CAMX_NEW IPEANR10(pCreateData->pNodeIdentifier, pCreateData);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Memory allocation failed");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10::Initialize(
    IPEModuleCreateData* pCreateData)
{
    CamxResult result   = CamxResultSuccess;
    m_pHWSetting        = NULL;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IPEANR10Titan480;
            break;

        default:
            m_pHWSetting = CAMX_NEW IPEANR10Titan17x;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength = m_pHWSetting->GetCommandLength();

        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);

        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to initilize common library data, no memory");
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to create HW Setting Class, no memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::UpdateIPEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10::UpdateIPEInternalData(
    ISPInputData* pInputData)
{
    CamxResult      result         = CamxResultSuccess;
    IpeIQSettings*  pIPEIQSettings = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    // Run NClib common IQ calculations.
    if ((TRUE == m_enableCommonIQ) &&
        (TRUE == m_moduleEnable))
    {
        Utils::Memcpy(&pIPEIQSettings->anrParameters, &m_ANRParameter, sizeof(m_ANRParameter));
    }
    else
    {
        // Using Default settings
        pIPEIQSettings->anrParameters.parameters[PASS_NAME_FULL].moduleCfg.EN  = m_moduleEnable;
        pIPEIQSettings->anrParameters.parameters[PASS_NAME_DC_4].moduleCfg.EN  = m_moduleEnable;
        pIPEIQSettings->anrParameters.parameters[PASS_NAME_DC_16].moduleCfg.EN = m_moduleEnable;
        // Hardcoded settings only has values for 3 passes
        pIPEIQSettings->anrParameters.parameters[PASS_NAME_DC_64].moduleCfg.EN = 0;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, " enable  Module enable on pass full - 64 : %d, %d, %d, %d",
                     pIPEIQSettings->anrParameters.parameters[PASS_NAME_FULL].moduleCfg.EN,
                     pIPEIQSettings->anrParameters.parameters[PASS_NAME_DC_4].moduleCfg.EN,
                     pIPEIQSettings->anrParameters.parameters[PASS_NAME_DC_16].moduleCfg.EN,
                     pIPEIQSettings->anrParameters.parameters[PASS_NAME_DC_64].moduleCfg.EN);

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIPETuningMetadata)
    {
        CAMX_STATIC_ASSERT(MAX_FACE_NUM <= TuningMaxFaceNumber);

        result = m_pHWSetting->UpdateTuningMetadata(pInputData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Update Tuning Metadata failed.");
            result = CamxResultSuccess; // Non-fatal error
        }

        TuningFaceData* pANRFaceDetection = NULL;

        switch (pInputData->titanVersion)
        {
            case CSLCameraTitanVersion::CSLTitan480:
                pANRFaceDetection = &pInputData->pIPETuningMetadata->IPETuningMetadata480.IPEANRFaceDetection;
                break;
            case CSLCameraTitanVersion::CSLTitan150:
            case CSLCameraTitanVersion::CSLTitan160:
            case CSLCameraTitanVersion::CSLTitan170:
            case CSLCameraTitanVersion::CSLTitan175:
                pANRFaceDetection = &pInputData->pIPETuningMetadata->IPETuningMetadata17x.IPEANRFaceDetection;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid hardware version: %d", pInputData->titanVersion);
                break;
        }

        if (NULL != pANRFaceDetection)
        {
            if (NULL != m_dependenceData.pFDData)
            {
                pANRFaceDetection->numberOfFace = ((m_dependenceData.pFDData->numberOfFace) > MAX_FACE_NUM) ?
                    MAX_FACE_NUM : (m_dependenceData.pFDData->numberOfFace);

                for (INT32 count = 0; count < pANRFaceDetection->numberOfFace; count++)
                {
                    pANRFaceDetection->faceCenterX[count] = m_dependenceData.pFDData->faceCenterX[count];
                    pANRFaceDetection->faceCenterY[count] = m_dependenceData.pFDData->faceCenterY[count];
                    pANRFaceDetection->faceRadius[count] = m_dependenceData.pFDData->faceRadius[count];
                }
            }


            if (TRUE == pIPEIQSettings->anrParameters.parameters[PASS_NAME_FULL].moduleCfg.EN)
            {
                DebugDataTagID  dataTagID;

                if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
                {
                    dataTagID = DebugDataTagID::TuningIPEANRFace;
                }
                else
                {
                    dataTagID = DebugDataTagID::TuningIPEANRFaceOffline;
                }

                result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                            dataTagID,
                            DebugDataTagType::TuningFaceData,
                            1,
                            pANRFaceDetection,
                            sizeof(TuningFaceData));
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                    result = CamxResultSuccess; // Non-fatal error
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10::ValidateDependenceParams(
    const ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    /// @todo (CAMX-730) validate dependency parameters
    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IQSettingsPtr [0x%p] ", pInputData->pipelineIPEData.pIPEIQSettings);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::SetMaxSupportedPassesForUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEANR10::SetMaxSupportedPassesForUsecase(
    const ISPInputData* pInputData)
{
    if ((pInputData->pTuningData->TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase ==
        ChiModeUsecaseSubModeType::Video) ||
        (pInputData->pTuningData->TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase ==
            ChiModeUsecaseSubModeType::Preview))
    {
        m_dependenceData.maxSupportedPassesForUsecase = PASS_NAME_MAX - 1;
    }
    else if ((pInputData->pTuningData->TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase ==
        ChiModeUsecaseSubModeType::Snapshot) ||
        (pInputData->pTuningData->TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase ==
            ChiModeUsecaseSubModeType::Liveshot) ||
            (pInputData->pTuningData->TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase ==
                ChiModeUsecaseSubModeType::ZSL))
    {
        m_dependenceData.maxSupportedPassesForUsecase = PASS_NAME_MAX;
    }
    else
    {
        m_dependenceData.maxSupportedPassesForUsecase = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPEANR10::CheckDependenceChange(
    ISPInputData* pInputData)
{

    BOOL isChanged = FALSE;

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pAECUpdateData)  &&
        (NULL != pInputData->pAWBUpdateData)  &&
        (NULL != pInputData->pHwContext))
    {
        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMIPEIQSetting*>(pInputData->pOEMIQSetting))->ANR10Enable;

            isChanged      = (TRUE == m_moduleEnable);
        }
        else
        {
            if (NULL != pInputData->pTuningData)
            {
                TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
                CAMX_ASSERT(NULL != pTuningManager);

                // Search through the tuning data (tree), only when there
                // are changes to the tuning mode data as an optimization
                if ((TRUE == pInputData->tuningModeChanged)    &&
                    (TRUE == pTuningManager->IsValidChromatix()))
                {
                    m_pChromatix = pTuningManager->GetChromatix()->GetModule_anr10_ipe(
                                       reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                       pInputData->pTuningData->noOfSelectionParameter);
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if (m_pChromatix != m_dependenceData.pChromatix)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "updating chromatix pointer");
                        m_dependenceData.pChromatix = m_pChromatix;
                        m_moduleEnable              = ((TRUE == m_pChromatix->enable_section.enable_chroma_noise_reduction) ||
                            (TRUE == m_pChromatix->enable_section.enable_luma_noise_reduction)) ? 1 : 0;

                        isChanged = (TRUE == m_moduleEnable);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get Chromatix");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Tuning Pointer");
            }
        }

        if (TRUE == m_moduleEnable)
        {
            if (TRUE ==
                IQInterface::s_interpolationTable.ANR10TriggerUpdate(&pInputData->triggerData, &m_dependenceData))
            {
                isChanged = TRUE;
            }

            m_dependenceData.frameNum = pInputData->frameNum;
            m_dependenceData.bitWidth =
                ImageFormatUtils::Is10BitFormat(pInputData->pipelineIPEData.fullInputFormat) ? 10 : 8;

            // Update face information / optical center info
            m_dependenceData.numPasses             = pInputData->pipelineIPEData.numPasses;
            m_dependenceData.opticalCenterX        = pInputData->opticalCenterX;
            m_dependenceData.opticalCenterY        = pInputData->opticalCenterY;
            m_dependenceData.pWarpGeometriesOutput = pInputData->pipelineIPEData.pWarpGeometryData;
            m_dependenceData.pImageDimensions      = &pInputData->pipelineIPEData.inputDimension;
            m_dependenceData.pMarginDimensions     = &pInputData->pipelineIPEData.marginDimension;
            m_dependenceData.pANRParameters        = &m_ANRParameter;
            m_dependenceData.pFDData               = &pInputData->fDData;
            m_dependenceData.validateANRSettings   = m_validateANRSettings;
            SetMaxSupportedPassesForUsecase(pInputData);
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
                             " gain %f, lux %f, frame %d  bit %d w %d, h %d mw %d, mh %d",
                             m_dependenceData.luxIndex, m_dependenceData.AECGain, m_dependenceData.frameNum,
                             m_dependenceData.bitWidth, m_dependenceData.pImageDimensions->widthPixels,
                             m_dependenceData.pImageDimensions->heightLines,
                             m_dependenceData.pMarginDimensions->widthPixels,
                             m_dependenceData.pMarginDimensions->heightLines);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ANR10 Module is not enabled");
        }
    }
    else
    {
        if (NULL != pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc,
                           "Invalid Input: pNewAECUpdate %p pNewAWBUpdate %p pHwContext %p",
                           pInputData->pAECUpdateData,
                           pInputData->pAWBUpdateData,
                           pInputData->pHwContext);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult      result     = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Number of Passes [0x%d] enable common IQ %d video_preview(1)/still(0)",
        m_dependenceData.numPasses, m_enableCommonIQ, pInputData->pipelineIPEData.realtimeFlag);

    if (TRUE == m_enableCommonIQ)
    {
        // running calculation
        m_dependenceData.realtimeFlag = pInputData->pipelineIPEData.realtimeFlag;

        result = IQInterface::IPEANR10CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE ANR10 Calculation Failed.");
        }
    }
    else
    {
        m_pHWSetting->SetupRegisterSetting(pInputData);
    }

    if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
    {
        m_pHWSetting->DumpRegConfig();;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);

        if ((CamxResultSuccess == result) && ((TRUE == m_moduleEnable) ||
            (TRUE == pInputData->tuningModeChanged)))
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
            }

            if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
            {
                result = m_pHWSetting->CreateCmdList(pInputData, NULL);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Runcalculation Failed");
            }

            if (CamxResultSuccess == result)
            {
                result = UpdateIPEInternalData(pInputData);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "CreateCmdList Failed");
            }


            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Operation failed %d", result);
            }

        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEANR10::GetModuleData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEANR10::GetModuleData(
    VOID* pModuleData)
{
    CAMX_ASSERT(NULL != pModuleData);
    m_pHWSetting->GetModuleData(pModuleData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEANR10::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    ANRNcLibOutputData data;

    result = IQInterface::IPEANR10GetInitializationData(&data);
    CAMX_ASSERT(CamxResultSuccess == result);

    // Add 1 for ouput of interpolation library
    UINT interpolationSize = (sizeof(anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct) * (ANRMaxNonLeafNode + 1));
    if (NULL == m_dependenceData.pInterpolationData)
    {
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    if ((NULL == m_dependenceData.pNCChromatix) && (CamxResultSuccess == result))
    {
        m_dependenceData.pNCChromatix = CAMX_CALLOC(data.ANR10ChromatixSize);
        if (NULL == m_dependenceData.pNCChromatix)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEANR10::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }

    if (NULL != m_dependenceData.pNCChromatix)
    {
        CAMX_FREE(m_dependenceData.pNCChromatix);
        m_dependenceData.pNCChromatix = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEANR10::IPEANR10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEANR10::IPEANR10(
    const CHAR*          pNodeIdentifier,
    IPEModuleCreateData* pCreateData)
{
    m_pNodeIdentifier     = pNodeIdentifier;
    m_type                = ISPIQModuleType::IPEANR;
    m_moduleEnable        = TRUE;
    m_pChromatix          = NULL;
    m_32bitDMILength      = 0;
    m_64bitDMILength      = 0;
    m_validateANRSettings = FALSE;

    Utils::Memset(&m_dependenceData, 0x0, sizeof(m_dependenceData));
    Utils::Memset(&m_ANRParameter,   0x0, sizeof(m_ANRParameter));



    if (TRUE == pCreateData->initializationData.registerBETEn)
    {
        m_enableCommonIQ = TRUE;
    }
    else
    {
        Titan17xContext* pContext = static_cast<Titan17xContext*>(pCreateData->initializationData.pHwContext);
        m_enableCommonIQ      = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableANRCommonIQModule;
        m_validateANRSettings = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->validateANRSettings;
    }

    CAMX_LOG_INFO(CamxLogGroupIQMod,
        "IPE ANR m_cmdLength %d , m_EnableCommonIQ %d",
        m_cmdLength,
        m_enableCommonIQ);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPEANR10::~IPEANR10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEANR10::~IPEANR10()
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
