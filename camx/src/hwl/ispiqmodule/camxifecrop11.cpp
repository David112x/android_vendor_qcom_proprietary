////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecrop11.cpp
/// @brief CAMXIFECROP10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifecrop11titan480.h"
#include "camxifecrop11.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop11::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop11::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEPipelinePath modulePath = pCreateData->pipelineData.IFEPath;

        if ((IFEPipelinePath::FDPath           == modulePath) ||
            (IFEPipelinePath::VideoFullPath    == modulePath) ||
            (IFEPipelinePath::VideoDS4Path     == modulePath) ||
            (IFEPipelinePath::VideoDS16Path    == modulePath) ||
            (IFEPipelinePath::PixelRawDumpPath == modulePath) ||
            (IFEPipelinePath::DisplayFullPath  == modulePath) ||
            (IFEPipelinePath::DisplayDS4Path   == modulePath) ||
            (IFEPipelinePath::DisplayDS16Path  == modulePath))
        {
            IFECrop11* pModule = CAMX_NEW IFECrop11(pCreateData);
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
                CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create IFECrop11 object.");
            }
            pCreateData->pModule = pModule;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Null pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop11::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop11::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFECrop11Titan480;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        static_cast<IFECrop11Titan480*>(m_pHWSetting)->Initialize(m_modulePath, m_output);
        m_cmdLength = m_pHWSetting->GetCommandLength();
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
// IFECrop11::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFECrop11::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop11::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop11::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    INT32            CAMIFWidth;
    INT32            CAMIFHeight;
    CamxResult       result                  = CamxResultSuccess;
    CropInfo*        pSensorCAMIFCrop        = &pInputData->pStripeConfig->CAMIFCrop;
    CropWindow*      pInputCropWindow        = &pInputData->pStripeConfig->HALCrop[m_output];
    StreamDimension* pHALStream              = &pInputData->pStripeConfig->stream[m_output];
    IFEScalerOutput* pCalculatedScalerOutput = &pInputData->pCalculatedData->scalerOutput[m_output];

    if ((0 != pHALStream->width) && (0 != pHALStream->height))
    {
        CAMIFWidth  = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
        CAMIFHeight = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;

        // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
        if (TRUE == pInputData->sensorData.isYUV)
        {
            CAMIFWidth >>= 1;
        }

        // Validate Crop window from HAL
        if ((pInputCropWindow->left + pInputCropWindow->width >  CAMIFWidth)  ||
            (pInputCropWindow->top + pInputCropWindow->height >  CAMIFHeight) ||
            (0 == pInputCropWindow->width)                                    ||
            (0 == pInputCropWindow->height)                                   ||
            (0 == CAMIFWidth)                                                 ||
            (0 == CAMIFHeight))
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Crop window %d, %d, %d, %d, input dimension %d * %d",
                           pInputCropWindow->left,
                           pInputCropWindow->top,
                           pInputCropWindow->width,
                           pInputCropWindow->height,
                           CAMIFWidth, CAMIFHeight);
            result = CamxResultEInvalidArg;
            CAMX_ASSERT_ALWAYS();
        }

        // Scaler output Validation for Full, FD, DS4, DS16 path
        if ((CamxResultSuccess == result)                                   &&
            (0                 != pHALStream->width)                        &&
            (0                 != pHALStream->height)                       &&
            ((PixelRawOutput   != m_output) &&
            ((TRUE             == Utils::FEqual(pCalculatedScalerOutput->scalingFactor, 0.0f)) ||
             (0                == pCalculatedScalerOutput->dimension.width)                    ||
             (0                == pCalculatedScalerOutput->dimension.height))))
        {
            result = CamxResultEInvalidArg;
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Scaler ouput for output path %d", m_output);
        }

    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop11::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECrop11::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL                 result                  = FALSE;
    IFEScalerOutput*     pCalculatedScalerOutput = &pInputData->pCalculatedData->scalerOutput[m_output];
    CropWindow*          pInputCropWindow        = &pInputData->pStripeConfig->HALCrop[m_output];
    StreamDimension*     pHALStream              = &pInputData->pStripeConfig->stream[m_output];

    // Generic dependency  check
    if ((pInputCropWindow->left     != m_pState->cropWindow.left)       ||
        (pInputCropWindow->top      != m_pState->cropWindow.top)        ||
        (pInputCropWindow->width    != m_pState->cropWindow.width)      ||
        (pInputCropWindow->height   != m_pState->cropWindow.height)     ||
        (pHALStream->width          != m_pState->streamDimension.width) ||
        (pHALStream->height         != m_pState->streamDimension.height)||
        (TRUE                       == pInputData->forceTriggerUpdate))
    {
        if ((0 != pHALStream->width) && (0 != pHALStream->height))
        {
            // App data
            m_pState->cropWindow                    = *pInputCropWindow;
            m_pState->modifiedCropWindow            = *pInputCropWindow;
            m_pState->streamDimension.width         = pHALStream->width;
            m_pState->streamDimension.height        = pHALStream->height;

            // Internal data
            m_pState->scalerOutput.dimension.width  = pCalculatedScalerOutput->dimension.width;
            m_pState->scalerOutput.dimension.height = pCalculatedScalerOutput->dimension.height;
            m_pState->scalerOutput.scalingFactor    = pCalculatedScalerOutput->scalingFactor;
            m_pState->scalerOutput.preCropEnable    = pCalculatedScalerOutput->preCropEnable;
            m_pState->scalerOutput.scalingLimitHit  = pCalculatedScalerOutput->scalingLimitHit;

            // result
            m_moduleEnable = TRUE;
            result         = TRUE;

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Path %d, preCropEnable %d, Scaler Dim[%d * %d]",
                             m_output,
                             pCalculatedScalerOutput->preCropEnable,
                             pCalculatedScalerOutput->dimension.width,
                             pCalculatedScalerOutput->dimension.height);
        }
        else
        {
            m_moduleEnable = FALSE;
            result         = FALSE;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid IFE output dimensions [%d, %d] for path [%d]",
                                            pHALStream->width,
                                            pHALStream->height,
                                            m_output);
        }
    }

    if (TRUE == pInputData->pStripeConfig->overwriteStripes)
    {
        ISPInternalData* pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
        if (TRUE == pFrameLevel->cropDependencyChanged[m_output])
        {
            m_moduleEnable = TRUE;
            result = TRUE;
        }
    }
    else if (PixelRawOutput == m_output)
    {
        m_moduleEnable = FALSE;
        result         = TRUE;
    }

    pInputData->pCalculatedData->cropDependencyChanged[m_output] = result;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop11::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop11::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    CropOutputData outputData;
    outputData.pCropState       = m_pState;
    outputData.modulePath       = m_modulePath;
    outputData.ifeOutputPath    = m_output;

    VOID* pSettingData = static_cast<VOID*>(pInputData);
    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        result = m_pHWSetting->PackIQRegisterSetting(pSettingData, &outputData);

        if ((CamxResultSuccess == result) &&
            (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex)))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop11::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECrop11::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    IFEModuleEnableConfig* pCropModuleEnable = &pInputData->pCalculatedData->moduleEnable;

    VOID* pSettingData = static_cast<VOID*>(pInputData);
    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        CamxResult result = CamxResultSuccess;
        result = m_pHWSetting->SetupRegisterSetting(pSettingData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Setup Register is failed");
        }

        switch (m_output)
        {
            case FDOutput:
                pCropModuleEnable->FDprocessingModules.RoundClampLumaCropEnable     = m_moduleEnable;
                pCropModuleEnable->FDprocessingModules.RoundClampChromaCropEnable   = m_moduleEnable;
                pCropModuleEnable->FDprocessingModules.CropEnable = m_moduleEnable;
                break;

            case FullOutput:
                pCropModuleEnable->videoProcessingModules.RoundClampLumaCropEnable     = m_moduleEnable;
                pCropModuleEnable->videoProcessingModules.RoundClampChromaCropEnable   = m_moduleEnable;
                pCropModuleEnable->videoProcessingModules.CropEnable = m_moduleEnable;
                break;

            case DS4Output:
                pCropModuleEnable->videoProcessingModules.DS4RoundClampLumaCropEnable     = m_moduleEnable;
                pCropModuleEnable->videoProcessingModules.DS4RoundClampChromaCropEnable   = m_moduleEnable;
                pCropModuleEnable->videoProcessingModules.DS4LumaEnable = m_moduleEnable;
                pCropModuleEnable->videoProcessingModules.DS4ChromaEnable = m_moduleEnable;

                // PostCrop is disabled if PreCrop is enabled
                if (CropTypeCentered == pInputData->pStripeConfig->cropType)
                {
                    if (0 == pCropModuleEnable->videoProcessingModules.DS4PreCropEnable)
                    {
                        pCropModuleEnable->videoProcessingModules.DS4PostCropEnable = m_moduleEnable;
                    }
                }
                else
                {
                    pCropModuleEnable->videoProcessingModules.DS4PostCropEnable = m_moduleEnable;
                }
                break;

            case DS16Output:
                pCropModuleEnable->videoProcessingModules.DS16RoundClampLumaCropEnable     = m_moduleEnable;
                pCropModuleEnable->videoProcessingModules.DS16RoundClampChromaCropEnable   = m_moduleEnable;
                pCropModuleEnable->videoProcessingModules.DS16LumaEnable = m_moduleEnable;
                pCropModuleEnable->videoProcessingModules.DS16ChromaEnable = m_moduleEnable;

                // PostCrop is disabled if PreCrop is enabled
                if (CropTypeCentered == pInputData->pStripeConfig->cropType)
                {
                    if (0 == pCropModuleEnable->videoProcessingModules.DS16PreCropEnable)
                    {
                        pCropModuleEnable->videoProcessingModules.DS16PostCropEnable = m_moduleEnable;
                    }
                }
                else
                {
                    pCropModuleEnable->videoProcessingModules.DS16PostCropEnable = m_moduleEnable;
                }
                break;

            case DisplayFullOutput:
                pCropModuleEnable->displayProcessingModules.RoundClampLumaCropEnable     = m_moduleEnable;
                pCropModuleEnable->displayProcessingModules.RoundClampChromaCropEnable   = m_moduleEnable;
                pCropModuleEnable->displayProcessingModules.CropEnable = m_moduleEnable;
                break;

            case DisplayDS4Output:
                pCropModuleEnable->displayProcessingModules.DS4RoundClampLumaCropEnable     = m_moduleEnable;
                pCropModuleEnable->displayProcessingModules.DS4RoundClampChromaCropEnable   = m_moduleEnable;

                // PostCrop is disabled if PreCrop is enabled
                if (CropTypeCentered == pInputData->pStripeConfig->cropType)
                {
                    if (0 == pCropModuleEnable->displayProcessingModules.DS4PreCropEnable)
                    {
                        pCropModuleEnable->displayProcessingModules.DS4PostCropEnable = m_moduleEnable;
                    }
                }
                else
                {
                    pCropModuleEnable->displayProcessingModules.DS4PostCropEnable = m_moduleEnable;
                }
                break;

            case DisplayDS16Output:
                pCropModuleEnable->displayProcessingModules.DS16RoundClampLumaCropEnable     = m_moduleEnable;
                pCropModuleEnable->displayProcessingModules.DS16RoundClampChromaCropEnable   = m_moduleEnable;

                // PostCrop is disabled if PreCrop is enabled
                if (CropTypeCentered == pInputData->pStripeConfig->cropType)
                {
                    if (0 == pCropModuleEnable->displayProcessingModules.DS16PreCropEnable)
                    {
                        pCropModuleEnable->displayProcessingModules.DS16PostCropEnable = m_moduleEnable;
                    }
                }
                else
                {
                    pCropModuleEnable->displayProcessingModules.DS16PostCropEnable = m_moduleEnable;
                }
                break;

            case PixelRawOutput:
                pCropModuleEnable->IQModules.pixelRawPathEnable = m_moduleEnable;
                break;

            default:
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid format %d", m_output);
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECrop11::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop11::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(static_cast<UINT>(m_modulePath) < IFEMaxNonCommonPaths);
    if (NULL != pInputData)
    {
        m_pState = &pInputData->pStripeConfig->stateCrop[static_cast<UINT>(m_modulePath)];

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
// IFECrop11::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECrop11::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(static_cast<UINT>(m_modulePath) < IFEMaxNonCommonPaths);

    if (NULL != pInputData)
    {
        m_pState = &pInputData->pStripeConfig->stateCrop[static_cast<UINT>(m_modulePath)];

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
// IFECrop11::IFECrop11
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECrop11::IFECrop11(
    IFEModuleCreateData* pCreateData)
{
    m_type              = ISPIQModuleType::IFECrop;
    m_moduleEnable      = TRUE;
    m_32bitDMILength    = 0;
    m_64bitDMILength    = 0;
    m_modulePath        = pCreateData->pipelineData.IFEPath;
    m_output            = FullOutput;

    switch (m_modulePath)
    {
        case IFEPipelinePath::FDPath:
            m_output = FDOutput;
            break;

        case IFEPipelinePath::VideoFullPath:
            m_output = FullOutput;
            break;

        case IFEPipelinePath::VideoDS4Path:
            m_output = DS4Output;
            break;

        case IFEPipelinePath::VideoDS16Path:
            m_output = DS16Output;
            break;

        case IFEPipelinePath::PixelRawDumpPath:
            m_output = PixelRawOutput;
            break;

        case IFEPipelinePath::DisplayFullPath:
            m_output = DisplayFullOutput;
            break;

        case IFEPipelinePath::DisplayDS4Path:
            m_output = DisplayDS4Output;
            break;

        case IFEPipelinePath::DisplayDS16Path:
            m_output = DisplayDS16Output;
            break;

        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid path");
            break;
    }

    pCreateData->initializationData.pStripeConfig->stateCrop[static_cast<UINT>(m_modulePath)].cropWindow.left   = 1;
    pCreateData->initializationData.pStripeConfig->stateCrop[static_cast<UINT>(m_modulePath)].cropWindow.top    = 1;
    pCreateData->initializationData.pStripeConfig->stateCrop[static_cast<UINT>(m_modulePath)].cropWindow.width  = 1;
    pCreateData->initializationData.pStripeConfig->stateCrop[static_cast<UINT>(m_modulePath)].cropWindow.height = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECrop11::~IFECrop11
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECrop11::~IFECrop11()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
