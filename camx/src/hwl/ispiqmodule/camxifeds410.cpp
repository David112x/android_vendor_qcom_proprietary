////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeds410.cpp
/// @brief CAMXIFEDS410 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifeds410titan17x.h"
#include "camxifeds410.h"

#include "camxnode.h"
#include "camxtuningdatamanager.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS410::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEDS410* pModule = CAMX_NEW IFEDS410(pCreateData);

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEDS410 object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEDS410 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS410::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEDS410Titan17x;
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
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        result      = CamxResultENoMemory;
        m_cmdLength = 0;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFEDS410::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS410::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult       result        = CamxResultSuccess;
    m_dependenceData.stream_height = m_pState->MNDSOutput.height;
    m_dependenceData.modulePath    = static_cast<UINT>(m_modulePath);

    result = m_pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&m_dependenceData), NULL);
    if (CamxResultSuccess != result)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupISP, "DS410 Calculation Failed. result %d", result);
    }

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS410::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    IFEScalerOutput* pCalculatedScalerOutput = &pInputData->pCalculatedData->scalerOutput[0];

    if (m_modulePath == IFEPipelinePath::VideoDS4Path)
    {
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4LumaEnable      =
            m_pState->moduleFlags.isDS4PathEnable;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4ChromaEnable    =
            m_pState->moduleFlags.isDS4PathEnable;

        // DS16 need to know if DS4 path is enabled or not
        pInputData->pCalculatedData->isVideoDS4Enable = m_pState->moduleFlags.isDS4PathEnable;

        // Post Crop module on DS4 path needs to know DS4 output dimension
        pCalculatedScalerOutput[DS4Output].dimension.width  = Utils::AlignGeneric32(
            pCalculatedScalerOutput[FullOutput].dimension.width , DS4Factor * 2 ) / DS4Factor;
        pCalculatedScalerOutput[DS4Output].dimension.height = Utils::AlignGeneric32(
            pCalculatedScalerOutput[FullOutput].dimension.height , DS4Factor * 2 ) / DS4Factor;
        pCalculatedScalerOutput[DS4Output].scalingFactor    = pCalculatedScalerOutput[FullOutput].scalingFactor * DS4Factor;
    }

    if (m_modulePath == IFEPipelinePath::VideoDS16Path)
    {
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16LumaEnable     =
            m_pState->moduleFlags.isDS16PathEnable;
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16ChromaEnable   =
            m_pState->moduleFlags.isDS16PathEnable;

        // Post Crop module on DS16 path needs to know DS4 output dimension
        pCalculatedScalerOutput[DS16Output].dimension.width  = Utils::AlignGeneric32(
            pCalculatedScalerOutput[FullOutput].dimension.width, DS16Factor * 2 ) / DS16Factor;
        pCalculatedScalerOutput[DS16Output].dimension.height = Utils::AlignGeneric32(
            pCalculatedScalerOutput[FullOutput].dimension.height, DS16Factor * 2 ) / DS16Factor;
        pCalculatedScalerOutput[DS16Output].scalingFactor    = pCalculatedScalerOutput[FullOutput].scalingFactor * DS16Factor;
    }

    if (m_modulePath == IFEPipelinePath::DisplayDS4Path)
    {
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4LumaEnable   =
            m_pState->moduleFlags.isDS4PathEnable;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4ChromaEnable =
            m_pState->moduleFlags.isDS4PathEnable;

        pInputData->pCalculatedData->isDisplayDS4Enable = m_pState->moduleFlags.isDS4PathEnable;

        // Post Crop module on DS4 path needs to know DS4 output dimension
        pCalculatedScalerOutput[DisplayDS4Output].dimension.width  = Utils::AlignGeneric32(
            pCalculatedScalerOutput[DisplayFullOutput].dimension.width , DS4Factor * 2 ) / DS4Factor;
        pCalculatedScalerOutput[DisplayDS4Output].dimension.height = Utils::AlignGeneric32(
            pCalculatedScalerOutput[DisplayFullOutput].dimension.height , DS4Factor * 2 ) / DS4Factor;

        pCalculatedScalerOutput[DisplayDS4Output].scalingFactor =
            pCalculatedScalerOutput[DisplayFullOutput].scalingFactor * DS4Factor;
    }

    if (m_modulePath == IFEPipelinePath::DisplayDS16Path)
    {
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16LumaEnable    =
            m_pState->moduleFlags.isDS16PathEnable;
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16ChromaEnable  =
            m_pState->moduleFlags.isDS16PathEnable;

        // Post Crop module on DS16 path needs to know DS4 output dimension
        pCalculatedScalerOutput[DisplayDS16Output].dimension.width  = Utils::AlignGeneric32(
            pCalculatedScalerOutput[DisplayFullOutput].dimension.width, DS16Factor * 2 ) / DS16Factor;
        pCalculatedScalerOutput[DisplayDS16Output].dimension.height = Utils::AlignGeneric32(
            pCalculatedScalerOutput[DisplayFullOutput].dimension.height, DS16Factor * 2 ) / DS16Factor;

        pCalculatedScalerOutput[DisplayDS16Output].scalingFactor =
            pCalculatedScalerOutput[DisplayFullOutput].scalingFactor * DS16Factor;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEDS410::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL                                   result                   = FALSE;
    StreamDimension*                       pHALStream               = &pInputData->pStripeConfig->stream[0];
    IFEScalerOutput*                       pCalculatedScalerOutput  = &pInputData->pCalculatedData->scalerOutput[0];
    TuningDataManager*                     pTuningManager           = NULL;

    pTuningManager = pInputData->pTuningDataManager;
    CAMX_ASSERT(NULL != pTuningManager);


        /// @todo (CAMX-919) Need to check if chromatix data has changed
    if ((IFEPipelinePath::VideoDS4Path == m_modulePath) || (IFEPipelinePath::DisplayDS4Path == m_modulePath))
    {
        DS4PreCropInfo* pPreCropInfo = &pInputData->pCalculatedData->preCropInfo;
        UINT32          fullPortIdx  = FullOutput;
        UINT32          DS4PortIdx   = DS4Output;
        if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
        {
            pPreCropInfo = &pInputData->pCalculatedData->dispPreCropInfo;
            fullPortIdx  = DisplayFullOutput;
            DS4PortIdx   = DisplayDS4Output;
        }
        UINT32 width  = (pPreCropInfo->YCrop.lastPixel - pPreCropInfo->YCrop.firstPixel) + 1;
        UINT32 height = (pPreCropInfo->YCrop.lastLine -  pPreCropInfo->YCrop.firstLine) + 1;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Idx: %u lastPixel - first pixel + 1 = width [%d - %d] = %d",
            DS4PortIdx, pPreCropInfo->YCrop.lastPixel, pPreCropInfo->YCrop.firstPixel, width);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Idx: %u lastline - first line + 1 = height  [%d - %d] = %d",
            DS4PortIdx, pPreCropInfo->YCrop.lastLine, pPreCropInfo->YCrop.firstLine, height);

        if ((width  != m_pState->MNDSOutput.width)                               ||
            (height != m_pState->MNDSOutput.height)                              ||
            (pHALStream[DS4PortIdx].width  != m_pState->DS4PathOutput.width)     ||
            (pHALStream[DS4PortIdx].height != m_pState->DS4PathOutput.height)    ||
            (TRUE == pInputData->forceTriggerUpdate))
        {
            if ((TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                m_dependenceData.pDS4Chromatix = pTuningManager->GetChromatix()->GetModule_ds4to1v10_ife_full_dc4(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }
            if (pInputData->pStripeConfig->cropType == CropTypeCentered)
            {
                m_pState->MNDSOutput.width  = width;
                m_pState->MNDSOutput.height = height;
            }
            else
            {
                m_pState->MNDSOutput.width  = pCalculatedScalerOutput[fullPortIdx].dimension.width;
                m_pState->MNDSOutput.height = pCalculatedScalerOutput[fullPortIdx].dimension.height;
            }

            m_pState->DS4PathOutput.width  = pHALStream[DS4PortIdx].width;
            m_pState->DS4PathOutput.height = pHALStream[DS4PortIdx].height;

            m_pState->moduleFlags.isDS4PathEnable = TRUE;

            result = TRUE;
        }

        if (TRUE == pInputData->pStripeConfig->overwriteStripes)
        {
            ISPInternalData* pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
            if (TRUE == pFrameLevel->scalerDependencyChanged[DS4PortIdx])
            {
                m_moduleEnable = TRUE;
                result         = TRUE;
            }

            m_pState->MNDSOutput.width  = width;
            m_pState->MNDSOutput.height = height;
        }
        pInputData->pCalculatedData->cropDependencyChanged[DS4PortIdx] = result;
    }

    if ((IFEPipelinePath::VideoDS16Path == m_modulePath) || (IFEPipelinePath::DisplayDS16Path == m_modulePath))
    {
        DS4PreCropInfo* pPreCropInfo     = &pInputData->pCalculatedData->preCropInfo;
        DS4PreCropInfo* pPreCropInfoDS16 = &pInputData->pCalculatedData->preCropInfoDS16;
        UINT32          fullPortIdx      = FullOutput;
        UINT32          DS16PortIdx      = DS16Output;
        if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
        {
            pPreCropInfo     = &pInputData->pCalculatedData->dispPreCropInfo;
            pPreCropInfoDS16 = &pInputData->pCalculatedData->dispPreCropInfoDS16;
            fullPortIdx      = DisplayFullOutput;
            DS16PortIdx      = DisplayDS16Output;
        }
        UINT32 width  = (pInputData->pStripeConfig->cropType == CropTypeCentered) ?
            (pPreCropInfo->YCrop.lastPixel - pPreCropInfo->YCrop.firstPixel) + 1 :
            (pPreCropInfoDS16->YCrop.lastPixel - pPreCropInfoDS16->YCrop.firstPixel) + 1;
        UINT32 height = (pInputData->pStripeConfig->cropType == CropTypeCentered) ?
            (pPreCropInfo->YCrop.lastLine - pPreCropInfo->YCrop.firstLine) + 1 :
            (pPreCropInfoDS16->YCrop.lastLine - pPreCropInfoDS16->YCrop.firstLine) + 1;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Idx: %u lastPixel - first pixel + 1 = width [%d - %d] = %d",
            DS16PortIdx, pPreCropInfo->YCrop.lastPixel, pPreCropInfo->YCrop.firstPixel, width);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Idx: %u lastline - first line + 1 = height [%d - %d] = %d",
            DS16PortIdx, pPreCropInfo->YCrop.lastLine, pPreCropInfo->YCrop.firstLine, height);

        if ((width  != m_pState->MNDSOutput.width)                              ||
            (height != m_pState->MNDSOutput.height)                             ||
            (pHALStream[DS16PortIdx].width  != m_pState->DS16PathOutput.width)  ||
            (pHALStream[DS16PortIdx].height != m_pState->DS16PathOutput.height) ||
            (TRUE == pInputData->forceTriggerUpdate))
        {
            if ((TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                m_dependenceData.pDS16Chromatix = pTuningManager->GetChromatix()->GetModule_ds4to1v10_ife_dc4_dc16(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }

            if (pInputData->pStripeConfig->cropType == CropTypeCentered)
            {
                m_pState->MNDSOutput.width  =  Utils::AlignGeneric32(width, DS4Factor ) / DS4Factor;
                m_pState->MNDSOutput.height = Utils::AlignGeneric32(height, DS4Factor ) / DS4Factor;
            }
            else
            {
                m_pState->MNDSOutput.width  =  pCalculatedScalerOutput[fullPortIdx].dimension.width;
                m_pState->MNDSOutput.height =  pCalculatedScalerOutput[fullPortIdx].dimension.height;
            }
            m_pState->DS16PathOutput.width  = pHALStream[DS16PortIdx].width;
            m_pState->DS16PathOutput.height = pHALStream[DS16PortIdx].height;

            m_pState->moduleFlags.isDS16PathEnable = TRUE;

            result = TRUE;
        }

        if (TRUE == pInputData->pStripeConfig->overwriteStripes)
        {
            ISPInternalData* pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
            if (TRUE == pFrameLevel->scalerDependencyChanged[DS16PortIdx])
            {
                m_moduleEnable = TRUE;
                result = TRUE;
            }

            m_pState->MNDSOutput.width  = width;
            m_pState->MNDSOutput.height = height;
        }
        pInputData->pCalculatedData->cropDependencyChanged[DS16PortIdx] = result;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS410::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    CamxResult       result                  = CamxResultSuccess;
    IFEScalerOutput* pCalculatedScalerOutput = &pInputData->pCalculatedData->scalerOutput[0];

    // Full path MNDS output Validation for DS4 path
    if ((IFEPipelinePath::VideoDS4Path == m_modulePath)                   &&
        (0 != pInputData->pStripeConfig->stream[DS4Output].width)    &&
        (0 != pInputData->pStripeConfig->stream[DS4Output].height)   &&
        ((0 == pCalculatedScalerOutput[FullOutput].dimension.width)  ||
         (0 == pCalculatedScalerOutput[FullOutput].dimension.height)))
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Full path MNDS ouput for DS4 path");
    }

    // Full path MNDS output Validation, Both MNDS and DS4 should be enabled for DS16 path
    if ((IFEPipelinePath::VideoDS16Path == m_modulePath)                      &&
        (0 != pInputData->pStripeConfig->stream[DS16Output].width)       &&
        (0 != pInputData->pStripeConfig->stream[DS16Output].height)      &&
        ((0     == pCalculatedScalerOutput[FullOutput].dimension.width)  ||
         (0     == pCalculatedScalerOutput[FullOutput].dimension.height) ||
         (FALSE == pInputData->pCalculatedData->isVideoDS4Enable)))
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Full path MNDS ouput for DS16 path");
    }

    // Full Display path MNDS output Validation for DS4 path
    if ((IFEPipelinePath::DisplayDS4Path == m_modulePath)                   &&
        (0 != pInputData->pStripeConfig->stream[DisplayDS4Output].width)    &&
        (0 != pInputData->pStripeConfig->stream[DisplayDS4Output].height)   &&
        ((0 == pCalculatedScalerOutput[DisplayFullOutput].dimension.width)  ||
         (0 == pCalculatedScalerOutput[DisplayFullOutput].dimension.height)))
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Display Full path MNDS ouput for Display DS4 path");
    }

    // Full Display path MNDS output Validation, Both Display MNDS and Display DS4 should be enabled for DS16 path
    if ((IFEPipelinePath::DisplayDS16Path == m_modulePath)                      &&
        (0 != pInputData->pStripeConfig->stream[DisplayDS16Output].width)       &&
        (0 != pInputData->pStripeConfig->stream[DisplayDS16Output].height)      &&
        ((0     == pCalculatedScalerOutput[DisplayFullOutput].dimension.width)  ||
         (0     == pCalculatedScalerOutput[DisplayFullOutput].dimension.height) ||
         (FALSE == pInputData->pCalculatedData->isDisplayDS4Enable)))
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Display Full path MNDS ouput for Display DS16 path");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS410::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(static_cast<UINT>(m_modulePath) < IFEMaxNonCommonPaths);
    if (NULL != pInputData)
    {
        m_pState = &pInputData->pStripeConfig->stateDS[static_cast<UINT>(m_modulePath)];

        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);
        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);

                if (CamxResultSuccess == result)
                {
                    VOID* pSettingData = static_cast<VOID*>(pInputData);
                    result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
                }
            }
            if (CamxResultSuccess == result)
            {
                UpdateIFEInternalData(pInputData);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS410::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(static_cast<UINT>(m_modulePath) < IFEMaxNonCommonPaths);

    if (NULL != pInputData)
    {
        m_pState = &pInputData->pStripeConfig->stateDS[static_cast<UINT>(m_modulePath)];

        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);
        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);
            }

            if (CamxResultSuccess == result)
            {
                UpdateIFEInternalData(pInputData);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410::IFEDS410
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDS410::IFEDS410(
    IFEModuleCreateData* pCreateData)
{
    m_type           = ISPIQModuleType::IFEDS4;
    m_moduleEnable   = TRUE;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
    m_modulePath     = pCreateData->pipelineData.IFEPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEDS410::~IFEDS410
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDS410::~IFEDS410()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
