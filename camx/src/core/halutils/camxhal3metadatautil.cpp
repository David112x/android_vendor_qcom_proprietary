////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxhal3metadatautil.cpp
/// @brief Implements HAL3MetadataUtil methods. This will have Android camera_metadata signature all over
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <system/camera_metadata.h>

#include "camxhal3metadatatags.h"
#include "camxhal3metadatatagtypes.h"
#include "camxhal3metadatautil.h"
#include "camxhal3module.h"
#include "camxhwcontext.h"
#include "camxincs.h"
#include "camxpropertyblob.h"
#include "camxvendortags.h"

CAMX_NAMESPACE_BEGIN

// Definition of static variable declared in class
UINT32 HAL3MetadataUtil::s_allTags[MaxMetadataTags];

UINT32 HAL3MetadataUtil::s_vendorTags[MaxMetadataTags];

UINT32 HAL3MetadataUtil::s_staticTags[MaxMetadataTags];

SIZE_T HAL3MetadataUtil::s_tagOffset[MaxMetadataTags];

SIZE_T HAL3MetadataUtil::s_maxMetadataSize;

MetadataInfoTable HAL3MetadataUtil::s_metadataInfoTable;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetMetadataType
///
/// @brief Static method to convert camx Metadata to Android type
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CAMX_INLINE camera_metadata* GetMetadataType(
    Metadata* pMetadata)
{
    return static_cast<camera_metadata*>(pMetadata);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Android returns the following values from its camera_metadata APIs, but doesn't
// define them in header.So there is no option but to shadow them here.
static const INT32 MetadataUtilOK       = 0;
static const INT32 MetadataUtilError    = 1;
static const INT32 MetadataUtilNotFound = -ENOENT;

static const INT32 MaxFileLen = 256; ///< Maximum file length

// @brief: List of camera metadata tags
static const CameraMetadataTag g_cameraMetadataTags[] =
{
    ColorCorrectionMode,
    ColorCorrectionTransform,
    ColorCorrectionGains,
    ColorCorrectionAberrationMode,
    ColorCorrectionAvailableAberrationModes,

    ControlAEAntibandingMode,
    ControlAEExposureCompensation,
    ControlAELock,
    ControlAEMode,
    ControlAERegions,
    ControlAETargetFpsRange,
    ControlAEPrecaptureTrigger,
    ControlAFMode,
    ControlAFRegions,
    ControlAFTrigger,
    ControlAWBLock,
    ControlAWBMode,
    ControlAWBRegions,
    ControlCaptureIntent,
    ControlEffectMode,
    ControlMode,
    ControlSceneMode,
    ControlVideoStabilizationMode,
    ControlAEAvailableAntibandingModes,
    ControlAEAvailableModes,
    ControlAEAvailableTargetFPSRanges,
    ControlAECompensationRange,
    ControlAECompensationStep,
    ControlAFAvailableModes,
    ControlAvailableEffects,
    ControlAvailableSceneModes,
    ControlAvailableVideoStabilizationModes,
    ControlAWBAvailableModes,
    ControlMaxRegions,
    ControlSceneModeOverrides,
    ControlAEPrecaptureId,
    ControlAEState,
    ControlAFState,
    ControlAFTriggerId,
    ControlAWBState,
    ControlAvailableHighSpeedVideoConfigurations,
    ControlAELockAvailable,
    ControlAWBLockAvailable,
    ControlAvailableModes,
    ControlPostRawSensitivityBoostRange,
    ControlPostRawSensitivityBoost,
    ControlZslEnable,

    DemosaicMode,

    EdgeMode,
    EdgeStrength,
    EdgeAvailableEdgeModes,

    FlashFiringPower,
    FlashFiringTime,
    FlashMode,
    FlashColorTemperature,
    FlashMaxEnergy,
    FlashState,

    FlashInfoAvailable,
    FlashInfoChargeDuration,

    HotPixelMode,
    HotPixelAvailableHotPixelModes,

    JPEGGpsCoordinates,
    JPEGGpsProcessingMethod,
    JPEGGpsTimestamp,
    JPEGOrientation,
    JPEGQuality,
    JPEGThumbnailQuality,
    JPEGThumbnailSize,
    JPEGAvailableThumbnailSizes,
    JPEGMaxSize,
    JPEGSize,

    LensAperture,
    LensFilterDensity,
    LensFocalLength,
    LensFocusDistance,
    LensOpticalStabilizationMode,
    LensFacing,
    LensPoseRotation,
    LensPoseTranslation,
    LensFocusRange,
    LensState,
    LensIntrinsicCalibration,
    LensRadialDistortion,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    LensPoseReference,
    LensDistortion,
#endif // Android-P or better
    LensInfoAvailableApertures,
    LensInfoAvailableFilterDensities,
    LensInfoAvailableFocalLengths,
    LensInfoAvailableOpticalStabilization,
    LensInfoHyperfocalDistance,
    LensInfoMinimumFocusDistance,
    LensInfoShadingMapSize,
    LensInfoFocusDistanceCalibration,

    NoiseReductionMode,
    NoiseReductionStrength,
    NoiseReductionAvailableNoiseReductionModes,

    QuirksMeteringCropRegion,
    QuirksTriggerAFWithAuto,
    QuirksUseZslFormat,
    QuirksUsePartialResult,
    QuirksPartialResult,

    RequestFrameCount,
    RequestId,
    RequestInputStreams,
    RequestMetadataMode,
    RequestOutputStreams,
    RequestType,
    RequestMaxNumOutputStreams,
    RequestMaxNumReprocessStreams,
    RequestMaxNumInputStreams,
    RequestPipelineDepth,
    RequestPipelineMaxDepth,
    RequestPartialResultCount,
    RequestAvailableCapabilities,
    RequestAvailableRequestKeys,
    RequestAvailableResultKeys,
    RequestAvailableCharacteristicsKeys,
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    RequestAvailableSessionKeys,
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))

    ScalerCropRegion,
    ScalerAvailableFormats,
    ScalerAvailableJPEGMinDurations,
    ScalerAvailableJPEGSizes,
    ScalerAvailableMaxDigitalZoom,
    ScalerAvailableProcessedMinDurations,
    ScalerAvailableProcessedSizes,
    ScalerAvailableRawMinDurations,
    ScalerAvailableRawSizes,
    ScalerAvailableInputOutputFormatsMap,
    ScalerAvailableStreamConfigurations,
    ScalerAvailableMinFrameDurations,
    ScalerAvailableStallDurations,
    ScalerCroppingType,

    SensorExposureTime,
    SensorFrameDuration,
    SensorSensitivity,
    SensorReferenceIlluminant1,
    SensorReferenceIlluminant2,
    SensorCalibrationTransform1,
    SensorCalibrationTransform2,
    SensorColorTransform1,
    SensorColorTransform2,
    SensorForwardMatrix1,
    SensorForwardMatrix2,
    SensorBaseGainFactor,
    SensorBlackLevelPattern,
    SensorMaxAnalogSensitivity,
    SensorOrientation,
    SensorProfileHueSaturationMapDimensions,
    SensorTimestamp,
    SensorTemperature,
    SensorNeutralColorPoint,
    SensorNoiseProfile,
    SensorProfileHueSaturationMap,
    SensorProfileToneCurve,
    SensorGreenSplit,
    SensorTestPatternData,
    SensorTestPatternMode,
    SensorAvailableTestPatternModes,
    SensorRollingShutterSkew,
    SensorOpticalBlackRegions,
    SensorDynamicBlackLevel,
    SensorDynamicWhiteLevel,
    SensorOpaqueRawSize,

    SensorInfoActiveArraySize,
    SensorInfoSensitivityRange,
    SensorInfoColorFilterArrangement,
    SensorInfoExposureTimeRange,
    SensorInfoMaxFrameDuration,
    SensorInfoPhysicalSize,
    SensorInfoPixelArraySize,
    SensorInfoWhiteLevel,
    SensorInfoTimestampSource,
    SensorInfoLensShadingApplied,
    SensorInfoPreCorrectionActiveArraySize,

    ShadingMode,
    ShadingStrength,
    ShadingAvailableModes,

    StatisticsFaceDetectMode,
    StatisticsHistogramMode,
    StatisticsSharpnessMapMode,
    StatisticsHotPixelMapMode,
    StatisticsFaceIds,
    StatisticsFaceLandmarks,
    StatisticsFaceRectangles,
    StatisticsFaceScores,
    StatisticsHistogram,
    StatisticsSharpnessMap,
    StatisticsLensShadingCorrectionMap,
    StatisticsLensShadingMap,
    StatisticsPredictedColorGains,
    StatisticsPredictedColorTransform,
    StatisticsSceneFlicker,
    StatisticsHotPixelMap,
    StatisticsLensShadingMapMode,

    StatisticsInfoAvailableFaceDetectModes,
    StatisticsInfoHistogramBucketCount,
    StatisticsInfoMaxFaceCount,
    StatisticsInfoMaxHistogramCount,
    StatisticsInfoMaxSharpnessMapValue,
    StatisticsInfoSharpnessMapSize,
    StatisticsInfoAvailableHotPixelMapModes,
    StatisticsInfoAvailableLensShadingMapModes,

    TonemapCurveBlue,
    TonemapCurveGreen,
    TonemapCurveRed,
    TonemapMode,
    TonemapMaxCurvePoints,
    TonemapAvailableToneMapModes,
    TonemapGamma,
    TonemapPresetCurve,

    LedTransmit,
    LedAvailableLeds,

    InfoSupportedHardwareLevel,

    BlackLevelLock,

    SyncFrameNumber,
    SyncMaxLatency,

    ReprocessEffectiveExposureFactor,
    ReprocessMaxCaptureStall,

    DepthMaxDepthSamples,
    DepthAvailableDepthStreamConfigurations,
    DepthAvailableDepthMinFrameDurations,
    DepthAvailableDepthStallDurations,
    DepthDepthIsExclusive,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    LogicalMultiCameraPhysicalIDs,
    LogicalMultiCameraSensorSyncType,
    DistortionCorrectionMode,
    DistortionCorrectionAvailableModes,
    HEICAvailableHEICStreamConfigurations,
    HEICAvailableHEICMinFrameDurations,
    HEICAvailableHEICStallDurations,
    HEICInfoSupported,
    HEICInfoMaxJpegAppSegmentsCount,
#endif // Android-P or better
};

static UniformMetadataMap gUniformMetadataMap[]
{
    { PropertyIDAECFrameControl, PropertyIDUsecaseAECFrameControl, "org.quic.camera2.statsconfigs", "AECFrameControl", 0 },
    { PropertyIDAWBFrameControl, PropertyIDUsecaseAWBFrameControl, "org.quic.camera2.statsconfigs", "AWBFrameControl", 0 },
    { PropertyIDAFFrameControl,  PropertyIDUsecaseAFFrameControl,  "org.quic.camera2.statsconfigs", "AFFrameControl",  0 },
    { PropertyIDAECStatsControl, PropertyIDUsecaseAECStatsControl, "org.quic.camera2.statsconfigs", "AECStatsControl", 0 },
    { PropertyIDAWBStatsControl, PropertyIDUsecaseAWBStatsControl, "org.quic.camera2.statsconfigs", "AWBStatsControl", 0 },
    { PropertyIDAFStatsControl,  PropertyIDUsecaseAFStatsControl,  "org.quic.camera2.statsconfigs", "AFStatsControl",  0 },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::CalculateLinearOffsets
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::CalculateLinearOffsets()
{
    UINT32 numVendorTags = VendorTagManager::GetAllTagCount();
    UINT32 lastGTagIndex  = MaxMetadataTagCount - 1;

    // Populate table of linear offsets if every supported tag is used
    // Currently used for the sticky logic in the metadata pools.
    s_tagOffset[0] = 0;

    UINT align = static_cast<UINT>(alignof(MAXALIGN_T));

    for (UINT i = 0; i < MaxMetadataTagCount - 1; i++)
    {
        // Each tag is offset from the previous (s_tagOffset[i]) by its size (the factor of the below)
        s_tagOffset[i + 1] = Utils::ByteAlign(s_tagOffset[i] + (TagElements[i] *
                                                                TagFactor[i]   *
                                                                GetSizeByType(TagType[i])), align);
    }


    if (0 < numVendorTags)
    {
        UINT offset = MaxMetadataTagCount;


        // First one special, as it uses the sizes from the google tag tables
        s_tagOffset[offset] = Utils::ByteAlign(s_tagOffset[offset - 1] +
                                               (TagElements[lastGTagIndex] *
                                                TagFactor[lastGTagIndex]   *
                                                GetSizeByType(TagType[lastGTagIndex])), align);

        for (UINT i = 0; i < numVendorTags - 1; i++)
        {
            // last + sizeof vendor tag
            s_tagOffset[offset + 1] = Utils::ByteAlign(s_tagOffset[offset] +
                                                       VendorTagManager::VendorTagSize(i), align);
            offset++;
        }

        s_maxMetadataSize = Utils::ByteAlign(s_tagOffset[offset] +
                                             VendorTagManager::VendorTagSize(numVendorTags - 1), align);
    }
    else
    {
        s_maxMetadataSize = Utils::ByteAlign(s_tagOffset[lastGTagIndex] +
                                             (TagElements[lastGTagIndex] *
                                              TagFactor[lastGTagIndex]   *
                                              GetSizeByType(TagType[lastGTagIndex])), align);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetTagCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 HAL3MetadataUtil::GetTagCount(
    UINT32  visibility)
{
    return VendorTagManager::GetTagCount(static_cast<TagSectionVisibility>(visibility));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetAllTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::GetAllTags(
    UINT32* pVendorTags,
    UINT32  visibility)
{
    return VendorTagManager::GetAllTags(static_cast<VendorTag *>(pVendorTags),
        static_cast<TagSectionVisibility>(visibility));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::CalculateSizeAllMeta
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::CalculateSizeAllMeta(
    SIZE_T* pEntryCapacity,
    SIZE_T* pDataSize,
    UINT32  visibility)
{
    CamxResult                result          = CamxResultSuccess;
    UINT32                    i               = 0;
    UINT32                    j               = 0;
    UINT32                    k               = 0;
    SIZE_T                    smallIndex      = 0;
    SIZE_T                    bigIndex        = 0;
    UINT32                    vendorTagCount  = 0;
    StaticMetadataKeysInfo    requestKeysInfo = { 0 };
    StaticMetadataKeysInfo    resultKeysInfo  = { 0 };
    const HwEnvironment*      pHwEnvironment  = HwEnvironment::GetInstance();

    result = pHwEnvironment->GetStaticMetadataKeysInfo(&requestKeysInfo, RequestAvailableRequestKeys);

    if (CamxResultSuccess == result)
    {
        result = pHwEnvironment->GetStaticMetadataKeysInfo(&resultKeysInfo, RequestAvailableResultKeys);
    }

    if (CamxResultSuccess == result)
    {
        SIZE_T                    requestKeySize    = requestKeysInfo.numKeys;
        SIZE_T                    resultKeySize     = resultKeysInfo.numKeys;

        const CameraMetadataTag*  pRequestKeys      = requestKeysInfo.pStaticKeys;
        const CameraMetadataTag*  pResultKeys       = resultKeysInfo.pStaticKeys;
        const CameraMetadataTag*  pSmallArray       = NULL;
        const CameraMetadataTag*  pBigArray         = NULL;

        // Total size reserved for metadata tags will be the superset of the sizes of Request and Result tags + Vendor tags

        if (requestKeySize > resultKeySize)
        {
            bigIndex    = requestKeySize;
            smallIndex  = resultKeySize;
            pBigArray   = &pRequestKeys[0];
            pSmallArray = &pResultKeys[0];
        }
        else
        {
            bigIndex    = resultKeySize;
            smallIndex  = requestKeySize;
            pBigArray   = &pResultKeys[0];
            pSmallArray = &pRequestKeys[0];
        }

        *pEntryCapacity = bigIndex;
        *pDataSize = 0;
        // Initially, pDataSize = sum of max sizes of all keys in big array
        for (i = 0; i < bigIndex; i++)
        {
            *pDataSize += GetMaxSizeByTag(pBigArray[i]);
            HAL3MetadataUtil::s_allTags[i] = static_cast<UINT32>(pBigArray[i]);
        }

        for (i = 0; i < smallIndex; i++)
        {
            for (j = 0; j < bigIndex; j++)
            {
                if (pSmallArray[i] == pBigArray[j])
                {
                    break;
                }
            }
            if (j == bigIndex)
            {
                // pDataSize += max size of key at pSmallArray[i]
                *pDataSize += GetMaxSizeByTag(pSmallArray[i]);
                s_allTags[*pEntryCapacity] = static_cast<UINT32>(pSmallArray[i]);
                (*pEntryCapacity)++;
            }
        }

        vendorTagCount = VendorTagManager::GetTagCount(static_cast<TagSectionVisibility>(visibility));
        CAMX_ASSERT(MaxMetadataTags >= vendorTagCount);

        if ((vendorTagCount > 0) &&
            (vendorTagCount < MaxMetadataTags))
        {
            VendorTagManager::GetAllTags(s_vendorTags, static_cast<TagSectionVisibility>(visibility));
            for (k = 0; k < vendorTagCount; k++)
            {
                HAL3MetadataUtil::s_allTags[*pEntryCapacity] = static_cast<UINT32>(s_vendorTags[k]);
                (*pEntryCapacity)++;
            }
            s_metadataInfoTable.totalVendorTags = vendorTagCount;
        }
        else
        {
            if (vendorTagCount >= MaxMetadataTags)
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "VendorTagCount :%d not matching criteria", vendorTagCount);
            }
        }

        CAMX_ASSERT(MaxMetadataTags >= *pEntryCapacity);

        // Now we add the Vendor Tag Blob
        *pDataSize      += VendorTagManager::GetVendorTagBlobSize(static_cast<TagSectionVisibility>(visibility));

        // Also populate Mapped IDs
        VendorTagManager::PopulateControlVendorTagId();
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetGroupName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* HAL3MetadataUtil::GetGroupName(
    UINT32 tag)
{
    const CHAR* pGroupName;

    switch (tag & PropertyGroupMask)
    {
        case InputMetadataSectionMask:
            pGroupName = "Input";
            break;
        case UsecaseMetadataSectionMask:
            pGroupName = "Usecase";
            break;
        case StaticMetadataSectionMask:
            pGroupName = "Static";
            break;
        case PropertyIDPerFrameResultBegin:
            pGroupName = "PropertyResult";
            break;
        case PropertyIDPerFrameInternalBegin:
            pGroupName = "PropertyInternal";
            break;
        case PropertyIDUsecaseBegin:
            pGroupName = "PropertyUsecase";
            break;
        case PropertyIDPerFrameDebugDataBegin:
            pGroupName = "PropertyDebug";
            break;
        default:
            pGroupName = "";
            break;
    }

    return pGroupName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::DumpMetadataTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::DumpMetadataTable()
{
    CHAR  dumpFilename[MaxFileLen];
    OsUtils::SNPrintF(dumpFilename,
        sizeof(dumpFilename),
        "%s/metadata/metadatatable.txt",
        ConfigFileDirectory);

    FILE* pFrameFile = OsUtils::FOpen(dumpFilename, "wb");
    if (NULL != pFrameFile)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "Metadata Table... file name %s", dumpFilename);

        for (UINT32 tagIndex = 0; tagIndex < s_metadataInfoTable.totalMetadataTags; ++tagIndex)
        {
            OsUtils::FPrintF(pFrameFile, "TagID %x \t TagName %s \t\t\t Size %u \t Type %u \t TagType %u \n",
                s_metadataInfoTable.metadataInfoArray[tagIndex].tag,
                s_metadataInfoTable.metadataInfoArray[tagIndex].tagName,
                s_metadataInfoTable.metadataInfoArray[tagIndex].size,
                s_metadataInfoTable.metadataInfoArray[tagIndex].type,
                s_metadataInfoTable.metadataInfoArray[tagIndex].tagType);
        }
        OsUtils::FClose(pFrameFile);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "Cannot dump Metadata Table... file name %s", dumpFilename);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::InitializeMetadataTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3MetadataUtil::InitializeMetadataTable()
{
    CamxResult result   = CamxResultSuccess;
    UINT32     tagIndex = 0;

    UINT32 propertyBegin[] =
    {
        PropertyIDPerFrameResultBegin,
        PropertyIDPerFrameInternalBegin,
        PropertyIDUsecaseBegin,
        PropertyIDPerFrameDebugDataBegin
    };

    UINT32 propertyEnd[] =
    {
        PropertyIDPerFrameResultEnd,
        PropertyIDPerFrameInternalEnd,
        PropertyIDUsecaseEnd,
        PropertyIDPerFrameDebugDataEnd
    };

    s_metadataInfoTable.totalPropertyTags = 0;
    for (UINT32 index = 0; index < CAMX_ARRAY_SIZE(propertyBegin); ++index)
    {
        s_metadataInfoTable.totalPropertyTags += propertyEnd[index] - propertyBegin[index] + 1;
    }
    CAMX_ASSERT(MaxMetadataTags >= s_metadataInfoTable.totalPropertyTags);

    s_metadataInfoTable.totalCameraTags = CAMX_ARRAY_SIZE(g_cameraMetadataTags);

    s_metadataInfoTable.totalVendorTags = VendorTagManager::GetTagCount(TagSectionVisibility::TagSectionVisibleToAll);
    CAMX_ASSERT(MaxMetadataTags >= s_metadataInfoTable.totalVendorTags);

    if (s_metadataInfoTable.totalVendorTags < MaxMetadataTags)
    {
        VendorTagManager::GetAllTags(s_vendorTags, TagSectionVisibility::TagSectionVisibleToAll);
    }

    s_metadataInfoTable.totalMetadataTags = s_metadataInfoTable.totalCameraTags +
        s_metadataInfoTable.totalVendorTags +
        s_metadataInfoTable.totalPropertyTags;

    CAMX_ASSERT(MaxMetadataTags > s_metadataInfoTable.totalMetadataTags);

    if (MaxMetadataTags > s_metadataInfoTable.totalMetadataTags)
    {
        s_metadataInfoTable.totalCameraTagSize = 0;
        for (UINT32 count = 0; count < s_metadataInfoTable.totalCameraTags; ++count)
        {
            UINT32 tag = g_cameraMetadataTags[count];

            GetNameByTag(s_metadataInfoTable.metadataInfoArray[tagIndex].tagName,
                sizeof(s_metadataInfoTable.metadataInfoArray[tagIndex].tagName),
                tag);

            s_metadataInfoTable.metadataInfoArray[tagIndex].index       = tagIndex;
            s_metadataInfoTable.metadataInfoArray[tagIndex].tagType     = AndroidCameraTag;
            s_metadataInfoTable.metadataInfoArray[tagIndex].pGroupName  = GetGroupName(tag);
            s_metadataInfoTable.metadataInfoArray[tagIndex].tag         = tag;
            s_metadataInfoTable.metadataInfoArray[tagIndex].size        = static_cast<UINT32>(GetMaxSizeByTag(tag));
            s_metadataInfoTable.metadataInfoArray[tagIndex].type        = HAL3MetadataUtil::GetTypeByTag(tag);
            s_metadataInfoTable.metadataInfoArray[tagIndex].visibility  = TagSectionVisibility::TagSectionVisibleToAll;

            CAMX_ASSERT(CAMX_ARRAY_SIZE(TagSizeByType) > s_metadataInfoTable.metadataInfoArray[tagIndex].type);

            s_metadataInfoTable.metadataInfoArray[tagIndex].count = s_metadataInfoTable.metadataInfoArray[tagIndex].size /
                TagSizeByType[s_metadataInfoTable.metadataInfoArray[tagIndex].type];

            s_metadataInfoTable.totalCameraTagSize += s_metadataInfoTable.metadataInfoArray[tagIndex].size;
            tagIndex++;
        }

        s_metadataInfoTable.totalVendorTagSize = 0;
        for (UINT32 count = 0; count < s_metadataInfoTable.totalVendorTags; ++count)
        {
            UINT32 tag = s_vendorTags[count];

            GetNameByTag(s_metadataInfoTable.metadataInfoArray[tagIndex].tagName,
                sizeof(s_metadataInfoTable.metadataInfoArray[tagIndex].tagName),
                tag);

            s_metadataInfoTable.metadataInfoArray[tagIndex].index       = tagIndex;
            s_metadataInfoTable.metadataInfoArray[tagIndex].tagType     = CamXVendorTag;
            s_metadataInfoTable.metadataInfoArray[tagIndex].pGroupName  = GetGroupName(tag);
            s_metadataInfoTable.metadataInfoArray[tagIndex].tag         = tag;
            s_metadataInfoTable.metadataInfoArray[tagIndex].size        = static_cast<UINT32>(GetMaxSizeByTag(tag));
            s_metadataInfoTable.metadataInfoArray[tagIndex].type        = HAL3MetadataUtil::GetTypeByTag(tag);
            s_metadataInfoTable.metadataInfoArray[tagIndex].visibility  = VendorTagManager::GetTagVisibility(tag);

            CAMX_ASSERT(CAMX_ARRAY_SIZE(TagSizeByType) > s_metadataInfoTable.metadataInfoArray[tagIndex].type);

            s_metadataInfoTable.metadataInfoArray[tagIndex].count = s_metadataInfoTable.metadataInfoArray[tagIndex].size /
                TagSizeByType[s_metadataInfoTable.metadataInfoArray[tagIndex].type];

            s_metadataInfoTable.totalVendorTagSize += s_metadataInfoTable.metadataInfoArray[tagIndex].size;
            tagIndex++;
        }

        s_metadataInfoTable.totalPropertySize = 0;
        for (UINT32 count = 0; count < CAMX_ARRAY_SIZE(propertyBegin); ++count)
        {
            for (UINT32 tag = propertyBegin[count]; tag <= propertyEnd[count]; ++tag)
            {
                GetNameByTag(s_metadataInfoTable.metadataInfoArray[tagIndex].tagName,
                    sizeof(s_metadataInfoTable.metadataInfoArray[tagIndex].tagName),
                    tag);

                s_metadataInfoTable.metadataInfoArray[tagIndex].index       = tagIndex;
                s_metadataInfoTable.metadataInfoArray[tagIndex].tagType     = CamXProperty;
                s_metadataInfoTable.metadataInfoArray[tagIndex].pGroupName  = GetGroupName(tag);
                s_metadataInfoTable.metadataInfoArray[tagIndex].tag         = tag;
                s_metadataInfoTable.metadataInfoArray[tagIndex].size        = static_cast<UINT32>(GetPropertyTagSize(tag));
                s_metadataInfoTable.metadataInfoArray[tagIndex].type        = TYPE_BYTE;
                s_metadataInfoTable.metadataInfoArray[tagIndex].visibility  = TagSectionVisibility::TagSectionVisibleToOEM;

                CAMX_ASSERT(CAMX_ARRAY_SIZE(TagSizeByType) > s_metadataInfoTable.metadataInfoArray[tagIndex].type);

                s_metadataInfoTable.metadataInfoArray[tagIndex].count = s_metadataInfoTable.metadataInfoArray[tagIndex].size;

                s_metadataInfoTable.totalPropertySize += s_metadataInfoTable.metadataInfoArray[tagIndex].size;
                tagIndex++;
            }
        }

        s_metadataInfoTable.totalMetadataSize = s_metadataInfoTable.totalCameraTagSize +
            s_metadataInfoTable.totalVendorTagSize +
            s_metadataInfoTable.totalPropertySize;

        s_metadataInfoTable.totalTriggerTags = PropertyIDPerFrameResultEnd - PropertyIDNodeComplete0 + 1;

        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "METADATA TABLE: Total tagCount %u camera %u vendortag %u property %u trigger %u"
            " Total size %u camera %u vendortag %u property %u",
            s_metadataInfoTable.totalMetadataTags,
            s_metadataInfoTable.totalCameraTags,
            s_metadataInfoTable.totalPropertyTags,
            s_metadataInfoTable.totalVendorTags,
            s_metadataInfoTable.totalTriggerTags,
            s_metadataInfoTable.totalMetadataSize,
            s_metadataInfoTable.totalCameraTagSize,
            s_metadataInfoTable.totalPropertySize,
            s_metadataInfoTable.totalVendorTagSize);

        DumpMetadataTable();
    }

    // Update property indices
    s_metadataInfoTable.mainPropertyIndex = s_metadataInfoTable.totalCameraTags + s_metadataInfoTable.totalVendorTags;
    s_metadataInfoTable.internalPropertyIndex = s_metadataInfoTable.mainPropertyIndex +
        (PropertyIDPerFrameResultEnd - PropertyIDPerFrameResultBegin + 1);
    s_metadataInfoTable.usecasePropertyIndex = s_metadataInfoTable.internalPropertyIndex +
        (PropertyIDPerFrameInternalEnd - PropertyIDPerFrameInternalBegin + 1);
    s_metadataInfoTable.debugPropertyIndex = s_metadataInfoTable.usecasePropertyIndex +
        (PropertyIDUsecaseEnd - PropertyIDUsecaseBegin + 1);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetMetadataInfoByTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const MetadataInfo* HAL3MetadataUtil::GetMetadataInfoByTag(
    UINT32  tag)
{
    const MetadataInfo* pMetadataInfo   = NULL;
    UINT32              tagIndex        = GetUniqueIndexByTag(tag);
    if (s_metadataInfoTable.totalMetadataTags > tagIndex)
    {
        pMetadataInfo = GetMetadataInfoByIndex(tagIndex);
    }
    else
    {
        pMetadataInfo = GetMetadataInfoByIndex(0);
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid tag %x", tag);
    }

    return pMetadataInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::CalculateSizeStaticMeta
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::CalculateSizeStaticMeta(
    SIZE_T* pEntryCapacity,
    SIZE_T* pDataSize,
    UINT32 visibility)
{
    CamxResult              result                  = CamxResultSuccess;
    StaticMetadataKeysInfo  characteristicsKeysInfo = { 0 };
    UINT32                  vendorTagCount          = 0;
    UINT32                  k                       = 0;
    const HwEnvironment*    pHwEnvironment          = HwEnvironment::GetInstance();

    result = pHwEnvironment->GetStaticMetadataKeysInfo(&characteristicsKeysInfo, RequestAvailableCharacteristicsKeys);

    if (CamxResultSuccess == result)
    {
        SIZE_T                    characteristicsKeysSize = characteristicsKeysInfo.numKeys;
        const CameraMetadataTag*  pCharacteristicsKeys    = characteristicsKeysInfo.pStaticKeys;

        *pEntryCapacity = 0;
        *pDataSize      = 0;

        // Initially, pDataSize = sum of max sizes of all keys in big array
        for (UINT32 i = 0; i < characteristicsKeysSize; i++)
        {
            (*pEntryCapacity)++;
            *pDataSize += GetMaxSizeByTag(pCharacteristicsKeys[i]);
            HAL3MetadataUtil::s_staticTags[i] = static_cast<UINT32>(pCharacteristicsKeys[i]);
        }

        vendorTagCount = VendorTagManager::GetTagCount(static_cast<TagSectionVisibility>(visibility));
        CAMX_ASSERT(MaxMetadataTags - characteristicsKeysSize > vendorTagCount);
        if ((vendorTagCount > 0) &&
            (vendorTagCount < MaxMetadataTags))
        {
            VendorTagManager::GetAllTags(s_vendorTags, static_cast<TagSectionVisibility>(visibility));
            for (k = 0; k < vendorTagCount; k++)
            {
                HAL3MetadataUtil::s_allTags[*pEntryCapacity] = static_cast<UINT32>(s_vendorTags[k]);
                (*pEntryCapacity)++;
            }
        }
        else
        {
            if (vendorTagCount >= MaxMetadataTags)
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "VendorTagCount :%d not matching criteria", vendorTagCount);
            }
        }

        CAMX_ASSERT(MaxMetadataTags >= *pEntryCapacity);

        // Now we add the Vendor Tag Blob
        *pDataSize += VendorTagManager::GetVendorTagBlobSize(static_cast<TagSectionVisibility>(visibility));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::CreateMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Metadata* HAL3MetadataUtil::CreateMetadata(
    SIZE_T entryCapacity,
    SIZE_T dataSize)
{
    Metadata* pMetadata = allocate_camera_metadata(entryCapacity, dataSize);

    return pMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::FreeMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::FreeMetadata(
    Metadata* pMetadata)
{
    if (NULL != pMetadata)
    {
        free_camera_metadata(GetMetadataType(pMetadata));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::ResetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::ResetMetadata(
    Metadata* pMetadata,
    SIZE_T    dataCapacity,
    SIZE_T    entryCapacity)
{
    place_camera_metadata(pMetadata, 0xffffffff, entryCapacity, dataCapacity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::CopyMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3MetadataUtil::CopyMetadata(
    Metadata*   pDstMetadata,
    Metadata*   pSrcMetadata,
    UINT32      visibility)
{
    CamxResult                result          = CamxResultSuccess;
    camera_metadata_entry_t   entry;
    StaticMetadataKeysInfo    resultKeysInfo  = { 0 };
    StaticMetadataKeysInfo    requestKeysInfo = { 0 };
    const HwEnvironment*      pHwEnvironment  = HwEnvironment::GetInstance();

    if (NULL == pSrcMetadata || NULL == pDstMetadata)
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid Argument");
        return CamxResultEFailed;
    }

    camera_metadata* pMetadata        = GetMetadataType(pDstMetadata);
    SIZE_T           entryCapacity    = get_camera_metadata_entry_capacity(pMetadata);
    SIZE_T           dataCapacity     = get_camera_metadata_data_capacity(pMetadata);

    if ((get_camera_metadata_size(GetMetadataType(pSrcMetadata)) <
        get_camera_metadata_size(GetMetadataType(pDstMetadata))) ||
        (entryCapacity < get_camera_metadata_entry_count(GetMetadataType(pSrcMetadata))))
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid Argument. Size Invalid");
        return CamxResultEFailed;
    }

    place_camera_metadata(pMetadata,
            get_camera_metadata_size(pMetadata),
            get_camera_metadata_entry_capacity(pMetadata),
            get_camera_metadata_data_capacity(pMetadata));
    for (UINT32 i =0; i < get_camera_metadata_entry_count(GetMetadataType(pSrcMetadata)); i++)
    {
        result = get_camera_metadata_entry(static_cast<camera_metadata_t *>(pSrcMetadata), i, &entry);
        if (result == CamxResultSuccess)
        {
            UINT32 tag = entry.tag;
            if ((FALSE == VendorTagManager::isVendorTag(tag)) ||
                    Utils::IsBitMaskSet(visibility, (VendorTagManager::GetTagVisibility(tag))) ||
                    (static_cast<TagSectionVisibility>(VendorTagManager::GetTagVisibility(tag)) == TagSectionVisibleToAll))
            {
                VOID* pData = NULL;

                switch(entry.type)
                {
                    case TYPE_BYTE:
                        pData      = static_cast<VOID*>(entry.data.u8);
                        break;
                    case TYPE_INT32:
                        pData = static_cast<VOID*>(entry.data.i32);
                        break;
                    case TYPE_FLOAT:
                        pData = static_cast<VOID*>(entry.data.f);
                        break;
                    case TYPE_INT64:
                        pData = static_cast<VOID*>(entry.data.i64);
                        break;
                    case TYPE_DOUBLE:
                        pData = static_cast<VOID*>(entry.data.d);
                        break;
                    case TYPE_RATIONAL:
                        pData = static_cast<VOID*>(entry.data.r);
                        break;
                    default:
                        result = CamxResultEFailed;
                        break;
                }

                if (result != CamxResultEFailed && pData != NULL)
                {
                    result = add_camera_metadata_entry(pMetadata, tag, pData, entry.count);
                }
                if (result != CamxResultSuccess)
                {
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "Could not copy metadata");
                    // Error Copy
                    break;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3MetadataUtil::GetMetadata(
    Metadata*   pMetadata,
    UINT32      tag,
    VOID**      ppData)
{
    camera_metadata_entry_t entry;
    CamxResult              result = CamxResultEFailed;

    *ppData = NULL;

    // Remove type identifer from tag
    tag &= ~DriverInternalGroupMask;

    if (MetadataUtilOK == find_camera_metadata_entry(GetMetadataType(pMetadata), tag, &entry))
    {
        switch(entry.type)
        {
            case TYPE_BYTE:
                *ppData = static_cast<VOID*>(entry.data.u8);
                break;
            case TYPE_INT32:
                *ppData = static_cast<VOID*>(entry.data.i32);
                break;
            case TYPE_FLOAT:
                *ppData = static_cast<VOID*>(entry.data.f);
                break;
            case TYPE_INT64:
                *ppData = static_cast<VOID*>(entry.data.i64);
                break;
            case TYPE_DOUBLE:
                *ppData = static_cast<VOID*>(entry.data.d);
                break;
            case TYPE_RATIONAL:
                *ppData = static_cast<VOID*>(entry.data.r);
                break;
            default:
                break;
        }
        result = CamxResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetMetadataCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T HAL3MetadataUtil::GetMetadataCount(
    Metadata*   pMetadata,
    UINT32      tag)
{
    camera_metadata_entry_t entry;
    SIZE_T                  count = 0;

    // Remove type identifer from tag
    tag &= ~DriverInternalGroupMask;

    if (MetadataUtilOK == find_camera_metadata_entry(GetMetadataType(pMetadata), tag, &entry))
    {
        count = entry.count;
    }
    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetMetadataByIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::GetMetadataByIndex(
    Metadata*   pMetadata,
    SIZE_T      index,
    UINT32*     pTag,
    SIZE_T*     pCount,
    VOID**      ppData)
{
    camera_metadata_entry_t entry = {0};

    *ppData = NULL;
    *pTag   = MaxMetadataTags;

    if (MetadataUtilOK == get_camera_metadata_entry(static_cast<camera_metadata*>(pMetadata), index, &entry))
    {
        *pTag   = entry.tag;
        *pCount = entry.count;

        UINT8 type = HAL3MetadataUtil::GetTypeByTag(entry.tag);

        switch(type)
        {
            case TYPE_BYTE:
                *ppData = static_cast<VOID*>(entry.data.u8);
                break;
            case TYPE_INT32:
                *ppData = static_cast<VOID*>(entry.data.i32);
                break;
            case TYPE_FLOAT:
                *ppData = static_cast<VOID*>(entry.data.f);
                break;
            case TYPE_INT64:
                *ppData = static_cast<VOID*>(entry.data.i64);
                break;
            case TYPE_DOUBLE:
                *ppData = static_cast<VOID*>(entry.data.d);
                break;
            case TYPE_RATIONAL:
                *ppData = static_cast<VOID*>(entry.data.r);
                break;
            default:
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::MergeMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3MetadataUtil::MergeMetadata(
    Metadata*       pDstMetadata,
    const Metadata* pSrcMetadata)
{
    CamxResult         result             = CamxResultSuccess;
    INT                status             = 0;
    camera_metadata_t* pMergeSrcMetadata  =
        // NOWHINE CP036a: Non-const required by standard function
        const_cast<camera_metadata_t*>(static_cast<const camera_metadata_t*>(pSrcMetadata));
    camera_metadata_t* pMergeDstMetadata  =
        // NOWHINE CP036a: Non-const required by standard function
        const_cast<camera_metadata_t*>(static_cast<const camera_metadata_t*>(pDstMetadata));

    SIZE_T totalEntries = get_camera_metadata_entry_count(pMergeSrcMetadata);

    for (UINT i = 0; i < totalEntries; i++)
    {
        camera_metadata_entry_t srcEntry;
        camera_metadata_entry_t dstEntry;
        camera_metadata_entry_t updatedEntry;

        get_camera_metadata_entry(pMergeSrcMetadata, i, &srcEntry);

        status = find_camera_metadata_entry(pMergeDstMetadata, srcEntry.tag, &dstEntry);

        if (MetadataUtilOK != status)
        {
            status = add_camera_metadata_entry(GetMetadataType(pMergeDstMetadata),
                                               srcEntry.tag,
                                               srcEntry.data.i32,
                                               srcEntry.count);
        }
        else
        {
            status = update_camera_metadata_entry(GetMetadataType(pMergeDstMetadata),
                                                  dstEntry.index,
                                                  srcEntry.data.i32,
                                                  srcEntry.count,
                                                  &updatedEntry);
        }

        if (MetadataUtilOK != status)
        {
            result = CamxResultEFailed;
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::AppendMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3MetadataUtil::AppendMetadata(
    Metadata*       pDstMetadata,
    const Metadata* pSrcMetadata)
{
    CamxResult result = CamxResultSuccess;
    INT status;
    // NOWHINE CP036a: Since google function is non-const, had to add the const_cast
    status = append_camera_metadata(reinterpret_cast<camera_metadata_t*>(const_cast<Metadata*>(pDstMetadata)),
                                    static_cast<const camera_metadata_t*>(pSrcMetadata));

    if (status != 0)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Could not add append src metadata to dst with status = %d", status);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::UpdateMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3MetadataUtil::UpdateMetadata(
    Metadata*   pMetadata,
    UINT32      tag,
    const VOID* pData,
    SIZE_T      count,
    BOOL        updateAllowed)
{
    INT32       status  = MetadataUtilError;
    CamxResult  result  = CamxResultEFailed;

    // Remove type identifer from tag
    tag &= ~DriverInternalGroupMask;

    INT32       type       = get_camera_metadata_tag_type(tag);
    SIZE_T      bufferSize = get_camera_metadata_size(GetMetadataType(pMetadata));
    UINTPTR_T   bufAddr    = reinterpret_cast<UINTPTR_T>(pMetadata);
    UINTPTR_T   dataAddr   = reinterpret_cast<UINTPTR_T>(pData);

    if ((-1 == type) ||
        ((dataAddr > bufAddr) && (dataAddr < (bufAddr + bufferSize))) ||
        (0 == GetSizeByType(GetTypeByTag(tag))))
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot set Metadata for Tag: 0x%x ", tag);
        return result;
    }

    camera_metadata_entry_t entry;

    // We do not need to worry about the size of the overall metadata, as we have already allocated max size
    CAMX_ASSERT_MESSAGE(count <= (GetMaxSizeByTag(tag) / GetSizeByType(GetTypeByTag(tag))),
                        "Tag %08x (type: %d, index: %d) too large for allocated size: tag size: %d, tag type size: %d, "
                        "count %d > elements allocated: %d",
                        tag, GetTypeByTag(tag), CAMX_TAG_TO_INDEX(tag),
                        GetMaxSizeByTag(tag), GetSizeByType(GetTypeByTag(tag)),
                        count, GetMaxSizeByTag(tag) / GetSizeByType(GetTypeByTag(tag)));

    status = find_camera_metadata_entry(GetMetadataType(pMetadata), tag, &entry);
    // As previous reference to metadata are invalidated, if a meta value change from nonzero to zero sized data,
    // add metadata with count 0.
    if ((MetadataUtilNotFound == status))
    {
        status = add_camera_metadata_entry(GetMetadataType(pMetadata), tag, pData, count);
    }
    else if (MetadataUtilOK == status)
    {
        if (FALSE == updateAllowed)
        {
            SIZE_T sizeOfTag = GetSizeByType(GetTypeByTag(tag));

            if ((count != entry.count) ||
                (0 != Utils::Memcmp(reinterpret_cast<VOID*>(entry.data.u8), pData, sizeOfTag * count)))
            {
                CAMX_LOG_META( "DUPLICATE tag write with different data for %08x!", tag);

                CAMX_LOG_META( "=======");
                for (UINT i = 0; i < count; i++)
                {
                    switch(sizeOfTag)
                    {
                        case 1:
                            CAMX_LOG_META(
                                           " %016hhx | %016hhx",
                                           reinterpret_cast<const BYTE*>(entry.data.u8)[i],
                                           reinterpret_cast<const BYTE*>(pData)[i]);
                            break;
                        case 4:
                            CAMX_LOG_META(
                                           " %016x | %016x",
                                           reinterpret_cast<const UINT32*>(entry.data.u8)[i],
                                           reinterpret_cast<const UINT32*>(pData)[i]);
                            break;
                        case 8:
                            CAMX_LOG_META(
                                           " %016llx | %016llx",
                                           reinterpret_cast<const UINT64*>(entry.data.u8)[i],
                                           reinterpret_cast<const UINT64*>(pData)[i]);
                            break;
                        default:
                            CAMX_LOG_META( "Unsupported Tag Size");
                            break;
                    }
                }
                CAMX_LOG_META( "-------");
                CAMX_LOG_META( "Old data retained to prevent metadata issues. Please resolve conflicts");

                status = CamxResultEFailed;
            }
        }
        else
        {
            if ((0 != entry.count) && (0 == count))
            {
                // We have an existing entry with count not 0, and now need to update with count=0.
                // In this case, need to delete the entry and add it back with count=0,
                // otherwise offset will not be updated to 0 and thus validation fails.
                status = delete_camera_metadata_entry(GetMetadataType(pMetadata), entry.index);
                if (MetadataUtilOK == status)
                {
                    status = add_camera_metadata_entry(GetMetadataType(pMetadata), tag, pData, count);
                }
            }
            else
            {
                status = update_camera_metadata_entry(GetMetadataType(pMetadata), entry.index, pData, count, NULL);
            }
        }
    }

    result = (MetadataUtilOK == status) ? CamxResultSuccess : CamxResultEFailed;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetPropertyBlob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::GetPropertyBlob(
    Metadata*           pMetadata,
    MainPropertyBlob**  ppBlob)
{
    CamxResult result = CamxResultSuccess;

    camera_metadata_entry_t entry;

    // If we do not find the private tag in the metadata, we need to add it
    if (MetadataUtilNotFound == find_camera_metadata_entry(GetMetadataType(pMetadata), VendorTagPrivate, &entry))
    {
        INT32            status = MetadataUtilError;
        MainPropertyBlob blob   = {};

        // add will fail if set_vendor_tag_query_ops() has NOT been called first by the application
        status = add_camera_metadata_entry(GetMetadataType(pMetadata), VendorTagPrivate, &blob, sizeof(MainPropertyBlob));
        if (MetadataUtilOK != status)
        {
            result = CamxResultEFailed;
        }
        else
        {
            // we are guaranteed to find the tag here
            status = find_camera_metadata_entry(GetMetadataType(pMetadata), VendorTagPrivate, &entry);
            CAMX_ASSERT(MetadataUtilOK == status);
        }
    }

    if (CamxResultSuccess == result)
    {
        *ppBlob = reinterpret_cast<MainPropertyBlob*>(entry.data.u8);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3MetadataUtil::GetPropertyTagSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T HAL3MetadataUtil::GetPropertyTagSize(
    UINT32 tag)
{
    UINT32 group = (tag & DriverInternalGroupMask);
    SIZE_T size = 0;
    UINT32 index = (tag & ~DriverInternalGroupMask);

    switch (group)
    {
        case static_cast<UINT32>(PropertyGroup::Result) << 16:
            if (index < CAMX_ARRAY_SIZE(MainPropertySizes))
            {
                size = MainPropertySizes[index];
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid Tag given: %x", tag);
            }
            break;
        case static_cast<UINT32>(PropertyGroup::Internal) << 16:
            if (index < CAMX_ARRAY_SIZE(InternalPropertySizes))
            {
                size = InternalPropertySizes[index];
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid Tag given: %x", tag);
            }
            break;
        case static_cast<UINT32>(PropertyGroup::Usecase) << 16:
            if (index < CAMX_ARRAY_SIZE(UsecasePropertySizes))
            {
                size = UsecasePropertySizes[index];
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid Tag given: %x", tag);
            }
            break;
        case static_cast<UINT32>(PropertyGroup::DebugData) << 16:
            if (index < CAMX_ARRAY_SIZE(DebugDataPropertySizes))
            {
                size = DebugDataPropertySizes[index];
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid Tag given: %x", tag);
            }
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid Tag given: %x", tag);
            break;
    }
    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetMaxSizeByTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T HAL3MetadataUtil::GetMaxSizeByTag(
    UINT32 tag)
{
    SIZE_T index = CAMX_TAG_TO_INDEX(tag);
    SIZE_T size;

    if (IsProperty(tag))
    {
        size = GetPropertyTagSize(tag);
    }
    else if (MaxMetadataTagCount > index)
    {
        // Remove type identifer from tag
        tag &= ~DriverInternalGroupMask;

        size = GetSizeByType(GetTypeByTag(tag)) * GetNumElementsByTag(tag);

        if (0 == TagFactor[index])
        {
            const HwEnvironment* pHwEnvironment = HwEnvironment::GetInstance();

            StaticMetadataKeysInfo keyInfo = {0};
            CamxResult             result  = pHwEnvironment->GetStaticMetadataKeysInfo(&keyInfo,
                                                                                       static_cast<CameraMetadataTag>(tag));

            if (CamxResultSuccess == result)
            {
                size *= keyInfo.numKeys;
            }
        }
        else if (1 != TagFactor[index])
        {
            size *= TagFactor[index];
        }
    }
    else
    {
        size = VendorTagManager::VendorTagSize(index - MaxMetadataTagCount);
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetSizeByType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T HAL3MetadataUtil::GetSizeByType(
    UINT8 type)
{
    SIZE_T size = 0;

    switch (type)
    {
        case TYPE_BYTE:
            size = sizeof(UINT8);
            break;

        case TYPE_INT32:
            size = sizeof(UINT32);
            break;

        case TYPE_FLOAT:
            size = sizeof(FLOAT);
            break;

        case TYPE_INT64:
            size = sizeof(UINT64);
            break;

        case TYPE_DOUBLE:
            size = sizeof(DOUBLE);
            break;

        case TYPE_RATIONAL:
            size = sizeof(camera_metadata_rational_t);
            break;

        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Got unsupported type %d", type);
            break;
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::IsDebugDataEnable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HAL3MetadataUtil::IsDebugDataEnable()
{
    return (0 < HAL3MetadataUtil::DebugDataSize(DebugDataType::AllTypes)) ? TRUE : FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::DebugDataSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T HAL3MetadataUtil::DebugDataSize(
    DebugDataType debugDataType)
{
    const StaticSettings*   pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    SIZE_T                  debugDataSize   = 0;

    // Get 3A size only if enable
    if (TRUE == pStaticSettings->enable3ADebugData)
    {
        switch (debugDataType)
        {
            case DebugDataType::AEC:
                debugDataSize = pStaticSettings->debugDataSizeAEC;
                break;
            case DebugDataType::AWB:
                debugDataSize = pStaticSettings->debugDataSizeAWB;
                break;
            case DebugDataType::AF:
                debugDataSize = pStaticSettings->debugDataSizeAF;
                break;
            default:
                break;
        }
    }
    else if (TRUE == pStaticSettings->enableConcise3ADebugData)
    {
        switch (debugDataType)
        {
            case DebugDataType::AEC:
                debugDataSize = pStaticSettings->conciseDebugDataSizeAEC;
                break;
            case DebugDataType::AWB:
                debugDataSize = pStaticSettings->conciseDebugDataSizeAWB;
                break;
            case DebugDataType::AF:
                debugDataSize = pStaticSettings->conciseDebugDataSizeAF;
                break;
            default:
                break;
        }
    }

    // Get tuning size if enable
    if (TRUE == pStaticSettings->enableTuningMetadata)
    {
        switch (debugDataType)
        {
            case DebugDataType::IFETuning:
                debugDataSize = pStaticSettings->tuningDumpDataSizeIFE;
                break;
            case DebugDataType::IPETuning:
                debugDataSize = pStaticSettings->tuningDumpDataSizeIPE;
                break;
            case DebugDataType::BPSTuning:
                debugDataSize = pStaticSettings->tuningDumpDataSizeBPS;
                break;
            default:
                break;
        }
    }

    // Get total size
    if (DebugDataType::AllTypes == debugDataType)
    {
        SIZE_T size3A       = 0;
        SIZE_T sizeTuning   = 0;

        if (TRUE == pStaticSettings->enable3ADebugData)
        {
            size3A = pStaticSettings->debugDataSizeAEC +
                     pStaticSettings->debugDataSizeAWB +
                     pStaticSettings->debugDataSizeAF;
        }
        else if (TRUE == pStaticSettings->enableConcise3ADebugData)
        {
            size3A = pStaticSettings->conciseDebugDataSizeAEC +
                     pStaticSettings->conciseDebugDataSizeAWB +
                     pStaticSettings->conciseDebugDataSizeAF;
        }

        if (TRUE == pStaticSettings->enableTuningMetadata)
        {
            sizeTuning = pStaticSettings->tuningDumpDataSizeIFE +
                         pStaticSettings->tuningDumpDataSizeIPE +
                         pStaticSettings->tuningDumpDataSizeBPS;
        }

        debugDataSize = size3A + sizeTuning;
    }

    return debugDataSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::DebugDataOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T HAL3MetadataUtil::DebugDataOffset(
    DebugDataType debugDataType)
{
    SIZE_T                  offset          = 0;
    const StaticSettings*   pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    SIZE_T                  sizeAEC         = 0;
    SIZE_T                  sizeAWB         = 0;
    SIZE_T                  sizeAF          = 0;
    SIZE_T                  sizeIFE         = 0;
    SIZE_T                  sizeIPE         = 0;
    SIZE_T                  sizeBPS         = 0;

    // Get 3A size only if enable
    if (TRUE == pStaticSettings->enable3ADebugData)
    {
        sizeAEC = pStaticSettings->debugDataSizeAEC;
        sizeAWB = pStaticSettings->debugDataSizeAWB;
        sizeAF  = pStaticSettings->debugDataSizeAF;
    }
    else if (TRUE == pStaticSettings->enableConcise3ADebugData)
    {
        sizeAEC = pStaticSettings->conciseDebugDataSizeAEC;
        sizeAWB = pStaticSettings->conciseDebugDataSizeAWB;
        sizeAF  = pStaticSettings->conciseDebugDataSizeAF;
    }

    // Get tuning size if enable
    if (TRUE == pStaticSettings->enableTuningMetadata)
    {
        sizeIFE = pStaticSettings->tuningDumpDataSizeIFE;
        sizeIPE = pStaticSettings->tuningDumpDataSizeIPE;
        sizeBPS = pStaticSettings->tuningDumpDataSizeBPS;
    }

    switch (debugDataType)
    {
        case DebugDataType::AEC:
            offset = 0;
            break;
        case DebugDataType::AWB:
            offset = sizeAEC;
            break;
        case DebugDataType::AF:
            offset = sizeAEC + sizeAWB;
            break;
        case DebugDataType::IFETuning:
            offset = sizeAEC + sizeAWB + sizeAF;
            break;
        case DebugDataType::IPETuning:
            offset = sizeAEC + sizeAWB + sizeAF + sizeIFE;
            break;
        case DebugDataType::BPSTuning:
            offset = sizeAEC + sizeAWB + sizeAF + sizeIFE + sizeIPE;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Not supported data type");
            break;
    };

    return offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxPropertyIDToString
///
/// @brief  Returns a pointer to a const string which is a human readible representation of the PropertyID
///
/// @param  id  Tag to stringize
///
/// @return String version of the tag or NULL if not valid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE const CHAR* CamxPropertyIDToString(
    PropertyID      id)
{
    const CHAR* pTagName = NULL;

    switch (id >> 16)
    {
        case static_cast<UINT32>(PropertyGroup::Result) :
            if ((id & ~PropertyIDPerFrameResultBegin) < CAMX_ARRAY_SIZE(pMainPropertyStrings))
            {
                pTagName = pMainPropertyStrings[id & ~PropertyIDPerFrameResultBegin];
            }
            break;
        case static_cast<UINT32>(PropertyGroup::Internal) :
            if ((id & ~PropertyIDPerFrameInternalBegin) < CAMX_ARRAY_SIZE(pInternalPropertyStrings))
            {
                pTagName = pInternalPropertyStrings[id & ~PropertyIDPerFrameInternalBegin];
            }
            break;
        case static_cast<UINT32>(PropertyGroup::Usecase) :
            if ((id & ~PropertyIDUsecaseBegin) < CAMX_ARRAY_SIZE(pUsecasePropertyStrings))
            {
                pTagName = pUsecasePropertyStrings[id & ~PropertyIDUsecaseBegin];
            }
            break;
        case static_cast<UINT32>(PropertyGroup::DebugData) :
            if ((id & ~PropertyIDPerFrameDebugDataBegin) < CAMX_ARRAY_SIZE(pDebugDataPropertyStrings))
            {
                pTagName = pDebugDataPropertyStrings[id & ~PropertyIDPerFrameDebugDataBegin];
            }
            break;
        default:
            // Unknown PropertyID
            CAMX_ASSERT_ALWAYS_MESSAGE("Unknown PropertyID %08x", id);
            break;
    }

    return pTagName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetNameByTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::GetNameByTag(
    CHAR*       pDst,
    SIZE_T      sizeDst,
    UINT32      tag)
{
    const CHAR* pPropertyPrefix     = "";
    const CHAR* pPropertyName       = "";

    if (tag != InvalidIndex)
    {
        // Print based on the section
        UINT32 section = tag >> 16;
        if ((section & VENDOR_SECTION) != 0)
        {
            pPropertyName   = VendorTagManager::GetTagName(tag);
            pPropertyPrefix = VendorTagManager::GetSectionName(tag);
            OsUtils::SNPrintF(pDst, sizeDst, "%s.%s", pPropertyPrefix, pPropertyName);
        }
        else if ((section == static_cast<UINT32>(PropertyGroup::Result)) ||
                 (section == static_cast<UINT32>(PropertyGroup::Internal)) ||
                 (section == static_cast<UINT32>(PropertyGroup::Usecase)) ||
                 (section == static_cast<UINT32>(PropertyGroup::DebugData)))
        {
            pPropertyName = CamxPropertyIDToString(tag);
            if (NULL != pPropertyName)
            {
                OsUtils::StrLCpy(pDst, CamxPropertyIDToString(tag), sizeDst);
            }
        }
        else
        {
            // MetadataPoolSection::Input, MetadataPoolSection::Usecase, MetadataPoolSection::Static, or default
            CameraMetadataTagToString(static_cast<CameraMetadataTag>(tag), &pPropertyPrefix, &pPropertyName);
            OsUtils::SNPrintF(pDst, sizeDst, "%s%s", pPropertyPrefix, pPropertyName);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3MetadataUtil::GetPropertyName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* HAL3MetadataUtil::GetPropertyName(
    UINT32      tag)
{
    const CHAR* pName = "";

    if (tag != InvalidIndex)
    {
        pName = CamxPropertyIDToString(tag);
    }

    return pName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3MetadataUtil::GetTagByIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 HAL3MetadataUtil::GetTagByIndex(
    UINT32 index)
{
    UINT32 tag = InvalidIndex;
    if (s_metadataInfoTable.totalMetadataTags > index)
    {
        tag = s_metadataInfoTable.metadataInfoArray[index].tag;
    }
    return tag;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3MetadataUtil::DumpMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::DumpMetadata(
    const Metadata* pMetadata)
{
    if (NULL != pMetadata)
    {
        const camera_metadata* pAndroidMeta = static_cast<const camera_metadata*>(pMetadata);

        for (UINT i = 0; i < get_camera_metadata_entry_count(pAndroidMeta); i++)
        {
            camera_metadata_ro_entry_t entry = { 0 };
            get_camera_metadata_ro_entry(pAndroidMeta, i, &entry);
            PrintTagData(pMetadata, InputMetadataSectionMask | entry.tag);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3MetadataUtil::PrintTagData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::PrintTagData(
    const Metadata* pMetadata,
    UINT32          tag)
{
    const camera_metadata*      pAndroidMeta = static_cast<const camera_metadata*>(pMetadata);
    camera_metadata_ro_entry_t  entry = { 0 };
    const uint8_t*              pData;
    const CHAR*                 pTagName = "";
    const CHAR*                 pTagSection = "";
    UINT                        pool = (tag & DriverInternalGroupMask);

    if (((static_cast<UINT32>(PropertyGroup::Result) << 16) == pool)   ||
        ((static_cast<UINT32>(PropertyGroup::Internal) << 16) == pool) ||
        ((static_cast<UINT32>(PropertyGroup::Usecase) << 16) == pool)  ||
        ((static_cast<UINT32>(PropertyGroup::DebugData) << 16) == pool)      )
    {
        pTagName = CamxPropertyIDToString(tag);
        switch (pool)
        {
            case static_cast<UINT32>(PropertyGroup::Result) << 16:
                pTagSection = "Result";
                break;
            case static_cast<UINT32>(PropertyGroup::Internal) << 16:
                pTagSection = "Internal";
                break;
            case static_cast<UINT32>(PropertyGroup::Usecase) << 16:
                pTagSection = "Usecase";
                break;
            case static_cast<UINT32>(PropertyGroup::DebugData) << 16:
                pTagSection = "Debug";
                break;
            default:
                pTagSection = "ERROR";
                break;
        }
        CAMX_LOG_META("    Pool: %s  Tag: %s", pTagSection, pTagName);
        CAMX_LOG_META("         TODO: property data dump");
    }
    else if (tag & 0x80000000)
    {
        CAMX_LOG_META("    Tag: %08x    TODO: vendor tag support", tag);

    }
    else if (MetadataUtilOK == find_camera_metadata_ro_entry(pAndroidMeta, (tag & ~DriverInternalGroupMask), &entry))
    {
        static UINT valuesPerLine[NUM_TYPES] =
        {
            16, // TYPE_BYTE
            4,  // TYPE_INT32
            8,  // TYPE_FLOAT
            2,  // TYPE_INT64
            4,  // TYPE_DOUBLE
            3,  // TYPE_RATIONAL
        };

        if (entry.type < NUM_TYPES)
        {
            size_t          typeSize = camera_metadata_type_size[entry.type];

            pData = entry.data.u8;
            CameraMetadataTagToString(static_cast<CameraMetadataTag>(tag), &pTagSection, &pTagName);

            CHAR        valueString[200];
            UINT        stringOffset;
            SIZE_T      count = entry.count;

            SIZE_T lines = count / valuesPerLine[entry.type];
            if (count % valuesPerLine[entry.type] != 0)
            {
                lines++;
            }

            CAMX_LOG_META("    Pool: %s  Tag: %s  Count: %zd", pTagSection, pTagName, count);
            if (0 == count)
            {
                CAMX_LOG_META("        count == 0, data being removed");
            }

            SIZE_T  index = 0;
            UINT    j;
            UINT    k;
            for (j = 0; j < lines; j++)
            {
                stringOffset = 0;

                for (k = 0;
                     k < valuesPerLine[entry.type] && count > 0;
                     k++, count--, index += typeSize)
                {
                    switch (entry.type)
                    {
                        case TYPE_BYTE:
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "%hhu ", *(pData + index));
                            break;
                        case TYPE_INT32:
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "%d ",
                                *reinterpret_cast<const int32_t*>(pData + index));
                            break;
                        case TYPE_FLOAT:
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "%0.8f ",
                                *reinterpret_cast<const FLOAT*>(pData + index));
                            break;
                        case TYPE_INT64:
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "%lld ",
                                *reinterpret_cast<const int64_t*>(pData + index));
                            break;
                        case TYPE_DOUBLE:
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "%0.8f ",
                                *reinterpret_cast<const DOUBLE*>(pData + index));
                            break;
                        case TYPE_RATIONAL:
                        {
                            int32_t numerator = *reinterpret_cast<const int32_t*>(pData + index);
                            int32_t denominator = *reinterpret_cast<const int32_t*>(pData + index + 4);
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "(%d / %d) ", numerator, denominator);
                            break;
                        }
                        default:
                            break;
                    }
                }
                CAMX_LOG_META("         %s", valueString);
            }
        }
    }
    else
    {
        CAMX_LOG_META("    ERROR: TAG NOT FOUND: %08x", tag);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3MetadataUtil::PrintTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::PrintTag(
    UINT32          tag,
    VOID*           pPayload,
    UINT32          count)
{
    const CHAR*                 pTagName;
    const CHAR*                 pTagSection;
    UINT                        pool     = (tag & DriverInternalGroupMask);
    UINT32                      tagIndex = GetUniqueIndexByTag(tag);
    BYTE*                       pData    = static_cast<BYTE*>(pPayload);

    if (IsProperty(tag))
    {
        pTagName = CamxPropertyIDToString(tag);
        switch (pool)
        {
            case static_cast<UINT32>(PropertyGroup::Result) << 16:
                pTagSection = "Result";
                break;
            case static_cast<UINT32>(PropertyGroup::Internal) << 16:
                pTagSection = "Internal";
                break;
            case static_cast<UINT32>(PropertyGroup::Usecase) << 16:
                pTagSection = "Usecase";
                break;
            case static_cast<UINT32>(PropertyGroup::DebugData) << 16:
                pTagSection = "Debug";
                break;
            default:
                pTagSection = "ERROR";
                break;
        }
        CAMX_LOG_META("    Pool: %s  Tag: %s", pTagSection, pTagName);
        CAMX_LOG_META("         TODO: property data dump");
    }
    else if (tag & 0x80000000)
    {
        CAMX_LOG_META("    Tag: %08x    TODO: vendor tag support", tag);
    }
    else if (CAMX_ARRAY_SIZE(TagType) > tagIndex)
    {
        UINT32 type = TagType[tagIndex];

        static UINT valuesPerLine[NUM_TYPES] =
        {
            16, // TYPE_BYTE
            4,  // TYPE_INT32
            8,  // TYPE_FLOAT
            2,  // TYPE_INT64
            4,  // TYPE_DOUBLE
            3,  // TYPE_RATIONAL
        };

        if (NUM_TYPES > type)
        {
            size_t          tagSize  = GetMaxSizeByTag(tag);
            size_t          typeSize = camera_metadata_type_size[type];

            CameraMetadataTagToString(static_cast<CameraMetadataTag>(tag), &pTagSection, &pTagName);

            CHAR        valueString[200];
            UINT        stringOffset;

            SIZE_T lines = count / valuesPerLine[type];
            if (count % valuesPerLine[type] != 0)
            {
                lines++;
            }

            CAMX_LOG_META("    Pool: %s  Tag: %s  Count: %zd", pTagSection, pTagName, count);
            if (0 == count)
            {
                CAMX_LOG_META("        count == 0, data being removed");
            }

            SIZE_T  index = 0;
            UINT    j;
            UINT    k;
            for (j = 0; j < lines; j++)
            {
                stringOffset = 0;

                for (k = 0; k < valuesPerLine[type] && count > 0; k++, count--, index += typeSize)
                {
                    switch (type)
                    {
                        case TYPE_BYTE:
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "%hhu ", *(pData + index));
                            break;
                        case TYPE_INT32:
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "%d ",
                                *reinterpret_cast<const int32_t*>(pData + index));
                            break;
                        case TYPE_FLOAT:
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "%0.8f ",
                                *reinterpret_cast<const FLOAT*>(pData + index));
                            break;
                        case TYPE_INT64:
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "%lld ",
                                *reinterpret_cast<const int64_t*>(pData + index));
                            break;
                        case TYPE_DOUBLE:
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "%0.8f ",
                                *reinterpret_cast<const DOUBLE*>(pData + index));
                            break;
                        case TYPE_RATIONAL:
                        {
                            int32_t numerator = *reinterpret_cast<const int32_t*>(pData + index);
                            int32_t denominator = *reinterpret_cast<const int32_t*>(pData + index + 4);
                            stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                                (sizeof(valueString) - stringOffset), "(%d / %d) ", numerator, denominator);
                            break;
                        }
                        default:
                            break;
                    }
                }
                CAMX_LOG_META("         %s", valueString);
            }
        }
    }
    else
    {
        CAMX_LOG_META("    ERROR: TAG NOT FOUND: %08x", tag);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3MetadataUtil::AppendPropertyPackingInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3MetadataUtil::AppendPropertyPackingInfo(
    UINT32               tagId,
    BYTE*                pValue,
    PropertyPackingInfo* pPackingInfo)
{
    CamxResult result = CamxResultSuccess;
    if (MaxNumProperties > pPackingInfo->count)
    {
        UINT32 index = pPackingInfo->count;
        pPackingInfo->tagId[index]    = tagId;
        pPackingInfo->pAddress[index] = pValue;
        ++pPackingInfo->count;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Count %d greater than maximum properties %d", pPackingInfo->count, MaxNumProperties);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3MetadataUtil::PackPropertyInfoToBlob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3MetadataUtil::PackPropertyInfoToBlob(
    PropertyPackingInfo* pPackingInfo,
    VOID*                pPropertyBlob)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pPropertyBlob)
    {
        UINT32* pCount = reinterpret_cast<UINT32*>(pPropertyBlob);
        *pCount = pPackingInfo->count;
        VOID* pWriteOffset = Utils::VoidPtrInc(pPropertyBlob, sizeof(pPackingInfo->count));

        for (UINT32 index = 0; index < pPackingInfo->count; index++)
        {
            // Write TagId
            SIZE_T tagSize = GetPropertyTagSize(pPackingInfo->tagId[index]);
            Utils::Memcpy(pWriteOffset, &pPackingInfo->tagId[index], sizeof(UINT32));
            pWriteOffset = Utils::VoidPtrInc(pWriteOffset, sizeof(UINT32));

            // Write tag address
            Utils::Memcpy(pWriteOffset, pPackingInfo->pAddress[index], tagSize);
            pWriteOffset = Utils::VoidPtrInc(pWriteOffset, tagSize);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Property blob is null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3MetadataUtil::UnPackBlobToPropertyInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3MetadataUtil::UnPackBlobToPropertyInfo(
    PropertyPackingInfo* pPackingInfo,
    VOID*                pPropertyBlob)
{
    // First 4 bytes correspoint to number of tags
    CamxResult result = CamxResultSuccess;
    if (NULL != pPropertyBlob)
    {
        UINT32* pCount = reinterpret_cast<UINT32*>(pPropertyBlob);
        pPackingInfo->count = *pCount;

        VOID* pReadOffset = Utils::VoidPtrInc(pPropertyBlob, sizeof(pPackingInfo->count));
        for (UINT32 i = 0; i < pPackingInfo->count; i++)
        {
            // First 4 bytes correspond to tagId followed by the tagValue
            pPackingInfo->tagId[i] = *reinterpret_cast<UINT32*>(pReadOffset);
            pReadOffset = Utils::VoidPtrInc(pReadOffset, sizeof(UINT32));

            SIZE_T tagSize = GetPropertyTagSize(pPackingInfo->tagId[i]);
            pPackingInfo->pAddress[i] = reinterpret_cast<BYTE*>(pReadOffset);
            pReadOffset = Utils::VoidPtrInc(pReadOffset, tagSize);
        }
    }

    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Property blob is null");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3MetadataUtil::GetTagSizeByUniqueIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 HAL3MetadataUtil::GetTagSizeByUniqueIndex(
    UINT32 tagIndex)
{
    return static_cast<UINT32>(GetMaxSizeByTag(GetTagByIndex(tagIndex)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3MetadataUtil::DumpTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3MetadataUtil::DumpTag(
    FILE*               pFile,
    UINT32              tag,
    BYTE*               pData,
    UINT32              count,
    const MetadataInfo* pInfo)
{
    CAMX_UNREFERENCED_PARAM(tag);

    static UINT valuesPerLine[NUM_TYPES] =
    {
        16, // TYPE_BYTE
        8,  // TYPE_INT32
        8,  // TYPE_FLOAT
        4,  // TYPE_INT64
        4,  // TYPE_DOUBLE
        3,  // TYPE_RATIONAL
    };

    if (NUM_TYPES > pInfo->type)
    {
        size_t typeSize = camera_metadata_type_size[pInfo->type];

        CHAR        valueString[200];
        UINT        stringOffset;

        SIZE_T lines = count / valuesPerLine[pInfo->type];
        if (count % valuesPerLine[pInfo->type] != 0)
        {
            lines++;
        }

        SIZE_T  index = 0;
        UINT    j;
        UINT    k;
        OsUtils::FPrintF(pFile, "\t *** Start Payload****", valueString);
        for (j = 0; j < lines; j++)
        {
            stringOffset = 0;

            for (k = 0; k < valuesPerLine[pInfo->type] && count > 0; k++, count--, index += typeSize)
            {
                switch (pInfo->type)
                {
                    case TYPE_BYTE:
                        stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                            (sizeof(valueString) - stringOffset), "%hhu ", *(pData + index));
                        break;
                    case TYPE_INT32:
                        stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                            (sizeof(valueString) - stringOffset), "%d ",
                            *reinterpret_cast<const int32_t*>(pData + index));
                        break;
                    case TYPE_FLOAT:
                        stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                            (sizeof(valueString) - stringOffset), "%0.8f ",
                            *reinterpret_cast<const FLOAT*>(pData + index));
                        break;
                    case TYPE_INT64:
                        stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                            (sizeof(valueString) - stringOffset), "%lld ",
                            *reinterpret_cast<const int64_t*>(pData + index));
                        break;
                    case TYPE_DOUBLE:
                        stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                            (sizeof(valueString) - stringOffset), "%0.8f ",
                            *reinterpret_cast<const DOUBLE*>(pData + index));
                        break;
                    case TYPE_RATIONAL:
                    {
                        int32_t numerator = *reinterpret_cast<const int32_t*>(pData + index);
                        int32_t denominator = *reinterpret_cast<const int32_t*>(pData + index + 4);
                        stringOffset += OsUtils::SNPrintF(&valueString[stringOffset],
                            (sizeof(valueString) - stringOffset), "(%d / %d) ", numerator, denominator);
                        break;
                    }
                    default:
                        break;
                }
            }
            OsUtils::FPrintF(pFile, "\n\t         %s", valueString);
        }
        OsUtils::FPrintF(pFile, "\n\t *** End Payload****\n", valueString);
    }
}

CAMX_NAMESPACE_END
