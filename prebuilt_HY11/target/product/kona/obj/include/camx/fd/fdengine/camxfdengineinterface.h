////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxfdengineinterface.h
/// @brief interface definition for Face Detection Engine
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXFDENGINEINTERFACE_H
#define CAMXFDENGINEINTERFACE_H

#include "camxdefs.h"
#include "chifdproperty.h"

CAMX_NAMESPACE_BEGIN

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef VOID* FDEngineHandle;  ///< FD Engine handle
static const VOID* FDEngineInvalidHandle = NULL;

static const FDEngineHandle InvalidFDEngineHandle = NULL;

// Roll angle indices in DT config angle array
static const UINT32 FDEngineAngleUnspecified    = 0;
static const UINT32 FDEngineAngleFront          = 1;
static const UINT32 FDEngineAngleHalfProfile    = 2;
static const UINT32 FDEngineAngleFullProfile    = 3;
static const UINT32 FDEngineMaxPoseAngles       = 4;

// tracking accuracy values
static const UINT32 FDEngineTrakcingAccuracyNormal = 15; ///< Normal Accuracy mode
static const UINT32 FDEngineTrakcingAccuracyHigh   = 0; ///< Normal Accuracy mode

// Search Density
static const UINT32 FDEngineSearchDessityHighest = 200;
static const UINT32 FDEngineSearchDessityHigh    = 150;
static const UINT32 FDEngineSearchDessityNormal  = 100;
static const UINT32 FDEngineSearchDessityLow     = 75;
static const UINT32 FDEngineSearchDessityLowest  = 50;

/// Max Image Planes
static const UINT32 MaxPlaneCount = 3;

typedef INT32 FacialAttributeMask;
static const FacialAttributeMask FacialAttriModeSmile = 0x00000001;
static const FacialAttributeMask FacialAttriModeGaze  = 0x00000002;
static const FacialAttributeMask FacialAttriModeBlink = 0x00000004;

/// @brief FDEngine Image Plane Type
enum PlaneType
{
    Y = 0,  ///< Luma plane
    CB_CR,  ///< CbCr plane
    CR_CB   ///< CrCb plane
};

/// @brief FDEngine Image Plane
struct ImagePlane
{
    PlaneType   type;       ///< Plane type
    UINT8*      pBuffer;     ///< Plane buffer location
    UINT32      stride;     ///< Stride of Image Plane
    UINT32      scanline;   ///< Scanline of image plane
};

/// @brief FDEngine Image Type
struct FDEngineImage
{
    UINT32      width;                  ///< Image width
    UINT32      height;                 ///< Image height
    UINT32      planeCount;             ///< Number of planes
    ImagePlane  planes[MaxPlaneCount];  ///< Plane information
};

/// @brief CO parameters
struct FDEngineCOParams
{
    UINT8   majorVersion;   ///< Major version of SW CO library
    UINT8   minorVersion;   ///< Minor version of SW CO library
};

/// @brief threshold info for DT
struct FDEngineDTThresInfo
{
    INT32   direction;                  ///< Direction of each face angle
    INT32   angle;                      ///< Roll angle setting threshold value with direction as central
    INT32   targetThreshold;            ///< Confidence threshold within selected area
    INT32   nonTargetThreshold;         ///< Confidence threshold outside selected area
    INT32   innerTargetThreshold;       ///< Inner confidence threshold within selected area
    INT32   innerNonTargetThreshold;    ///< Inner confidence threshold outside selected area
};

/// @brief enumerates detection operation runtime choice
enum FDEngineClientType
{
    ClientHW = 0,   ///< Client type HW
    ClientDSP,      ///< Client type DSP
    ClientSW        ///< Client type SW
};

/// @brief DT parameters
struct FDEngineDTParams
{
    UINT8               majorVersion;                            ///< Major version of SW DT library.
    UINT8               minorVersion;                            ///< Minor version of SW DT library.
    INT32               minFaceSize;                             ///< Minimum Face size for detection.
    INT32               maxFaceSize;                             ///< Maximum Face size for detection.
    UINT32              faceAngle[FDEngineMaxPoseAngles];        ///< Detection angles for whole face search and face search.
                                                                 ///< Array of angle values for different profiles.
    UINT32              upFrontFaceAngle[FDEngineMaxPoseAngles]; ///< Detection angles specified when device orientation
                                                                 ///< is available.
    CHIRectangle        faceSearchEdgeMask;                      ///< Search area for whole face search and face search.
    INT32               searchDensity;                           ///< Search density for whole face search and face search.
    INT32               threshold;                               ///< faces with a degree of confidence lower than 'threshold'
                                                                 ///< value will not be output.
    FDEngineDTThresInfo thresholdInfo[FDEngineMaxPoseAngles];    ///< Threshold information with angle.
    INT32               initialFaceSearchCycle;                  ///< Initial face search cycle i.e the number of frames
                                                                 ///< required to complete initial face search.
    INT32               newFaceSearchCycle;                      ///< New face search cycle i.e the number of frames required to
                                                                 ///< complete new face search.
    INT32               newFaceSearchInterval;                   ///< Search interval for new face search. These number of
                                                                 ///< frames are skipped for new face search.
    UINT32              upFrontNoFaceSearchCycle;                ///< Search cycle when there are no faces detected already.
                                                                 ///< Used if upFront is enabled and upFront angle can be
                                                                 ///< determined.
    UINT32              upFrontNewFaceSearchCycle;               ///< Search cycle when atleast one face is detected. Used
                                                                 ///< if upFront is enabled and upFront angle can be determined
    UINT32              upFrontNewFaceSearchInterval;            ///< Face search interval. Used if upFront is enabled and
                                                                 ///< upFront angle can be determined.
    INT32               maxRetryCount;                           ///< Max retry count for when a face is lost
                                                                 ///< during tracking in Movie mode.
    INT32               maxHoldCount;                            ///< Max hold count for when a face is lost
                                                                 ///< during tracking in Movie mode.
    INT32               posSteadinessParam;                      ///< Steadiness param for the position of the results
                                                                 ///< rectangle during tracking.
    INT32               sizeSteadinessParam;                     ///< Same as posSteadinessParam, it describes the size
                                                                 ///< steadiness.
    INT32               trackingSwapParam;                       ///< Parameter used when swapping faces during tracking.
                                                                 ///< Allows the library to decide which faces are to be
                                                                 ///< discarded. when the number of faces detected exceeds
                                                                 ///< the max detection count.
    INT32               delayCount;                              ///< Delay count used to output only continuously detected
                                                                 ///< faces.
    CHIRectangle        trackingEdgeMask;                        ///< Search area for tracking. Crop area to restrict the area
                                                                 ///< targeted for face tracking.
    INT32               accuracy;                                ///< Accuracy used during tracking.
    BOOL                rollAngleExtension;                      ///< Boolean flag to set restriction on roll angle while
                                                                 ///< tracking.
    BOOL                yawAngleExtension;                       ///< Same as rollAngleExtension. used to set flag for yaw
                                                                 ///< angle.
    BOOL                useHeadTracking;                         ///< Flag whether to activate head tracking.
    BOOL                directionMask;                           ///< Boolean flag to set restriction on the detection roll
                                                                 ///< angles for new face search.
};

/// @brief enumerates PT detection mode
enum FDEnginePTMode
{
    PTDefault = 0,  ///< PT search in default mode
    PTPerf,         ///< PT search in perf mode
    PTTurbo         ///< PT search in turbo mode
};

/// @brief enumerates PT confidence mode
enum FDEnginePTConfMode
{
    PTConfEnable = 0,   ///< PT with confidence enable mode
    PTConfDisable       ///< PT with confidence disable mode
};

/// @brief Modes to set PT information
enum FDEnginePTInputType
{
    PTUseFDInfo = 0,          ///< PT set face information using FD result, including ROI center, pose and angle
    PTUseCoordinates,         ///< PT set face position by 4 corners of ROI
    PTUseFDHandle,            ///< PT set face information from SW FD handle
    
};

/// @brief PT parameters
struct FDEnginePTParams
{
    UINT8                 majorVersion;   ///< Major version of SW PT library
    UINT8                 minorVersion;   ///< Minor version of SW PT library
    FDEnginePTMode        mode;           ///< PT detection Mode
    FDEnginePTConfMode    confMode;       ///< PT Confidence Mode
    FDEnginePTInputType   inputType;      ///< PT Face information input type
};

/// @brief CT detection mode
enum FDEngineCTInputType
{
    CTUseCoordinates = 0,  ///< CT Default mode, set facial parts information with points
    CTUsePTHandle          ///< CT set facial parts information with PT handle
};

/// @brief CT result mode
enum FDEngineCTResultMode
{
    CTResult89Points = 0,  ///< CT Default result, 89 points
};

/// @brief CT parameters
struct FDEngineCTParams
{
    UINT8                  majorVersion;   ///< Major version of SW CT library
    UINT8                  minorVersion;   ///< Minor version of SW CT library
    FDEngineCTInputType    inputType;      ///< CT mode
    FDEngineCTResultMode   resultMode;     ///< mode for CT result
};

/// @brief SM detection mode
enum FDEngineSMInputType
{
    SMUseCoordinates = 0,  ///< SM set eye and mouth infomraiton by coordinates
    SMUsePTHandle          ///< SM set eye and mouth information using PT Handle
};

/// @brief SM parameters
struct FDEngineSMParams
{
    UINT8                 majorVersion;   ///< Major version of SW SM library
    UINT8                 minorVersion;   ///< Minor version of SW SM library
    FDEngineSMInputType   inputType;      ///< SM mode to set points
};

/// @brief GB set informaiton mode
enum FDEngineGBInputType
{
    GBUseCoordinates = 0,  ///< GB Default mode, set eye points by coordinates
    GBUsePTHandle          ///< GB set eye points using PT handle
};

/// @brief GB parameters
struct FDEngineGBParams
{
    UINT8                 majorVersion;   ///< Major version of SW GB library
    UINT8                 minorVersion;   ///< Minor version of SW GB library
    FDEngineGBInputType   inputType;      ///< GB mode to set points
};

/// @brief FDEngine False Positive filter parameters
struct FDEngineFPFilterParams
{
    INT32               faceSearchDensity;           ///< Density parameter
    UINT32              faceSizePerc;                ///< Face size percentage to calculte min, max face size for config
    UINT32              faceBoxBorderPerc;           ///< Additional box area to set as CROP rect for face detection dt config
    FLOAT               faceSpreadTolerance;         ///< Face spread tolerance that can be allowed while accepting the face
    FDEngineDTThresInfo thresholdInfo;               ///< Threshold information with angle
    FLOAT               dlThreshold;                 ///< DL parameter, face candidates with probability
                                                     ///< lower than threshold will be discarded
    UINT32              maxNumOfFacesSecondCheck;    ///< Max number of faces allowed for FP filter second check
    UINT32              maxFaceSizeSecondCheck;      ///< Max size of face for FP filter second check
    UINT32              minFaceSizeSecondCheck;      ///< Min size of face for FP filter second check
    UINT32              minHWConfidenceSecondCheck;  ///< Min HW confidence of face for FP filter second check
    UINT32              minDLConfidenceSecondCheck;  ///< Min DL confidence of face for FP filter second check
    BOOL                uprightFaceOnlySecondCheck;  ///< Flag to run FP filter second check only for upright faces
};

/// @brief FDEngine ROI Generator parameters
struct FDEngineROIGenParams
{
    INT32               faceSearchDensity;           ///< Density parameter
    UINT32              faceSizePerc;                ///< Face size percentage to calculte min, max face size for dt config
    UINT32              faceBoxBorderPerc;           ///< Additional box area to set as CROP rect for face detection dt config
    FLOAT               faceSpreadTolerance;         ///< Face spread tolerance that can be allowed while accepting the face
    FDEngineDTThresInfo thresholdInfo;               ///< Threshold information with angle
};

/// @brief DSP paramters
struct FDEngineDSPParams
{
    UINT8   majorVersion;   ///< Major version of DSP library
    UINT8   minorVersion;   ///< Minor version of DSP library
    INT32   absapi;         ///< Whether to use dsp abs api
    INT32   clock;          ///< DSP clock speed
    INT32   bus;            ///< DSP bus bandwidth
    INT32   latency;        ///< DSP HW latency
    INT32   dcvsClock;      ///< DCVS dsp target corner clock Range 0 to 6
    BOOL    enableDCVS;     ///< Whether to enable DSP dcvs
};

/// @brief FD FDEngineROIMgrConfig config
struct FDEngineROIMgrConfig
{
    INT32   holdCount;                      ///< Hold count
    INT32   delayCount;                     ///< Delay count
    INT32   swGoodFaceThreshold;            ///< Good face detection threshold
    INT32   swThreshold;                    ///< Face detection threshold
    INT32   swTrackingThreshold;            ///< Face tracking threshold
    FLOAT   tolMoveDist;                    ///< Tolerance for moving distance
    FLOAT   tolSizeRatioMin;                ///< Minimum allowance of size ratio
    FLOAT   tolSizeRatioMax;                ///< Maximum allowance of size ratio
    FLOAT   tolAngle;                       ///< Tolerance for angle variation
    INT32   strictSwGoodFaceThreshold;      ///< Strict Good face detection threshold
    INT32   strictSwThreshold;              ///< Strict face detection threshold
    INT32   strictSwTrackingThreshold;      ///< Strict face tracking threshold
    INT32   angleDiffForStrictThreshold;    ///< Angle difference for Strict thresholds
};

/// @brief Configuration parameter of Face Detection Engine
struct FDEngineConfig
{
    BOOL                    enableSWFPFilter;    ///< Whether to enable sw false positive filtering.
    BOOL                    enableFacialParts;   ///< Whether to enable Facial Parts
    BOOL                    enableContour;       ///< Whether to enable Coutour exexution
    BOOL                    enableSmile;         ///< Whether to enable Smile execution
    BOOL                    enableGaze;          ///< Whether to enable Gaze execution
    BOOL                    enableBlink;         ///< Whether to enable Blink execution
    BOOL                    lockFacesEnable;     ///< Whether to lock faces during tracking that were detected.
                                                 ///< Locked faces will not be ignored if num_faces detected
                                                 ///< exceeds max number of faces
    FDEngineCOParams        paramsCO;            ///< CO config parameters
    FDEngineDTParams        paramsDT;            ///< DT config parameters
    FDEnginePTParams        paramsPT;            ///< PT config parameters
    FDEngineCTParams        paramsCT;            ///< CT config parameters
    FDEngineSMParams        paramsSM;            ///< SM config parameters
    FDEngineGBParams        paramsGB;            ///< GB config parameters
    FDEngineDSPParams       paramsDSP;           ///< DSP config parameters
    FDEngineFPFilterParams  paramsFPFilter;      ///< False Positive filter config parameters
    FDEngineROIGenParams    paramsROIGen;        ///< False Positive filter config parameters
    FDEngineROIMgrConfig    ROIMgrConfig;        ///< ROI Manager config parameters
};

/// @brief enumerates detection mode
enum FDEngineDetectionMode
{
    FullSearch = 0,     ///< Full search mode
    ContinuousSearch,   ///< Continuous search mode
    PartialSearch,      ///< Partial search mode
};

/// @brief Enumerates FD engine type
enum FDEngineType
{
    Standard_ARM = 0,        ///< Standard ARM engine
    Standard_DSP,            ///< Standard DSP engine
    DL_ARM,                  ///< Deeplearning based ARM engine
    DL_DSP,                  ///< Deeplearning based DSP engine
    DL_DSP_and_Standard_ARM, ///< Deeplearning based DSP and Standard ARM combined engine
};

/// @brief Initialization parameters
struct FDEngineCreateParams
{
    BOOL                    createFacialParts;      ///< Facial parts feature enable/disable
    BOOL                    createFacialContour;    ///< Facial Contour feature enable/disable
    BOOL                    createFacialGazeBlink;  ///< Facial Gaze & blink estimation feature enable/disable
    BOOL                    createFacialSmile;      ///< Facial smile estimation feature enable/disable
    BOOL                    createROIManager;       ///< Create FD engine with ROI manager
    BOOL                    createFPFilter;         ///< Create FD engine with False Positive filter
    BOOL                    createROIGenerator;     ///< Create FD engine with ROI Generator
    BOOL                    createSWDetection;      ///< Create FD engine with SW detection
    BOOL                    createDLFD;             ///< Create FD engine with DL-FD solution
    UINT32                  maxNumInputFace;        ///< Max number of input faces, set this if createROIManager is TRUE
    UINT32                  maxNumOutputFace;       ///< Max number of output faces
    FDEngineDetectionMode   detectionMode;          ///< Standard detection mode
    FDEngineType            detectionEngineType;    ///< Engine platform and type for face detection
    FDEngineClientType      clientType;             ///< Client type
};

/// @brief Number of faces to be processed for each feature
struct FDEngineProcessLimit
{
    UINT32 partsLimit;             ///< Number of faces to be processed for facial parts detection
    UINT32 contourLimit;           ///< Number of faces to be processed for facial contour detection
    UINT32 smileLimit;             ///< Number of faces to be processed for smile detection
    UINT32 gazeLimit;              ///< Number of faces to be processed for gaze detection
    UINT32 blinkLimit;             ///< Number of faces to be processed for blink detection
};

/// @brief Per frame update infomation
struct FDEnginePerFrameInfo
{
  UINT64               frameID;          ///< Frame ID of current frame
  INT32                deviceRotation;   ///< DeviceRotation data
  BOOL                 bufferCached;     ///< Whether input frame is accessed by CPU
  FDEngineImage        imageInfo;        ///< Frame buffer information
  FDEngineProcessLimit procLimit;        ///< Number of faces to be processed for each feature
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDEngineCreate
///
/// @brief  Create FD engine session
///
/// @param  pFDEngineCreateParams     Create parameters for FD Engine creation
/// @param  phFDEngineHandle          FD Engine handle returned to caller
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult FDEngineCreate(
    FDEngineCreateParams*   pFDEngineCreateParams,
    FDEngineHandle*         phFDEngineHandle);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDEngineDestroy
///
/// @brief  Destroy FD engine session handle
///
/// @param  hFDEngineHandle  FD Engine handle
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult FDEngineDestroy(
    FDEngineHandle  hFDEngineHandle);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDEngineGetConfig
///
/// @brief  Get FD engine session config
///
/// @param  hFDEngineHandle  FD Engine handle
/// @param  pConfig          Config to get
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult FDEngineGetConfig(
    FDEngineHandle  hFDEngineHandle,
    FDEngineConfig* pConfig);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDEngineSetConfig
///
/// @brief  Set FD engine session config
///
/// @param  hFDEngineHandle  FD Engine handle
/// @param  pConfig          Config got from upper layer
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult FDEngineSetConfig(
    FDEngineHandle  hFDEngineHandle,
    FDEngineConfig* pConfig);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDEngineRunDetect
///
/// @brief  Run SW Face Detection, Facial Parts, Face Recognition & Intermittent SW Detection
///
/// @param  hFDEngineHandle    FD Engine handle
/// @param  pPerFrameInfo      Information update per frame
/// @param  pResult            Output face result
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult FDEngineRunDetect(
    FDEngineHandle          hFDEngineHandle,
    FDEnginePerFrameInfo*   pPerFrameInfo,
    FDResults*              pResult);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDEngineRunDLFD
///
/// @brief  Run DL-FD detection
///
/// @param  hFDEngineHandle    FD Engine handle
/// @param  pPerFrameInfo      Information update per frame
/// @param  pResult            Output face result
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult FDEngineRunDLFD(
    FDEngineHandle          hFDEngineHandle,
    FDEnginePerFrameInfo*   pPerFrameInfo,
    FDResults*              pResult);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDEngineProcessResult
///
/// @brief  Process Face Results to combine results and remove False positives
///
/// @param  hFDEngineHandle  FD Engine handle
/// @param  pPerFrameInfo    Information update per frame
/// @param  pMainResult      Face result by Main Detector -- typically faster HW
/// @param  pAuxResult       Face result by Aux assist Detector -- typically SW or DL
/// @param  pOutputResult    Output face result
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult FDEngineProcessResult(
    FDEngineHandle          hFDEngineHandle,
    FDEnginePerFrameInfo*   pPerFrameInfo,
    FDIntermediateResults*  pMainResult,
    FDIntermediateResults*  pAuxResult,
    FDResults*              pOutputResult);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDEngineFacialPartsDetect
///
/// @brief  Detect Facial Part points for existing faces
///
/// @param  hFDEngineHandle  FD Engine handle
/// @param  pPerFrameInfo    Information update per frame
/// @param  pFDResult        Face information container, used as both input and output structure
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult FDEngineFacialPartsDetect(
    FDEngineHandle          hFDEngineHandle,
    FDEnginePerFrameInfo*   pPerFrameInfo,
    FDResults*              pFDResult);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDEngineFacialContourDetect
///
/// @brief  Detect Facial Contour points for existing faces
///
/// @param  hFDEngineHandle  FD Engine handle
/// @param  pPerFrameInfo    Information update per frame
/// @param  pFDResult        Face information container, used as both input and output structure
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult FDEngineFacialContourDetect(
    FDEngineHandle          hFDEngineHandle,
    FDEnginePerFrameInfo*   pPerFrameInfo,
    FDResults*              pFDResult);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDEngineFacialAttriDetect
///
/// @brief  Detect Facial Attributes for existing faces
///
/// @param  hFDEngineHandle  FD Engine handle
/// @param  pPerFrameInfo    Information update per frame
/// @param  pFDResult        Face information container, used as both input and output structure
/// @param  featureMask      Selection of features to run, could be SM, GB or SM&GB
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult FDEngineFacialAttriDetect(
    FDEngineHandle          hFDEngineHandle,
    FDEnginePerFrameInfo*   pPerFrameInfo,
    FDResults*              pFDResult,
    FacialAttributeMask     featureMask);

#ifdef __cplusplus
}
#endif // __cplusplus

CAMX_NAMESPACE_END

#endif // end of CAMXFDENGINEINTERFACE_H
