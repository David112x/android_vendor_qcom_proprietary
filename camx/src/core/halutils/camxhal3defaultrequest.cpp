////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3defaultrequest.cpp
/// @brief Definitions for HAL3DefaultRequest class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3defaultrequest.h"
#include "camxjpegquanttable.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3DefaultRequest* HAL3DefaultRequest::GetInstance(
    UINT32 cameraId)
{
    CAMX_ASSERT(cameraId < MaxNumImageSensors);

    static HAL3DefaultRequest s_HAL3DefaultRequestSingleton[MaxNumImageSensors];
    return &s_HAL3DefaultRequestSingleton[cameraId];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::ConstructDefaultRequestSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const Metadata* HAL3DefaultRequest::ConstructDefaultRequestSettings(
    UINT32                 cameraId,
    Camera3RequestTemplate requestTemplate)
{
    UINT requestTemplateIndex = requestTemplate - 1; // Need to subtract 1 from the template since the templates start at 0x1

    if (NULL == GetInstance(cameraId)->m_pMetadataTemplates[requestTemplateIndex])
    {
        CamxResult              result                  = CamxResultSuccess;
        const HwEnvironment*    pHWEnvironment          = HwEnvironment::GetInstance();
        Metadata*               pMetadata               = NULL;
        SIZE_T                  slotMetadataEntryCount  = 0;
        SIZE_T                  slotMetadataDataSize    = 0;
        SIZE_T                  count                   = 0;
        HwCameraInfo            cameraInfo              = {};

        CAMX_ASSERT(NULL != pHWEnvironment);

        result = pHWEnvironment->GetCameraInfo(cameraId, &cameraInfo);

        if ((CamxResultSuccess == result)               &&
            (requestTemplate >= RequestTemplatePreview) &&
            (requestTemplate <= RequestTemplateManual))
        {
            HAL3MetadataUtil::CalculateSizeAllMeta(&slotMetadataEntryCount, &slotMetadataDataSize,
                TagSectionVisibility::TagSectionVisibleToAll);

            pMetadata = HAL3MetadataUtil::CreateMetadata(slotMetadataEntryCount, slotMetadataDataSize);
            if ((NULL != pMetadata) && (NULL != cameraInfo.pPlatformCaps) && (NULL != cameraInfo.pSensorCaps))
            {
                DefaultRequest defaultRequest = {};
                GetDefaultRequest(&defaultRequest, requestTemplate, &cameraInfo);

                // All template request must be in supportted request array
                SIZE_T numAvailableRequest = cameraInfo.pPlatformCaps->numRequestKeys;

                for (SIZE_T key = 0; key < numAvailableRequest; key++)
                {
                    switch (cameraInfo.pPlatformCaps->requestKeys[key])
                    {
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Color correction
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case ColorCorrectionMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ColorCorrectionMode,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.colorCorrectionMode)),
                                                                   1);
                            break;

                        case ColorCorrectionTransform:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ColorCorrectionTransform,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.colorCorrectionTransform)),
                                                                   9);
                            break;

                        case ColorCorrectionGains:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ColorCorrectionGains,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.colorCorrectionGains)),
                                                                   4);
                            break;

                        case ColorCorrectionAberrationMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ColorCorrectionAberrationMode,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.colorCorrectionAberrationMode)),
                                                                   1);
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Control
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case ControlAEAntibandingMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAEAntibandingMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.AEAntibandingMode)),
                                                                   1);
                            break;

                        case ControlAEExposureCompensation:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAEExposureCompensation,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.AEExposureCompensation)),
                                                                   1);
                            break;

                        case ControlAELock:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAELock,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.AELock)),
                                                                   1);
                            break;

                        case ControlAEMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAEMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.AEMode)),
                                                                   1);
                            break;

                        case ControlAERegions:
                            count  = sizeof(defaultRequest.AERegions) / sizeof(INT32);
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAERegions,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.AERegions)),
                                                                   count);
                            break;

                        case ControlAETargetFpsRange:
                            count  = sizeof(defaultRequest.AETargetFPSRange) / sizeof(INT32);
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAETargetFpsRange,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.AETargetFPSRange)),
                                                                   count);
                            break;

                        case ControlAEPrecaptureTrigger:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAEPrecaptureTrigger,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.AEPrecaptureTrigger)),
                                                                   1);
                            break;

                        case ControlAFMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAFMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.AFMode)),
                                                                   1);
                            break;

                        case ControlAFRegions:
                            count = sizeof(defaultRequest.AFRegions) / sizeof(INT32);
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAFRegions,
                                reinterpret_cast<VOID*>(&(defaultRequest.AFRegions)), count);
                            break;

                        case ControlAFTrigger:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAFTrigger,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.AFTrigger)),
                                                                   1);
                            break;

                        case ControlAWBLock:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAWBLock,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.AWBLock)),
                                                                   1);
                            break;

                        case ControlAWBMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAWBMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.AWBMode)),
                                                                   1);
                            break;

                        case ControlAWBRegions:
                            count  = sizeof(defaultRequest.AWBRegions) / sizeof(INT32);
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlAWBRegions,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.AWBRegions)),
                                                                   count);
                            break;

                        case ControlCaptureIntent:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlCaptureIntent,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.captureIntent)),
                                                                   1);
                            break;

                        case ControlEffectMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlEffectMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.effectMode)),
                                                                   1);
                            break;

                        case ControlMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlMode,
                                                       reinterpret_cast<VOID*>(&(defaultRequest.controlMode)),
                                                       1);
                            break;

                        case ControlSceneMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlSceneMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.sceneMode)),
                                                                   1);
                            break;

                        case ControlVideoStabilizationMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlVideoStabilizationMode,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.videoStabilizationMode)),
                                                                   1);
                            break;

                        case ControlPostRawSensitivityBoost:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlPostRawSensitivityBoost,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.postRawSensitivityBoost)),
                                                                   1);
                            break;

                        case ControlZslEnable:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ControlZslEnable,
                                reinterpret_cast<VOID*>
                                (&(defaultRequest.controlZslEnable)),
                                1);
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Demosaic
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case DemosaicMode:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "DemosaicMode metadata not supported");
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Edge
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case EdgeMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, EdgeMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.edgeMode)),
                                                                   1);
                            break;

                        case EdgeStrength:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "EdgeStrength metadata not supported");
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Flash
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case FlashFiringPower:
                        case FlashFiringTime:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "FlashFiringPower and FlashFiringTime metadata not supported");
                            break;

                        case FlashMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, FlashMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.flashMode)),
                                                                   1);
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Hot pixel
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case HotPixelMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, HotPixelMode,
                                                       reinterpret_cast<VOID*>(&(defaultRequest.hotPixelMode)),
                                                       1);
                            break;
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // JPEG
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case JPEGThumbnailSize:
                            count  = sizeof(defaultRequest.thumbnailSize) / sizeof(INT32);
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, JPEGThumbnailSize,
                                reinterpret_cast<VOID*>(&(defaultRequest.thumbnailSize)),
                                count);
                            break;

                        case JPEGGpsCoordinates:
                        case JPEGGpsProcessingMethod:
                        case JPEGGpsTimestamp:
                        case JPEGOrientation:
                        case JPEGQuality:
                        case JPEGThumbnailQuality:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL,
                                             "JPEGGpsCoordinates, JPEGGpsProcessingMethod, JPEGGpsTimestamp, "
                                             "JPEGOrientation, JPEGQuality, JPEGThumbnailQuality "
                                             "metadata not supported");
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Lens
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case LensAperture:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, LensAperture,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.aperture)),
                                                                   1);
                            break;

                        case LensFilterDensity:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, LensFilterDensity,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.filterDensity)),
                                                                   1);
                            break;

                        case LensFocalLength:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, LensFocalLength,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.focalLength)),
                                                                   1);
                            break;

                        case LensFocusDistance:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, LensFocusDistance,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.focalDistance)),
                                                                   1);
                            break;
                        case LensOpticalStabilizationMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, LensOpticalStabilizationMode,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.opticalStabilizationMode)),
                                                                   1);
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Noise Reduction
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case NoiseReductionMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, NoiseReductionMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.noiseRedutionMode)),
                                                                   1);
                            break;

                        case NoiseReductionStrength:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "NoiseReductionStrength metadata not supported");
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Request
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case RequestId:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, RequestId,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.requestId)),
                                                                   1);
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Scaler
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case ScalerCropRegion:
                            count  = sizeof(defaultRequest.scalerCropRegion) / sizeof(INT32);
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ScalerCropRegion,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.scalerCropRegion)),
                                                                   count);
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Sensor
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case SensorExposureTime:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, SensorExposureTime,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.sensorExposureTime)),
                                                                   1);
                            break;

                        case SensorFrameDuration:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, SensorFrameDuration,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.sensorFrameDuration)),
                                                                   1);
                            break;

                        case SensorSensitivity:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, SensorSensitivity,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.sensorSensitivity)),
                                                                   1);
                            break;

                        case SensorTestPatternData:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "SensorTestPatternData metadata not supported");
                            break;

                        case SensorTestPatternMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, SensorTestPatternMode,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.sensorTestPatternMode)),
                                                                   1);
                            break;

                        case SensorTimestamp:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "SensorTimestamp metadata not supported");
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Shading
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case ShadingMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, ShadingMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.shadingMode)),
                                                                   1);
                            break;

                        case ShadingStrength:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "ShadingStrength metadata not supported");
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Statistics
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case StatisticsFaceDetectMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, StatisticsFaceDetectMode,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.faceDetectMode)),
                                                                   1);
                            break;

                        case StatisticsHistogramMode:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "StatisticsHistogramMode metadata not supported");
                            break;

                        case StatisticsSharpnessMapMode:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "StatisticsSharpnessMapMode metadata not supported");
                            break;

                        case StatisticsHotPixelMapMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, StatisticsHotPixelMapMode,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.hotPixelMapMode)),
                                                                   1);
                            break;

                        case StatisticsLensShadingMapMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, StatisticsLensShadingMapMode,
                                                                   reinterpret_cast<VOID*>
                                                                   (&(defaultRequest.lensShadingMapMode)),
                                                                   1);
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Tonemap
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case TonemapCurveBlue:
                        case TonemapCurveGreen:
                        case TonemapCurveRed:
                        case TonemapGamma:
                        case TonemapPresetCurve:
                            CAMX_ASSERT((TonemapModeFast        == defaultRequest.tonemapMode) ||
                                        (TonemapModeHighQuality == defaultRequest.tonemapMode));
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL,
                                             "TonemapCurveBlue, TonemapCurveGreen, TonemapCurveRed, TonemapGamma, "
                                             "and TonemapPresetCurve metadata not supported");
                            break;

                        case TonemapMode:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, TonemapMode,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.tonemapMode)),
                                                                   1);
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // LED
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case LedTransmit:
                            // No needed for LED contol for now
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "LedTransmit metadata not supported");
                            break;

                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Black level
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case BlackLevelLock:
                            result = HAL3MetadataUtil::SetMetadata(pMetadata, BlackLevelLock,
                                                                   reinterpret_cast<VOID*>(&(defaultRequest.blackLevelLock)),
                                                                   1);
                            break;
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Depth Support
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case LensPoseRotation:
                        {
                            FLOAT availableLensPoseRotation[LensPoseRotationSize] = { 0 };
                            count = cameraInfo.pSensorCaps->lensPoseRotationCount;
                            for (UINT i = 0; i < count; i++)
                            {
                                availableLensPoseRotation[i] =
                                    static_cast<FLOAT>(cameraInfo.pSensorCaps->lensPoseRotationDC[i]);
                            }

                            result = HAL3MetadataUtil::SetMetadata(pMetadata, LensPoseRotation,
                                                                    static_cast<VOID*>(availableLensPoseRotation),
                                                                    count);
                            break;
                        }
                        case LensPoseTranslation:
                        {
                            FLOAT availableLensPoseTranslation[LensPoseTranslationSize] = { 0 };
                            count = cameraInfo.pSensorCaps->lensPoseTranslationCount;

                            for (UINT i = 0; i < count; i++)
                            {
                                availableLensPoseTranslation[i] =
                                        static_cast<FLOAT>(cameraInfo.pSensorCaps->lensPoseTranslationDC[i]);
                            }

                            result = HAL3MetadataUtil::SetMetadata(pMetadata, LensPoseTranslation,
                                                                    static_cast<VOID*>(availableLensPoseTranslation),
                                                                    count);
                            break;
                        }
                        case LensIntrinsicCalibration:
                        {
                            FLOAT availableLensIntrinsicCalibration[LensIntrinsicCalibrationSize] = { 0 };
                            count = cameraInfo.pSensorCaps->lensIntrinsicCalibrationCount;

                            for (UINT i = 0; i < count; i++)
                            {
                                availableLensIntrinsicCalibration[i] =
                                        static_cast<FLOAT>(cameraInfo.pSensorCaps->lensIntrinsicCalibrationDC[i]);
                            }

                            result = HAL3MetadataUtil::SetMetadata(pMetadata, LensIntrinsicCalibration,
                                                                    static_cast<VOID*>(availableLensIntrinsicCalibration),
                                                                    count);
                            break;
                        }
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
                        case LensDistortion:
                        {
                            FLOAT availableLensDistortion[LensDistortionSize] = { 0 };
                            count = cameraInfo.pSensorCaps->lensDistortionCount;

                            for (UINT i = 0; i < count; i++)
                            {
                                availableLensDistortion[i] =
                                        static_cast<FLOAT>(cameraInfo.pSensorCaps->lensDistortionDC[i]);
                            }

                            result = HAL3MetadataUtil::SetMetadata(pMetadata, LensDistortion,
                                                                    static_cast<VOID*>(availableLensDistortion),
                                                                    count);
                            break;
                        }
#endif // Android-P or better
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        // Reprocess
                        ////////////////////////////////////////////////////////////////////////////////////////////////////////
                        case ReprocessEffectiveExposureFactor:
                            // No needed for reprocess contol for now
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "ReprocessEffectiveExposureFactor metadata not supported");
                            break;

                        default:
                            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Invalid request metadata key: %d",
                                             cameraInfo.pPlatformCaps->requestKeys[key]);
                            break;
                    }
                }

                UINT32      jpegDefaultQTableLumaVendorTag;
                UINT32      jpegDefaultQTableChromaVendorTag;
                CamxResult  resultTag = CamxResultSuccess;

                /// @note This result is needed only for setting metadata
                UINT16 defaultJPEGQTableLuma[QuantTableSize];
                resultTag = VendorTagManager::QueryVendorTagLocation(QuantizationTableVendorTagSection,
                                                                     QuantizationTableLumaVendorTagName,
                                                                     &jpegDefaultQTableLumaVendorTag);

                if (CamxResultSuccess == resultTag)
                {
                    Utils::Memcpy(defaultJPEGQTableLuma,
                                  cameraInfo.pPlatformCaps->defaultJPEGQuantTableLuma,
                                  sizeof(defaultJPEGQTableLuma));

                    result = HAL3MetadataUtil::SetMetadata(pMetadata, jpegDefaultQTableLumaVendorTag,
                                                           static_cast<VOID*>(defaultJPEGQTableLuma),
                                                           sizeof(defaultJPEGQTableLuma));

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupHAL,
                                       "Failed to write vendor tag Quantization Table Luma to default metadata");
                    }
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupHAL, "Failed vendor tag location for OEMJPEGLumaQuantizationTable");
                }

                /// @note This result is needed only for setting metadata
                UINT16 defaultJPEGQTableChroma[QuantTableSize];
                resultTag = VendorTagManager::QueryVendorTagLocation(QuantizationTableVendorTagSection,
                                                                     QuantizationTableChromaVendorTagName,
                                                                     &jpegDefaultQTableChromaVendorTag);
                if (CamxResultSuccess == resultTag)
                {
                    Utils::Memcpy(defaultJPEGQTableChroma,
                                  cameraInfo.pPlatformCaps->defaultJPEGQuantTableChroma,
                                  sizeof(defaultJPEGQTableChroma));
                    /// @note This can be ignored becaues all standard tag set properly
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, jpegDefaultQTableChromaVendorTag,
                                                           static_cast<VOID*>(defaultJPEGQTableChroma),
                                                           sizeof(defaultJPEGQTableChroma));
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupHAL,
                                       "Failed to write vendor tag Quantization Table Chroma to default metadata");
                    }
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupHAL, "Failed vendor tag location for OEMJPEGChromaQuantizationTable");
                }
                // Update vendor tags default requests.
                // Update reference crop value
                RefCropWindowSize refCropWindow;
                UINT32            refCropWindowVendorTag;
                refCropWindow.width  = cameraInfo.pSensorCaps->activeArraySize.width;
                refCropWindow.height = cameraInfo.pSensorCaps->activeArraySize.height;
                result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ref.cropsize", "RefCropSize",
                    &refCropWindowVendorTag);
                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag location for ref crop window");
                if (CamxResultSuccess == result)
                {
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, refCropWindowVendorTag,
                        reinterpret_cast<VOID*>(&refCropWindow),
                        sizeof(refCropWindow));
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                        "Failed to write vendor tag ref crop window to default metadata.");
                    CAMX_LOG_VERBOSE(CamxLogGroupHAL,
                        "refCropWindow %dx%d cameraid %d activeArraySize %dx%d xmin %d yMin %d",
                        refCropWindow.width, refCropWindow.height, cameraId,
                        cameraInfo.pSensorCaps->activeArraySize.width, cameraInfo.pSensorCaps->activeArraySize.height,
                        cameraInfo.pSensorCaps->activeArraySize.xMin, cameraInfo.pSensorCaps->activeArraySize.yMin);
                }

                // Update saturation default value
                UINT32 saturationVendorTag;
                INT32  defaultSaturationValue = cameraInfo.pPlatformCaps->saturationRange.defaultValue;
                result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.saturation", "use_saturation",
                                                                  &saturationVendorTag);
                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag location for saturation");
                if (CamxResultSuccess == result)
                {
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, saturationVendorTag,
                                                           reinterpret_cast<VOID*>(&defaultSaturationValue),
                                                           1);
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                        "Failed to write vendor tag saturation to default metadata.");
                }
                // Update sharpness default value
                UINT32 sharpnessVendorTag;
                INT32  defaultHwSharpnessValue = cameraInfo.pPlatformCaps->sharpnessRange.defValue;
                result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sharpness", "strength",
                                                                  &sharpnessVendorTag);
                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag location for sharpness");
                if (CamxResultSuccess == result)
                {
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, sharpnessVendorTag,
                                                           reinterpret_cast<VOID*>(&defaultHwSharpnessValue),
                                                           1);
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                        "Failed to write vendor tag sharpness to default metadata.");
                }

                // Update ae_bracket default value
                UINT32 aeBracketVendorTag   = 0;
                UINT8  defaultAeBracketMode = 0;
                result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.ae_bracket", "mode",
                                                                  &aeBracketVendorTag);
                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag location for ae_bracket");
                if (CamxResultSuccess == result)
                {
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, aeBracketVendorTag,
                                                           reinterpret_cast<VOID*>(&defaultAeBracketMode),
                                                           1);
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                        "Failed to write vendor tag ae_bracket mode to default metadata.");
                }

                // Update tuning feature1 default value
                const StaticSettings* pStaticSettings      = HwEnvironment::GetInstance()->GetStaticSettings();
                INT32                 defaultFeature1Value = pStaticSettings->feature1;
                UINT32                feature1VendorTag;

                result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.tuning.feature", "Feature1Mode",
                                                                  &feature1VendorTag);
                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag location for tuning Feature1Mode");
                if (CamxResultSuccess == result)
                {
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, feature1VendorTag,
                                                           reinterpret_cast<VOID*>(&defaultFeature1Value),
                                                           1);
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                        "Failed to write vendor tag tuning Feature1Mode todefault metadata.");
                }

                // Update tuning feature2 default value
                UINT32 feature2VendorTag;
                INT32  defaultFeature2Value = pStaticSettings->feature2;

                result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.tuning.feature", "Feature2Mode",
                                                                  &feature2VendorTag);
                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag location for tuning Feature2Mode");
                if (CamxResultSuccess == result)
                {
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, feature2VendorTag,
                                                           reinterpret_cast<VOID*>(&defaultFeature2Value),
                                                           1);
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                        "Failed to write vendor tag tuning Feature2Mode to default metadata.");
                }

                // fps limit disable, allows high frame rate without HFR
                UINT32 disableFPSLimitsTag;
                BYTE   defaultDisableFPSlimitsValue = 0;

                result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.manualExposure",
                                                                  "disableFPSLimits",
                                                                  &disableFPSLimitsTag);
                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                    "Failed to get vendor tag location for disableFPSLimits");
                if (CamxResultSuccess == result)
                {
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, disableFPSLimitsTag,
                                                           reinterpret_cast<VOID*>(&defaultDisableFPSlimitsValue),
                                                           1);
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                        "Failed to write vendor tag disableFPSLimits to default metadata.");
                }

                // sensor frame duration override
                UINT32 overrideSensorFrameDurationTag;
                INT64  defaultOverrideSensorFrameDurationValue = 0;

                result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.manualExposure",
                                                                  "overrideSensorFrameDuration",
                                                                  &overrideSensorFrameDurationTag);
                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                    "Failed to get vendor tag location for overrideSensorFrameDuration");
                if (CamxResultSuccess == result)
                {
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, overrideSensorFrameDurationTag,
                                                           reinterpret_cast<VOID*>(&defaultOverrideSensorFrameDurationValue),
                                                           1);
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                        "Failed to write vendor tag overrideSensorFrameDuration to default metadata.");
                }

                // Update resource cost validation
                UINT32 overrideResourceCostValidationTag;
                BYTE   overrideResourceCostValidation = 0;

                result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sessionParameters",
                                                                  "overrideResourceCostValidation",
                                                                  &overrideResourceCostValidationTag);
                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                    "Failed to get vendor tag location for overrideResourceCostValidation");
                if (CamxResultSuccess == result)
                {
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, overrideResourceCostValidationTag,
                                                           reinterpret_cast<VOID*>(&overrideResourceCostValidation),
                                                           1);
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                        "Failed to write vendor tag overrideResourceCostValidation to default metadata.");
                }

                // Disable EarlyPCRenable by default
                BYTE                  defaultEarlyPCRenableValue = 0;
                UINT32                earlyPCRenableVendorTag;
                result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.EarlyPCRenable", "EarlyPCRenable",
                                                                   &earlyPCRenableVendorTag);
                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag location for EarlyPCRenable");
                if (CamxResultSuccess == result)
                {
                    result = HAL3MetadataUtil::SetMetadata(pMetadata, earlyPCRenableVendorTag,
                                                           reinterpret_cast<VOID*>(&defaultEarlyPCRenableValue),
                                                           1);
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                                        "Failed to write vendor tag EarlyPCRenable to default metadata.");
                }

            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL,
                               "Invalid pointer(s): pMetadata=%p, cameraInfo.pPlatformCaps=%p, cameraInfo.pSensorCaps=%p",
                               pMetadata, cameraInfo.pPlatformCaps, cameraInfo.pSensorCaps);
            }
        }
        GetInstance(cameraId)->m_pMetadataTemplates[requestTemplateIndex] = pMetadata;
    }

    return GetInstance(cameraId)->m_pMetadataTemplates[requestTemplateIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::GetDefaultRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3DefaultRequest::GetDefaultRequest(
    DefaultRequest*              pDefaultRequest,
    const Camera3RequestTemplate requestTemplate,
    const HwCameraInfo*          pCameraInfo)
{
    CAMX_ASSERT(NULL != pDefaultRequest);
    CAMX_ASSERT(NULL != pCameraInfo);

    GetDefaultColorCorrectionRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultControlRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultEdgeRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultFlashRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultHotPixelRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultLensRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultNoiseReductionRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultRequestId(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultScalerRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultSensorRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultShadingRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultStatisticsRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultTonemapRequest(pDefaultRequest, requestTemplate, pCameraInfo);
    GetDefaultBlackLevelLockRequest(pDefaultRequest, requestTemplate, pCameraInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::GetDefaultColorCorrectionRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3DefaultRequest::GetDefaultColorCorrectionRequest(
    DefaultRequest*              pDefaultRequest,
    const Camera3RequestTemplate requestTemplate,
    const HwCameraInfo*          pCameraInfo)
{
    CAMX_ASSERT(NULL != pDefaultRequest);
    CAMX_ASSERT(NULL != pCameraInfo);

    // Default common request for all template type
    pDefaultRequest->colorCorrectionMode             = ColorCorrectionModeFast;
    pDefaultRequest->colorCorrectionAberrationMode   = ColorCorrectionAberrationModeOff;


    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numAbberationsModes; i++)
    {
        if (ColorCorrectionAberrationModeFast == pCameraInfo->pPlatformCaps->abberationModes[i])
        {
            pDefaultRequest->colorCorrectionAberrationMode = ColorCorrectionAberrationModeFast;
            break;
        }
    }

    // Update color correction mode
    if (RequestTemplateManual == requestTemplate)
    {
        pDefaultRequest->colorCorrectionMode = ColorCorrectionModeTransformMatrix;
    }

    Rational zero = { 0, 1 };
    Rational one = { 1, 1 };
    Matrix3x3Rational identity3x3Matrix =
    {
        {
            { one, zero, zero },
            { zero, one, zero },
            { zero, zero, one }
        }
    };
    pDefaultRequest->colorCorrectionTransform = identity3x3Matrix;

    // Update color correction aberration mode in following order
    // 1. Use ColorCorrectionAberrationModeHighQuality for still template if available,
    //    otherwise, try using ColorCorrectionAberrationModeFast
    // 2. Use ColorCorrectionAberrationModeFast for all other templates if available.
    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numAbberationsModes; i++)
    {
        if ((ColorCorrectionAberrationModeHighQuality == pCameraInfo->pPlatformCaps->abberationModes[i]) &&
            (RequestTemplateStillCapture == requestTemplate))
        {
            pDefaultRequest->colorCorrectionAberrationMode = ColorCorrectionAberrationModeHighQuality;
            break;
        }
        else if ((ColorCorrectionAberrationModeFast == pCameraInfo->pPlatformCaps->abberationModes[i]) &&
                 (RequestTemplateStillCapture != requestTemplate))
        {
            pDefaultRequest->colorCorrectionAberrationMode = ColorCorrectionAberrationModeFast;
            break;
        }
        else if (ColorCorrectionAberrationModeFast == pCameraInfo->pPlatformCaps->abberationModes[i])
        {
            pDefaultRequest->colorCorrectionAberrationMode = ColorCorrectionAberrationModeFast;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::GetDefaultControlRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3DefaultRequest::GetDefaultControlRequest(
    DefaultRequest*              pDefaultRequest,
    const Camera3RequestTemplate requestTemplate,
    const HwCameraInfo*          pCameraInfo)
{
    CAMX_ASSERT(NULL != pDefaultRequest);
    CAMX_ASSERT(NULL != pCameraInfo);

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    Region                activeArraySize = pCameraInfo->pSensorCaps->activeArraySize;

    if ((TRUE  == pCameraInfo->pSensorCaps->isQuadCFASensor) &&
        (FALSE == pStaticSettings->exposeFullSizeForQCFA))
    {
        activeArraySize = pCameraInfo->pSensorCaps->QuadCFAActiveArraySize;
    }

    // Default common request for all template type
    pDefaultRequest->AEAntibandingMode          = ControlAEAntibandingModeOff;
    pDefaultRequest->AEExposureCompensation     = pCameraInfo->pPlatformCaps->defaultAECompensationValue;
    pDefaultRequest->AELock                     = ControlAELockOff;
    pDefaultRequest->AEMode                     = ControlAEModeOn;
    pDefaultRequest->AERegions.xMin             = activeArraySize.xMin;
    pDefaultRequest->AERegions.yMin             = activeArraySize.yMin;
    pDefaultRequest->AERegions.xMax             = activeArraySize.xMin + activeArraySize.width;
    pDefaultRequest->AERegions.yMax             = activeArraySize.yMin + activeArraySize.height;
    pDefaultRequest->AERegions.weight           = 0;
    pDefaultRequest->AETargetFPSRange.min       = 0;
    pDefaultRequest->AETargetFPSRange.max       = 0;
    pDefaultRequest->AEPrecaptureTrigger        = ControlAEPrecaptureTriggerIdle;
    pDefaultRequest->AFMode                     = ControlAFModeAuto;


    pDefaultRequest->AFRegions.xMin   = activeArraySize.xMin;
    pDefaultRequest->AFRegions.yMin   = activeArraySize.yMin;
    pDefaultRequest->AFRegions.xMax   = activeArraySize.xMin + activeArraySize.width;
    pDefaultRequest->AFRegions.yMax   = activeArraySize.yMin + activeArraySize.height;
    pDefaultRequest->AFRegions.weight = 0;

    pDefaultRequest->AFTrigger                  = ControlAFTriggerIdle;
    pDefaultRequest->AWBLock                    = ControlAWBLockOff;
    pDefaultRequest->AWBMode                    = ControlAWBModeAuto;
    pDefaultRequest->AWBRegions.xMin            = activeArraySize.xMin;
    pDefaultRequest->AWBRegions.yMin            = activeArraySize.yMin;
    pDefaultRequest->AWBRegions.xMax            = activeArraySize.xMin + activeArraySize.width;
    pDefaultRequest->AWBRegions.yMax            = activeArraySize.yMin + activeArraySize.height;
    pDefaultRequest->AWBRegions.weight          = 0;
    pDefaultRequest->captureIntent              = ControlCaptureIntentEnd;
    pDefaultRequest->effectMode                 = ControlEffectModeOff;
    pDefaultRequest->controlMode                = ControlModeAuto;
    pDefaultRequest->sceneMode                  = ControlSceneModeDisabled;
    pDefaultRequest->videoStabilizationMode     = ControlVideoStabilizationModeOff;
    pDefaultRequest->postRawSensitivityBoost    = pCameraInfo->pPlatformCaps->minPostRawSensitivityBoost;
    pDefaultRequest->controlZslEnable           = FALSE;
    pDefaultRequest->colorCorrectionGains.red       =1.0f;
    pDefaultRequest->colorCorrectionGains.greenEven =1.0f;
    pDefaultRequest->colorCorrectionGains.greenOdd  =1.0f;
    pDefaultRequest->colorCorrectionGains.blue      =1.0f;
    pDefaultRequest->thumbnailSize                  = pCameraInfo->pPlatformCaps->defaultJPEGThumbnailSize;

    // Update AE antibanding mode
    // Use ControlAEAntibandingModeAuto mode for all templates if available
    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numAntibandingModes; i++)
    {
        if (ControlAEAntibandingModeAuto == pCameraInfo->pPlatformCaps->antibandingModes[i])
        {
            pDefaultRequest->AEAntibandingMode = ControlAEAntibandingModeAuto;
            break;
        }
    }

    // Update AE mode
    // Default AE mode should be ControlAEModeOn except for manual template
    if (RequestTemplateManual == requestTemplate)
    {
        pDefaultRequest->AEMode = ControlAEModeOff;
    }

    // Update AE target FPS range by following rule
    // 1. Use maximum available AE FPS range for preview, still, and ZSL template for better image quality
    // 2. Choose maximum fixed FPS range for video and other templates.
    INT32 maxFPS    = 0;
    INT32 maxRange  = 0;
    INT32 range     = 0;

    for (UINT i = 0; i < pCameraInfo->pHwEnvironmentCaps->numAETargetFPSRanges; i++)
    {
        range = pCameraInfo->pHwEnvironmentCaps->AETargetFPSRanges[i].max -
                pCameraInfo->pHwEnvironmentCaps->AETargetFPSRanges[i].min;

        if (RequestTemplatePreview          == requestTemplate ||
            RequestTemplateStillCapture     == requestTemplate ||
            RequestTemplateZeroShutterLag   == requestTemplate)
        {
            if (range > maxRange)
            {
                pDefaultRequest->AETargetFPSRange.min   = pCameraInfo->pHwEnvironmentCaps->AETargetFPSRanges[i].min;
                pDefaultRequest->AETargetFPSRange.max   = pCameraInfo->pHwEnvironmentCaps->AETargetFPSRanges[i].max;
                maxRange                                = range;
            }
        }
        else if (range == 0 &&
                 maxFPS < pCameraInfo->pHwEnvironmentCaps->AETargetFPSRanges[i].max)
        {
            pDefaultRequest->AETargetFPSRange.min   = pCameraInfo->pHwEnvironmentCaps->AETargetFPSRanges[i].min;
            pDefaultRequest->AETargetFPSRange.max   = pCameraInfo->pHwEnvironmentCaps->AETargetFPSRanges[i].max;
            maxFPS                                  = pCameraInfo->pHwEnvironmentCaps->AETargetFPSRanges[i].max;
        }
    }

    // Update AF mode by following order
    // 1. Always use ControlAFModeOff for manual template
    // 2. Use ControlAFModeContinuousPicture for preview, still, and ZSL template if available,
    //    otherwise, try using ControlAFModeAuto
    // 3. Use ControlAFModeContinuousVideo for video and video snapshot if available, otherwise, try using ControlAFModeAuto
    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numAFModes; i++)
    {
        if ((ControlAFModeOff       == pCameraInfo->pPlatformCaps->AFModes[i]) &&
            (RequestTemplateManual  == requestTemplate))
        {
            pDefaultRequest->AFMode = ControlAFModeOff;
            break;
        }
        if ((ControlAFModeContinuousPicture == pCameraInfo->pPlatformCaps->AFModes[i]) &&
            ((RequestTemplatePreview        == requestTemplate) ||
             (RequestTemplateStillCapture   == requestTemplate) ||
             (RequestTemplateZeroShutterLag == requestTemplate)))
        {
            if (TRUE == pCameraInfo->pSensorCaps->isFixedFocus)
            {
                pDefaultRequest->AFMode = ControlAFModeOff;
            }
            else
            {
                pDefaultRequest->AFMode = ControlAFModeContinuousPicture;
            }
            break;
        }
        else if ((ControlAFModeContinuousVideo  == pCameraInfo->pPlatformCaps->AFModes[i]) &&
                 ((RequestTemplateVideoRecord   == requestTemplate) ||
                  (RequestTemplateVideoSnapshot == requestTemplate)))
        {
            if (TRUE == pCameraInfo->pSensorCaps->isFixedFocus)
            {
                pDefaultRequest->AFMode = ControlAFModeOff;
            }
            else
            {
                pDefaultRequest->AFMode = ControlAFModeContinuousVideo;
            }
            break;
        }
        else if (ControlAFModeAuto == pCameraInfo->pPlatformCaps->AFModes[i])
        {
            if (TRUE == pCameraInfo->pSensorCaps->isFixedFocus)
            {
                pDefaultRequest->AFMode = ControlAFModeOff;
            }
            else
            {
                pDefaultRequest->AFMode = ControlAFModeAuto;
            }
        }
    }

    // Update AWB mode
    // Default AWB mode should be ControlAWBModeAuto except for manual template
    if (RequestTemplateManual == requestTemplate)
    {
        pDefaultRequest->AWBMode = ControlAWBModeOff;
    }

    // Update capture Intent
    switch (requestTemplate)
    {
        case RequestTemplatePreview:
            pDefaultRequest->captureIntent = ControlCaptureIntentPreview;
            break;
        case RequestTemplateStillCapture:
            pDefaultRequest->captureIntent = ControlCaptureIntentStillCapture;
            break;
        case RequestTemplateVideoRecord:
            pDefaultRequest->captureIntent = ControlCaptureIntentVideoRecord;
            break;
        case RequestTemplateVideoSnapshot:
            pDefaultRequest->captureIntent = ControlCaptureIntentVideoSnapshot;
            break;
        case RequestTemplateZeroShutterLag:
            pDefaultRequest->captureIntent = ControlCaptureIntentZeroShutterLag;
            break;
        case RequestTemplateManual:
            pDefaultRequest->captureIntent = ControlCaptureIntentManual;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid request template %d", requestTemplate);
            break;
    }

    // Update control mode
    // Default control mode should be ControlModeAuto except for manual template
    if (RequestTemplateManual == requestTemplate)
    {
        pDefaultRequest->controlMode = ControlModeOff;
    }

    // Update scene mode
    // Use ControlSceneModeFacePriority for all template if available
    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numSceneModes; i++)
    {
        if (ControlSceneModeFacePriority == pCameraInfo->pPlatformCaps->sceneModes[i])
        {
            pDefaultRequest->sceneMode = ControlSceneModeFacePriority;
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::GetDefaultEdgeRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3DefaultRequest::GetDefaultEdgeRequest(
    DefaultRequest*              pDefaultRequest,
    const Camera3RequestTemplate requestTemplate,
    const HwCameraInfo*          pCameraInfo)
{
    CAMX_ASSERT(NULL != pDefaultRequest);
    CAMX_ASSERT(NULL != pCameraInfo);

    pDefaultRequest->edgeMode = EdgeModeOff;

    // Update edge mode by following order
    // 1. Use EdgeModeZeroShutterLag for ZSL template if available, otherwise, try using EdgeModeFast
    // 2. Use EdgeModeHighQuality for still template if available, otherwise, try using EdgeModeFast
    // 3. Use EdgeModeFast for all other templates if available
    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numEdgeModes; i++)
    {
        if ((EdgeModeZeroShutterLag         == pCameraInfo->pPlatformCaps->edgeModes[i]) &&
            (RequestTemplateZeroShutterLag  == requestTemplate))
        {
            pDefaultRequest->edgeMode = EdgeModeZeroShutterLag;
            break;
        }
        else if ((EdgeModeHighQuality           == pCameraInfo->pPlatformCaps->edgeModes[i]) &&
                 (RequestTemplateStillCapture   == requestTemplate))
        {
            pDefaultRequest->edgeMode = EdgeModeHighQuality;
            break;
        }
        else if (EdgeModeFast == pCameraInfo->pPlatformCaps->edgeModes[i])
        {
            pDefaultRequest->edgeMode = EdgeModeFast;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::GetDefaultLensRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3DefaultRequest::GetDefaultLensRequest(
    DefaultRequest*              pDefaultRequest,
    const Camera3RequestTemplate requestTemplate,
    const HwCameraInfo*          pCameraInfo)
{
    CAMX_ASSERT(NULL != pDefaultRequest);
    CAMX_ASSERT(NULL != pCameraInfo);

    // Default common request
    pDefaultRequest->aperture                   = pCameraInfo->pSensorCaps->aperatures[0];
    pDefaultRequest->filterDensity              = pCameraInfo->pSensorCaps->NDFs[0];
    pDefaultRequest->focalLength                = pCameraInfo->pSensorCaps->focalLengths[0];
    pDefaultRequest->focalDistance              = pCameraInfo->pSensorCaps->minimumFocusDistance;
    pDefaultRequest->opticalStabilizationMode   = LensOpticalStabilizationModeOff;

    // Use LensOpticalStabilizationModeOn for preview, ZSL, video, and video snapshot template if OIS is available.
    if (TRUE == pCameraInfo->pSensorCaps->hasOIS)
    {
        if ((RequestTemplatePreview         == requestTemplate) ||
            (RequestTemplateVideoRecord     == requestTemplate) ||
            (RequestTemplateVideoSnapshot   == requestTemplate) ||
            (RequestTemplateStillCapture   == requestTemplate) ||
            (RequestTemplateZeroShutterLag  == requestTemplate))
        {
            pDefaultRequest->opticalStabilizationMode = LensOpticalStabilizationModeOn;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::GetDefaultNoiseReductionRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3DefaultRequest::GetDefaultNoiseReductionRequest(
    DefaultRequest*              pDefaultRequest,
    const Camera3RequestTemplate requestTemplate,
    const HwCameraInfo*          pCameraInfo)
{
    CAMX_ASSERT(NULL != pDefaultRequest);
    CAMX_ASSERT(NULL != pCameraInfo);

    pDefaultRequest->noiseRedutionMode = NoiseReductionModeOff;

    // Update noise redution mode by following order
    // 1. Use NoiseReductionModeZeroShutterLag for ZSL template if available, otherwise, try using NoiseReductionModeFast
    // 2. Use NoiseReductionModeHighQuality for still template if available, otherwise, try using NoiseReductionModeFast
    // 3. Use NoiseReductionModeFast for all other templates if available
    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numNoiseReductionModes; i++)
    {
        if ((NoiseReductionModeZeroShutterLag   == pCameraInfo->pPlatformCaps->noiseReductionModes[i]) &&
            (RequestTemplateZeroShutterLag      == requestTemplate))
        {
            pDefaultRequest->noiseRedutionMode = NoiseReductionModeZeroShutterLag;
            break;
        }
        else if ((NoiseReductionModeHighQuality == pCameraInfo->pPlatformCaps->noiseReductionModes[i]) &&
                 (RequestTemplateStillCapture   == requestTemplate))
        {
            pDefaultRequest->noiseRedutionMode = NoiseReductionModeHighQuality;
            break;
        }
        else if (NoiseReductionModeFast == pCameraInfo->pPlatformCaps->noiseReductionModes[i])
        {
            pDefaultRequest->noiseRedutionMode = NoiseReductionModeFast;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::GetDefaultShadingRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3DefaultRequest::GetDefaultShadingRequest(
    DefaultRequest*              pDefaultRequest,
    const Camera3RequestTemplate requestTemplate,
    const HwCameraInfo*          pCameraInfo)
{
    CAMX_ASSERT(NULL != pDefaultRequest);
    CAMX_ASSERT(NULL != pCameraInfo);

    pDefaultRequest->shadingMode = ShadingModeOff;

    // Update shading mode by following rule
    // 1. Use ShadingModeHighQuality for still template if available, otherwise, try using ShadingModeFast
    // 2. Use ShadingModeFast for all other templates if available
    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numShadingModes; i++)
    {
        if ((ShadingModeHighQuality         == pCameraInfo->pPlatformCaps->shadingModes[i]) &&
            (RequestTemplateStillCapture    == requestTemplate))
        {
            pDefaultRequest->shadingMode = ShadingModeHighQuality;
            break;
        }
        else if (ShadingModeFast == pCameraInfo->pPlatformCaps->shadingModes[i])
        {
            pDefaultRequest->shadingMode = ShadingModeFast;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::GetDefaultTonemapRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3DefaultRequest::GetDefaultTonemapRequest(
    DefaultRequest*              pDefaultRequest,
    const Camera3RequestTemplate requestTemplate,
    const HwCameraInfo*          pCameraInfo)
{
    CAMX_ASSERT(NULL != pDefaultRequest);
    CAMX_ASSERT(NULL != pCameraInfo);

    pDefaultRequest->tonemapMode = TonemapModeFast;

    // Use TonemapModeHighQuality for still template if available
    for (UINT i = 0; i < pCameraInfo->pPlatformCaps->numTonemapModes; i++)
    {
        if ((TonemapModeHighQuality         == pCameraInfo->pPlatformCaps->tonemapModes[i]) &&
            (RequestTemplateStillCapture    == requestTemplate))
        {
            pDefaultRequest->tonemapMode = TonemapModeHighQuality;
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3DefaultRequest::~HAL3DefaultRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3DefaultRequest::~HAL3DefaultRequest()
{
    for (UINT i = 0; i < RequestTemplateCount; i++)
    {
        if (NULL != m_pMetadataTemplates[i])
        {
            HAL3MetadataUtil::FreeMetadata(m_pMetadataTemplates[i]);
        }
    }
}

CAMX_NAMESPACE_END
