////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxpdafdata.cpp
/// @brief Implements PDAFData methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxpdafconfig.h"
#include "camxpropertyblob.h"
#include "camxmetadatapool.h"
#include "camxactuatordata.h"
#include "camximagesensormoduledata.h"
#include "camximagesensordata.h"
#include "chipdlibinterface.h"
#include "camxpdafdata.h"
#include "camxnode.h"
#include "camxtitan17xcontext.h"


CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::PDAFData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PDAFData::PDAFData(
    PDAFConfigurationData* pPDAFConfigData)
{
    if (NULL != pPDAFConfigData)
    {
        m_pPDAFConfigData = pPDAFConfigData;
        m_isPdafEnabled   = TRUE;
    }
    else
    {
        m_pPDAFConfigData = NULL;
        m_isPdafEnabled   = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::PDAFInit
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::PDAFInit(
    CHIPDLib*               pPDLib,
    UINT32                  resIdx,
    ActuatorData*           pActuatorData,
    ImageSensorData*        pSensorData,
    const EEPROMOTPData*    pOTPData,
    StatsCameraInfo*        pCameraInfo,
    VOID*                   pTuningManager,
    VOID*                   pCAMIFT3DataPattern,
    PDLibBufferFormat       bufferFormat,
    HwContext*              pHwContext,
    PDHWEnableConditions*   pPDHWEnableConditions,
    SensorCropInfo*         pSensorCrop,
    ChiTuningModeParameter* pChiTuningModeParameter)
{
    CamxResult           result               = CamxResultSuccess;
    PDLibDataBufferInfo* pT3DataPattern       = NULL;
    UINT32               PDAFMode             = 0;
    PDAFModeInformation* pPDAFModeInfo        = NULL;
    UINT16               actuatorDataBitwidth = 0;

    if (FALSE == m_isPdafEnabled)
    {
        // No need to log an error when PDAF data is not present
        // This indicates that PDAF is not supported in the sensor or is disabled
        CAMX_LOG_INFO(CamxLogGroupSensor, "PDAF Disabled");
        result = CamxResultEFailed;
    }

    if ((NULL != pPDLib)            &&
        (NULL != pActuatorData)     &&
        (NULL != pSensorData)       &&
        (NULL != pOTPData)          &&
        (NULL != m_pPDAFConfigData) &&
        (NULL != pSensorCrop)       &&
        (NULL != pChiTuningModeParameter))
    {
        PDLibCreateParams PDLibInitParams = {};
        Utils::Memset(&PDLibInitParams, 0, sizeof(PDLibCreateParams));

        PDLibInitParams.initParam.cameraInfo               = *pCameraInfo;

        // Initialize Sensor Mode specific crop info.
        StatsSensorModeCropInfo cropInfo    = {};
        cropInfo.firstLine                  = pSensorCrop->firstLine;
        cropInfo.firstPixel                 = pSensorCrop->firstPixel;
        cropInfo.lastLine                   = pSensorCrop->lastLine;
        cropInfo.lastPixel                  = pSensorCrop->lastPixel;

        PDLibInitParams.initParam.sensorCropInfo = cropInfo;

        // Initializing Tuning Mode Parameter for Sensor
        PDLibInitParams.tuningStats                        = { 0 };

        PDLibInitParams.tuningStats.pTuningSetManager      = pTuningManager;
        PDLibInitParams.tuningStats.numSelectors           = MaxTuningMode;
        PDLibInitParams.tuningStats.pTuningModeSelectors   =
            reinterpret_cast<TuningMode*>(&(pChiTuningModeParameter->TuningMode[0]));

        result = GetCurrentPDAFModeIndex(resIdx, &PDAFMode);

        if (CamxResultSuccess == result)
        {
            pPDAFModeInfo = &m_pPDAFConfigData->PDModeInfo[PDAFMode];
        }

        if (CamxResultSuccess == result)
        {
            result = GetPDAFOTPData(&PDLibInitParams, pOTPData, pPDAFModeInfo);
        }

        if (CamxResultSuccess == result)
        {
            result = GetPDAFActuatorData(pActuatorData, &PDLibInitParams, &actuatorDataBitwidth);
        }

        if (CamxResultSuccess == result)
        {
            result = GetPDAFSensorData(pSensorData, &PDLibInitParams, resIdx);
        }

        if ((CamxResultSuccess == result) && (NULL != pPDAFModeInfo) &&
           (TRUE == pPDAFModeInfo->PDSensorNativePatternInfoExists))
        {
            if (TRUE == pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPatternExists)
            {

                Utils::Memcpy(&PDLibInitParams.initParam.nativePatternInfo.patternInfo.blockPattern.blockDimension,
                             &pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPattern.PDBlockDimensions,
                             (sizeof(PDLibDimensionInfo)));

                result = FillPDAFPixelCoordinates(pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPattern,
                                                  &PDLibInitParams.initParam.nativePatternInfo.patternInfo.blockPattern);
                if (CamxResultSuccess == result)
                {
                    result = AdjustNativePDAFOffsets(&PDLibInitParams.initParam.nativePatternInfo.patternInfo.blockPattern);
                }
            }

            if (CamxResultSuccess == result)
            {
                PDLibInitParams.initParam.nativePatternInfo.horizontalDownscaleFactor        =
                    pPDAFModeInfo->PDSensorNativePatternInfo.PDDownscaleFactorHorizontal;

                PDLibInitParams.initParam.nativePatternInfo.verticalDownscaleFactor          =
                    pPDAFModeInfo->PDSensorNativePatternInfo.PDDownscaleFactorVertical;

                PDLibInitParams.initParam.nativePatternInfo.patternInfo.horizontalBlockCount =
                    pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockCountHorizontal;

                PDLibInitParams.initParam.nativePatternInfo.patternInfo.verticalBlockCount   =
                    pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockCountVertical;
            }

            PDLibInitParams.initParam.nativePatternInfo.cropRegion.x      =
                pPDAFModeInfo->PDSensorNativePatternInfo.PDCropRegion.xStart;

            PDLibInitParams.initParam.nativePatternInfo.cropRegion.y      =
                pPDAFModeInfo->PDSensorNativePatternInfo.PDCropRegion.yStart;

            PDLibInitParams.initParam.nativePatternInfo.cropRegion.width  =
                pPDAFModeInfo->PDSensorNativePatternInfo.PDCropRegion.width;

            PDLibInitParams.initParam.nativePatternInfo.cropRegion.height =
                pPDAFModeInfo->PDSensorNativePatternInfo.PDCropRegion.height;
        }

        if (CamxResultSuccess == result)
        {
            PDLibInitParams.initParam.defocusConfidenceThreshold           =
                m_pPDAFConfigData->PDCommonInfo.PDDefocusConfidenceThreshold;

            if ((TRUE == m_pPDAFConfigData->PDCommonInfo.PDBlackLevelExists) &&
                (0 < m_pPDAFConfigData->PDCommonInfo.PDBlackLevel))
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Overriding sensor black level with PD black level");
                PDLibInitParams.initParam.blackLevel = m_pPDAFConfigData->PDCommonInfo.PDBlackLevel;
            }

            PDLibInitParams.initParam.nativePatternInfo.orientation        =
                static_cast<PDLibSensorOrientation>(m_pPDAFConfigData->PDCommonInfo.PDOrientation);

            if (TRUE == pPDAFModeInfo->PDOffsetCorrectionExists)
            {
                PDLibInitParams.initParam.nativePatternInfo.PDOffsetCorrection =
                    pPDAFModeInfo->PDOffsetCorrection;
            }
            else
            {
                PDLibInitParams.initParam.nativePatternInfo.PDOffsetCorrection = 0;
            }

            if (TRUE == pPDAFModeInfo->lcrPDOffsetCorrectionExists)
            {
                PDLibInitParams.initParam.nativePatternInfo.lcrPDOffset =
                    pPDAFModeInfo->lcrPDOffsetCorrection;
            }
            PDLibInitParams.initParam.bufferDataInfo.lcrBufferData.bufferFormat = bufferFormat;

            switch(pPDAFModeInfo->PDType)
            {
                case PDAFType::PDType3:
                    pT3DataPattern = reinterpret_cast<PDLibDataBufferInfo*>(pCAMIFT3DataPattern);
                    if (NULL == pT3DataPattern)
                    {
                        CAMX_LOG_WARN(CamxLogGroupSensor, "PDAF Error: pT3DataPattern is NULL");
                        result = CamxResultEFailed;
                        break;
                    }

                    result = GetPDAFSensorType(pPDAFModeInfo->PDType,
                        static_cast<VOID*>(&PDLibInitParams.initParam.bufferDataInfo.sensorType));

                    if (CamxResultSuccess == result)
                    {
                        PDLibInitParams.initParam.bufferDataInfo.imageOverlap     = pT3DataPattern->imageOverlap;
                        PDLibInitParams.initParam.bufferDataInfo.bufferFormat     = pT3DataPattern->bufferFormat;
                        PDLibInitParams.initParam.bufferDataInfo.ispBufferHeight  = pT3DataPattern->ispBufferHeight;
                        PDLibInitParams.initParam.bufferDataInfo.isp1BufferStride = pT3DataPattern->isp1BufferStride;
                        PDLibInitParams.initParam.bufferDataInfo.isp1BufferWidth  = pT3DataPattern->isp1BufferWidth;
                        PDLibInitParams.initParam.bufferDataInfo.isp2BufferStride = pT3DataPattern->isp2BufferStride;
                        PDLibInitParams.initParam.bufferDataInfo.isp2BufferWidth  = pT3DataPattern->isp2BufferWidth;

                        PDLibInitParams.initParam.bufferDataInfo.isp1BlockPattern.verticalPDOffset      =
                            pT3DataPattern->isp1BlockPattern.verticalPDOffset;
                        PDLibInitParams.initParam.bufferDataInfo.isp1BlockPattern.horizontalPDOffset    =
                            pT3DataPattern->isp1BlockPattern.horizontalPDOffset;
                        PDLibInitParams.initParam.bufferDataInfo.isp1BlockPattern.pixelCount            =
                            pT3DataPattern->isp1BlockPattern.pixelCount;
                        PDLibInitParams.initParam.bufferDataInfo.isp1BlockPattern.blockDimension.height =
                            pT3DataPattern->isp1BlockPattern.blockDimension.height;
                        PDLibInitParams.initParam.bufferDataInfo.isp1BlockPattern.blockDimension.width  =
                            pT3DataPattern->isp1BlockPattern.blockDimension.width;

                        for (UINT index = 0; index < pT3DataPattern->isp1BlockPattern.pixelCount; index++)
                        {
                            PDLibInitParams.initParam.bufferDataInfo.isp1BlockPattern.pixelCoordinate[index].x =
                                pT3DataPattern->isp1BlockPattern.pixelCoordinate[index].x;
                            PDLibInitParams.initParam.bufferDataInfo.isp1BlockPattern.pixelCoordinate[index].y =
                                pT3DataPattern->isp1BlockPattern.pixelCoordinate[index].y;

                            if (TRUE == pPDAFModeInfo->PDSensorNativePatternInfoExists)
                            {
                                PDAFPixelShieldInformation PDPixelType =
                                    pPDAFModeInfo->PDSensorNativePatternInfo.
                                    PDBlockPattern.PDPixelCoordinates[index].PDPixelShieldInformation;

                                result = GetPDAFPixelType(
                                    PDPixelType,
                                    &PDLibInitParams.initParam.bufferDataInfo.isp1BlockPattern.pixelCoordinate[index].type);
                                if (CamxResultEFailed == result)
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: GetPDAFPixelType failed for Type3 Data");
                                    break;
                                }
                            }
                        }

                        if (CamxResultSuccess == result)
                        {
                            PDLibInitParams.initParam.bufferDataInfo.isp2BlockPattern.verticalPDOffset      =
                                pT3DataPattern->isp2BlockPattern.verticalPDOffset;
                            PDLibInitParams.initParam.bufferDataInfo.isp2BlockPattern.horizontalPDOffset    =
                                pT3DataPattern->isp2BlockPattern.horizontalPDOffset;
                            PDLibInitParams.initParam.bufferDataInfo.isp2BlockPattern.pixelCount            =
                                pT3DataPattern->isp2BlockPattern.pixelCount;
                            PDLibInitParams.initParam.bufferDataInfo.isp2BlockPattern.blockDimension.height =
                                pT3DataPattern->isp2BlockPattern.blockDimension.height;
                            PDLibInitParams.initParam.bufferDataInfo.isp2BlockPattern.blockDimension.width  =
                                pT3DataPattern->isp2BlockPattern.blockDimension.width;

                            for (UINT index = 0; index < pT3DataPattern->isp2BlockPattern.pixelCount; index++)
                            {
                                PDLibInitParams.initParam.bufferDataInfo.isp2BlockPattern.pixelCoordinate[index].x =
                                    pT3DataPattern->isp2BlockPattern.pixelCoordinate[index].x;
                                PDLibInitParams.initParam.bufferDataInfo.isp2BlockPattern.pixelCoordinate[index].y =
                                    pT3DataPattern->isp2BlockPattern.pixelCoordinate[index].y;

                                if ((TRUE == pPDAFModeInfo->PDSensorNativePatternInfoExists) &&
                                    (TRUE == pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockPatternExists))
                                {
                                    PDAFPixelShieldInformation PDPixelType =
                                        pPDAFModeInfo->PDSensorNativePatternInfo.
                                        PDBlockPattern.PDPixelCoordinates[index].PDPixelShieldInformation;
                                    result = GetPDAFPixelType(PDPixelType,
                                                              &PDLibInitParams.initParam.bufferDataInfo.isp2BlockPattern.
                                                                pixelCoordinate[index].type);
                                    if (CamxResultEFailed == result)
                                    {
                                        CAMX_LOG_WARN(CamxLogGroupSensor, "PDAF Error: GetPDAFPixelType failed for Type3");
                                        break;
                                    }
                                }
                                else
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupSensor,
                                                   "PDAF Error: PDSensorNativePattern or "
                                                   "PDNativeBlockPattern not available for Type3");
                                    result = CamxResultEFailed;
                                    break;
                                }
                            }
                        }
                    }
                    break;
                case PDAFType::PDType2:
                    // Type 2 Buffer Pattern Information

                    result = GetPDAFSensorType(pPDAFModeInfo->PDType,
                        static_cast<VOID*>(&PDLibInitParams.initParam.bufferDataInfo.sensorType));

                    if (FALSE == pPDAFModeInfo->PDSensorNativePatternInfoExists)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor,
                                       "PDAF Error: PDSensorNativePatternInfo not available for Type2");
                        result = CamxResultEFailed;
                    }

                    if (FALSE == pPDAFModeInfo->PDBufferBlockPatternInfoExists)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor,
                                       "PDAF Error: PDBufferBlockPatternInfo not available for Type2");
                        result = CamxResultEFailed;
                    }

                    if (CamxResultSuccess == result)
                    {
                        result = GetPDAFBufferDataFormat(
                                pPDAFModeInfo->PDSensorNativePatternInfo.PDNativeBufferFormat,
                                &PDLibInitParams.initParam.nativePatternInfo.bufferFormat);
                    }

                    if (CamxResultSuccess == result)
                    {
                        result = FillPDAFPixelCoordinates(
                                pPDAFModeInfo->PDBufferBlockPatternInfo.PDBlockPattern,
                                &PDLibInitParams.initParam.bufferDataInfo.isp1BlockPattern);
                    }

                    if (CamxResultSuccess == result)
                    {
                        result = GetPDAFBufferDataFormat(
                                pPDAFModeInfo->PDBufferBlockPatternInfo.PDBufferFormat,
                                &PDLibInitParams.initParam.bufferDataInfo.bufferFormat);
                    }

                    if (CamxResultSuccess == result)
                    {
                        PDLibInitParams.initParam.bufferDataInfo.isp1BufferStride =
                            pPDAFModeInfo->PDBufferBlockPatternInfo.PDStride;

                        if (TRUE == pPDAFModeInfo->PDBufferBlockPatternInfo.PDBlockPatternExists)
                        {
                            PDLibInitParams.initParam.bufferDataInfo.ispBufferHeight =
                                pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockCountVertical *
                                pPDAFModeInfo->PDBufferBlockPatternInfo.PDBlockPattern.PDBlockDimensions.height;

                            PDLibInitParams.initParam.bufferDataInfo.isp1BufferWidth =
                                pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockCountHorizontal *
                                pPDAFModeInfo->PDBufferBlockPatternInfo.PDBlockPattern.PDBlockDimensions.width;

                            Utils::Memcpy(&PDLibInitParams.initParam.bufferDataInfo.isp1BlockPattern.blockDimension,
                                        &pPDAFModeInfo->PDBufferBlockPatternInfo.PDBlockPattern.PDBlockDimensions,
                                        (sizeof(PDLibDimensionInfo)));
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupSensor,
                                    "PDAF Error: PDBufferBlockPatternInfo not available for Type2");
                            result = CamxResultEFailed;
                        }

                        // Image overlap only valid for Type3 where there is a possibility of two ISPs
                        PDLibInitParams.initParam.bufferDataInfo.imageOverlap = 0;
                    }
                    break;
                case PDAFType::PDType2PD:
                    // 2PD Buffer Pattern Information
                    result = GetPDAFSensorType(pPDAFModeInfo->PDType,
                        static_cast<VOID*>(&PDLibInitParams.initParam.bufferDataInfo.sensorType));

                    if (FALSE == pPDAFModeInfo->PDSensorNativePatternInfoExists)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor,
                                       "PDAF Error: PDSensorNativePatternInfo not available for Type2PD");
                        result = CamxResultEFailed;
                    }

                    if (FALSE == pPDAFModeInfo->PDBufferBlockPatternInfoExists)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor,
                                       "PDAF Error: PDBufferBlockPatternInfo not available for Type2PD");
                        result = CamxResultEFailed;
                    }

                    if (CamxResultSuccess == result)
                    {
                        result = GetPDAFBufferDataFormat(
                                pPDAFModeInfo->PDSensorNativePatternInfo.PDNativeBufferFormat,
                                &PDLibInitParams.initParam.nativePatternInfo.bufferFormat);
                    }

                    if (CamxResultSuccess == result)
                    {
                        result = GetPDAFBufferDataFormat(
                                pPDAFModeInfo->PDBufferBlockPatternInfo.PDBufferFormat,
                                &PDLibInitParams.initParam.bufferDataInfo.bufferFormat);
                    }

                    if ((CamxResultSuccess == result) && (TRUE == pPDAFModeInfo->PDPixelOrderTypeExists))
                    {
                        PDLibInitParams.initParam.pixelOrderType =
                            static_cast<PDLibPixelOrderType>(pPDAFModeInfo->PDPixelOrderType);
                    }

                    if (CamxResultSuccess == result)
                    {
                        PDLibInitParams.initParam.bufferDataInfo.isp1BufferStride =
                            pPDAFModeInfo->PDBufferBlockPatternInfo.PDStride;

                        PDLibInitParams.initParam.bufferDataInfo.ispBufferHeight  =
                            pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockCountVertical;

                        PDLibInitParams.initParam.bufferDataInfo.isp1BufferWidth  =
                            pPDAFModeInfo->PDSensorNativePatternInfo.PDBlockCountHorizontal;
                    }
                    break;
                case PDAFType::PDType1:

                    if (10 == actuatorDataBitwidth)
                    {
                        PDLibInitParams.initParam.defocusBitShift = 14;
                    }
                    else if (12 == actuatorDataBitwidth)
                    {
                        PDLibInitParams.initParam.defocusBitShift = 12;
                    }
                    else
                    {
                        CAMX_LOG_WARN(CamxLogGroupSensor, "PD Defocus bit shift is 0. Actuator data Bitwidth is %d",
                                                          actuatorDataBitwidth);
                        PDLibInitParams.initParam.defocusBitShift = 0;
                    }

                    PDLibInitParams.initParam.sensorPDStatsFormat =
                        static_cast<PDLibSensorPDStatsFormat>(pPDAFModeInfo->PDSensorPDStatsFormat);

                    result = CamxResultSuccess;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid PDAF Type %d", pPDAFModeInfo->PDType);
                    result = CamxResultEFailed;
                    break;
            }

            // PD settings
            const StaticSettings*   pSettings                             = pHwContext->GetStaticSettings();
            CAMX_ASSERT(NULL != pSettings);
            PDLibInitParams.initParam.settingsInfo.enablePDLibLog         = pSettings->enablePDLibLog;
            PDLibInitParams.initParam.settingsInfo.enablePDLibProfiling   = pSettings->enablePDLibProfiling;
            PDLibInitParams.initParam.settingsInfo.enablePDLibTestMode    = pSettings->enablePDLibTestMode;
            PDLibInitParams.initParam.settingsInfo.disableLCR             = pSettings->disablePDLibLCR;
            PDLibInitParams.initParam.settingsInfo.enablePDLibDump        = pSettings->enablePDLibDump;
            PDLibInitParams.initParam.settingsInfo.profile3A              = pSettings->profile3A;


            if (NULL != pPDHWEnableConditions)
            {
                PDLibInitParams.initParam.pdHWEnableConditions.isLCREnableConditionsMet         =
                    pPDHWEnableConditions->isLCREnableConditionsMet;
                PDLibInitParams.initParam.pdHWEnableConditions.isDualPDEnableConditionsMet      =
                    pPDHWEnableConditions->isDualPDEnableConditionsMet;
                PDLibInitParams.initParam.pdHWEnableConditions.isSparsePDEnableConditionsMet    =
                    pPDHWEnableConditions->isSparsePDEnableConditionsMet;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "PropertyIDUsecasePDHWEnableConditions is not published");
                PDLibInitParams.initParam.pdHWEnableConditions.isLCREnableConditionsMet         = FALSE;
                PDLibInitParams.initParam.pdHWEnableConditions.isDualPDEnableConditionsMet      = FALSE;
                PDLibInitParams.initParam.pdHWEnableConditions.isSparsePDEnableConditionsMet    = FALSE;
            }

            if (CamxResultSuccess == result)
            {
                CDKResult returnValue = CDKResultSuccess;

                PrintDebugPDAFData(PDLibInitParams);
                CAMX_LOG_INFO(CamxLogGroupSensor, "Initializing PDAF %s", m_pPDAFConfigData->PDCommonInfo.PDAFName);
                returnValue = pPDLib->PDLibInitialize(pPDLib, &PDLibInitParams);
                if (CDKResultSuccess != returnValue)
                {
                    CAMX_LOG_WARN(CamxLogGroupSensor, "PDAF Error: PD Library Initialization failed");
                    result = CamxResultEFailed;
                }
            }
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupSensor, "PDAF Error: Invalid Handle, Failed to initialize PD Library");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::GetInitialHWPDConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::GetInitialHWPDConfig(
    CHIPDLib*   pPDLib,
    PDHwConfig* pHWConfig)
{
    PDLibGetParams  getParams   = {};
    CDKResult       returnValue = CDKResultSuccess;
    CamxResult      result      = CamxResultSuccess;

    if (NULL != pPDLib && NULL != pPDLib->PDLibGetParam)
    {
        getParams.type = PDLibGetParamHWConfig;

        returnValue = pPDLib->PDLibGetParam(pPDLib, &getParams);

        if (CDKResultSuccess != returnValue)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: PD Library Get Param failed type %d", getParams.type);
            result = CamxResultEFailed;
        }
        else
        {
            *pHWConfig = getParams.outputData.config;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: Invalid Handle, Failed to Get PDHw config Library");
        result = CamxResultEFailed;
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::GetPDAFOTPData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::GetPDAFOTPData(
    PDLibCreateParams*      pPDLibInitParams,
    const EEPROMOTPData*    pOTPData,
    PDAFModeInformation*    pPDAFModeInfo)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pPDLibInitParams) && (NULL != pPDAFModeInfo))
    {
        if (NULL != pOTPData)
        {
            pPDLibInitParams->initParam.infinityDac = pOTPData->AFCalibration.infinityDAC;
            pPDLibInitParams->initParam.macroDac = pOTPData->AFCalibration.macroDAC;

            switch (pPDAFModeInfo->PDType)
            {
                case PDAFType::PDType1:
                {
                    m_calibrationDataType1 = pOTPData->PDAFDCCCalibration;
                    pPDLibInitParams->initParam.pCalibrationParam = static_cast<void *>(&m_calibrationDataType1);
                    break;
                }
                case PDAFType::PDType2PD:
                case PDAFType::PDType2:
                case PDAFType::PDType3:
                default:
                {
                    m_calibrationDataType2 = pOTPData->PDAF2DCalibration;
                    pPDLibInitParams->initParam.pCalibrationParam = static_cast<void *>(&m_calibrationDataType2);
                    break;
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: OTP data not present");
            result = CamxResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: Invalid pointer");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::GetPDAFActuatorData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::GetPDAFActuatorData(
    ActuatorData*      pActuatorData,
    PDLibCreateParams* pPDLibInitParams,
    UINT16*            pActuatorDataBitwidth)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pActuatorData) && (NULL != pPDLibInitParams))
    {
        pPDLibInitParams->initParam.actuatorSensitivity = pActuatorData->GetSensitivity();
        *pActuatorDataBitwidth                          = pActuatorData->GetActuatorDataBitwidth();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: Actuator data not present");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::GetPDAFSensorData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::GetPDAFSensorData(
    ImageSensorData*   pSensorData,
    PDLibCreateParams* pPDLibInitParams,
    UINT32             resIdx)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pSensorData) && (NULL != pPDLibInitParams))
    {
        const ResolutionInformation* pResolutionInfo  = pSensorData->GetResolutionInfo();
        const PixelArrayInfo*        pActiveArrayInfo = pSensorData->GetActiveArraySize();

        pPDLibInitParams->initParam.blackLevel                            =
            pSensorData->GetSensorBlackLevel();

        pPDLibInitParams->initParam.isHdrModeEnabled                      =
            IsHDREnabled(&pResolutionInfo->resolutionData[resIdx]);

        pPDLibInitParams->initParam.nativePatternInfo.bayerPattern        =
            static_cast<PDLibSensorBayerPattern>(pResolutionInfo->resolutionData[resIdx].colorFilterArrangement);

        pPDLibInitParams->initParam.nativePatternInfo.currentImageWidth   =
            pResolutionInfo->resolutionData[resIdx].streamInfo.streamConfiguration[0].frameDimension.width;

        pPDLibInitParams->initParam.nativePatternInfo.currentImageHeight  =
            pResolutionInfo->resolutionData[resIdx].streamInfo.streamConfiguration[0].frameDimension.height;

        pPDLibInitParams->initParam.pixelDepth                            =
            pResolutionInfo->resolutionData[resIdx].streamInfo.streamConfiguration[0].bitWidth;

        pPDLibInitParams->initParam.nativePatternInfo.originalImageHeight =
            pActiveArrayInfo->activeDimension.height;

        pPDLibInitParams->initParam.nativePatternInfo.originalImageWidth  =
            pActiveArrayInfo->activeDimension.width;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: Sensor data not present");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::IsHDREnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT PDAFData::IsHDREnabled(
    ResolutionData* pResolutionData)
{
    UINT   isHDREnabled    = 0;
    UINT32 capabilityCount = 0;

    CAMX_ASSERT(NULL != pResolutionData);
    for (capabilityCount = 0; capabilityCount < pResolutionData->capabilityCount; capabilityCount++)
    {
        if ((pResolutionData->capability[capabilityCount] == SensorCapability::IHDR) ||
            (pResolutionData->capability[capabilityCount] == SensorCapability::ZZHDR))
        {
            isHDREnabled = 1;
        }
    }
    return isHDREnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::FillPDAFPixelCoordinates
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::FillPDAFPixelCoordinates(
    PDAFBlockPattern PDBlockPattern,
    VOID*            pData)
{
    CamxResult         result          = CamxResultSuccess;
    UINT32             i               = 0;
    PDLibBlockPattern* pPDLibPixelInfo = static_cast<PDLibBlockPattern*>(pData);

    CAMX_ASSERT(NULL != pData);

    pPDLibPixelInfo->horizontalPDOffset = PDBlockPattern.PDOffsetHorizontal;
    pPDLibPixelInfo->verticalPDOffset   = PDBlockPattern.PDOffsetVertical;
    pPDLibPixelInfo->pixelCount         = PDBlockPattern.PDPixelCount;

    for (i = 0; i < pPDLibPixelInfo->pixelCount; i++)
    {
        pPDLibPixelInfo->pixelCoordinate[i].x = PDBlockPattern.PDPixelCoordinates[i].PDXCoordinate;

        pPDLibPixelInfo->pixelCoordinate[i].y = PDBlockPattern.PDPixelCoordinates[i].PDYCoordinate;

        result = GetPDAFPixelType(PDBlockPattern.PDPixelCoordinates[i].PDPixelShieldInformation,
            &pPDLibPixelInfo->pixelCoordinate[i].type);
        if (CamxResultEFailed == result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: FillPDAFPixelCoordinates failed");
            break;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::AdjustNativePDAFOffsets
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::AdjustNativePDAFOffsets(
    VOID*            pData)
{
    CamxResult         result          = CamxResultSuccess;
    UINT               i               = 0;
    PDLibBlockPattern* pPDLibPixelInfo = static_cast<PDLibBlockPattern*>(pData);

    if (NULL != pPDLibPixelInfo)
    {
        for (i = 0; i < pPDLibPixelInfo->pixelCount; i++)
        {
            pPDLibPixelInfo->pixelCoordinate[i].x =
                pPDLibPixelInfo->pixelCoordinate[i].x - pPDLibPixelInfo->horizontalPDOffset;

            pPDLibPixelInfo->pixelCoordinate[i].y =
                pPDLibPixelInfo->pixelCoordinate[i].y - pPDLibPixelInfo->verticalPDOffset;
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: Could not adjust the PD native offset");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::GetPDAFSensorType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::GetPDAFSensorType(
    PDAFType PDType,
    VOID*    pData)
{
    CamxResult      result      = CamxResultSuccess;
    PDLibSensorType sensorType;

    CAMX_ASSERT(NULL != pData);

    switch (PDType)
    {
        case PDAFType::PDType1:
        {
            sensorType = PDLibSensorType::PDLibSensorType1;
            break;
        }
        case PDAFType::PDType2:
        {
            sensorType = PDLibSensorType::PDLibSensorType2;
            break;
        }
        case PDAFType::PDType3:
        {
            sensorType = PDLibSensorType::PDLibSensorType3;
            break;
        }
        case PDAFType::PDType2PD:
        {
            sensorType = PDLibSensorType::PDLibSensorDualPD;
            break;
        }
        case PDAFType::PDTypeUnknown:
        default:
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: Invalid PDAF Type");
            sensorType = PDLibSensorType::PDLibSensorInvalid;
            break;
        }
    }

    Utils::Memcpy(pData, &sensorType, sizeof(PDLibSensorType));
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::GetPDAFPixelType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::GetPDAFPixelType(
    PDAFPixelShieldInformation PDPixelType,
    VOID*                      pData)
{
    CamxResult           result = CamxResultSuccess;
    PDLibPixelShieldInfo pixelType;

    CAMX_ASSERT(NULL != pData);

    switch (PDPixelType)
    {
        case PDAFPixelShieldInformation::LEFTDIODE:
        {
            pixelType = PDLibPixelShieldInfo::PDLibLeftDiodePixel;
            break;
        }
        case PDAFPixelShieldInformation::RIGHTDIODE:
        {
            pixelType = PDLibPixelShieldInfo::PDLibRightDiodePixel;
            break;
        }
        case PDAFPixelShieldInformation::LEFTSHIELDED:
        {
            pixelType = PDLibPixelShieldInfo::PDLibLeftShieldedPixel;
            break;
        }
        case PDAFPixelShieldInformation::RIGHTSHIELDED:
        {
            pixelType = PDLibPixelShieldInfo::PDLibRightShieldedPixel;
            break;
        }
        case PDAFPixelShieldInformation::UPDIODE:
        {
            pixelType = PDLibPixelShieldInfo::PDLibUpDiodePixel;
            break;
        }
        case PDAFPixelShieldInformation::DOWNDIODE:
        {
            pixelType = PDLibPixelShieldInfo::PDLibDownDiodePixel;
            break;
        }
        case PDAFPixelShieldInformation::UPSHIELDED:
        {
            pixelType = PDLibPixelShieldInfo::PDLibUpShieldedPixel;
            break;
        }
        case PDAFPixelShieldInformation::DOWNSHIELDED:
        {
            pixelType = PDLibPixelShieldInfo::PDLibDownShieldedPixel;
            break;
        }
        case PDAFPixelShieldInformation::TOPLEFTDIODE:
        {
            pixelType = PDLibPixelShieldInfo::PDLibTopLeftDiodePixel;
            break;
        }
        case PDAFPixelShieldInformation::TOPRIGHTDIODE:
        {
            pixelType = PDLibPixelShieldInfo::PDLibTopRightDiodePixel;
            break;
        }
        case PDAFPixelShieldInformation::BOTTOMLEFTDIODE:
        {
            pixelType = PDLibPixelShieldInfo::PDLibBottomLeftDiodePixel;
            break;
        }
        case PDAFPixelShieldInformation::BOTTOMRIGHTDIODE:
        {
            pixelType = PDLibPixelShieldInfo::PDLibBottomRightDiodePixel;
            break;
        }
        default:
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: Invalid PDAF Pixel Shield Type");
            pixelType = PDLibPixelShieldInfo::PDLibRightShieldedPixel;
            break;
        }
    }
    Utils::Memcpy(pData, &pixelType, sizeof(PDLibPixelShieldInfo));
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::GetPDAFBufferDataFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::GetPDAFBufferDataFormat(
    PDAFBufferDataFormat PDBufferDataFormat,
    VOID*                pData)
{
    CamxResult        result = CamxResultSuccess;
    PDLibBufferFormat PDBufferFormat;
    CAMX_ASSERT(NULL != pData);

    switch (PDBufferDataFormat)
    {
        case PDAFBufferDataFormat::MIPI8:
        {
            PDBufferFormat = PDLibBufferFormat::PDLibBufferFormatMipi8;
            break;
        }
        case PDAFBufferDataFormat::MIPI10:
        {
            PDBufferFormat = PDLibBufferFormat::PDLibBufferFormatMipi10;
            break;
        }
        case PDAFBufferDataFormat::PACKED10:
        {
            PDBufferFormat = PDLibBufferFormat::PDLibBufferFormatPacked10;
            break;
        }
        case PDAFBufferDataFormat::UNPACKED16:
        {
            PDBufferFormat = PDLibBufferFormat::PDLibBufferFormatUnpacked16;
            break;
        }
        default:
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF Error: Invalid PDAF Buffer Data Format");
            PDBufferFormat = PDLibBufferFormat::PDLibBufferFormatUnpacked16;
            break;
        }
    }
    Utils::Memcpy(pData, &PDBufferFormat, sizeof(PDLibBufferFormat));
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::PrintDebugPDAFData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID PDAFData::PrintDebugPDAFData(
    PDLibCreateParams  PDAFData)
{
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "PDAF Debug Data for %s", m_pPDAFConfigData->PDCommonInfo.PDAFName);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\t=====PDAF Information:=====");
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tActuator Sensitivity: %f",
        PDAFData.initParam.actuatorSensitivity);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tBlack Level: %d",
        PDAFData.initParam.blackLevel);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tPixel Depth: %d",
        PDAFData.initParam.pixelDepth);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tIs Hdr Mode Enabled: %d",
        PDAFData.initParam.isHdrModeEnabled);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tPDPixelOverflowThreshold: %d",
        PDAFData.initParam.PDPixelOverflowThreshold);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tMacro DAC: %d",
        PDAFData.initParam.macroDac);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tInfinity DAC: %d",
        PDAFData.initParam.infinityDac);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tPixel order Type: %d",
        PDAFData.initParam.pixelOrderType);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\t=====Native Pattern Information:=====");
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\toriginalImageWidth: %d",
        PDAFData.initParam.nativePatternInfo.originalImageWidth);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\toriginalImageHeight: %d",
        PDAFData.initParam.nativePatternInfo.originalImageHeight);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tcurrentImageWidth: %d",
        PDAFData.initParam.nativePatternInfo.currentImageWidth);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tcurrentImageHeight: %d",
        PDAFData.initParam.nativePatternInfo.currentImageHeight);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\thorizontalDownscaleFactor: %f",
        PDAFData.initParam.nativePatternInfo.horizontalDownscaleFactor);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tverticalDownscaleFactor: %f",
        PDAFData.initParam.nativePatternInfo.verticalDownscaleFactor);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tPDOffsetCorrection: %f",
        PDAFData.initParam.nativePatternInfo.PDOffsetCorrection);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tlcrOffset: %f",
        PDAFData.initParam.nativePatternInfo.lcrPDOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tcropRegion.height: %d",
        PDAFData.initParam.nativePatternInfo.cropRegion.height);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tcropRegion.width: %d",
        PDAFData.initParam.nativePatternInfo.cropRegion.width);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tcropRegion.x: %d",
        PDAFData.initParam.nativePatternInfo.cropRegion.x);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tcropRegion.y: %d",
        PDAFData.initParam.nativePatternInfo.cropRegion.y);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tbufferFormat: %d",
        PDAFData.initParam.nativePatternInfo.bufferFormat);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\torientation: %d",
        PDAFData.initParam.nativePatternInfo.orientation);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tpatternInfo.horizontalBlockCount: %d",
        PDAFData.initParam.nativePatternInfo.patternInfo.horizontalBlockCount);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tpatternInfo.verticalBlockCount: %d",
        PDAFData.initParam.nativePatternInfo.patternInfo.verticalBlockCount);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tpatternInfo.blockPattern.horizontalPDOffset: %d",
        PDAFData.initParam.nativePatternInfo.patternInfo.blockPattern.horizontalPDOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tpatternInfo.blockPattern.verticalPDOffset: %d",
        PDAFData.initParam.nativePatternInfo.patternInfo.blockPattern.verticalPDOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tpatternInfo.blockPattern.blockDimension.height: %d",
        PDAFData.initParam.nativePatternInfo.patternInfo.blockPattern.blockDimension.height);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tpatternInfo.blockPattern.blockDimension.width: %d",
        PDAFData.initParam.nativePatternInfo.patternInfo.blockPattern.blockDimension.width);
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tpatternInfo.blockPattern.pixelCount: %d",
        PDAFData.initParam.nativePatternInfo.patternInfo.blockPattern.pixelCount);

    for (UINT i = 0; i < PDAFData.initParam.nativePatternInfo.patternInfo.blockPattern.pixelCount; i++)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor,
            "\tpatternInfo.blockPattern.pixelCount[%d].x = %d",
            i,
            PDAFData.initParam.nativePatternInfo.patternInfo.blockPattern.pixelCoordinate[i].x);

        CAMX_LOG_VERBOSE(CamxLogGroupSensor,
            "\tpatternInfo.blockPattern.pixelCount[%d].y = %d",
            i,
            PDAFData.initParam.nativePatternInfo.patternInfo.blockPattern.pixelCoordinate[i].y);

        CAMX_LOG_VERBOSE(CamxLogGroupSensor,
            "\tpatternInfo.blockPattern.pixelCount[%d].type = %d",
            i,
            PDAFData.initParam.nativePatternInfo.patternInfo.blockPattern.pixelCoordinate[i].type);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\t=====PDAF Buffer data pattern=====");
    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tBuffer format: %d",
        PDAFData.initParam.bufferDataInfo.bufferFormat);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\timageOverlap: %d",
        PDAFData.initParam.bufferDataInfo.imageOverlap);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tsensorType: %d",
        PDAFData.initParam.bufferDataInfo.sensorType);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tispBufferHeight: %d",
        PDAFData.initParam.bufferDataInfo.ispBufferHeight);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tisp1BufferWidth: %d",
        PDAFData.initParam.bufferDataInfo.isp1BufferWidth);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tisp1BufferStride: %d",
        PDAFData.initParam.bufferDataInfo.isp1BufferStride);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tisp1BlockPattern.verticalPDOffset: %d",
        PDAFData.initParam.bufferDataInfo.isp1BlockPattern.verticalPDOffset);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tisp1BlockPattern.horizontalPDOffset: %d",
        PDAFData.initParam.bufferDataInfo.isp1BlockPattern.horizontalPDOffset);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tisp1BlockPattern.blockDimension.height: %d",
        PDAFData.initParam.bufferDataInfo.isp1BlockPattern.blockDimension.height);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tisp1BlockPattern.blockDimension.width: %d",
        PDAFData.initParam.bufferDataInfo.isp1BlockPattern.blockDimension.width);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "\tisp1BlockPattern.pixelCount: %d",
        PDAFData.initParam.bufferDataInfo.isp1BlockPattern.pixelCount);

    for (UINT i = 0; i < PDAFData.initParam.bufferDataInfo.isp1BlockPattern.pixelCount; i++)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor,
            "\tpatternInfo.blockPattern.pixelCount[%d].x = %d",
            i,
            PDAFData.initParam.bufferDataInfo.isp1BlockPattern.pixelCoordinate[i].x);

        CAMX_LOG_VERBOSE(CamxLogGroupSensor,
            "\tpatternInfo.blockPattern.pixelCount[%d].y = %d",
            i,
            PDAFData.initParam.bufferDataInfo.isp1BlockPattern.pixelCoordinate[i].y);

        CAMX_LOG_VERBOSE(CamxLogGroupSensor,
            "\tpatternInfo.blockPattern.pixelCount[%d].type = %d",
            i,
            PDAFData.initParam.bufferDataInfo.isp1BlockPattern.pixelCoordinate[i].type);
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::GetCurrentPDAFModeIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::GetCurrentPDAFModeIndex(
    UINT32  resIdx,
    UINT32* pPDAFMode)
{
    CamxResult result = CamxResultEFailed;
    UINT PDAFModeCount = 0;

    if (NULL != m_pPDAFConfigData)
    {
        for (PDAFModeCount = 0; PDAFModeCount < m_pPDAFConfigData->PDModeInfoCount; PDAFModeCount++)
        {
            if (m_pPDAFConfigData->PDModeInfo[PDAFModeCount].PDSensorMode == resIdx)
            {
                *pPDAFMode = PDAFModeCount;
                result     = CamxResultSuccess;
                CAMX_LOG_INFO(CamxLogGroupSensor, "Current Sensor Mode: %d, Corresponding PDAF Mode: %d", resIdx, *pPDAFMode);
                break;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::GetPDAFLibraryName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHAR* PDAFData::GetPDAFLibraryName(
    UINT32 resIdx)
{
    CamxResult result       = CamxResultEFailed;
    UINT32     PDAFMode     = 0;
    CHAR*      pPDAFLibName = NULL;

    result = GetCurrentPDAFModeIndex(resIdx, &PDAFMode);
    if (CamxResultSuccess == result)
    {
        pPDAFLibName = m_pPDAFConfigData->PDModeInfo[PDAFMode].PDAFLibraryName;
    }

    if (NULL == pPDAFLibName)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "No PDAF library name supplied for PDAF mode %d", PDAFMode);
    }
    return pPDAFLibName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::GetPDAFModeInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult PDAFData::GetPDAFModeInformation(
    UINT32                 PDAFModeIdx,
    PDAFModeInformation**  ppPDAFModeInfo)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != m_pPDAFConfigData) && (PDAFModeIdx <= m_pPDAFConfigData->PDModeInfoCount))
    {
        *ppPDAFModeInfo = &m_pPDAFConfigData->PDModeInfo[PDAFModeIdx];
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "PDAF information for PDAFModeIdx %d doesn't exist", PDAFModeIdx);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PDAFData::~PDAFData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PDAFData::~PDAFData()
{
    m_pPDAFConfigData = NULL;
    m_isPdafEnabled   = FALSE;
}

CAMX_NAMESPACE_END
