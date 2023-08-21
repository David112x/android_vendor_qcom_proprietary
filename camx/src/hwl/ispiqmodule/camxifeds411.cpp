////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeds411.cpp
/// @brief CAMXIFEDS411 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifeds411titan480.h"
#include "camxifeds411.h"

#include "camxnode.h"
#include "camxtuningdatamanager.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS411::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEDS411* pModule = CAMX_NEW IFEDS411(pCreateData);
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
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Null pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS411::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEDS411Titan480;
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
// IFEDS411::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFEDS411::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS411::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (IFEMaxNonCommonPaths <= static_cast<UINT>(m_modulePath))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid m_modulePath");
    }

    if (CamxResultSuccess == result)
    {
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
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS411::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult       result       = CamxResultSuccess;

    m_dependenceData.modulePath   = static_cast<UINT>(m_modulePath);

    result = m_pHWSetting->PackIQRegisterSetting(static_cast<VOID*>(&m_dependenceData),
                                                 static_cast<VOID*>(m_pState));
    if (CamxResultSuccess != result)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupISP, "DS411 Calculation Failed. result %d", result);
    }

    if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
    {
        m_pHWSetting->DumpRegConfig();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS411::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    IFEScalerOutput* pCalculatedScalerOutput    = &pInputData->pCalculatedData->scalerOutput[0];

    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        // Post Crop module on DS16 path needs to know DS4 output dimension
        pCalculatedScalerOutput[DS16Output].dimension.width     = Utils::AlignGeneric32(
            m_pState->MNDSOutput.width / DS4Factor, 2);
        pCalculatedScalerOutput[DS16Output].dimension.height    = Utils::AlignGeneric32(
            m_pState->MNDSOutput.height / DS4Factor, 2);

        pCalculatedScalerOutput[DS16Output].scalingFactor   = pCalculatedScalerOutput[FullOutput].scalingFactor *
            DS16Factor;
        pCalculatedScalerOutput[DS16Output].preCropEnable   = TRUE;

    }
    if (m_modulePath == IFEPipelinePath::DisplayDS4Path)
    {
        pInputData->pCalculatedData->isDisplayDS4Enable = m_pState->moduleFlags.isDS4PathEnable;

        // Post Crop module on DS4 path needs to know DS4 output dimension
        pCalculatedScalerOutput[DisplayDS4Output].dimension.width  = Utils::AlignGeneric32(
            m_pState->MNDSOutput.width / DS4Factor, 2);
        pCalculatedScalerOutput[DisplayDS4Output].dimension.height = Utils::AlignGeneric32(
            m_pState->MNDSOutput.height / DS4Factor, 2);

        pCalculatedScalerOutput[DisplayDS4Output].scalingFactor =
            pCalculatedScalerOutput[DisplayFullOutput].scalingFactor * DS4Factor;
        pCalculatedScalerOutput[DisplayDS4Output].preCropEnable = TRUE;
    }
    if (m_modulePath == IFEPipelinePath::DisplayDS16Path)
    {
        // Post Crop module on DS16 path needs to know DS4 output dimension
        pCalculatedScalerOutput[DisplayDS16Output].dimension.width  = Utils::AlignGeneric32(
            m_pState->MNDSOutput.width / DS4Factor, 2);
        pCalculatedScalerOutput[DisplayDS16Output].dimension.height = Utils::AlignGeneric32(
            m_pState->MNDSOutput.height / DS4Factor, 2);
        pCalculatedScalerOutput[DisplayDS16Output].scalingFactor    =
            pCalculatedScalerOutput[DisplayFullOutput].scalingFactor * DS16Factor;
        pCalculatedScalerOutput[DisplayDS16Output].preCropEnable    = TRUE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEDS411::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL                result                   = FALSE;
    StreamDimension*    pHALStream               = &pInputData->pStripeConfig->stream[0];
    IFEScalerOutput*    pCalculatedScalerOutput  = &pInputData->pCalculatedData->scalerOutput[0];
    TuningDataManager*  pTuningManager           = pInputData->pTuningDataManager;

    CAMX_ASSERT(NULL != pTuningManager);

    // If the HAL Stream width and height are not same to Stripe Config, re-stored the width & height here.
    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        UINT32          fullPortIdx  = DisplayFullOutput;
        UINT32          DS4PortIdx   = DisplayDS4Output;
        UINT32          DS16PortIdx  = DisplayDS16Output;
        DS4PreCropInfo* pPreCropInfo = &pInputData->pCalculatedData->dispPreCropInfo;
        INT32           width        = pPreCropInfo->YCrop.lastPixel - pPreCropInfo->YCrop.firstPixel + 1;
        INT32           height       = pPreCropInfo->YCrop.lastLine  - pPreCropInfo->YCrop.firstLine  + 1;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Pre Crop[%d, %d, %d, %d], m_pState Pre Crop[%d, %d, %d, %d]",
                         pPreCropInfo->YCrop.firstPixel,
                         pPreCropInfo->YCrop.firstLine,
                         pPreCropInfo->YCrop.lastPixel,
                         pPreCropInfo->YCrop.lastLine,
                         m_pState->preCropInfo.YCrop.firstPixel,
                         m_pState->preCropInfo.YCrop.firstLine,
                         m_pState->preCropInfo.YCrop.lastPixel,
                         m_pState->preCropInfo.YCrop.lastLine);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS output[%d * %d], m_pState MNDS output[%d * d]",
                         pCalculatedScalerOutput[fullPortIdx].dimension.width,
                         pCalculatedScalerOutput[fullPortIdx].dimension.height,
                         m_pState->MNDSOutput.width,
                         m_pState->MNDSOutput.height);

        if ((pCalculatedScalerOutput[fullPortIdx].dimension.width  != m_pState->MNDSOutput.width)             ||
            (pCalculatedScalerOutput[fullPortIdx].dimension.height != m_pState->MNDSOutput.height)            ||
            (pHALStream[DS4PortIdx].width                          != m_pState->DS4PathOutput.width)          ||
            (pHALStream[DS4PortIdx].height                         != m_pState->DS4PathOutput.height)         ||
            (pPreCropInfo->YCrop.firstPixel                        != m_pState->preCropInfo.YCrop.firstPixel) ||
            (pPreCropInfo->YCrop.firstLine                         != m_pState->preCropInfo.YCrop.firstLine)  ||
            (pPreCropInfo->YCrop.lastPixel                         != m_pState->preCropInfo.YCrop.lastPixel)  ||
            (pPreCropInfo->YCrop.lastLine                          != m_pState->preCropInfo.YCrop.lastLine)   ||
            (TRUE == pInputData->forceTriggerUpdate))
        {

            if ((TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                m_dependenceData.pDS4Chromatix = pTuningManager->GetChromatix()->GetModule_ds4to1v11_ife_disp_full_dc4(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
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
                result = TRUE;
            }

            m_pState->overWriteStripes  = pInputData->pStripeConfig->overwriteStripes;
        }

        m_pState->MNDSOutput.width    = width;
        m_pState->MNDSOutput.height   = height;
        m_pState->preCropInfo         = *pPreCropInfo;

        m_dependenceData.streamWidth  = m_pState->MNDSOutput.width;
        m_dependenceData.streamHeight = m_pState->MNDSOutput.height;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "scaler [%d x %d] DS4 [%d x %d]  ",
                         m_pState->MNDSOutput.width,
                         m_pState->MNDSOutput.height,
                         m_pState->DS4PathOutput.width,
                         m_pState->DS4PathOutput.height);

        pInputData->pCalculatedData->cropDependencyChanged[DS4PortIdx] = result;
    }

    if ((IFEPipelinePath::VideoDS16Path == m_modulePath) || (IFEPipelinePath::DisplayDS16Path == m_modulePath))
    {
        DS4PreCropInfo* pPreCropInfoDS16 = NULL;

        UINT32   fullPortIdx = FullOutput;
        UINT32   DS16PortIdx = DS16Output;
        UINT32   DS4PortIdx  = DS4Output;

        if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
        {
            fullPortIdx = DisplayFullOutput;
            DS16PortIdx = DisplayDS16Output;
            DS4PortIdx  = DisplayDS4Output;
        }

        if (IFEPipelinePath::VideoDS16Path == m_modulePath)
        {
            pPreCropInfoDS16 = &pInputData->pCalculatedData->preCropInfoDS16;
        }
        else
        {
            pPreCropInfoDS16 = &pInputData->pCalculatedData->dispPreCropInfoDS16;
        }

        INT32 width  = (pPreCropInfoDS16->YCrop.lastPixel - pPreCropInfoDS16->YCrop.firstPixel) + 1;
        INT32 height = (pPreCropInfoDS16->YCrop.lastLine  - pPreCropInfoDS16->YCrop.firstLine)  + 1;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Idx: %u lastPixel - first pixel + 1 = width [%d - %d] = %d",
                         DS16PortIdx, pPreCropInfoDS16->YCrop.lastPixel, pPreCropInfoDS16->YCrop.firstPixel, width);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Idx: %u lastline - first line + 1 = height [%d - %d] = %d",
                         DS16PortIdx, pPreCropInfoDS16->YCrop.lastLine, pPreCropInfoDS16->YCrop.firstLine, height);


        if ((pCalculatedScalerOutput[fullPortIdx].dimension.width  != m_pState->MNDSOutput.width)     ||
            (pCalculatedScalerOutput[fullPortIdx].dimension.height != m_pState->MNDSOutput.height)    ||
            (pHALStream[DS16PortIdx].width                         != m_pState->DS16PathOutput.width) ||
            (pHALStream[DS16PortIdx].height                        != m_pState->DS16PathOutput.height) ||
            (TRUE == pInputData->forceTriggerUpdate))
        {
            if ((TRUE == pTuningManager->IsValidChromatix()) &&
                (NULL != pInputData->pTuningData))
            {

                m_dependenceData.pDS16Chromatix = pTuningManager->GetChromatix()->GetModule_ds4to1v11_ife_disp_dc4_dc16(
                    reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                    pInputData->pTuningData->noOfSelectionParameter);
            }
            m_pState->DS16PathOutput.width  = pHALStream[DS16PortIdx].width;
            m_pState->DS16PathOutput.height = pHALStream[DS16PortIdx].height;

            m_pState->moduleFlags.isDS16PathEnable = TRUE;

            m_dependenceData.streamWidth  = pCalculatedScalerOutput[DS4PortIdx].dimension.width;
            m_dependenceData.streamHeight = pCalculatedScalerOutput[DS4PortIdx].dimension.height;

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Stream WxH [%d x %d]",
                             m_dependenceData.streamWidth,
                             m_dependenceData.streamHeight);

            result = TRUE;
        }

        if (TRUE == pInputData->pStripeConfig->overwriteStripes)
        {
            ISPInternalData* pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
            if (TRUE == pFrameLevel->scalerDependencyChanged[DS16PortIdx])
            {
                m_moduleEnable = TRUE;
                result         = TRUE;
            }

            m_pState->overWriteStripes    = pInputData->pStripeConfig->overwriteStripes;
            m_dependenceData.streamWidth  = m_pState->MNDSOutput.width;
            m_dependenceData.streamHeight = m_pState->MNDSOutput.height;
        }

        m_pState->MNDSOutput.width  = width;
        m_pState->MNDSOutput.height = height;
        m_pState->preCropInfo       = *pPreCropInfoDS16;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "scaler [%d x %d] DS16 [%d x %d]  ",
                         m_pState->MNDSOutput.width,
                         m_pState->MNDSOutput.height,
                         m_pState->DS16PathOutput.width,
                         m_pState->DS16PathOutput.height);

        pInputData->pCalculatedData->cropDependencyChanged[DS16PortIdx] = result;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS411::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    CamxResult       result                  = CamxResultSuccess;
    IFEScalerOutput* pCalculatedScalerOutput = &pInputData->pCalculatedData->scalerOutput[0];

    // Full path MNDS output Validation, Both MNDS and DS4 should be enabled for DS16 path
    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {

        if ((0      != pInputData->pStripeConfig->stream[DS16Output].width)  &&
            (0      != pInputData->pStripeConfig->stream[DS16Output].height) &&
            ((0     == pCalculatedScalerOutput[FullOutput].dimension.width)  ||
             (0     == pCalculatedScalerOutput[FullOutput].dimension.height) ||
             (FALSE == pInputData->pCalculatedData->isVideoDS4Enable)))
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Full path MNDS ouput for DS16 path");
        }

        if (CamxResultSuccess == result)
        {
            DS4PreCropInfo* pPreCropInfo = &pInputData->pCalculatedData->preCropInfoDS16;
            INT32           width        = (pPreCropInfo->YCrop.lastPixel - pPreCropInfo->YCrop.firstPixel) + 1;
            INT32           height       = (pPreCropInfo->YCrop.lastLine  - pPreCropInfo->YCrop.firstLine)  + 1;
            if ((0 > width) || (0 > height))
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "VideoDS16Path Pre Crop[%d, %d, %d, %d], width = %d, height = %d",
                        pPreCropInfo->YCrop.firstPixel,
                        pPreCropInfo->YCrop.firstLine,
                        pPreCropInfo->YCrop.lastPixel,
                        pPreCropInfo->YCrop.lastLine,
                        width,
                        height);
                result = CamxResultEInvalidArg;
            }
        }
    }

    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        if ((0  != pInputData->pStripeConfig->stream[DisplayDS4Output].width)    &&
            (0  != pInputData->pStripeConfig->stream[DisplayDS4Output].height)   &&
            ((0 == pCalculatedScalerOutput[DisplayFullOutput].dimension.width)   ||
             (0 == pCalculatedScalerOutput[DisplayFullOutput].dimension.height)))
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Display Full path MNDS ouput for Display DS4 path");
        }

        if (CamxResultSuccess == result)
        {
            DS4PreCropInfo* pPreCropInfo = &pInputData->pCalculatedData->dispPreCropInfo;
            INT32           width        = pPreCropInfo->YCrop.lastPixel - pPreCropInfo->YCrop.firstPixel + 1;
            INT32           height       = pPreCropInfo->YCrop.lastLine  - pPreCropInfo->YCrop.firstLine  + 1;
            if ((0 > width) || (0 > height))
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Pre Crop[%d, %d, %d, %d], width = %d, height = %d",
                        pPreCropInfo->YCrop.firstPixel,
                        pPreCropInfo->YCrop.firstLine,
                        pPreCropInfo->YCrop.lastPixel,
                        pPreCropInfo->YCrop.lastLine,
                        width,
                        height);
                result = CamxResultEInvalidArg;
            }
        }
    }

    if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        if ((0      != pInputData->pStripeConfig->stream[DisplayDS16Output].width)       &&
            (0      != pInputData->pStripeConfig->stream[DisplayDS16Output].height)      &&
            ((0     == pCalculatedScalerOutput[DisplayFullOutput].dimension.width)       ||
             (0     == pCalculatedScalerOutput[DisplayFullOutput].dimension.height)      ||
             (FALSE == pInputData->pCalculatedData->isDisplayDS4Enable)))
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Display Full path MNDS ouput for Display DS16 path");
        }

        if (CamxResultSuccess == result)
        {
            DS4PreCropInfo* pPreCropInfo = &pInputData->pCalculatedData->dispPreCropInfoDS16;
            INT32           width        = (pPreCropInfo->YCrop.lastPixel - pPreCropInfo->YCrop.firstPixel) + 1;
            INT32           height       = (pPreCropInfo->YCrop.lastLine  - pPreCropInfo->YCrop.firstLine)  + 1;
            if ((0 > width) || (0 > height))
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "VideoDS16Path Pre Crop[%d, %d, %d, %d], width = %d, height = %d",
                        pPreCropInfo->YCrop.firstPixel,
                        pPreCropInfo->YCrop.firstLine,
                        pPreCropInfo->YCrop.lastPixel,
                        pPreCropInfo->YCrop.lastLine,
                        width,
                        height);
                result = CamxResultEInvalidArg;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS411::Execute(
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
                    result             = m_pHWSetting->CreateCmdList(pSettingData, NULL);
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
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411::IFEDS411
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDS411::IFEDS411(
    IFEModuleCreateData* pCreateData)
{
    m_type           = ISPIQModuleType::IFEDS4;
    m_moduleEnable   = TRUE;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
    m_modulePath     = pCreateData->pipelineData.IFEPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEDS411::~IFEDS411
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDS411::~IFEDS411()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
