////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chitrackerinterface.h
/// @brief Touch to track algorithm interfaces
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE GR016: Typedefs must not be used for structs, classes, and enumerations

#ifndef CHITRACKERINTERFACE_H
#define CHITRACKERINTERFACE_H

#include "chi.h"
#include "chistatsinterfacedefs.h"
#include "chistatsdebug.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/// Data structures that are exposed to OEM must be packed to have the expected layout.
#pragma pack(push, CDK_PACK_SIZE)

/// @brief Represents Tracker states type
typedef enum
{
    TrackerStateIdle,               ///< Tracker is idle
    TrackerStateRegistering,        ///< Tracker got registration command, registering ROI
    TrackerStateRegistered,         ///< Tracker finished registration, got valid track instance id
    TrackerStateTracking,           ///< Tracker node is tracking on the registered object
    TrackerStateInvalid,            ///< TrackerAlgoState invalid type
} TrackerAlgoState;

/// @brief String representation of enum TrackerAlgoState, must be in same sequence of TrackerAlgoState
static const CHAR* TrackerAlgoStateStrings[] =
{
    "TrackerStateIdle",             ///< Idle
    "TrackerStateRegistering",      ///< Registering
    "TrackerStateRegistered",       ///< Registered
    "TrackerStateTracking",         ///< Tracking
    "TrackerStateInvalid"           ///< Invalid
};

/// @brief TrackerCMDType   Tracker algorithm command trigger type
typedef enum
{
    TrackerCMDTrack,            ///< To trigger the object track operation to get the latest ROI for the recognized object
    TrackerCMDReg,              ///< To trigger the object registration with provided ROI
    TrackerCMDCancel,           ///< To cancel sync and unregister the tracking object
    TrackerCMDInvalid           ///< TrackerCMDType invalid type
} TrackerCMDType;

static const CHAR* TrackerCMDTypeStrings[] =
{
    "TrackerCMDTrack",
    "TrackerCMDReg",
    "TrackerCMDCancel",
    "TrackerCMDInvalid"
};

typedef enum
{
    TrackerModeDefault,         ///< Default mode
    TrackerModePerformance,     ///< Performance mode, where the fastest implementation is selected for all
    TrackerModeCpuOffload,      ///< CPU Offload mode, Preference is given to offload  to any available hardware/dsp.
    TrackerModeLowPower,        ///< Power Save mode, where preference is given to an implementation that draws least power.
    TrackerModeCpuPerformance,  ///< CPU Performance mode.
} TrackerOperationMode;

typedef enum
{
    TrackerPrecisionHigh = 0,   ///< High precision mode - High processing load, but results are more precise
    TrackerPrecisionLow  = 1,   ///< Low precision mode  - Low processing load, but results are less precise
} TrackerPrecisionMode;

/// @brief TrackerBufFormatDimension     Structure to create algorithm instance
typedef struct
{
    ChiBufferFormat         imgBufFmt;          ///< Img Buffer format
    StatsDimension          imgBufDim;          ///< Img buffer dimension
    UINT32                  planeStride;        ///< Img buffer planeStride
    UINT32                  scaleDownRatio;     ///< Img buffer scale down ratio for cv lib
    TrackerOperationMode    operationMode;      ///< CV lib Operation performance mode
    TrackerPrecisionMode    precisionMode;      ///< CV lib Precision mode
} TrackerBufFormatDimension;

/// @brief TrackerResults   Algorithm output structure
typedef struct
{
    UINT32                  confidence;         ///< Confidence output by tracker algo
    StatsRectangle          statsRectROI;       ///< statsRectROI output by tracker algo
    TrackerAlgoState        trackerAlgoState;   ///< Current tracker algorithm state
} TrackerResults;

/// @brief Identifies the type of input to Tracker algorithm
typedef enum
{
    TrackerInputTypeInvalid         = -1,           ///< Invalid type. Payload: NULL & set size to zero.
    TrackerInputTypeCameraID,                       ///< CameraID,                  UINT32
    TrackerInputTypeRequestNumber,                  ///< Request number             UINT64
    TrackerInputTypeCMDType,                        ///< Payload type:              TrackerCMDType
    TrackerInputTypeRegisterROI,                    ///< Input touch ROI,           CHIRectangle
    TrackerInputTypeImgBuf,                         ///< Input Image buffer,        BYTE*
    TrackerInputTypeCount,                          ///< Count of input types
} TrackerAlgoInputType;

/// @brief Represents Tracker inputs base structure to the Tracker algorithm, used as part of TrackerAlgoInputList
typedef struct
{
    VOID*                   pTrackerInput;          ///< Pointer to the payload. See TrackerAlgoInputType for details.
    UINT32                  sizeOfTrackerInput;     ///< Size of input payload
    TrackerAlgoInputType    inputType;              ///< Type of the payload
} TrackerAlgoInput;

/// @brief Represents Tracker module inputs to the Tracker algorithm
typedef struct
{
    TrackerAlgoInput*       pTrackerInputs;             ///< Pointer to TrackerAlgoInput array
    UINT32                  inputCount;                 ///< Number of inputs
} TrackerAlgoInputList;

/// @brief Represents Tracker output type
typedef enum
{
    TrackerOutputTypeInvalid            = -1,           ///< Invalid type
    TrackerOutputTypeTrackROIResult,                    ///< TrackerResults type
    TrackerOutputTypeCount,                             ///< Count of the enum
} TrackerAlgoOutputType;

/// @brief Represents Tracker output from the algorithm
typedef struct
{
    VOID*                   pTrackerOutput;             ///< Pointer to the payload. See TrackerAlgoOutputType for details
                                                        ///  Algorithm implementer needs to do a deep copy to the given memory
    UINT32                  sizeOfTrackerOutput;        ///< Size of input payload pointed to by pTrackerOutput
    UINT32                  sizeOfWrittenTrackerOutput; ///< Size of payload written back. If no output set to zero.
    TrackerAlgoOutputType   outputType;                 ///< Type of the payload
} TrackerAlgoOutput;

/// @brief Represents Tracker output list from the algorithm
typedef struct
{
    TrackerAlgoOutput*      pTrackerOutputs;    ///< Pointer to TrackerAlgoOutput elements array.
    UINT32                  outputCount;        ///< Number of elements in pTrackerOutputs
} TrackerAlgoOutputList;

/// @brief Represents Tracker  type
typedef enum
{
    TrackerGetParamIntputTypeInvalid = -1,              ///< Invalid type. Payload: NULL & set size to zero
    TrackerGetParamInputTypeCount,                      ///< Count of the enum
} TrackerAlgoGetParamInputType;

/// @brief Represents the input data to query which contains any associated data necessary to support that query
typedef struct
{
    VOID*                           pGetParamInput;         ///< Pointer to input data given in the query info to generate
                                                            ///  the output. See TrackerAlgoGetParamInputType for details.
    UINT32                          sizeOfGetParamInput;    ///< Size of input payload
    TrackerAlgoGetParamInputType    getParamInputType;      ///< Type of the payload
} TrackerAlgoGetParamInput;

/// @brief Represents Tracker module inputs to the Tracker algorithm
typedef struct
{
    TrackerAlgoGetParamInput*       pGetParamInputs;    ///< Pointer to TrackerAlgoGetParamInput array
    UINT32                          getParamInputCount; ///< Number of inputs
} TrackerAlgoGetParamInputList;

/// @brief Represents Tracker output type
typedef enum
{
    TrackerGetParamOutputTypeInvalid = -1,              ///< Invalid type. Payload: NULL & set size to zero
    TrackerGetParamOutputTypeCount,                     ///< Count of the enum
} TrackerAlgoGetParamOutputType;

/// @brief Represents Tracker get output information from the algorithm
typedef struct
{
    VOID*                           pGetParamOutput;                ///< Pointer to the payload.
    UINT32                          sizeOfGetParamOutput;           ///< Size of input payload pointed to by pGetParamOutput
    UINT32                          sizeOfWrittenGetParamOutput;    ///< Size of payload written back. If no output set to zero.
    TrackerAlgoGetParamOutputType   getParamOutputType;             ///< Type of the payload
} TrackerAlgoGetParamOutput;

/// @brief Represents Tracker get output list from the algorithm
typedef struct
{
    TrackerAlgoGetParamOutput*      pGetParamOutputs;       ///< Pointer to TrackerAlgorithmOutput array.
    UINT32                          getParamOutputCount;    ///< Number of elements in pTrackerOutputs
} TrackerAlgoGetParamOutputList;

/// @brief Defines the Tracker get parameter information
typedef struct
{
    TrackerAlgoGetParamInputList    inputInfoList;  ///< Data that's needed for processing the output
    TrackerAlgoGetParamOutputList   outputInfoList; ///< Output requested in the query
} TrackerAlgoGetParam;

/// @brief Identifies the type of input parameter to Tracker algorithm
typedef enum
{
    TrackerSetParamTypeInvalid          = -1,           ///< Invalid type
    TrackerSetParamTypeMinConfidence,                   ///< Minimum confidence level to consider object as present. type: UINT
    TrackerSetParamTypeCount,                           ///< Count of the enum
} TrackerAlgoSetParamType;

/// @brief Represents an Tracker input parameter
typedef struct
{
    const VOID*                 pTrackerSetParam;       ///< Pointer to Tracker input param data
                                                        ///  See TrackerAlgoSetParamType for details
    UINT32                      sizeOfInputParam;       ///< Size of data pointed to by pTrackerInputParam
    TrackerAlgoSetParamType     setParamType;           ///< Type of the input parameter
} TrackerAlgoSetParam;

/// @brief Represents an Tracker input parameter passed to CHITrackerAlgorithm::TrackerSetParam()
typedef struct
{
    TrackerAlgoSetParam*        pTrackerSetParams;      ///< Pointer to Tracker input param data
    UINT32                      inputParamsCount;       ///< Number of input param parameters
} TrackerAlgoSetParamList;

/// @brief Identifies the type of create parameter to Tracker algorithm
typedef enum
{
    TrackerCreateParamsTypeInvalid      = -1,       ///< Invalid type
    TrackerCreateParamsTypeBufFmtDim,               ///< Image buf format and dimension, TrackerBufFmtDim type
    TrackerCreateParamsTypeLogger,                  ///< Logger function
    TrackerCreateParamsTypeCameraInfo,              ///< CameraInfo of type StatsCameraInfo
    TrackerCreateParamsTypeCount,                   ///< Count of TrackerAlgoCreateParamType
} TrackerAlgoCreateParamType;

/// @brief Represents Tracker create parameter base structure, used as part of TrackerAlgoCreateParamList
typedef struct
{
    VOID*                       pCreateParam;           ///< Payload: See TrackerAlgoCreateParamType for details
    UINT32                      sizeOfCreateParam;      ///< Size of payload pointed by data.
    TrackerAlgoCreateParamType  createParamType;        ///< Create parameter type.
} TrackerAlgoCreateParam;

/// @brief Represents Tracker create parameters information necessary to create Tracker algorithm. See CreateTracker.
typedef struct
{
    TrackerAlgoCreateParam*     pCreateParams;          ///< A pointer to CreateTracker array
    UINT32                      createParamsCount;      ///< Number of create parameters.
} TrackerAlgoCreateParamList;

/// @brief Identifies the type of destroy parameter
typedef enum
{
    TrackerDestroyParamTypeInvalid               = -1,  ///< Type Invalid
    TrackerDestroyParamTypeCameraID,                    ///< Type camera ID
    TrackerDestroyParamTypeCount,                       ///< Count of destroy params
} TrackerAlgoDestroyParamType;

/// @brief Represents an Tracker destroy parameter
typedef struct
{
    TrackerAlgoDestroyParamType destroyParamType;       ///< Type of parameter passed
    VOID*                       pParam;                 ///< Payload of the particular parameter type
    UINT32                      sizeOfParam;            ///< Size of the payload.
} TrackerAlgoDestroyParam;

/// @brief Represents Tracker algorithm destroy parameters
typedef struct
{
    TrackerAlgoDestroyParam*    pParamList;         ///< Pointer to Tracker destroy-parameter
    UINT32                      paramCount;         ///< Number of input destroy-parameters
} TrackerAlgoDestroyParamList;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Interface for Tracker algorithm.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct CHITrackerAlgorithm
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TrackerProcess
    ///
    /// @brief  Runs the Tracker algorithm on the given inputs.
    ///
    /// @param  pCHITrackerAlgorithm   Pointer to CHITrackerAlgorithm instance
    /// @param  pInputs                Pointer to stats inputs interface
    /// @param  pOutputs               Pointer to stats data output by the algorithm
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult (*TrackerProcess)(
        CHITrackerAlgorithm*           pCHITrackerAlgorithm,
        const TrackerAlgoInputList*    pInputs,
        TrackerAlgoOutputList*         pOutputs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TrackerGetParam
    ///
    /// @brief  This method retrieves the most up-to-date Tracker algorithm information. The output
    ///         is generally updated after calling CHITrackerAlgorithm::Process()
    ///
    /// @param  pCHITrackerAlgorithm   Pointer to CHITrackerAlgorithm instance
    /// @param  pGetParam              Describes the metadata to query for, and contains any associated data necessary to
    ///                                support that query
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult(*TrackerGetParam)(
        CHITrackerAlgorithm*    pCHITrackerAlgorithm,
        TrackerAlgoGetParam*    pGetParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TrackerSetParam
    ///
    /// @brief  Sets parameters to Tracker.
    ///
    /// @param  pCHITrackerAlgorithm   Pointer to CHITrackerAlgorithm instance
    /// @param  pSetParams             Tracker input parameters to set.
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult(*TrackerSetParam)(
        CHITrackerAlgorithm*             pCHITrackerAlgorithm,
        const TrackerAlgoSetParamList*   pSetParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TrackerGetFeatureCapability
    ///
    /// @brief  Sets parameters to Tracker.
    ///
    /// @param  pCHITrackerAlgorithm    Pointer to CHITrackerAlgorithm instance
    /// @param  pFeatures               Features supported by the algorithm
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult(*TrackerGetFeatureCapability)(
        CHITrackerAlgorithm*    pCHITrackerAlgorithm,
        UINT64*                 pFeatures);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TrackerDestroy
    ///
    /// @brief  This method destroys the derived instance of the interface
    ///
    /// @param  pCHITrackerAlgorithm    Pointer to CHITrackerAlgorithm instance
    /// @param  pDestroyParams          Pointer to destroy Param List
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID (*TrackerDestroy)(
        CHITrackerAlgorithm*                 pCHITrackerAlgorithm,
        const TrackerAlgoDestroyParamList*   pDestroyParams);
} CHITrackerAlgorithm;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CreateTracker
///
/// @brief  This method creates an instance to the CHITrackerAlgorithm.
///
/// @param  pCreateParams           Pointer to create params
/// @param  ppCHITrackerAlgorithm   Pointer to the created CHITrackerAlgorithm instance
///
/// @return CDKResultSuccess upon success.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*CREATETRACKER)(
    const TrackerAlgoCreateParamList*   pCreateParams,
    CHITrackerAlgorithm**               ppCHITrackerAlgorithm);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHITRACKERINTERFACE_H
