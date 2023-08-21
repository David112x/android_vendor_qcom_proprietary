////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chihafinterface.h
/// @brief Hybrid Auto Focus Framework Based Algorithm interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE GR016: Typedefs must not be used for structures, classes, and enumerations

#ifndef CHIHAFINTERFACE_H
#define CHIHAFINTERFACE_H

#include "chistatsinterfacedefs.h"
#include "chiafcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/// Data structures that are exposed to OEM must be packed to have the expected layout.
#pragma pack(push, CDK_PACK_SIZE)

/// @brief HAF Algorithm type
typedef enum
{
    HAFAlgoTypeCAF      = 0,            ///< Algorithm type is CAF
    HAFAlgoTypePDAF,                    ///< Algorithm type is PDAF
    HAFAlgoTypeTOF,                     ///< Algorithm type is TOF
    HAFAlgoTypeMixer,                   ///< default mixer, root algorithm pointed to from HAF
    HAFAlgoTypeFOLLOWER,                ///< Algorithm type is follower
    HAFAlgoTypePDAFTOFMixer,            ///< Algorithm type is mixer to deal with PDAF and TOF algorithm
    HAFAlgoTypeDepthAF,                 ///< Algorithm type is DepthAF
    HAFAlgoTypePDAFDepthAFMixer,        ///< Algorithm type is mixer to deal with PDAF and DepthAF algorithm
    HAFAlgoTypeMax      = 0x7FFFFFFF,   ///< Maximum limit
} HAFAlgoType;

/// @brief HAF Algorithm sensitivity type
typedef enum
{
    HAFSensitivityLevelLow       = 0,           ///< Low sensitivity
    HAFSensitivityLevelMedium    = 50,          ///< Default sensitivity
    HAFSensitivityLevelHigh      = 100,         ///< High sensitivity
    HAFSensitivityLevelMax       = 0x7FFFFFFF   ///< Max Sensitivity
} HAFSensitivityLevel;

/// @brief HAF Algorithm progress level
typedef enum
{
    HAFProgressInvalid          = -1,       ///< Invalid progress
    HAFProgressLevel0           = 0,        ///< level 0 progress
    HAFProgressLevel25          = 25,       ///< level 25 progress
    HAFProgressLevel50          = 50,       ///< level 50 progress
    HAFProgressLevel75          = 75,       ///< level 75 progress
    HAFProgressLevel100         = 100,      ///< level 100 progress
} HAFProgressLevelType;

//< @brief Identifies the type of SetParamInput Type
typedef enum
{
    HAFSetParamTypeInvalid                  = -1,           ///< Invalid type
    HAFSetParamTypeHardwareCapabilities,                    ///< Sets Hardware Capabilities
                                                            ///  Payload AFHardwareCapability
    HAFSetParamTypeTuningData,                              ///< Tuning
                                                            ///  Payload: StatsTuningData
    HAFSetParamTypeISPStreamInfo,                           ///< Sets the stream information from ISP
                                                            ///  Payload: StatsISPStreamInfo
    HAFSetParamTypeSensorInfo,                              ///< Sensor information
                                                            ///  Payload: AFSensorInfo
    HAFSetParamTypeAECInfo,                                 ///< AEC output information
                                                            ///  Payload: AFAECInfo
    HAFSetParamTypeROI,                                     ///< ROI information in case of Touch/Face focus mode
                                                            ///  Payload: AFROIInfo
    HAFSetParamTypeDebugData,                               ///< Buffer to collect debug data
                                                            ///  Payload: StatsDataPointer
    HAFSetParamTypeMeteringMode,                            ///< Indicates the chosen auto focus metering mode
                                                            ///  Payload: AFMeteringType
    HAFSetParamTypeFocusMode,                               ///< Indicates the chosen Focus Mode
                                                            ///  Payload: AFFocusMode
    HAFSetParamTypeSearchRange,                             ///< Indicates the search range
                                                            ///  Payload: AFSearchRange
    HAFSetFocusControlCommand,                              ///< Indicates focus command from upper layer
                                                            ///  Payload:AFControlCommand
    HAFSetParamTypeRunMode,                                 ///< Indicates the camera mode
                                                            ///  Payload: AFRunMode
    HAFSetParamTypeManualLensMoveInfo,                      ///< Indicates Manual Lens Move Info
                                                            ///  Payload : AF_ManualLensMoveInfo
    HAFSetParamTypeIsFirstRun,                              ///< HAF is first run flag
                                                            ///  Payload: BOOL
    HAFSetParamTypeGravityData,                             ///< gravity vector information
                                                            ///  Payload: AFGravityData
    HAFSetParamTypeGyroData,                                ///< gyro information
                                                            ///  Payload: AFGyroData
    HAFSetParamTypeLinearAccelerometerData,                 ///< LinearAccelerometer data
                                                            ///  Payload: AFLinearAccelerationData
    HAFSetParamTypeSpotLightInfo,                           ///< Spotlight Information
                                                            ///  Payload: AFSpotlightInfo
    HAFSetParamTypeSADInfo,                                 ///< Sum of absolute difference information used for scene change detection
                                                            ///  Payload: FLOAT
    HAFSetParamTypeSADWithReferenceInfo,                    ///< Sum of absolute difference with reference used for scene change detection
                                                            ///  Payload: FLOAT
    HAFSetParamTypePDData,                                  ///< PDAF input to AF algorithm
                                                            ///  Payload: AFPDAFData
    HAFSetParamTypeTOFData,                                 ///< Laser Data input to AF algorithm
                                                            ///  Payload: AFTOFData
    HAFSetParamTypeVendorTagData,                           ///< Vendor tag data
                                                            ///  Payload type: StatsVendorTag
    HAFSetParamTypeFocusStatsInfo,                          ///< Focus Stats Info
                                                            ///  Payload: AFFocusStatsInfo
    HAFSetParamTypeBGStats,                                 ///< BG Stats Info
                                                            ///  Payload: StatsBayerGrid
    HAFSetParamTypeSettings,                                    ///< AF Settings
                                                            ///  Payload: AFAlgoSettingInfo
    HAFSetParamTypeMultiWindowEnable,                       ///<  PDAF multi window enable
                                                            ///  Payload: BOOL
    HAFSetParamRoleSwitch,                                  ///< Role Switch info
    HAFSetParamPeerInfo,                                    ///< Peer info
    HAFSetParamFaceROIPriority,                             ///< Face ROI Priority info
    HAFSetParamFaceROIInformation,                          ///< Face ROI Information
    HAFSetParamTrackROIPriority,                            ///< Track ROI priority info
    HAFSetParamTrackROIInformation,                         ///< Track ROI Information
    HAFSetParamTypeAFLEDAssist,                             ///< Led Assist info
                                                            ///  Payload: INT
    HAFSetParamStatsTSInfo,                                 ///< Stats timestamp  and frame id info
                                                            ///  Payload: Void*
    HAFSetParamSensitivity,                                 ///< HAF monitor sensitivity
    HAFSetParamInternalData,                                ///< Internal data for HAF algo communicate
                                                            /// Payload: VOID*
    HAFSetParamDepth16Stats,                                ///< Depth Stats Info
                                                            /// Payload: AFDepth16StatsInfo*
    HAFSetParamLastIndex,                                   ///< Last index of set params defined in this API
                                                            /// Other interfaces can extend from this index

    HAFSetParamTypeMax                      = 0x7FFFFFFF    ///< Max Type
} HAFAlgoSetParamType;

/// @brief Represents an AF SetParam input
typedef struct
{
    VOID*                       pData;          ///< Pointer to AF SetParma Input data See HAFAlgoSetParamType for details
    UINT32                      sizeOfData;     ///< Size of data pointed to by pHAFSetParam
    HAFAlgoSetParamType         setParamType;   ///< Type of the input parameter
} HAFAlgoSetParam;

/// @brief Represents AF input parameters
typedef struct
{
    HAFAlgoSetParam* pParamList;        ///< Pointer to AF input param
                                        ///  See HAFAlgoSetParamType for details
    UINT32           numberOfParams;    ///< Number of input parameters
} HAFAlgoSetParamList;

/// @brief Identifies the type of get parameter
typedef enum
{
    HAFGetParamTypeInvalid                  = -1,           ///< Invalid type
    HAFGetParamTypeSpotlightInfo,                           ///< Spotlight detection information
                                                            ///  Payload: AFSpotlightInfo
    HAFGetParamTypePublishedVendorTagsInfo,                 ///< The vendor tags published by the algorithm
                                                            ///<  Payload in: NULL. Payload out: StatsVendorTagInfoList
    HAFGetParamTypeDependentVendorTags,                     ///< The vendor tags dependent by the algorithm
                                                            ///  Payload in: NULL. Payload out: StatsVendorTagInfoList
    HAFGetParamTypeDebugData,                               ///< The AF debug data
                                                            ///  Payload in: NULL. Payload out: af_get_debug_data
    HAFGetParamTypeMax                       = 0xFFFFFFFF   ///< Max Type
} HAFAlgoGetParamType;

/// @brief Represents the input param given to the AFGetParam, and contains any associated data
///  necessary to support that query
typedef struct
{
    VOID*   pInputData;         ///< Pointer to input data given in the query information to generate the output
    UINT32  sizeOfInputData;    ///< Size of input data pointered to by pInputData
} HAFAlgoGetParamInput;


/// @brief Represents output data returned
typedef struct
{
    VOID*   pOutputData;        ///< Pointer to output data for which queried the information. See
                                ///  HAFAlgoGetParamType for details
    UINT32  sizeOfOutputData;   ///< Size of output data pointed to by pOutputData
} HAFAlgoGetParamOutput;


/// @brief Defines the AF get information data structure
typedef struct
{
    HAFAlgoGetParamType      type;          ///< Type of the get param
    HAFAlgoGetParamOutput    outputInfo;    ///< Placeholder for the queried data
} HAFAlgoGetParam;

/// @brief Multi window regions selection
typedef enum
{
    HAFPDAFMultiWindowTypeInvalid   = -1,               ///< Invalid type
    HAFPDAFMultiWindowTypeCentral,                      ///< Central window type. This type represents the main ROI grid
    HAFPDAFMultiWindowTypePeripheral,                   ///< Peripheral window type
                                                        ///  Peripheral window refers to the top,left,right,bottom around
                                                        /// the main ROI grid
    HAFPDAFMultiWindowTypeMax       =  0x7FFFFFFF       ///< Max Window Moving Window Type
} HAFPDAFMultiWindowType;

/// @brief lens position type
typedef enum
{
    HAFLensPositionInvalid = -1,           ///< Invalid type
    HAFLensposTypeMax      = 0x7FFFFFFF    ///< Max lens position Type
} HAFLensPositionType;

/// @brief Identifies the type of monitor input parameter
typedef enum
{
    HAFMonitorInputParamTypeInvalid         = -1,           ///< Invalid type
    HAFMonitorInputParamTypeCurrentPosition,                ///< Current Lens Position Information
                                                            ///  Payload: AFLensMoveInfo
    HAFMonitorInputParamTypeLastIndex,                      ///< Monitor input last index
    HAFMonitorInputParamTypeMax             = 0x7FFFFFFF    ///< Monitor Max Input Param Type
} HAFAlgoMonitorInputParamType;

/// @brief Represents an AF core input parameter
typedef struct
{
    VOID*                           pData;          ///< Pointer to AF input param data
                                                    ///  See HAFAlgoMonitorInputParamType for details
    UINT32                          sizeOfData;     ///< Size of data pointed to by pData
    HAFAlgoMonitorInputParamType    inputParamType; ///< Type of the input parameter
} HAFAlgoMonitorInputParam;

/// @brief Represents AF input parameters
typedef struct
{
    HAFAlgoMonitorInputParam*   pParamList;         ///< Pointer to HAF monitor input params
                                                    ///  See HAFAlgoMonitorInputParamType for details
    UINT32                      numberOfInputParam; ///< Number of input parameters
} HAFAlgoMonitorInputParamList;

/// @brief HAF Algo Confidence level type
typedef enum
{
    HAFConfidenceLevelNotAvailable =  -1,   ///< Invalid or unavailable confidence level
    HAFConfidenceLevelLow          =   0,   ///< Confidence is low
    HAFConfidenceLevelMedium       =  50,   ///< Confidence is medium
    HAFConfidenceLevelHigh         = 100,   ///< Confidence is high
    HAFConfidenceLevelAdvancedHigh = 150    ///< Confidence is advanced high
} HAFConfidenceLevelType;

/// @brief HAF Algo Focus level type
typedef enum
{
    HAFFocusLevelNotAvailable   = -1,   ///< Invalid or unavailable focus level
    HAFFocusLevelLow            =  0,   ///< Focus level is low - scene is not focused
    HAFFocusLevelHigh          = 100   ///< Focus level is high - scene may be focused
} HAFFocusLevelType;

/// @brief HAF Algo Stability level type
typedef enum
{
    HAFStabilityNotAvailable   = -1,   ///< Invalid or unavailable focus level
    HAFStabilityLow            =  0,   ///< Stability level is low - scene is not stable
    HAFStabilityHigh          = 100    ///< Stability level is high - scene is stable
} HAFStabilityType;

/// @brief Represents an AF monitor output parameter
typedef struct
{
    BOOL    triggerRefocus;                     ///< Need to trigger refocus
    INT32   targetPosition;                     ///< Estimated Logical Target position
    INT32   focusLevel;                         ///< Indication on how focused current scene is
    INT32   stability;                          ///< Indication on how stable current scene is
    INT32   confidence;                         ///< Indication on how confident the algorithm is on the output/// @brief Moving window selected ROI details
    INT32   windowType;                         ///< Window type
    INT32   NumberOfSelectedROI;                ///< Number of ROI indices filled in
    INT32   ROIIndices[PDLibMaxWindowCount];    ///< ROI indices
} HAFAlgoMonitorOutputParam;

/// @brief Identifies the type of search input parameter
typedef enum
{
    HAFSearchInputParamTypeInvalid          = -1,   ///< Invalid type
    HAFSearchInputParamTypeCurrentPosition  = 0,    ///< Current Lens Position
                                                    ///  Payload: AFLensPositionInfo
    HAFSearchInputParamTypeLastIndex,               ///< Search input last index
    HAFSearchInputParamTypeMax                      ///< Max Type
} HAFAlgoSearchInputParamType;

/// @brief Represents an AF search input parameter
typedef struct
{
    VOID*                       pData;          ///< Pointer to HAF input param data
                                                ///  See HAFAlgoSearchInputParamType for details
    UINT32                      sizeOfData;     ///< Size of data pointed to by pHAFInputParam
    HAFAlgoSearchInputParamType inputParamType; ///< Type of the input parameter
} HAFAlgoSearchInputParam;

/// @brief Represents list of HAF search input parameters
typedef struct
{
    HAFAlgoSearchInputParam*        pParamList;    ///< Pointer to HAF input param
                                                   ///  See HAFAlgoSearchInputParamType for details
    UINT32                          paramCount;    ///< Number of input parameters
} HAFAlgoSearchInputParamList;

/// @brief Represents an Search output parameter
typedef struct
{
    INT32                   progress;           ///< Progression indication on HAFProcessSearch API
    INT32                   nextPosition;       ///< Next lens position to be moved to
    INT32                   targetPosition;     ///< Estimated Target Position
    INT32                   rangeNear;          ///< Fine search range near
    INT32                   rangeFar;           ///< Fine search range far
    INT32                   fineStepSize;       ///< Fine scan step size
    INT32                   confidence;         ///< Confidence level on search result
    AFMoveDirectionType     searchDirection;    ///< Search Direction for Optimization
    BOOL                    isFrameSkip;        ///< Specify whether to skip request
} HAFAlgoSearchOutputParam;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief: Interface for HAF utility function
///  This interface is used by algorithm such as PDAF/CAF/TOF
///  For other customized HAF algorithm, this interface provide utility function to calculate some useful data.
///  This inteface is not create for override.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct HAFAlgoUtilInterface
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
     /// DistanceToLensPosition
     ///
     /// @brief  This method estimate target lens position from target distance
     ///
     /// @param  pAlgoUtil       Pointer to HAF Algorithm Util which can be get from HAF create parameter
     /// @param  distmm          Target distance in millimeter
     ///
     /// @return: Estimated target lens position
     ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT     (*DistanceToLensPosition)(const HAFAlgoUtilInterface* pAlgoUtil, INT distmm);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LensPositionToDistance
    ///
    /// @brief  This method estimate target distance from target lens position
    ///
    /// @param  pAlgoUtil       Pointer to HAF Algorithm Util which can be get from HAF create parameter
    /// @param  lensPosition    Target lens position
    ///
    /// @return Estimated target distance in millimeters
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT     (*LensPositionToDistance)(const HAFAlgoUtilInterface* pAlgoUtil, INT lensPosition);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPeerLensPosition
    ///
    /// @brief  This method map the lens position from main camera
    ///
    /// @param  pAlgoUtil       Pointer to HAF Algorithm Util which can be get from HAF create parameter
    /// @param  LensPosition    Lens position from main camera
    ///
    /// @return Mapped lens position for follower camera
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT     (*GetPeerLensPosition)   (const HAFAlgoUtilInterface* pAlgoUtil, INT LensPosition);
} HAFAlgoUtilInterface;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHIHAFAlgorithm
///
/// @brief Interface for CHIHAFAlgorithm.
///  This interface is for sub-alogrithms such as PDAF/CAF/TOF
///  For any customizations the OEM will need to implement this
///  interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct CHIHAFAlgorithm
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CHIHAFAlgorithmSetParam
    ///
    /// @brief  This method sets input parameters to AF HAF Algo.
    ///
    /// @param  pIHAFAlgo       Pointer to HAFAlgo instance
    /// @param  pInputParams    Pointer to input parameter
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult (*HAFAlgorithmSetParam)(
        CHIHAFAlgorithm*        pHAFAlgo,
        HAFAlgoSetParamList*    pSetParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HAFAlgorithmGetParam
    ///
    /// @brief  This method retrieves the most up-to-date AF HAF Algo information.
    ///
    /// @param  pHAFAlgo        Pointer to HAFAlgo instance
    /// @param  pGetParam       Describes the pGetParam for, and contains any associated data necessary to support
    ///                         that get parameter.
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult (*HAFAlgorithmGetParam)(
        CHIHAFAlgorithm*    pHAFAlgo,
        HAFAlgoGetParam*    pGetParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HAFAlgorithmProcessMonitor
    ///
    /// @brief  This method calls the algorithm's scene monitoring functionality.
    ///
    /// @param  pHAFAlgo                Pointer to HAFAlgo instance
    /// @param  pMonitorInputParamList  List of scene monitoring inputs
    /// @param  pMonitorOutputParam     Monitor output Parameters
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult (*HAFAlgorithmProcessMonitor)(
        CHIHAFAlgorithm*                pHAFAlgo,
        HAFAlgoMonitorInputParamList*   pMonitorInputParamList,
        HAFAlgoMonitorOutputParam*      pMonitorOutputParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HAFAlgorithmProcessSearch
    ///
    /// @brief  This method calls the algorithm's focus search functionality.
    ///
    /// @param  pHAFAlgo                Pointer to HAFAlgo instance
    /// @param  pSearchInputParamList   List of Search Input Parameters
    /// @param  pSearchOutputParam      Search Output Parameters
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult (*HAFAlgorithmProcessSearch)(
        CHIHAFAlgorithm*                pHAFAlgo,
        HAFAlgoSearchInputParamList*    pSearchInputParamList,
        HAFAlgoSearchOutputParam*       pSearchOutputParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HAFAlgorithmRebaseReference
    ///
    /// @brief  Notification of focus search end and reset any reference parameters if required.
    ///
    /// @param  pHAFAlgo   Pointer to HAFAlgo instance
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult (*HAFAlgorithmRebaseReference)(
        CHIHAFAlgorithm* pHAFAlgo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HAFAlgorithmClearData
    ///
    /// @brief  Request to clear Algorithm internal data to default state.
    ///
    /// @param  pHAFAlgo   Pointer to HAFAlgo instance
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult(*HAFAlgorithmClearData)(
        CHIHAFAlgorithm* pHAFAlgo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HAFAlgorithmDestroy
    ///
    /// @brief  This method destroys the HAFAlgo instance.
    ///
    /// @param  pHAFAlgo Pointer to HAFAlgo instance
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID (*HAFAlgorithmDestroy)(
        CHIHAFAlgorithm* pHAFAlgo);
} CHIHAFAlgorithm; // CHIHAFAlgorithm interface


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CreateHAFAlgorithm
///
/// @brief  Exposed method to create standalone HAF algorithm.
///
/// @param  pCreateParams     List of parameters passed during creation
/// @param  algoType          Type of HAF object to be created
/// @param  ppAlgoHandle      Pointer to the created CHIHAFAlgorithm instance
///
/// @return CDKResultSuccess upon success.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*CREATEHAFALGORITHM)(
    const AFAlgoCreateParamList*    pCreateParams,
    HAFAlgoType                     algoType,
    CHIHAFAlgorithm**               ppAlgoHandle);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHIHAFINTERFACE_H
