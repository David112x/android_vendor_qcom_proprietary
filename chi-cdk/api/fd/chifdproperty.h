////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file chifdproperty.h
/// @brief Define Qualcomm Technologies, Inc. FD property definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFDPROPERTY_H
#define CHIFDPROPERTY_H

#include "chicommontypes.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/// Data structures that are exposed to OEM must be packed to have the expected layout.
#pragma pack(push, CDK_PACK_SIZE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describes Common FD data structures
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const UINT8  FDMaxFaceCount              = 10;       ///< Max number of faces that FD solution can detect
static const UINT8  FDMaxFaceCountIntermediate  = 35;       ///< Max number of intermediate (probable) faces that FD processes
static const INT32  FDInvalidPoint              = -2000;    ///< Co-ordinate value indicating an invalid point

static const CHAR* VendorTagSectionOEMFDConfig  = "org.quic.camera2.oemfdconfig"; ///< OEM FD config vendor tag section
static const CHAR* VendorTagNameOEMFDConfig     = "OEMFDConfig";                  ///< OEM FD config vendor tag name

static const CHAR* VendorTagSectionOEMFDResults    = "org.quic.camera2.oemfdresults";   ///< OEM FD results vendor tag section
static const CHAR* VendorTagNameOEMFDResults       = "OEMFDResults";                    ///< OEM FD rsults vendor tag name

///< Facial attribute settings vendor tag section
static const CHAR* VendorTagSectionFacialAttrSettings = "org.codeaurora.qcamera3.facial_attr";
static const CHAR* VendorTagNameSmileSetting          = "smile_enable";   ///< Smile detection vendor tag name
static const CHAR* VendorTagNameGazeSetting           = "gaze_enable";    ///< Gaze detection vendor tag name
static const CHAR* VendorTagNameBlinkSetting          = "blink_enable";   ///< Blink detection vendor tag name
static const CHAR* VendorTagNameContourSetting        = "contour_enable"; ///< Contour detection vendor tag name

///< Stats facial attribute vendor tag section
static const CHAR* VendorTagSectionStats              = "org.codeaurora.qcamera3.stats";
static const CHAR* VendorTagNameBSGCAvailable         = "bsgc_available";    ///< BSGC available vendor tag name
static const CHAR* VendorTagNameSmileDegree           = "smile_degree";      ///< Smile degree vendor tag name
static const CHAR* VendorTagNameSmileConfidence       = "smile_confidence";  ///< Smile confidence vendor tag name
static const CHAR* VendorTagNameGazeFacePose          = "gaze_angle";        ///< Gaze face pose (yaw angle) vendor tag name
static const CHAR* VendorTagNameGazeFaceDirection     = "gaze_direction";    ///< Gaze face direction vendor tag name
static const CHAR* VendorTagNameGazeDirection         = "gaze_degree";       ///< Gaze direction vendor tag name
static const CHAR* VendorTagNameBlinkDetected         = "blink_detected";    ///< Blink detected vendor tag name
static const CHAR* VendorTagNameBlinkDegree           = "blink_degree";      ///< Blink degree vendor tag name
static const CHAR* VendorTagNameContourPoints         = "contours";          ///< Contour points vendor tag name

/// @brief This enum indicates the type for face size configuration
enum FDFaceSizeType
{
    FDFaceAdjFixed,     ///< When set, use absolute value set for minimmum
    FDFaceAdjFloating,  ///< When set, min face size is calculated based on ratio
};

/// @brief This enum indicates the value for angle configuration
enum FDSearchAngle
{
    FDAngleNone,    ///< Angle none
    FDAngle15,      ///< 15 degrees each side
    FDAngle45,      ///< 45 degrees each side
    FDAngle75,      ///< 75 degrees each side
    FDAngle105,     ///< 105 degrees each side
    FDAngle135,     ///< 135 degrees each side
    FDAngleAll,     ///< All angles
};

/// @brief This enum indicates the value for search accuracy for already detected faces
enum FDFaceAccuracy
{
    FDAccuracyHigh      = 0,    ///< High accuracy
    FDAccuracyNormal    = 15,   ///< Normal accuracy
};

/// @brief This enum indicates the value for face detection mode
enum FDDetectionMode
{
    FDDetectionModeDefault,     ///< Default detection mode
    FDDetectionModeFull,        ///< Full mode
    FDDetectionModeLite,        ///< Lite mode
    FDDetectionModeContinuous,  ///< Continuous mode
};

/// @brief This enum indicates the values for detection search density
enum FDSearchDensity
{
    FDSearchDensityHighest  = 200,  ///< Highest search density
    FDSearchDensityHigh     = 150,  ///< High search density
    FDSearchDensityNormal   = 100,  ///< Normal search density
    FDSearchDensityLow      = 75,   ///< Low search density
    FDSearchDensityLowest   = 50,   ///< Lowest search density
};

/// @brief This enum indicates the index values for each facial point
enum FDFacialPointIndex
{
    FDFacialPointLeftEye = 0,   ///< Index indicating facial point left eye
    FDFacialPointRightEye,      ///< Index indicating facial point right eye
    FDFacialPointMouth,         ///< Index indicating facial point mouth
    FDFacialPointLeftEyeIn,     ///< Index indicating facial point left eye in
    FDFacialPointLeftEyeOut,    ///< Index indicating facial point left eye out
    FDFacialPointRightEyeIn,    ///< Index indicating facial point right eye in
    FDFacialPointRightEyeOut,   ///< Index indicating facial point right eye out
    FDFacialPointMouthLeft,     ///< Index indicating facial point mout left
    FDFacialPointMouthRight,    ///< Index indicating facial point mouth right
    FDFacialPointNoseLeft,      ///< Index indicating facial point nose left
    FDFacialPointNoseRight,     ///< Index indicating facial point nose right
    FDFacialPointMouthUp,       ///< Index indicating facial point mouth up
    FDFacialPointMax,            ///< Index indicating facial point max
};

/// @brief This enum indicates the index values for each facial contour
enum FDContourIndex
{
    FDContourEyeLeftOut = 0,
    FDContourEyeLeftIn,
    FDContourEyeLeftUp,
    FDContourEyeLeftDown,
    FDContourEyeLeftOutUp,
    FDContourEyeLeftOutDown,
    FDContourEyeLeftInUp,
    FDContourEyeLeftInDown,
    FDContourBrowLeftOut,
    FDContourBrowLeftIn,
    FDContourBrowLeftUp,
    FDContourBrowLeftDown,
    FDContourBrowLeftOutUp,
    FDContourBrowLeftOutDown,
    FDContourBrowLeftInUp,
    FDContourBrowLeftInDown,
    FDContourEyeRightIn,
    FDContourEyeRightOut,
    FDContourEyeRightUP,
    FDContourEyeRightDown,
    FDContourEyeRightInUp,
    FDContourEyeRightInDown,
    FDContourEyeRightOutUp,
    FDContourEyeRightOutDown,
    FDContourBrowRightIn,
    FDContourBrowRightOut,
    FDContourBrowRightUP,
    FDContourBrowRightDown,
    FDContourBrowRightInUp,
    FDContourBrowRightInDown,
    FDContourBrowRightOutUp,
    FDContourBrowRightOutDown,
    FDContourNoseLeft,
    FDContourNoseRight,
    FDContourNose,
    FDContourNoseTip,
    FDContourNoseLeft3,
    FDContourNoseLeft4,
    FDContourNoseRight3,
    FDContourNoseRight4,
    FDContourMouthLeft,
    FDContourMouthRight,
    FDContourMouthUp,
    FDContourLipUp,
    FDContourLipDown,
    FDContourMouthDown,
    FDContourMouthLeftUP1,
    FDContourLipLeftUP1,
    FDContourLipLeftDown1,
    FDContourMouthLeftDown1,
    FDContourMouthRightUP1,
    FDContourLipRightUP1,
    FDContourLipRightDown1,
    FDContourMouthRightDown1,
    FDContourMouthLeftUP2,
    FDContourLipLeftUP2,
    FDContourLipLeftDown2,
    FDContourMouthLeftDown2,
    FDContourMouthRightUP2,
    FDContourLipRightUP2,
    FDContourLipRightDown2,
    FDContourMouthRightDown2,
    FDContourFaceLeft10,
    FDContourFaceLeft9,
    FDContourFaceLeft8,
    FDContourFaceLeft7,
    FDContourFaceLeft6,
    FDContourFaceLeft5,
    FDContourFaceLeft4,
    FDContourFaceLeft3,
    FDContourFaceLeft2,
    FDContourFaceLeft1,
    FDContourChin,
    FDContourFaceRight1,
    FDContourFaceRight2,
    FDContourFaceRight3,
    FDContourFaceRight4,
    FDContourFaceRight5,
    FDContourFaceRight6,
    FDContourFaceRight7,
    FDContourFaceRight8,
    FDContourFaceRight9,
    FDContourFaceRight10,
    FDContourEyePupilLeft,
    FDContourEyePupilRight,
    FDContourForehead,
    FDContourNoseCenter1,
    FDContourNoseCenter2,
    FDContourNoseCenter3,
    FDContourMax                   /* Number of Indices */
};

/// @brief Structure Face ROI information
struct FDFaceROI
{
    UINT32      confidence; ///< Confidence of the face
    CHIPoint    center;     ///< Center of the face
    UINT32      width;      ///< Width of the face
    UINT32      height;     ///< Height of the face
    INT32       rollAngle;  ///< Roll angle of the face
    INT32       pose;       ///< Pose angle of the face
};

/// @brief Structure Accurate face direction info
struct FDFaceDirection
{
    BOOL    valid;      ///< Whether this info is valid
    INT32   upDown;     ///< Up-Down direction of the face, range [-180, 179]
                        ///< Positive value means face is in upward direction
    INT32   leftRight;  ///< Left-Right direction of the face, range [-180, 179]
                        ///< Positive value means face is facing right
    INT32   rollAngle;  ///< Roll angle of the face, range [-180, 179]
                        ///< Positive value means face is rotated clock-wise in-plane
};

/// @brief Structure Face Point attributes
struct FDFacePoint
{
    CHIPoint    position;   ///< Position of the face point
    UINT32      confidence; ///< Confidence of the face point
};

/// @brief Structure Facial parts points
struct FDFaceParts
{
    BOOL        valid;                          ///< Whether this info is valid
    FDFacePoint facialPoint[FDFacialPointMax];  ///< Face facial points
};

/// @brief Structure Facial contour points
struct FDFaceContour
{
    BOOL        valid;                         ///< Whether this info is valid
    FDFacePoint contourPoint[FDContourMax];    ///< Face contour points
};

/// @brief Structure Facial Gaze direction
struct FDFaceGazeDirection
{
    BOOL    valid;              ///< Whether this info is valid
    INT32   leftRightGaze;      ///< left-Right gaze value, range [-90, 90]
                                ///< The direction is in respect to image plane
    INT32   upDownGaze;         ///< Up-Down gaze value, range [-90, 90]
                                ///< The direction is in respect to image plane
};

/// @brief Structure Facial Blink estimation
struct FDFaceBlinkEstimation
{
    BOOL    valid;              ///< Whether this info is valid
    BOOL    blinkDetected;      ///< Whether blink is detected, binary value, TRUE or FALSE
    INT32   leftBlink;          ///< Left eye blink ratio, range [1, 1000]
                                ///< A value closer to 1 means the eye is closer to being fully opened
                                ///< A value closer to 1000 means the eye is closer to being fully closed.
    INT32   rightBlink;         ///< Righ eye blink ratio, range [1, 1000]
                                ///< A value closer to 1 means the eye is closer to being fully opened
                                ///< A value closer to 1000 means the eye is closer to being fully closed.
};

/// @brief Structure Face smile attributes
struct FDFaceSmile
{
    BOOL    valid;      ///< Whether this info is valid
    INT32   degree;     ///< Smile degree, range [0, 100].
                        ///< Value of 0 indicates no smile, value of 100 indicates a full smile
    UINT32  confidence; ///< Confidence of smile degree, range [0, 1000]
                        ///< Higher value indicates a higher confidence in the estimation
};

/// @brief Structure Face attributes
struct FDFaceInfo
{
    INT32                 faceID;         ///< Face unique ID
    BOOL                  newFace;        ///< Whether this face is a new face
    FDFaceROI             faceROI;        ///< Face ROI information
    FDFaceDirection       direction;      ///< Face direction information
    FDFaceParts           facialParts;    ///< Facial parts information
    FDFaceContour         contour;        ///< Face contour information
    FDFaceGazeDirection   gaze;           ///< Gaze information
    FDFaceBlinkEstimation blink;          ///< Blink information
    FDFaceSmile           smile;          ///< Smile information
};

/// @brief Structure Face results
struct FDResults
{
    UINT8       numFacesDetected;           ///< Number of faces detected
    FDFaceInfo  faceInfo[FDMaxFaceCount];   ///< List of faces detected with face information
};

/// @brief Structure Face intermediate results
struct FDIntermediateResults
{
    UINT8       numFacesDetected;                       ///< Number of faces detected
    FDFaceInfo  faceInfo[FDMaxFaceCountIntermediate];   ///< List of intermediate probable faces detected with face information
};

CAMX_BEGIN_PACKED

/// FD Metadata results structure layouts

/// @brief Structure FD Point
struct FDPoint
{
    INT32   x;  ///< Co-ordinate indicating x value
    INT32   y;  ///< Co-ordinate indicating y value
} CAMX_PACKED;

/// @brief Structure FD Meta data face rect
struct FDMetaDataFaceRect
{
    FDPoint topLeft;        ///< Point indicating top left of face rectangle
    FDPoint bottomRight;    ///< Point indicating bottom right of face rectangle
} CAMX_PACKED;

/// @brief Structure FD Metadata face landmark layout
struct FDMetaDataFaceLandmark
{
    FDPoint leftEyeCenter;  ///< Point indicating left eye center
    FDPoint rightEyeCenter; ///< Point indicating right eye center
    FDPoint mouthCenter;    ///< Point indicating mouth center
} CAMX_PACKED;

/// @brief Structure FD Metadata blink degree layout
struct FDMetaDataBlinkDegree
{
    UINT8   leftBlink;  ///< Left eye blink ratio, range [1, 100]
    UINT8   rightBlink; ///< Right eye blink ratio, range [1, 100]
} CAMX_PACKED;

/// @brief Structure FD Metadata face direction layout (this was called gaze direction on old target)
struct FDMetaDataFaceDirection
{
    INT32   upDown;    ///< Face up-down direction value, range [-180, 179]
    INT32   leftRight; ///< Face left-right direction value, range [-180, 179]
    INT32   roll;      ///< Roll angle, range [-180, 179]
} CAMX_PACKED;

/// @brief Structure FD Metadata gaze direction layout (this was called gaze degree on old target)
struct FDMetaDataGazeDirection
{
    INT8   leftRight;  ///< Gaze left-right direction value, range [-90, 90]
    INT8   topBottom;  ///< Gaze top-bottom direction value, range [-90, 90]
} CAMX_PACKED;

/// @brief Structure Face Detection meta data results. These are inline with Android meta data tag expectations.
struct FDMetaDataResults
{
    UINT8                     numFaces;                           ///< Number of faces detected
    INT32                     faceID[FDMaxFaceCount];             ///< List of unique IDs for detected faces
    UINT8                     faceScore[FDMaxFaceCount];          ///< List of face confidence scores for detected faces
    FDMetaDataFaceRect        faceRect[FDMaxFaceCount];           ///< List of face rect info for detected faces
    FDMetaDataFaceLandmark    faceLandmark[FDMaxFaceCount];       ///< List of face landmark info for detected faces
    UINT8                     blinkDetected[FDMaxFaceCount];      ///< List of blink detected flags for detected faces
                                                                  ///< Blink detected range [0, 1] (FALSE, TRUE)
    FDMetaDataBlinkDegree     blinkDegree[FDMaxFaceCount];        ///< List of blink degree values for detected faces
    UINT8                     smileDegree[FDMaxFaceCount];        ///< List of smile degree info for detected faces
                                                                  ///< Smile degree range [0, 100]
    UINT8                     smileConfidence[FDMaxFaceCount];    ///< List of smile confidence info for detected faces
                                                                  ///< Smile confidence range [0, 100]
    INT8                      gazeAngle[FDMaxFaceCount];          ///< List of gaze angle (face pose) info for detected faces
                                                                  ///< Gaze angle (yaw angle) range: -90, -45, 0, 45, 90
    FDMetaDataFaceDirection   gazeFaceDirection[FDMaxFaceCount];  ///< List of gaze face direction info for detected faces
    FDMetaDataGazeDirection   gazeDirection[FDMaxFaceCount];      ///< List of gaze direction info for detected faces
} CAMX_PACKED;

struct FDMetaDataFaceContour
{
    FDPoint                   contourPoint[FDContourMax];         ///< Face contour points
} CAMX_PACKED;

/// @brief Structure Facial attribute contour results
struct FDMetaDataFaceContourResults
{
    UINT8                     numFaces;                           ///< Number of faces with contour info
    INT32                     faceID[FDMaxFaceCount];             ///< List of unique IDs for detected faces
    FDMetaDataFaceContour     faceContour[FDMaxFaceCount];        ///< List of face contour information for detected faces
} CAMX_PACKED;

CAMX_END_PACKED

/// @brief enum to define all the different stabilization modes
enum FDStabilizationMode
{
    Equal,                          ///< Values will be marked as stable when two consecutive values are equal
    Smaller,                        ///< Values will be marked as stable if new values are bigger than old ones
    Bigger,                         ///< Values will be marked as stable if new values are smaller than old ones
    CloserToReference,              ///< Values will be marked as stable when the distance to reference is smaller
    ContinuesEqual,                 ///< The same as Equal, but it works in continues mode
    ContinuesSmaller,               ///< The same as Smaller, but it works in continues mode
    ContinuesBigger,                ///< The same as Bigger, but it works in continues mode
    ContinuesCloserToReference,     ///< The same as CloserToReference, but it works in continues mode
    WithinThreshold                 ///< Values are marked as stable when values are within threshold
};

/// @brief enum to define all the different stabilization filters
enum FDStabilizationFilter
{
    NoFilter,      ///< No stabilization filter
    Temporal,      ///< Temporal filter
    Hysteresis,    ///< Hysteresis
    Average,       ///< Average filter
    Median         ///< Median filter
};

/// @brief Structure to define Temporal Filter
struct FDTemporalFilter
{
    UINT32 numerator;    ///< Strength numerator
    UINT32 denominator;  ///< Strength denominator
};

/// @brief Structure to define Hysteresis Filter
struct FDHysteresisFilter
{
    UINT32 startA;    ///< Start point of Zone A
    UINT32 endA;      ///< End point of Zone A
    UINT32 startB;    ///< Start point of Zone B
    UINT32 endB;      ///< End point of Zone B
};

/// @brief Structure to define Average Filter
struct FDAverageFilter
{
    UINT32 historyLength;         ///< History length of the filter
    UINT32 movingHistoryLength;   ///< History length of the filter for moving face
};

/// @brief Structure to define Median Filter
struct FDMedianFilter
{
    UINT32 historyLength;  ///< History length of the filter
};

/// @brief Structure to hold stabilization tuning parameters
struct FDStabilizationAttributeConfig
{
    BOOL                    enable;               ///< Enable stabilization
    FDStabilizationMode     mode;                 ///< Stabilization Mode
    UINT32                  minStableState;       ///< Minimum state count needed to go to a stable state
    UINT32                  stableThreshold;      ///< Stabilization threshold to go into stable state
    UINT32                  threshold;            ///< Within threshold, new values will not be accepted
    UINT32                  stateCount;           ///< Number of consecutive frames to wait until determining stable
    UINT32                  useReference;         ///< Stabilize data by a reference
    FDStabilizationFilter   filterType;           ///< Filter type to be used for stabilization
    UINT32                  movingThreshold;      ///< With threshold, object is not moved
    UINT32                  movingInitStateCount; ///< Init state count for moving face during stabilizing state
    FLOAT                   movingLinkFactor;     ///< Factor to determine moving face with previous 2 history
    union
    {
        FDTemporalFilter    temporalFilter;       ///< Temporal Filter
        FDHysteresisFilter  hysteresisFilter;     ///< HysteresisFilter
        FDAverageFilter     averageFilter;        ///< Average Filter
        FDMedianFilter      medianFilter;         ///< Median Filter
    };
};

/// @brief Structure to hold stabilization configuration
struct FDStabilizationConfig
{
    BOOL                            enable;         ///< Enable stabilization
    UINT32                          historyDepth;   ///< Depth of historical data
    FDStabilizationAttributeConfig  position;       ///< Position attibute configurations
    FDStabilizationAttributeConfig  size;           ///< Position attibute configurations
};

/// @brief Structure Face size configuration
struct FDFaceSize
{
    enum FDFaceSizeType type;   ///< Face size type
    union
    {
        UINT32  size;           ///< Face size absolute value
        FLOAT   ratio;          ///< Face size ratio. Size is calculated based on this ratio w.r.t input frame
    };
};

/// @brief Structure Face search angle, threshold configuration for the given profile
struct FDProfileConfig
{
    FDSearchAngle       searchAngle;                ///< Search Angle. Used in below cases :
                                                    ///  When upFront is disabled
                                                    ///  Even when upFront is enabled, if upfront angle cannot be determined
    FDSearchAngle       upFrontSearchAnggle;        ///< Search angle when upFront is enabled and upfront angle can be
                                                    ///< determined
    FDSearchAngle       priorityAngleRange;         ///< Priority angle range for threshold configuration when upfront
                                                    ///is enabled
    INT32               priorityThreshold;          ///< Threshold in priority region
    INT32               nonPriorityThreshold;       ///< Threshold in non-priority region
};

/// @brief Structure Face detection HW configuration
struct FDHwConfig
{
    BOOL            enable;                 ///< Whether to enable HW detection
    FDFaceSize      minFaceSize;            ///< Minimum face size configuration for HW
    FDFaceSize      maxFaceSize;            ///< Maximum face size configuration for HW
    BOOL            enableUpFrontAngles;    ///< Whether to enable upfront only detection when possible.
                                            /// When enabled,
                                            /// If determining upfront is possible, uses 'upFrontAngle' for configuration
                                            /// If determining upfront is not possible, uses 'angle' configuration
    FDSearchAngle   angle;                  ///< Angle configuration for faces in all poses
    UINT32          upFrontAngle;           ///< Angle configuration for faces in all poses for detecting only upfront faces
    UINT32          threshold;              ///< Threshold to determine a face
    UINT32          noFaceFrameSkip;        ///< Frame skip count when no faces detected
    UINT32          newFaceFrameSkip;       ///< Frame skip count when atleast one face is detected
    BOOL            enableHWFP;             ///< Whether to enable HW False Positive Filter
};

/// @brief Structure Face detection SW configuration
struct FDSwConfig
{
    BOOL            enable;                                 ///< Whether to enable SW detection
    FDFaceSize      minFaceSize;                            ///< Min face size configuration
    FDFaceSize      maxFaceSize;                            ///< Max face size configuration
    FDProfileConfig frontProfileConfig;                     ///< Angle configuration for front profile faces
    FDProfileConfig halfProfileConfig;                      ///< Angle configuration for half profile faces
    FDProfileConfig fullProfileConfig;                      ///< Angle configuration for full profile faces
    BOOL            enableUpFrontAngles;                    ///< Whether to enable upfront only detection when possible.
                                                            /// When enabled , If determining upfront is
                                                            ///     possible, uses 'upFrontAngle' for configuration
                                                            ///     not possible, uses 'angle' configuration
    BOOL            skipIfNoOrientation;                    ///< Skiip Sw detection if orientation information is not available
    FDDetectionMode detectionMode;                          ///< Face detection mode
    FDSearchDensity newFaceSearchDensity;                   ///< Face search density to detect new faces
    FDSearchDensity existingFaceSearchDensity;              ///< Face search density for the faces that are already detected
    BOOL            detectNewFacesInExistingFaceDirection;  ///< Detects new faces that are only in the direction of
                                                            ///  already detected faces
    UINT32          noFaceSearchCycle;                      ///< Search cycle when there are no faces detected already.
                                                            ///  Used if upFront is disabled or
                                                            /// if upFront angle can not be determined
    UINT32          newFaceSearchCycle;                     ///< Search cycle when atleast one face is detected
                                                            ///  Used if upFront is disabled or
                                                            /// if upFront angle can not be determined
    UINT32          newFaceSearchInterval;                  ///< Face search interval
                                                            ///  Used if upFront is disabled or
                                                            /// if upFront angle can not be determined
    UINT32          upFrontNoFaceSearchCycle;               ///< Search cycle when there are no faces detected already. Used
                                                            ///  if upFront is enabled and upFront angle can be determined
    UINT32          upFrontNewFaceSearchCycle;              ///< Search cycle when atleast one face is detected. Used
                                                            ///  if upFront is enabled and upFront angle can be determined
    UINT32          upFrontNewFaceSearchInterval;           ///< Face search interval. Used
                                                            ///  if upFront is enabled and upFront angle can be determined
    UINT32          positionSteadiness;                     ///< Face position steadiness value
    UINT32          sizeSteadiness;                         ///< Face size steadiness value
    BOOL            rollAngleExtension;                     ///< Roll angle extension
    BOOL            yawAngleExtension;                      ///< Yaw angle extension
    UINT32          noFaceFrameSkip;                        ///< Frame skip when no faces detected
    UINT32          newFaceFrameSkip;                       ///< Frame skiip when atleast one face is detected
};

/// @brief Structure Face detection false positive filter configuration
struct FDFPFilterConfig
{
    BOOL            enable;                          ///< Whether to enable false positive filtering
    UINT32          baseThreshold;                   ///< Threshold value to configure to FD algo while false positive detection
    UINT32          innerThreshold;                  ///< Inner threshold value to configure FD algo while false positive detection
    UINT32          expandFaceSizePercentage;        ///< Value used to determine face size range while false positive detection
    UINT32          expandBoxBorderPercentage;       ///< Value used to determine search window while false positive detection
    FLOAT           faceSpreadTolerance;             ///< Used to determine face spread tolerance while false positive detection
    FDSearchDensity searchDensity;                   ///< Value used to determine search density while false positive detection
    UINT32          maxNumOfFacesSecondCheck;        ///< Max number of faces allowed for FP filter second check
    UINT32          maxFaceSizeSecondCheck;          ///< Max size of face for FP filter second check
    UINT32          minFaceSizeSecondCheck;          ///< Min size of face for FP filter second check
    UINT32          minHWConfidenceSecondCheck;      ///< Min HW confidence of face for FP filter second check
    UINT32          minDLConfidenceSecondCheck;      ///< Min DL confidence of face for FP filter second check
    BOOL            uprightFaceOnlySecondCheck;      ///< Flag to run FP filter second check only for upright faces
};

/// @brief Structure Face detection false positive filter configuration
struct FDROIGeneratorConfig
{
    BOOL            enable;                     ///< Whether to enable false positive filtering
    UINT32          baseThreshold;              ///< Threshold value to configure to FD algo while ROI generation
    UINT32          innerThreshold;             ///< Inner threshold value to configure FD algo while false positive detection
    UINT32          expandFaceSizePercentage;   ///< Value used to determine face size range while ROI generation
    UINT32          expandBoxBorderPercentage;  ///< Value used to determine search window while ROI generation
    FLOAT           faceSpreadTolerance;        ///< Value used to determine face spread tolerance while ROI generation
    FDSearchDensity searchDensity;              ///< Value used to determine search density while ROI generation
};

/// @brief Structure Face detection ROI manager configuration
struct FDROIManagerConfig
{
    BOOL    enable;                         ///< Whether to enable Face ROI manager
    UINT32  newGoodFaceConfidence;          ///< Confidence to determine a face as good face while detection
    UINT32  newNormalFaceConfidence;        ///< Confidence to determine a face as face while detection
    UINT32  existingFaceConfidence;         ///< Confidence to determine an existing face
    UINT32  angleDiffForStrictConfidence;   ///< Angle in each direction after which strict confidence values are used
                                            ///  to determine the face
    UINT32  strictNewGoodFaceConfidence;    ///< Strict confidence to determine a face as good face while detection
    UINT32  strictNewNormalFaceConfidence;  ///< Strict confidence to determine a face as face while detection
    UINT32  strictExistingFaceConfidence;   ///< Strict confidence to determine an existing face
    FLOAT   faceLinkMoveDistanceRatio;      ///< Move distance ratio threshold to link a face
    FLOAT   faceLinkMinSizeRatio;           ///< Minimum size ratio threshold to link a face
    FLOAT   faceLinkMaxSizeRatio;           ///< Maximum size ratio threshold to link a face
    FLOAT   faceLinkRollAngleDifference;    ///< Roll angle difference threshold to link a face
};

/// @brief Structure for fd tuning data tool version, SW can ignore this
struct FDConfigVersion
{
    UINT32          id;                                 ///< Tuning id
    INT32           majorRevision;                      ///< Major version of tuning version
    INT32           minorRevision;                      ///< Minor version of tuning version
    INT32           incrRevision;                       ///< Incr version of tuning version
};

/// @brief Structure Face detection software preprocessing trigger
struct FDSWPreprocessConfig
{
    FLOAT   exposureShortBySafeThreshold;   ///< Trigger preprocessing of Face Detection frame based on ratio of short and
                                            ///  safe exposure sensitivities
    FLOAT   deltaEVFromTargetThreshold;     ///< Triggers preprocessing of Face Detection frame based on Delta EV from target
};

/// @brief Structure Facial attributes configuration
struct FDFacialAttributeConfig
{
    UINT32  PTDMaxNumberOfFaces;            ///< Maximum number of faces to run facial parts detection
    UINT32  SMDMaxNumberOfFaces;            ///< Maximum number of faces to run smile detection
    UINT32  GBDMaxNumberOfFaces;            ///< Maximum number of faces to run gaze and blink detection
    UINT32  CTDMaxNumberOfFaces;            ///< Maximum number of faces to run contour detection
    BOOL    bFDStabilizationOverride;       ///< Whether to published unstabilized FD results when CT is enabled
};

/// @brief Structure Face detection tuning configuration
struct FDConfig
{
    UINT32                  id;                           ///< Tuning id
    FDConfigVersion         FDConfigData;                 ///< Tuning config info
    UINT32                  maxNumberOfFaces;             ///< Maximum number of faces to detect in the overall FD solution
    UINT32                  maxFPSWithFaces;              ///< Maximum FPS to run face detection processing when
                                                          ///  faces are detected
    UINT32                  maxFPSWithNoFaces;            ///< Maximum FPS to run face detection processing when no
                                                          ///  faces are detected
    UINT32                  multiCameraMaxFPSWithFaces;   ///< Maximum FPS to run face detection processing when
                                                          ///  faces are detected
    UINT32                  multiCameraMaxFPSWithNoFaces; ///< Maximum FPS to run face detection processing when no
                                                          ///  faces are detected
    BOOL                    lockDetectedFaces;            ///< Lock faces that are already detected
    UINT32                  initialNoFrameSkipCount;      ///< First n frames which are always processed for face detection
    UINT32                  maxPendingFrames;             ///< Max allowed frames in pending queue
    UINT32                  delayCount;                   ///< Face delay count
    UINT32                  holdCount;                    ///< Face hold count
    UINT32                  retryCount;                   ///< Face retry count
    FDFaceAccuracy          accuracy;                     ///< Accuracy with which an existing face have to searched
    FDHwConfig              hwConfig;                     ///< HW detection configuration
    FDSwConfig              swConfig;                     ///< SW detection configuration
    FDFPFilterConfig        FPFilter;                     ///< FP filter configuration
    FDROIGeneratorConfig    ROIGenerator;                 ///< ROI generator configuration
    FDROIManagerConfig      managerConfig;                ///< ROI manager configuration
    FDStabilizationConfig   stabilization;                ///< Stabilization configuration
    FDSWPreprocessConfig    swPreprocess;                 ///< FD SW preprocessing trigger configuration
    FDFacialAttributeConfig facialAttrConfig;             ///< Facial attributes configuration
};

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHIFDPROPERTY_H
