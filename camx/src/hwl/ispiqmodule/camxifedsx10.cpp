////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedsx10.cpp
/// @brief CAMXIFEDSX10 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifedsx10titan480.h"
#include "camxifedsx10.h"
#include "camxiqinterface.h"

#include "camxnode.h"
#include "camxtuningdatamanager.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEDSX10* pModule = CAMX_NEW IFEDSX10(pCreateData);
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
            result = CamxResultENoMemory;
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create IFEDS410 object.");
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
// IFEDSX10::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEDSX10Titan480;
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
        m_32bitDMILength            = m_pHWSetting->Get32bitDMILength();
        result                      = AllocateCommonLibraryData();
        m_pOutputData               = CAMX_CALLOC(sizeof(DSX10OutputData));
        if (NULL == m_pOutputData)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting     = NULL;
            m_cmdLength      = 0;
            m_32bitDMILength = 0;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, no memory");
        result      = CamxResultENoMemory;
        m_cmdLength = 0;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(static_cast<UINT>(m_modulePath) < IFEMaxNonCommonPaths);

    if ((NULL != pInputData)                  &&
        (NULL != m_pHWSetting)                &&
        (NULL != pInputData->pCmdBuffer)      &&
        (NULL != pInputData->p32bitDMIBuffer) &&
        (NULL != pInputData->p32bitDMIBufferAddr))
    {

        m_pCropState = &pInputData->pStripeConfig->stateCrop[static_cast<UINT>(m_output)];
        m_pState     = &pInputData->pStripeConfig->stateDS[static_cast<UINT>(m_modulePath)];

        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);
        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                if (TRUE == pInputData->bankUpdate.isValid)
                {
                    m_pState->bankSelect = pInputData->bankUpdate.DSXYBank;
                }
                result = RunCalculation(pInputData);
                if (CamxResultSuccess == result)
                {
                    VOID* pSettingData = static_cast<VOID*>(pInputData);
                    result             = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);

                    m_pState->bankSelect ^= 1;
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
// IFEDSX10::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* IFEDSX10::GetRegCmd()
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult       result       = CamxResultSuccess;

    CalculateScalerOutput();
    m_dependenceData.inW     = m_pState->DSXInput.width;
    m_dependenceData.inH     = m_pState->DSXInput.height;
    m_dependenceData.outW    = m_pState->DSXOutput.width;
    m_dependenceData.outH    = m_pState->DSXOutput.height;
    m_dependenceData.offsetX = 0;
    m_dependenceData.offsetY = 0;

    if (FALSE == pInputData->isPrepareStripeInputContext)
    {
        m_dependenceData.offsetX = m_pState->preCropInfo.YCrop.firstPixel;
        m_dependenceData.offsetY = m_pState->preCropInfo.YCrop.firstLine;
    }

    m_dependenceData.scaleRatio = m_pState->DSXscaleFactor;
    m_dependenceData.downscale = m_dependenceData.scaleRatio;

    m_dependenceData.isPrepareStripeInputContext = pInputData->isPrepareStripeInputContext;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "scaleRatio %f DSXscaleFactor %f inWxinH =%dx%d  outW x outH %dx %d ",
        m_dependenceData.scaleRatio,
        m_pState->DSXscaleFactor,
        m_dependenceData.inW,
        m_dependenceData.inH,
        m_dependenceData.outW,
        m_dependenceData.outH);

    UINT32*          pDMIAddr    = pInputData->p32bitDMIBufferAddr;
    DSX10OutputData* pOutputData = static_cast<DSX10OutputData*>(m_pOutputData);
    Utils::Memset(pOutputData, 0, sizeof(DSX10OutputData));

    pDMIAddr += m_32bitDMIBufferOffsetDword + (pInputData->pStripeConfig->stripeId * m_32bitDMILength);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "pDMIAddr %p m_32bitDMILength %d stripeId %d m_32bitDMIBufferOffsetDword %d",
        pDMIAddr,
        m_32bitDMILength,
        pInputData->pStripeConfig->stripeId,
        m_32bitDMIBufferOffsetDword);

    pOutputData->pLumaKernelWeightsHoriz = reinterpret_cast<UINT64*>(reinterpret_cast<UCHAR*>(pDMIAddr));
    pOutputData->pLumaKernelWeightsVert  = reinterpret_cast<UINT64*>(
            (reinterpret_cast<UCHAR*>(pOutputData->pLumaKernelWeightsHoriz) + IFEDSX10LumaHorDMISize));
    pOutputData->pChromaKernelWeightsHoriz = reinterpret_cast<UINT64*>(
            (reinterpret_cast<UCHAR*>(pOutputData->pLumaKernelWeightsVert) + IFEDSX10LumaVerDMISize));
    pOutputData->pChromaKernelWeightsVert  = reinterpret_cast<UINT64*>(
            (reinterpret_cast<UCHAR*>(pOutputData->pChromaKernelWeightsHoriz) + IFEDSX10ChromaHorDMISize));

    pOutputData->pState = m_pState;
    result = IQInterface::DSX10CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, pOutputData);

    if (CamxResultSuccess != result)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupISP, "DSX10 Calculation Failed. result %d", result);
    }

    if (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex))
    {
        m_pHWSetting->DumpRegConfig();
        m_pHWSetting->DumpDMIData(pOutputData);
    }

    return result;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::PrepareStripeRunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10::PrepareStripeRunCalculation(
    ISPInputData* pInputData)
{
    CamxResult  result = CamxResultSuccess;

    CalculateScalerOutput();

    m_dependenceData.offsetX = 0;
    m_dependenceData.offsetY = 0;

    m_dependenceData.isPrepareStripeInputContext = pInputData->isPrepareStripeInputContext;

    // Input size after MNDS;
    m_stripeInput.lumaInputImageWidth   = m_pState->DSXInput.width;
    m_stripeInput.chromaInputImageWidth = m_pState->DSXInput.width/2;

    // From HPG, output image width for Chroma(should be half that of Luma)

    m_stripeInput.lumaOutputImageWidth   = m_pState->DSXOutput.width;
    m_stripeInput.chromaOutputImageWidth = m_pState->DSXOutput.width / 2;

    // step calculation
    m_stripeInput.lumaScaleRatio   = IQSettingUtils::FloatToQNumber(m_pState->DSXscaleFactor, DSX_COORD_FRAC_BITS);
    m_stripeInput.chromaScaleRatio = IQSettingUtils::FloatToQNumber(m_pState->DSXscaleFactor, DSX_COORD_FRAC_BITS);

    // The left responsible for initial phase required for correct alignment even when there is no crop.
    // The start* parameters are responsible for cropping.
    // So, when there is no desired cropping - the initial location should consist only of initial phase

    INT32 left          = m_stripeInput.lumaScaleRatio / 2 - (1 << (DSX_COORD_FRAC_BITS - 1));
    INT32 startX_Chroma = IQSettingUtils::FloatToQNumber(m_dependenceData.offsetX, DSX_COORD_FRAC_BITS -1);
    INT32 startX_Luma   = startX_Chroma * 2;

    m_stripeInput.lumaStartingLocation   = left + startX_Luma;
    m_stripeInput.chromaStartingLocation = left + startX_Chroma;

    return result;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDSX10::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    IFEScalerOutput* pCalculatedScalerOutput    = &pInputData->pCalculatedData->scalerOutput[0];

    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        // Post Crop module on DS4 path needs to know DS4 output dimension
        pCalculatedScalerOutput[DS4Output].dimension.width  = m_pState->DSXOutput.width;
        pCalculatedScalerOutput[DS4Output].dimension.height = m_pState->DSXOutput.height;
        pCalculatedScalerOutput[DS4Output].scalingFactor    =
            pCalculatedScalerOutput[FullOutput].scalingFactor * m_pState->DSXscaleFactor;
        pInputData->pCalculatedData->isVideoDS4Enable       = TRUE;
        pCalculatedScalerOutput[DS4Output].preCropEnable    = TRUE;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEDSX10::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL                    result                   = FALSE;
    StreamDimension*        pHALStream               = &pInputData->pStripeConfig->stream[0];
    IFEScalerOutput*        pCalculatedScalerOutput  = &pInputData->pCalculatedData->scalerOutput[0];
    TuningDataManager*      pTuningManager           = pInputData->pTuningDataManager;
    CropWindow*             pInputCropWindow         = &pInputData->pStripeConfig->HALCrop[FullOutput];
    ISPSensorConfigureData* pSensorData              = &pInputData->sensorData;
    DS4PreCropInfo*         pPreCropInfo             = &pInputData->pCalculatedData->preCropInfo;

    CAMX_ASSERT(NULL != pTuningManager);

    // If the HAL Stream width and height are not same to Stripe Config, re-stored the width & height here.
    if ((IFEPipelinePath::VideoDS4Path                              == m_modulePath)          &&
        ((pCalculatedScalerOutput[FullOutput].dimension.width  != m_pState->MNDSOutput.width)             ||
         (pCalculatedScalerOutput[FullOutput].dimension.height != m_pState->MNDSOutput.height)            ||
         (pHALStream[DS4Output].width                          != m_pState->DS4PathOutput.width)          ||
         (pHALStream[DS4Output].height                         != m_pState->DS4PathOutput.height)         ||
         (pPreCropInfo->YCrop.firstPixel                       != m_pState->preCropInfo.YCrop.firstPixel) ||
         (pPreCropInfo->YCrop.firstLine                        != m_pState->preCropInfo.YCrop.firstLine)  ||
         (pPreCropInfo->YCrop.lastPixel                        != m_pState->preCropInfo.YCrop.lastPixel)  ||
         (pPreCropInfo->YCrop.lastLine                         != m_pState->preCropInfo.YCrop.lastLine)   ||
         (pInputData->forceTriggerUpdate                       == TRUE) ))
    {
        if ((TRUE == pTuningManager->IsValidChromatix()) &&
            (NULL != pInputData->pTuningData))
        {

            m_dependenceData.pChromatix = pTuningManager->GetChromatix()->GetModule_dsx10_ife_video_full_dc4(
                reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                pInputData->pTuningData->noOfSelectionParameter);

            m_dependenceData.pDS4to1Chromatix = pTuningManager->GetChromatix()->GetModule_ds4to1v11_ife_video_dc4_dc16(
                reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                pInputData->pTuningData->noOfSelectionParameter);
        }

        /// IQInterface::s_interpolationTable.DSX10TriggerUpdate(&pInputData->triggerData,&m_dependenceData);

        // Cache input parameters
        m_pState->cropWindow            = *pInputCropWindow;
        m_pState->sensorWidth           = pSensorData->sensorOut.width;
        m_pState->sensorHeight          = pSensorData->sensorOut.height;
        m_pState->DS4PathOutput.width   = pHALStream[DS4Output].width;
        m_pState->DS4PathOutput.height  = pHALStream[DS4Output].height;

        // Update DSx input config
        DS4PreCropInfo* pPreCropInfo    = &pInputData->pCalculatedData->preCropInfo;
        m_pState->preCropInfo           = *pPreCropInfo;

        // MNDS output config
        m_pState->MNDSOutput.width  = pCalculatedScalerOutput[FullOutput].dimension.width;
        m_pState->MNDSOutput.height = pCalculatedScalerOutput[FullOutput].dimension.height;

        // Width/height with respect to 1st pixel/line
        m_pState->DSXInput.width    = pPreCropInfo->YCrop.lastPixel + 1;
        m_pState->DSXInput.height   = pPreCropInfo->YCrop.lastLine + 1;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "DSX(Full) pre Crop[%d, %d, %d, %d] Crop Dim [%d, %d], MNDS dim [%d * %d]",
                       pPreCropInfo->YCrop.firstPixel,
                       pPreCropInfo->YCrop.firstLine,
                       pPreCropInfo->YCrop.lastPixel,
                       pPreCropInfo->YCrop.lastLine,
                       m_pState->DSXInput.width,
                       m_pState->DSXInput.height,
                       m_pState->MNDSOutput.width,
                       m_pState->DSXInput.height);

        if (TRUE == pInputData->pStripeConfig->overwriteStripes)
        {
            ISPInternalData* pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
            if (TRUE == pFrameLevel->scalerDependencyChanged[DS4Output])
            {
                m_moduleEnable = TRUE;
                result         = TRUE;
            }

            m_pState->overWriteStripes  = pInputData->pStripeConfig->overwriteStripes;
        }
        // Need to update fullsize for prepareStriping config
        else if (TRUE == pInputData->isPrepareStripeInputContext)
        {
            m_pState->DSXInput.width    = pCalculatedScalerOutput[FullOutput].dimension.width;
            m_pState->DSXInput.height   = pCalculatedScalerOutput[FullOutput].dimension.height;
        }

        m_pState->cropType = pInputData->pStripeConfig->cropType;
        CAMX_LOG_INFO(CamxLogGroupISP, "output MNDS Output [%d %d], DS4PathOutput [%d, %d]",
            m_pState->MNDSOutput.width,
            m_pState->MNDSOutput.height,
            m_pState->DS4PathOutput.width,
            m_pState->DS4PathOutput.height);

        m_pState->moduleFlags.isDS4PathEnable                         = TRUE;
        pInputData->pCalculatedData->cropDependencyChanged[DS4Output] = TRUE;

        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::CalculateScalerOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDSX10::CalculateScalerOutput()
{
    UINT32 downScalingFactor = 0;
    UINT32 cropScaleFactor   = 0;
    UINT32 outputWidth       = 0;
    UINT32 outputHeight      = 0;
    FLOAT  inAspectRatio     = 0.f;
    FLOAT  outAspectRatio    = 0.f;
    INT32 inputWidth        = (m_pState->preCropInfo.YCrop.lastPixel - m_pState->preCropInfo.YCrop.firstPixel) + 1;
    INT32 inputHeight       = (m_pState->preCropInfo.YCrop.lastLine - m_pState->preCropInfo.YCrop.firstLine) + 1;

    m_pState->DSXscaleFactor   = DS4Factor;
    m_pState->DSXOutput.width  = Utils::AlignGeneric32(inputWidth, DS4Factor * 2) / DS4Factor;
    m_pState->DSXOutput.height = Utils::AlignGeneric32(inputHeight, DS4Factor * 2) / DS4Factor;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "DSXscaleFactor %f ", m_pState->DSXscaleFactor);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    CamxResult       result                  = CamxResultSuccess;
    IFEScalerOutput* pCalculatedScalerOutput = &pInputData->pCalculatedData->scalerOutput[0];

    // Full path MNDS output Validation for DS4  path
    if ((IFEPipelinePath::VideoDS4Path == m_modulePath)                                   &&
        (0 != pInputData->pStripeConfig->stream[DS4Output].width)                         &&
        (0 != pInputData->pStripeConfig->stream[DS4Output].height)                        &&
        ((0                       == pCalculatedScalerOutput[FullOutput].dimension.width) ||
         (0                       == pCalculatedScalerOutput[FullOutput].dimension.height)))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Video path MNDS ouput for DSX path stripe [%d x %d] pcalc [%d %d ]",
            pInputData->pStripeConfig->stream[DS4Output].width,
            pInputData->pStripeConfig->stream[DS4Output].height,
            pCalculatedScalerOutput[FullOutput].dimension.width,
            pCalculatedScalerOutput[FullOutput].dimension.height);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10::PrepareStripingParameters(
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

            m_dependenceData.offsetX = 0;
            m_dependenceData.offsetY = 0;

            // Check if dependent is valid and been updated compared to last request
            result = ValidateDependenceParams(pInputData);
            if (CamxResultSuccess == result)
            {
                if (TRUE == CheckDependenceChange(pInputData))
                {
                    result = PrepareStripeRunCalculation(pInputData);
                }

                if (NULL != pInputData->pStripingInput)
                {
                    pInputData->pStripingInput->stripingInput.DSXInputVideoFullv10.lumaStartLocationX   =
                        m_stripeInput.lumaStartingLocation;
                    pInputData->pStripingInput->stripingInput.DSXInputVideoFullv10.lumaInputWidth       =
                        m_stripeInput.lumaInputImageWidth;
                    pInputData->pStripingInput->stripingInput.DSXInputVideoFullv10.lumaOutWidth         =
                        m_stripeInput.lumaOutputImageWidth;
                    pInputData->pStripingInput->stripingInput.DSXInputVideoFullv10.lumaScaleRatioX      =
                        m_stripeInput.lumaScaleRatio;
                    pInputData->pStripingInput->stripingInput.DSXInputVideoFullv10.chromaStratLocationX =
                        m_stripeInput.chromaStartingLocation;
                    pInputData->pStripingInput->stripingInput.DSXInputVideoFullv10.chromaInputWidth     =
                        m_stripeInput.chromaInputImageWidth;
                    pInputData->pStripingInput->stripingInput.DSXInputVideoFullv10.chromaOutWidth       =
                        m_stripeInput.chromaOutputImageWidth;
                    pInputData->pStripingInput->stripingInput.DSXInputVideoFullv10.chromaScaleRatioX    =
                        m_stripeInput.chromaScaleRatio;
                    pInputData->pStripingInput->stripingInput.videofullDSXoutWidth                      =
                        m_pState->DSXOutput.width;
                    pInputData->pStripingInput->stripingInput.videofullDSXoutHeight                     =
                        m_pState->DSXOutput.height;

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
// IFEDSX10::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;
    DSXNcLibOutputData data;
    UINT               interpolationSize;

    result = IQInterface::IFEDSX10GetInitializationData(&data);

    if ((NULL == m_dependenceData.pUnpackedfield) && (CamxResultSuccess == result))
    {
        m_dependenceData.pUnpackedfield = CAMX_CALLOC(data.DSX10OutputDataSize);
        if (NULL == m_dependenceData.pUnpackedfield)
        {
            result = CamxResultENoMemory;
        }
    }

    if ((NULL == m_dependenceData.pDSXChromatix) && (CamxResultSuccess == result))
    {
        m_dependenceData.pDSXChromatix = CAMX_CALLOC(data.DSX10ChromatixSize);
        if (NULL == m_dependenceData.pDSXChromatix)
        {
            result = CamxResultENoMemory;
        }
    }

    interpolationSize = (sizeof(dsx_1_0_0::dsx10_rgn_dataType) * (DSX10MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for gtm_1_0_0::gtm10_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDSX10::DeallocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_dependenceData.pUnpackedfield)
    {
        CAMX_FREE(m_dependenceData.pUnpackedfield);
        m_dependenceData.pUnpackedfield = NULL;
    }

    if (NULL != m_dependenceData.pDSXChromatix)
    {
        CAMX_FREE(m_dependenceData.pDSXChromatix);
        m_dependenceData.pDSXChromatix = NULL;
    }

    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDSX10::IFEDSX10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDSX10::IFEDSX10(
    IFEModuleCreateData* pCreateData)
{
    m_type           = ISPIQModuleType::IFEDS4;
    m_moduleEnable   = TRUE;
    m_32bitDMILength = 0;
    m_64bitDMILength = 0;
    m_modulePath     = pCreateData->pipelineData.IFEPath;
    switch (m_modulePath)
    {
        case IFEPipelinePath::VideoDS4Path:
            m_output = DS4Output;
            break;
        default:
            m_output = DS4Output;
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEDSX10::~IFEDSX10
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDSX10::~IFEDSX10()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }

    if (NULL != m_pOutputData)
    {
        CAMX_FREE(m_pOutputData);
        m_pOutputData = NULL;
    }
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
