////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifemnds16.cpp
/// @brief CAMXIFEMNDS16 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifemnds16titan17x.h"
#include "camxifemnds16.h"
#include "camxhal3module.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN
static const UINT32 ScaleRatioLimit = 105;    ///< Scaling ratio limit beyond which artificats are seen


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEMNDS16* pModule = CAMX_NEW IFEMNDS16(pCreateData);

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEMNDS16 object.");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFEMNDS16 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEMNDS16Titan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength = m_pHWSetting->GetCommandLength();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        m_cmdLength = 0;
        result      = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16::CalculateScalerOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS16::CalculateScalerOutput(
    ISPInputData* pInputData)
{
    UINT32 downScalingFactor;
    UINT32 cropScaleFactor;
    UINT32 outputWidth;
    UINT32 outputHeight;
    FLOAT  inAspectRatio;
    FLOAT  outAspectRatio;
    UINT32 maxOutputWidth;

    CAMX_ASSERT_MESSAGE(0 != m_pState->cropWindow.height, "Invalid Crop height");
    CAMX_ASSERT_MESSAGE(0 != m_pState->streamDimension.height, "Invalid output height");
    CAMX_ASSERT_MESSAGE(0 != m_pState->inputHeight, "Invalid input height");

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    // Calculate the scaling factor, based on the crop window and Sensor output.
    // Every time the crop_window decreases the crop_factor increases (Scalar output increases)
    const UINT32 cropWidthScaleFactor   = (m_pState->inputWidth * Q12) / m_pState->cropWindow.width;
    const UINT32 cropheightScaleFactor  = (m_pState->inputHeight * Q12) / m_pState->cropWindow.height;

    // Calculate scaler input and IFE output aspect ratio, to calculate scaler output
    outAspectRatio = static_cast<FLOAT>(m_pState->streamDimension.width) / m_pState->streamDimension.height;
    inAspectRatio  = static_cast<FLOAT>(m_pState->inputWidth) / m_pState->inputHeight;

    // 1. Calculate Scaler output based on IFE output based input aspect ratio.
    if (inAspectRatio == outAspectRatio)
    {
        cropScaleFactor = Utils::MinUINT32(cropWidthScaleFactor, cropheightScaleFactor);
        outputWidth     = (m_pState->streamDimension.width * cropScaleFactor) / Q12;
        outputHeight    = (m_pState->streamDimension.height * cropScaleFactor) / Q12;
    }
    else if (inAspectRatio < outAspectRatio)
    {
        cropScaleFactor = cropWidthScaleFactor;
        outputWidth     = (m_pState->streamDimension.width * cropScaleFactor) / Q12;
        outputHeight    = (outputWidth * m_pState->inputHeight) / m_pState->inputWidth;
    }
    else
    {
        cropScaleFactor = cropheightScaleFactor;
        outputHeight    = (m_pState->streamDimension.height * cropScaleFactor) / Q12;
        outputWidth     = (outputHeight * m_pState->inputWidth) / m_pState->inputHeight;
    }

    // 2. Cannot do upscale, HW limitation
    if ((outputWidth > m_pState->inputWidth) || (outputHeight > m_pState->inputHeight))
    {
        outputWidth     = m_pState->inputWidth;
        outputHeight    = m_pState->inputHeight;
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Path %d[0 - FD, 1 Full], output dimension > input, set it to input dim [%d * %d]\n",
            m_output,
            outputWidth,
            outputHeight);
    }
    // 3. Check scale factor limitation, Output dimension can not be scaled beyond scaler capability
    if (FDOutput == m_output)
    {
        UINT32 maxFDScalerWidth = pInputData->maxOutputWidthFD * 2;
        if (IFEModuleMode::SingleIFENormal == pInputData->pipelineIFEData.moduleMode)
        {
            maxFDScalerWidth = pInputData->maxOutputWidthFD;
        }
        if (outputWidth > maxFDScalerWidth)
        {
            outputWidth = maxFDScalerWidth;
            outputHeight  = (outputWidth * m_pState->inputHeight) / m_pState->inputWidth;
            if (outputHeight < m_pState->streamDimension.height)
            {
                outputHeight= m_pState->streamDimension.height;
            }
        }
    }

    if ((outputWidth * MNDS16MaxScaleFactor) < m_pState->inputWidth)
    {
        outputWidth     = (m_pState->inputWidth + MNDS16MaxScaleFactor - 1) / MNDS16MaxScaleFactor;
        outputHeight    = (outputWidth * m_pState->inputHeight) / m_pState->inputWidth;
    }
    if ((outputHeight * MNDS16MaxScaleFactor) < m_pState->inputHeight)
    {
        outputHeight    = (m_pState->inputHeight + MNDS16MaxScaleFactor - 1) / MNDS16MaxScaleFactor;
        outputWidth     = (outputHeight * m_pState->inputWidth) / m_pState->inputHeight;
    }

    // 4. Check scale ratio limitation, Scaling between 105 to 100 could introduce artifacts
    downScalingFactor = (m_pState->inputWidth * 100) / outputWidth;

    m_pState->MNDSOutput.scalingLimitHit = FALSE;

    if ((downScalingFactor < ScaleRatioLimit) && (downScalingFactor >= 100))
    {
        if (Q12 != cropScaleFactor)
        {
            outputWidth  = (m_pState->inputWidth * 100) / ScaleRatioLimit;
            outputHeight = (m_pState->inputHeight * 100) / ScaleRatioLimit;

            if ((outputWidth  < m_pState->streamDimension.width) ||
                 (outputHeight < m_pState->streamDimension.height))
            {
                outputWidth     = m_pState->inputWidth;
                outputHeight    = m_pState->inputHeight;
            }

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Path %d[0-FD,1-Full], Scale ration 100-105, Dimension [%d * %d]\n",
                m_output,
                outputWidth,
                outputHeight);
        }
        else
        {
            outputWidth     = m_pState->inputWidth;
            outputHeight    = m_pState->inputHeight;
        }

        m_pState->MNDSOutput.scalingLimitHit = TRUE;
    }
    if (TRUE == pStaticSettings->capResolutionForSingleIFE)
    {
        maxOutputWidth = outputWidth;
        switch (m_modulePath)
        {
            case IFEPipelinePath::FDPath:
                maxOutputWidth = pInputData->maxOutputWidthFD;
                break;

            case IFEPipelinePath::VideoFullPath:
                maxOutputWidth = IFEMaxOutputWidthFull;
                break;

            default:
                maxOutputWidth = outputWidth;
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid path");
                break;
        }

        if (outputWidth > maxOutputWidth)
        {
            outputWidth = maxOutputWidth;
            outputHeight = m_pState->inputHeight * maxOutputWidth / m_pState->inputWidth;
        }
    }
    // 5. make sure its even number so CbCr will match Y instead of rounding down
    m_pState->MNDSOutput.dimension.height   = Utils::EvenFloorUINT32(outputHeight);
    m_pState->MNDSOutput.dimension.width    = Utils::EvenFloorUINT32(outputWidth);


    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Path %d[0 - FD, 1 Full], cropType %d Scaler output[%d * %d]\n",
                     m_output, pInputData->pStripeConfig->cropType,
                     outputWidth,
                     outputHeight);

    m_pState->MNDSOutput.input.width    = m_pState->inputWidth;
    m_pState->MNDSOutput.input.height   = m_pState->inputHeight;

    // Only if this is left or right stripe processing calculate MNDS padding to handle boundary pixels. This is currently
    // used in dual IFE, but single IFE mode also can take advantage of it if it becomes necessary.
    if (((CropTypeFromLeft == pInputData->pStripeConfig->cropType) ||
        (CropTypeFromRight == pInputData->pStripeConfig->cropType)) &&
        (FALSE == m_pInputData->pStripeConfig->overwriteStripes))
    {
        ISPInternalData*    pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
        UINT                scaleM      = pFrameLevel->scalerOutput[m_output].dimension.width;
        UINT                scaleN      = pFrameLevel->scalerOutput[m_output].input.width;
        IFEStripeStripeROI  input_ROI   = {0, 0};
        IFEStripeStripeROI  output_ROI  = {0, 0};

        if ((m_pState->MNDSOutput.dimension.width * static_cast<FLOAT>(scaleN) /
                scaleM) > static_cast<FLOAT>(m_pState->inputWidth))
        {
            m_pState->MNDSOutput.dimension.width =
                static_cast<UINT>(m_pState->inputWidth * static_cast<FLOAT>(scaleM) / scaleN);
        }
        m_pState->MNDSOutput.dimension.width = Utils::EvenFloorUINT32(m_pState->MNDSOutput.dimension.width);

        if (CropTypeFromRight == pInputData->pStripeConfig->cropType)
        {
            input_ROI.startX   = static_cast<uint16_t>(scaleN - m_pState->inputWidth);
            input_ROI.endX     = static_cast<uint16_t>(scaleN - 1);
            output_ROI.startX  = static_cast<uint16_t>(scaleM - m_pState->MNDSOutput.dimension.width);
            output_ROI.endX    = static_cast<uint16_t>(scaleM - 1);
        }
        else if (CropTypeFromLeft == pInputData->pStripeConfig->cropType)
        {
            input_ROI.startX   = 0;
            input_ROI.endX     = static_cast<uint16_t>(m_pState->inputWidth - 1);
            output_ROI.startX  = 0;
            output_ROI.endX    = static_cast<uint16_t>(m_pState->MNDSOutput.dimension.width - 1);
        }

        // MNScaleDownInStruct_V16_1D* pMNDSOut = NULL;
        if (FullOutput == m_output)
        {
            // pMNDSOut = &pInputData->pStripeConfig->pStripeOutput->mndsConfigVideoFullLuma;
            m_pState->MNDSOutput.version16.mnds_config_y = pInputData->pStripeConfig->pStripeOutput->mndsConfigVideoFullLuma;
            m_pState->MNDSOutput.version16.mnds_config_c = pInputData->pStripeConfig->pStripeOutput->mndsConfigVideoFullChroma;
        }
        else if (FDOutput == m_output)
        {
            // pMNDSOut = &pInputData->pStripeConfig->pStripeOutput->mndsConfigFDLuma;
            m_pState->MNDSOutput.version16.mnds_config_y = pInputData->pStripeConfig->pStripeOutput->mndsConfigFDLuma;
            m_pState->MNDSOutput.version16.mnds_config_c = pInputData->pStripeConfig->pStripeOutput->mndsConfigFDChroma;
        }
        else if (DisplayFullOutput == m_output)
        {
            // pMNDSOut = &pInputData->pStripeConfig->pStripeOutput->mndsConfigVideoFullLuma;
            m_pState->MNDSOutput.version16.mnds_config_y = pInputData->pStripeConfig->pStripeOutput->mndsConfigDispFullLuma;
            m_pState->MNDSOutput.version16.mnds_config_c = pInputData->pStripeConfig->pStripeOutput->mndsConfigDispFullChroma;
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid output path");
        }

        if (NULL != pInputData->pIFEStripeInterface)
        {
            pInputData->pIFEStripeInterface->IFEMNDSV16Config1d(static_cast<int16_t>(scaleN),
                                                                static_cast<int16_t>(scaleM),
                                                                0,
                                                                0,
                                                                input_ROI,
                                                                output_ROI,
                                                                &m_pState->MNDSOutput.version16.mnds_config_y,
                                                                &m_pState->MNDSOutput.version16.mnds_config_c);
        }

        if (outAspectRatio > inAspectRatio)
        {
            m_pState->MNDSOutput.scalingFactor = static_cast<FLOAT>(scaleN) / scaleM;
        }
        else
        {
            m_pState->MNDSOutput.scalingFactor = static_cast<FLOAT>(m_pState->inputHeight) /
                m_pState->MNDSOutput.dimension.height;
        }
    }
    else
    {
        if (outAspectRatio > inAspectRatio)
        {
            m_pState->MNDSOutput.scalingFactor = static_cast<FLOAT>(m_pState->inputWidth) /
                m_pState->MNDSOutput.dimension.width;
        }
        else
        {
            m_pState->MNDSOutput.scalingFactor = static_cast<FLOAT>(m_pState->inputHeight) /
                m_pState->MNDSOutput.dimension.height;
        }
    }

    // Only if this is left or right stripe processing and over write stripe then copy mnds values from stripe
    if (((CropTypeFromLeft == pInputData->pStripeConfig->cropType) ||
        (CropTypeFromRight == pInputData->pStripeConfig->cropType)) &&
        (TRUE == m_pInputData->pStripeConfig->overwriteStripes))
    {
        ISPInternalData* pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
        m_pState->MNDSOutput.dimension.height = pFrameLevel->scalerOutput[m_output].dimension.height;
        if (FullOutput == m_output)
        {
            // pMNDSOut = &pInputData->pStripeConfig->pStripeOutput->mndsConfigVideoFullLuma;
            m_pState->MNDSOutput.version16.mnds_config_y = pInputData->pStripeConfig->pStripeOutput->mndsConfigVideoFullLuma;
            m_pState->MNDSOutput.version16.mnds_config_c = pInputData->pStripeConfig->pStripeOutput->mndsConfigVideoFullChroma;
        }
        else if (FDOutput == m_output)
        {
            // pMNDSOut = &pInputData->pStripeConfig->pStripeOutput->mndsConfigFDLuma;
            m_pState->MNDSOutput.version16.mnds_config_y = pInputData->pStripeConfig->pStripeOutput->mndsConfigFDLuma;
            m_pState->MNDSOutput.version16.mnds_config_c = pInputData->pStripeConfig->pStripeOutput->mndsConfigFDChroma;
        }
        else if (DisplayFullOutput == m_output)
        {
            // pMNDSOut = &pInputData->pStripeConfig->pStripeOutput->mndsConfigVideoFullLuma;
            m_pState->MNDSOutput.version16.mnds_config_y = pInputData->pStripeConfig->pStripeOutput->mndsConfigDispFullLuma;
            m_pState->MNDSOutput.version16.mnds_config_c = pInputData->pStripeConfig->pStripeOutput->mndsConfigDispFullChroma;
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid output path");
        }
        if (outAspectRatio > inAspectRatio)
        {
            m_pState->MNDSOutput.scalingFactor = static_cast<FLOAT>(m_pState->MNDSOutput.version16.mnds_config_y.input) /
                m_pState->MNDSOutput.version16.mnds_config_y.output;
        }
        else
        {
            m_pState->MNDSOutput.scalingFactor = static_cast<FLOAT>(m_pState->inputHeight) /
                m_pState->MNDSOutput.dimension.height;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Path %d[0-FD,1-Full], CropType %d, MNDS output[%dx%d] input [%dx%d] fac %f",
                     m_output,
                     pInputData->pStripeConfig->cropType,
                     m_pState->MNDSOutput.dimension.width,
                     m_pState->MNDSOutput.dimension.height,
                     m_pState->inputWidth,
                     m_pState->inputHeight,
                     m_pState->MNDSOutput.scalingFactor);

    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.width, "Invalid MNDS output width");
    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.height, "Invalid MNDS output height");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    INT32       inputWidth;
    INT32       inputHeight;
    CamxResult  result           = CamxResultSuccess;
    CropInfo*   pSensorCAMIFCrop = &pInputData->pStripeConfig->CAMIFCrop;
    CropWindow* pInputCropWindow = &pInputData->pStripeConfig->HALCrop[m_output];

    inputWidth  = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
    inputHeight = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;

    // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
    if (TRUE == pInputData->sensorData.isYUV)
    {
        inputWidth >>= 1;
    }

    // Validate Crop window from HAL
    if (((pInputCropWindow->left + pInputCropWindow->width) > inputWidth)   ||
        ((pInputCropWindow->top + pInputCropWindow->height) >  inputHeight) ||
        (pSensorCAMIFCrop->lastLine  < pSensorCAMIFCrop->firstLine)         ||
        (pSensorCAMIFCrop->lastPixel < pSensorCAMIFCrop->firstPixel)        ||
        (0 == pInputCropWindow->width)                                      ||
        (0 == pInputCropWindow->height)                                     ||
        (0 == inputWidth)                                                   ||
        (0 == inputHeight))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Crop window %d, %d, %d, %d, input dimension %d * %d",
                       pInputCropWindow->left,
                       pInputCropWindow->top,
                       pInputCropWindow->width,
                       pInputCropWindow->height,
                       inputWidth, inputHeight);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFEMNDS16::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEMNDS16::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL                    result           = FALSE;
    CropInfo*               pSensorCAMIFCrop = &pInputData->pStripeConfig->CAMIFCrop;
    CropWindow*             pInputCropWindow = &pInputData->pStripeConfig->HALCrop[m_output];
    ISPHALConfigureData*    pHALData         = &pInputData->HALData;
    ISPSensorConfigureData* pSensorData      = &pInputData->sensorData;
    StreamDimension*        pHALStream       = &pInputData->pStripeConfig->stream[0];

    if ((pInputCropWindow->left       != m_pState->cropWindow.left)       ||
        (pInputCropWindow->top        != m_pState->cropWindow.top)        ||
        (pInputCropWindow->width      != m_pState->cropWindow.width)      ||
        (pInputCropWindow->height     != m_pState->cropWindow.height)     ||
        (pSensorCAMIFCrop->firstLine  != m_pState->CAMIFCrop.firstLine)   ||
        (pSensorCAMIFCrop->firstPixel != m_pState->CAMIFCrop.firstPixel)  ||
        (pSensorCAMIFCrop->lastLine   != m_pState->CAMIFCrop.lastLine)    ||
        (pSensorCAMIFCrop->lastPixel  != m_pState->CAMIFCrop.lastPixel)   ||
        (pHALStream[m_output].width   != m_pState->streamDimension.width) ||
        (pHALStream[m_output].height  != m_pState->streamDimension.height)||
        (pHALData->format[m_output]   != m_pixelFormat)                   ||
        (pSensorData->isBayer         != m_isBayer)                       ||
        (TRUE                         == pInputData->forceTriggerUpdate))
    {
        m_pState->inputWidth  = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
        m_pState->inputHeight = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;

        // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
        if (TRUE == pSensorData->isYUV)
        {
            m_pState->inputWidth >>= 1;
        }

        // Update module class variables with new data
        m_pState->cropWindow      = *pInputCropWindow;
        m_pState->streamDimension = pHALStream[m_output];
        m_pixelFormat     = pHALData->format[m_output];
        m_isBayer         = pSensorData->isBayer;
        m_pState->CAMIFCrop       = *pSensorCAMIFCrop;
        result = TRUE;

        if ((0 == m_pState->streamDimension.width) || (0 == m_pState->streamDimension.height))
        {
            m_moduleEnable = FALSE;
            result         = FALSE;
        }
        else
        {
            m_moduleEnable = TRUE;
        }
    }

    if (TRUE == pInputData->pStripeConfig->overwriteStripes)
    {
        ISPInternalData* pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
        if (TRUE == pFrameLevel->scalerDependencyChanged[m_output])
        {
            m_moduleEnable = TRUE;
            result = TRUE;
        }
    }

    pInputData->pCalculatedData->scalerDependencyChanged[m_output] = result;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    MNDSOutputData outputData;
    outputData.pMNDSState       = m_pState;
    outputData.ifeOutputPath    = m_output;
    outputData.pixelFormat      = m_pixelFormat;

    VOID* pSettingData = static_cast<VOID*>(pInputData);
    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        CalculateScalerOutput(pInputData);
        result = m_pHWSetting->PackIQRegisterSetting(pSettingData, &outputData);

        if ((CamxResultSuccess == result) &&
            (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod)))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS16::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    IFEScalerOutput*  pCalculatedScalerOutput = &pInputData->pCalculatedData->scalerOutput[0];

    // Update the upsteam modules with previous scaler output
    pCalculatedScalerOutput[m_output].dimension.width  = m_pState->MNDSOutput.dimension.width ;
    pCalculatedScalerOutput[m_output].dimension.height = m_pState->MNDSOutput.dimension.height;
    pCalculatedScalerOutput[m_output].scalingFactor    = m_pState->MNDSOutput.scalingFactor;
    pCalculatedScalerOutput[m_output].input.width      = m_pState->inputWidth;
    pCalculatedScalerOutput[m_output].input.height     = m_pState->inputHeight;
    pCalculatedScalerOutput[m_output].scalingLimitHit  = m_pState->MNDSOutput.scalingLimitHit;

    if (FDOutput == m_output)
    {
        pInputData->pCalculatedData->moduleEnable.FDprocessingModules.MNDSEnable = m_moduleEnable;
    }

    if (FullOutput == m_output)
    {
        pInputData->pCalculatedData->moduleEnable.videoProcessingModules.MNDSEnable = m_moduleEnable;
    }

    if (DisplayFullOutput == m_output)
    {
        pInputData->pCalculatedData->moduleEnable.displayProcessingModules.MNDSEnable = m_moduleEnable;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(static_cast<UINT>(m_modulePath) < IFEMaxNonCommonPaths);
    if (NULL != pInputData)
    {
        m_pState = &pInputData->pStripeConfig->stateMNDS[static_cast<UINT>(m_modulePath)];
    }
    m_pInputData = pInputData;

    if (NULL != pInputData)
    {
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
// IFEMNDS16::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result;

    CAMX_ASSERT(static_cast<UINT>(m_modulePath) < IFEMaxNonCommonPaths);

    if (NULL != pInputData)
    {
        m_pState     = &pInputData->pStripeConfig->stateMNDS[static_cast<UINT>(m_modulePath)];
        m_pInputData = pInputData;
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
// IFEMNDS16::IFEMNDS16
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEMNDS16::IFEMNDS16(
    IFEModuleCreateData* pCreateData)
{
    m_type              = ISPIQModuleType::IFEMNDS;
    m_moduleEnable      = TRUE;
    m_32bitDMILength    = 0;
    m_64bitDMILength    = 0;
    m_output            = FullOutput;
    m_modulePath        = pCreateData->pipelineData.IFEPath;

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

    pCreateData->initializationData.pStripeConfig->stateMNDS[static_cast<UINT>(m_modulePath)].cropWindow.left   = 1;
    pCreateData->initializationData.pStripeConfig->stateMNDS[static_cast<UINT>(m_modulePath)].cropWindow.top    = 1;
    pCreateData->initializationData.pStripeConfig->stateMNDS[static_cast<UINT>(m_modulePath)].cropWindow.width  = 1;
    pCreateData->initializationData.pStripeConfig->stateMNDS[static_cast<UINT>(m_modulePath)].cropWindow.height = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEMNDS16::~IFEMNDS16
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEMNDS16::~IFEMNDS16()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
