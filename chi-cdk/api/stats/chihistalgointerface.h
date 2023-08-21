////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chihistalgointerface.h
/// @brief Local Hist Algorithm interfaces
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIHISTALGOINTERFACE_H
#define CHIHISTALGOINTERFACE_H

#include "chistatsinterfacedefs.h"
#include "chistatsdebug.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/// Data structures that are exposed to OEM must be packed to have the expected layout.
#pragma pack(push, CDK_PACK_SIZE)


typedef enum
{
    HistAlgoProcessOutputTypeInvalid = -1,           ///< Invalid type
                                                     ///  Payload type: NULL & set size to zero
    HistAlgoProcessOutputTypeLTCRatio,               ///< LTC Ratio for Still. Converted Unit of
                                                     ///  LTCRatio (Percentage to Register Unit)
                                                     ///  Payload type: UINT8
    HistAlgoProcessOutputTypeLTCRatioPercentage,     ///< LTC Ratio Percentage for Still
                                                     ///  Payload type: INT32
    HistAlgoProcessOutputTypeLastIndex,              ///< Last valid index of the enum
                                                     ///  Payload type: None
    HistAlgoProcessOutputTypeMax = 0x7FFFFFFF,       ///< Max type

} HistAlgoProcessOutputType;

/// @brief Represents HistAlgoProcess output from the algorithm
typedef struct
{
    VOID*                           pHistAlgoProcessOutput;  ///< Pointer to the payload. See above type enum.
    UINT32                          sizeOfOutput;            ///< Size of input payload pointed to
                                                             ///  by pHistAlgoProcessOutput
    UINT32                          sizeOfWrittenOutput;     ///< Size of payload written back
    HistAlgoProcessOutputType       outputType;              ///< Type of the payload
} HistAlgoProcessOutput;

/// @brief Represents the array of HistAlgoProcess output list from the algorithm
typedef struct
{
    HistAlgoProcessOutput*  pHistAlgoProcessOutputList;   ///< Pointer to HistAlgoProcessOutput array.
    UINT32                  outputCount;                  ///< Number of elements in pHistAlgoProcessOutputList
} HistAlgoProcessOutputList;


/// @brief Identifies the type of input to HistAlgoProcess algorithm
typedef enum
{
    HistAlgoProcessInputTypeInvalid         = -1,           ///< Invalid type
                                                            ///  Payload: NULL & set size to zero.
    HistAlgoProcessInputTypeYPlaneImageAddress,             ///< Y plane image address
                                                            ///  Payload type: BYTE*
    HistAlgoProcessInputTypeDRCGainLTM,                     ///< DRC gain ^ LTM percentage
                                                            ///  Payload type: FLOAT
    HistAlgoProcessInputTypeDRCDarkGain,                    ///< DRC Dark Gain
                                                            ///  Payload type: FLOAT
    HistAlgoProcessInputTypeSensorGain,                     ///< Sensor real gain programed for this frame
                                                            ///  Payload type: FLOAT
    HistAlgoProcessInputTypeLastIndex,                      ///< Last valid index of the enum
                                                            ///  Payload type: None
    HistAlgoProcessInputTypeMax             = 0x7FFFFFFF,   ///< Max type
} HistAlgoProcessInputType;

/// @brief Represents HistAlgoProcess inputs base structure to the algo
typedef struct
{
    VOID*                           pHistAlgoProcessInput;  ///< Pointer to the payload.
    UINT32                          sizeOfInputType;        ///< Size of input payload
    HistAlgoProcessInputType        inputType;              ///< Type of the payload
} HistAlgoProcessInput;

/// @brief Represents HistAlgoProcess module inputs to the HistAlgoProcess algorithm
typedef struct
{
    HistAlgoProcessInput*   pHistAlgoProcessInputs;    ///< Pointer to HistAlgoProcessAlgoInput array
    UINT32                  inputCount;                ///  Number of inputs
} HistAlgoProcessInputList;

/// @brief Identifies the type of destroy parameter type
typedef enum
{
    HistAlgoProcessDestroyParamTypeInvalid               = -1,           ///< Type Invalid
    HistAlgoProcessDestroyParamTypeCameraCloseIndicator,                 ///< Camera Close Indicator
                                                                         ///< UINT 0 - Camera Close 1 Camera Open
    HistAlgoProcessDestroyParamTypeCameraInfo,                           ///< Camera Info
                                                                         ///  Payload: StatsCameraInfo
    HistAlgoProcessDestroyParamTypeCount,                                ///< Destroy Param Type Count
    HistAlgoProcessDestroyParamTypeMax                   = 0x7FFFFFFF    ///< Max Destroy Param Type
} HistAlgoProcessDestroyParamType;

/// @brief Represents an HistAlgoProcess destroy parameter
typedef struct
{
    HistAlgoProcessDestroyParamType     destroyParamType;       ///< Type of parameter passed
    VOID*                               pParam;                 ///< Payload of the particular parameter type
    UINT32                              sizeOfParam;            ///< Size of the payload.
} HistAlgoProcessDestroyParam;


/// @brief HistAlgoProcess Algo destroy parameters list
typedef struct
{
    HistAlgoProcessDestroyParam*   pParamList;    ///< Pointer to HistAlgoProcess create-parameter
    UINT32                         paramCount;    ///< Number of input destroy-parameters
} HistAlgoProcessDestroyParamList;

/// @brief Identifies the type of create parameter
typedef enum
{
    HistAlgoProcessCreateParamTypeInvalid     = -1,         ///< Type Invalid
    HistAlgoProcessCreateParamTypeCameraInfo,               ///< Set camera ID during algo creation
                                                            ///  Payload type: StatsCameraInfo
    HistAlgoProcessCreateParamTypeCount,                    ///< Create Param Type Count
    HistAlgoProcessCreateParamTypeMax = 0x7FFFFFFF          ///< Max Create Param Type
} HistAlgoProcessCreateParamType;

/// @brief Represents an HistAlgoProcess input parameter
typedef struct
{
    HistAlgoProcessCreateParamType  createParamType; ///< Type of parameter passed
    VOID*                           pParam;          ///< Payload of the particular parameter type
    UINT32                          sizeOfParam;     ///< Size of the payload.
} HistAlgoProcessCreateParam;

/// @brief Represents  algorithm creation parameters
typedef struct
{
    HistAlgoProcessCreateParam*    pParamList;    ///< Pointer to HistAlgoProcess create-parameter
    UINT32                         paramCount;    ///< Number of input create-parameters
} HistAlgoProcessCreateParamList;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Interface for HistAlgoProcess algorithm.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ChiHistAlgoProcess
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HistAlgoProcess
    ///
    /// @brief  Runs the HistAlgoProcess algorithm on the given inputs.
    ///
    /// @param  pCHIHistAlgoProcess    Pointer to CHIHistAlgoProcessAlgorithm instance
    /// @param  pInputs                Pointer to stats inputs interface
    /// @param  pOutputs               Pointer to stats data output by the algorithm
    ///
    /// @return CDKResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult(*HistAlgoProcess)(
        ChiHistAlgoProcess*                pCHIHistAlgoProcess,
        const HistAlgoProcessInputList*    pInputs,
        HistAlgoProcessOutputList*         pOutputs);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HistAlgoDestroy
    ///
    /// @brief  This method destroys the derived instance of the interface
    ///
    /// @param  pCHIHistAlgoProcess    Pointer to HistAlgoProcess instance
    /// @param  pDestroyParams         Pointer to destroy parameter list
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID(*HistAlgoDestroy)(
        ChiHistAlgoProcess*                      pCHIHistAlgoProcess,
        const HistAlgoProcessDestroyParamList*   pDestroyParams);

} ChiHistAlgoProcess;  // ChiHistAlgoProcess interface

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CREATEHISTOGRAMALGOPROCESS
///
/// @brief  This method creates an instance to the ChiHistAlgoProcess.
///
/// @param  pCreateParams        Pointer to create parameter list
/// @param  ppHistAlgoProcess    Pointer to the created HistAlgoProcess instance
///
/// @return CDKResultSuccess upon success.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*CREATEHISTOGRAMALGOPROCESS)(
    const HistAlgoProcessCreateParamList*  pCreateParams,
    ChiHistAlgoProcess**                   ppHistAlgoProcess);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHIHISTALGOINTERFACE_H
