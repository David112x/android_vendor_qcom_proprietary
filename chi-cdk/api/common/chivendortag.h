////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file chimulticamera.h
/// @brief Define Qualcomm Technologies, Inc. Multicamera input / output Meta data information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIMULTICAMERA_H
#define CHIMULTICAMERA_H

#include "camxcdktypes.h"
#include "chicommontypes.h"
#include "chi.h"

#ifdef __cplusplus

extern "C"
{
#endif // __cplusplus

/// @brief Data structures that are exposed to OEM must be packed to have the expected layout.
#pragma pack(push, CDK_PACK_SIZE)

/// @brief Defines maximum number of nodes for FOVs
#define MAX_NODES_NUMBER              4

/// @brief Defines maximum number of streams for a camera
#define MAX_NUM_STREAMS               11

/// @brief Max number of faces detected
static const UINT8 FDMaxFaces        = 10;

/// @brief Defines maximum number of streams for a camera
static const UINT32 MaxLinkedCameras = 4;

/// @brief Defines maximum number of entries in array required in AllNodeCropInfo
static const UINT32 MaxCropInfoEntries = 30;

/// @brief Defines maximum number of related camera
static const UINT   MaxDevicePerLogicalCamera    = 8;

///@brief Enumeration describing sensor mode
typedef enum
{
    NoSync = 0,      ///< No Hardware Sync
    MasterMode,      ///< Master Mode
    SlaveMode,       ///< Slave Mode
} SensorSyncMode;

///@brief Enumeration describing sensor status
typedef enum
{
    SensorNoPerf = 0,///< No Perf Ctrl
    SensorActive,    ///< Sensor In Active Status
    SensorStandby,   ///< Sensor In Sleeping Status
} SensorPerfCtrl;

///@brief Enumeration describing Buffer downscale type
typedef enum
{
    BUFFER_FULL,                ///< Buffer is full sized - no downscaling
    BUFFER_DS4,                 ///< Buffer is downscaled by 4
    BUFFER_DS16,                ///< Buffer is downscaled by 16
    MAX_BUFFER_DOWNSCALE_TYPES  ///< Max number of downscaling types
} BufferDownscaleType;

///@brief Structure describing sensor hardware sync mode configure
///Vendor tag section name:com.qti.chi.multicamerasensorconfig
///Vendor tag tag name:sensorsyncmodeconfig
typedef struct
{
    BOOL           isValid;        ///< flag to indicate if sync mode meta is valid
    SensorSyncMode sensorSyncMode; ///< sensor sync mode
} SensorSyncModeMetadata;

///@brief Enumeration describing seamless in-sensor control state
typedef enum
{
    None    = 0,
    InSensorHDR3ExpStart,
    InSensorHDR3ExpEnabled,
    InSensorHDR3ExpStop
}SeamlessInSensorState;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing the image size
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    INT                   width;                        ///< Image size width value
    INT                   height;                       ///< Image size height value
} ImageSizeData;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief structure for refernce crop window size
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    INT32 width;       ///< width for reference crop window
    INT32 height;      ///< height for reference crop window
} RefCropWindowSize;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing shift offset information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    INT32                horizonalShift;               ///< x cordinate of the pixel
    INT32                verticalShift;                ///< y cordinate of the pixel
} ImageShiftData;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Struct for a weighted region used for focusing/metering areas
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    INT32                xMin;                           ///< Top-left x-coordinate of the region
    INT32                yMin;                           ///< Top-left y-coordinate of the region
    INT32                xMax;                           ///< Width of the region
    INT32                yMax;                           ///< Height of the region
    INT32                weight;                         ///< Weight of the region
} WeightedRegion;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Struct for a weighted region used for focusing/metering areas
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT8                numFaces;                       ///< Number of faces detected
    UINT8                faceScore[FDMaxFaces];          ///< List of face confidence scores for detected faces
    CHIRECT              faceRect[FDMaxFaces];           ///< List of face rect information for detected faces
} FDMetadataResults;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing Low Power mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32                cameraId;                     ///< Camera type of the chosen frame
    BOOL                  isEnabled;                    ///< Flag to check if Camera is active
} LowPowerModeData;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing camera metadata needed by SAT node
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    BOOL                  isValid;                      ///< If this metadata is valid
    UINT32                cameraId;                     ///< Camera Id this metadata belongs to
    UINT32                masterCameraId;               ///< Master camera Id
    CHIRECTINT            userCropRegion;               ///< Overall user crop region
    CHIRECT               pipelineCropRegion;           ///< Pipeline specific crop region
    CHIRECT               ifeLimitCropRegion;           ///< Max Crop that IFE can apply
    CHIRECT               fovRectIFE;                   ///< IFE FOV wrt to active sensor array
    CHIRECT               fovRectIPE;                   ///< IPE FOV wrt to active sensor array
    CHIRECT               ifeAppliedCrop;               ///< Crop applied by IFE
    CHIRECT               sensorIFEAppliedCrop;         ///< Crop applied by Sensor+IFE
    CHIRECT               ifeResidualCrop;              ///< Crop remaining after IFE processing
    WeightedRegion        afFocusROI;                   ///< AF focus ROI
    CHIRECT               activeArraySize;              ///< Wide sensor active array size
    FLOAT                 lux;                          ///< LUX value
    INT32                 focusDistCm;                  ///< Focus distance in cm
    UINT32                afState;                      ///< AF state
    INT32                 isoSensitivity;               ///< ISO value
    INT64                 exposureTime;                 ///< Exposure time
    UINT64                sensorTimestamp;              ///< Sensor timestamp
    FDMetadataResults     fdMetadata;                   ///< Face detection metadata
} CameraMetadata;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure mapping physicalCameraId to a Node input port
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    CHILINKNODEDESCRIPTOR nodeDescriptor;
    UINT                  physicalCameraId;    ///< The physical cameraId associated with this nodeDescriptor
    BufferDownscaleType   bufferDownscaleType; ///< The buffer downscale type
} PhysicalCameraInputConfiguration;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Metadata structure containing a list of physicalCameraId to node input mappings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT                             numConfigurations;                 ///< The number of physical camera input configurations
    PhysicalCameraInputConfiguration configuration[MaxNumImageSensors]; ///< Array of physicalCameraId to Node Input properties
} ChiPhysicalCameraConfig;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing Multi camera SAT input meta data structure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32                frameId;                                       ///< Frame ID
    UINT8                 numInputs;                                     ///< Number of input metadata
    ImageShiftData        outputShiftSnapshot;                           ///< Snapshot frame shift due to spatial alignment
    CameraMetadata        cameraMetadata[MaxDevicePerLogicalCamera];     ///< Camera metdata for SAT
    BOOL                  isSnapshot;                                    ///< snapshot or preview meta
} InputMetadataOpticalZoom;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing Multi camera SAT output meta data structure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32                masterCameraId;                            ///< Current master camera id
    UINT32                recommendedMasterCameraId;                 ///< Recommended master camera id
    ImageShiftData        outputShiftPreview;                        ///< Preview frame shift due to spatial alignment
    ImageShiftData        outputShiftSnapshot;                       ///< Snapshot frame shift due to spatial alignment
    ImageSizeData         refResForOutputShift;                      ///< Reference resolution for the output shift
    CHIRECT               outputCrop;                                ///< Output Crop information
    UINT8                 numLinkedCameras;                          ///< Number of linked physical cameras
    LowPowerModeData      lowPowerMode[MaxDevicePerLogicalCamera];   ///< output lower power mode information
    BOOL                  hasProcessingOccured;                      ///< True if the SAT algorithm has processed this frame
                                                                     ///  False if SAT bypasses or copys its input to output
} OutputMetadataOpticalZoom;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing physical camera configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CameraConfiguration
{
    UINT32              cameraId;                ///< Camera Id
    FLOAT               transitionZoomRatioLow;  ///< Min Transition Zoom ratio at which the camera will be active
    FLOAT               transitionZoomRatioHigh; ///< Max Transition Zoom ratio at which the camera will be active
    BOOL                enableSmoothTransition;  ///< Enable Smooth Transition for the camera during SAT
    CHISENSORCAPS       sensorCaps;              ///< Capabilities related to the device's imaging characteristics
    CHILENSCAPS         lensCaps;                ///< Capabilities related to the device's lens characteristics
    CHISENSORMODEINFO   sensorModeInfo;          ///< Sensor mode information
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing set of physical configs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CameraConfigs
{
    UINT32              numPhysicalCameras;                         ///< Number of physical cameras
    CameraConfiguration cameraConfigs[MaxDevicePerLogicalCamera];   ///< Physical configuration of cameras
    UINT32              primaryCameraId;                            ///< Primary camera in the given Logical camera
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing Multi camera RTB input meta data structure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32                frameId;                                     ///< Frame ID
    UINT32                blurLevel;                                   ///< Blur intensity value to be applied to the Bokeh
    UINT32                blurMinValue;                                ///< Blur minimum value
    UINT32                blurMaxValue;                                ///< Blur maximum value
    CameraMetadata        cameraMetadata[MaxDevicePerLogicalCamera];   ///< Camera metdata for RTB
    UINT8                 numLinkedCameras;                            ///< Number of linked physical cameras
    BOOL                  isSnapshot;                                  ///< snapshot or preview meta
} InputMetadataBokeh;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing Multi camera RTB output meta data structure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32                masterCameraId;                   ///< Current master camera id
    UINT32                recommendedMasterCameraId;        ///< Recommended master camera id
    UINT32                activeMap;                        ///< Active map to indicate which pipeline is active
    UINT32                depthEffectInfo;                  ///< Depth Effect status information
} OutputMetadataBokeh;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure for Dual camera role, Id
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32                currentCameraId;               ///< Current camera id
    UINT32                logicalCameraId;               ///< Logical camera id
    UINT32                masterCameraId;                ///< master camera id
} MultiCameraIds;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure for Dual camera low power mode information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32                isLPMEnabled;                               ///< Low power mode status
    UINT32                isSlaveOperational;                         ///< Is Slave operational when LPM is enabled
    LowPowerModeData      lowPowerMode[MaxDevicePerLogicalCamera];    ///< LPM info for all the real time pipelines
    UINT8                 numLinkedCameras;                           ///< Number of linked physical cameras
} LowPowerModeInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure for Dual camera sync mode information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    BOOL                  isSyncModeEnabled;                    ///< Sync mode info to sync both the frames
} SyncModeInfo;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure for crop regions for capture request
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    CHIRECTINT            userCropRegion;                ///< Overall user crop region
    CHIRECT               pipelineCropRegion;            ///< Pipeline specific crop region
    CHIRECT               ifeLimitCropRegion;            ///< Max Crop that IFE can apply
} CaptureRequestCropRegions;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure for Feature2 input request information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32                internalFrameNumber;           ///< internal frame number
} Feature2InputRequestInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure for Feature2 active cameras
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32      cameraId;                           ///< Camera Id
    BOOL        isActive;                           ///< Boolean indicating if the camera is active
} Feature2ActiveCameras;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure for Feature2 mcc result
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    BOOL                    isValid;                                    ///< Boolean indicating if the result is valid
    BOOL                    isSnapshotFusion;                           ///< Boolean indicating if the snapshot fusion is enabled
    UINT32                  masterCameraId;                             ///< Camera Id for the master camera
    Feature2ActiveCameras   activeCameras[MaxDevicePerLogicalCamera];   ///< Camera info indicating the LPM status
    UINT32                  numActiveCameras;                           ///< Number of active Cameras in the activeCameras array
    UINT32                  activeMap;                                  ///< Active map to indicate which pipeline is active
} Feature2ControllerResult;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure for Gamma information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    BOOL    isGammaValid;              ///< Is Valid Gamma Entries
    UINT32  gammaG[65];                ///< Gamma Green Entries
} GAMMAINFO;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing the OEM JPEG EXIF App header Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    SIZE_T  size;   ///< Size in bytes of the EXIF App header data buffer
    VOID*   pData;  ///< Pointer to the EXIF App header data buffer
} OEMJPEGEXIFAppData;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure for Metadata Owner information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct MetadataOwnerInfo
{
    UINT32                  cameraId;                    ///< Camera Id who owns this metadata
} METADATAOWNERINFO;

typedef struct IdealRawInfo
{
    BOOL                    isIdealRaw;
} IDEALRAWINFO;


/// @brief struct containing the stream specific metadata for a particular stream
typedef struct
{
    VOID*              pSinkStreamPrivateInfo;        ///< Private information(ChiStream::pPrivateInfo) of the sink
                                                      ///  stream from which this stream buffer is generated
    INT                streamType;                    ///< Type of stream (optional)
    VOID*              pParentSinkStreamPrivateInfo;  ///< Private information(ChiStream::pPrivateInfo) of the
                                                      ///  Parent stream
    UINT               requestCropIndex;              ///< Index of crop entry in request crop info payload
    INT                index[MaxPerBufferMetadata];   ///  Index of the data entry corresponding to each buffer metadata
} StreamMapMetaEntry;

/// @brief struct containing the list of stream and thier index map
typedef struct
{
    UINT               streamMapEntryCount;                        ///< Number of valid streams
    StreamMapMetaEntry streamMapEntry[MaxOutputStreamsPerRequest]; ///< Mapping each stream to its index in the metadata
} StreamMapMeta;

/// @brief struct containing the current FOV and residual crop info for a stream
typedef struct
{
    CHIRectangle fov;               ///< Field of view w.r.t active pixel array
    CHIRectangle crop;              ///< Crop rectangle w.r.t to frame dimension
    CHIDimension frameDimension;    ///< Frame dimension
} StreamCropInfo;

/// @brief struct containing the current FOV and residual crop info for a stream
typedef struct
{
    UINT   nodeType;                ///< Node that publishes the metadata
    UINT   portId;                  ///< Port id associated with the metadata
    UINT   instanceId;              ///< Instance Id of the node
    INT    previousEntry;           ///< Index of the previous entry
} StreamMetaPublisherInfo;

/// @brief struct containing the graph of stream crop information
typedef struct
{
    StreamCropInfo          streamCropInfo;    ///< Array containing port crop info per port per node
    StreamMetaPublisherInfo publisherInfo;     ///< Information regarding the publisher of this crop metadata
} StreamCropLinkInfo;

/// @brief struct containing the stream crop information associated with each request
typedef struct
{
    UINT               streamCropCount;                       ///< Count of valid entries in the stream crop array
    StreamCropLinkInfo streamCropArray[MaxCropInfoEntries];   ///< Array containing port crop info per port per node
} RequestCropInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief CSID Binning modes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum class CSIDBinningMode
{
    HorizontalBinning   = 0,    ///< Horizontal binning  (Bayer binning)
    QCFABinning         = 1,    ///< Quad CFA binning
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief structure for IFE CSID binning configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    BOOL            isBinningEnabled;   ///< Flag to indeicate if CSID binning enabled
    CSIDBinningMode binningMode;        ///< binningMode: 0 H/V binning, 1 QuadraCFA binning
} CSIDBinningInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief structure for Snapshot output dimension
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    UINT32  width;  ///< The width of snapshot output
    UINT32  height; ///< The height of snapshot output
} GeoLibStreamOutput;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief structure for vendor stream mapping.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct StreamMap
{
    UINT32     width;                    ///< width  of ths stream
    UINT32     height;                   ///< height of ths stream
    INT32      format;                   ///< format of ths stream
    INT32      sequenceIndex;            ///< Configure stream order
    UINT32     streamIntent;             ///< Intent to describe if it is preview/snapshot
    UINT32     cameraIndex;              ///< Camera index 0/1/2
    INT32      isPhysicalStream;         ///< If stream needs from physicalcamera
    INT32      isThumbnailPostView;      ///< If this is snapshot postview stream
    INT32      reserved[8];              ///< Reserved
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Array for DebugDumpConfig.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    CHAR dumpFileName[MaxNumImageSensors][128];
} DumpFileName;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHIMULTICAMERA_H
