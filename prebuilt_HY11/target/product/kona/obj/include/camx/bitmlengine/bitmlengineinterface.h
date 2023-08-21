////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bitmlengineinterface.h
/// @brief Interface definition for BITMLEngine
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef BITMLENGINEINTERFACE_H
#define BITMLENGINEINTERFACE_H

#include "camxdefs.h"

CAMX_NAMESPACE_BEGIN
#ifdef __cplusplus
extern "C"
{
#endif // end of #ifdef __cplusplus

typedef UINT32 BitMLEngineResult;
static const BitMLEngineResult BitMLEngineSuccess            = 0;  ///< Success
static const BitMLEngineResult BitMLEngineFailure            = 1;  ///< BITMLEngine general failure
static const BitMLEngineResult BitMLEngineInvalidName        = 2;  ///< Unsupported feature name
static const BitMLEngineResult BitMLEngineInvalidID          = 3;  ///< Invalid BITMLEngine ID
static const BitMLEngineResult BitMLEngineInvalidParams      = 4;  ///< Invalid BITMLEngine ID
static const BitMLEngineResult BitMLEngineNSPFailure         = 5;  ///< Non-recoverable NSP failure
static const BitMLEngineResult BitMLEngineNSPFailedRecovered = 6;  ///< Recoverable NSP failure occured and Client session has been recovered
static const BitMLEngineResult BitMLEngineIncorrectState     = 7;  ///< Request is not supported in current state

typedef VOID* BitMLClientID;

/// @brief Tensor structure definition
struct BitMLTensor
{
    UINT32 size;   ///< Size of the tensor in number of bytes
    UINT32 batch;  ///< Batch of tensor
    UINT32 width;  ///< Width of tensor
    UINT32 height; ///< Height of tensor
    UINT32 depth;  ///< Depth of tensor
    UINT8* buffer; ///< Pointer to tensor buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BitMLEngineQueryVersion
///
/// @brief Get version of BITMLEngine
///
/// @param  major     output, major number of version
/// @param  minor     output, minor number of version
///
/// @return
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BitMLEngineQueryVersion(
    INT32* major,
    INT32* minor);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BitMLRegister
///
/// @brief Registers network in BitMLEngine
///
/// @param  clientName input, name of the network to be registered
/// @param  clientId    output, Will be populated witht a client ID
///
/// @return corresponding BitMLEngineResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BitMLEngineResult BitMLRegister(
    const CHAR* clinetName,
    BitMLClientID* clientId);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BitMLDeregister
///
/// @brief Deregisters network in BitMLEngine
///
/// @param  clientId input, ID of the client to deregister
///
/// @return corresponding BitMLEngineResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BitMLEngineResult BitMLDeregister(
    const BitMLClientID clientId);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BitMLSetupNetwork
///
/// @brief Performs network setup (e.g. appendnode)
///
/// @param  clientId input, ID of the client to setup network for
///
/// @return corresponding BitMLEngineResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BitMLEngineResult BitMLSetupNetwork(
    const BitMLClientID clientId);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BitMLExecute
///
/// @brief Executes network associated with client id
///
/// @param  clientId      input, ID of the client which network should be executed
/// @param  inputTensor  input, Input data
/// @param  outputTensor output, Will be populated with output data form the network's execution
///
/// @return corresponding BitMLEngineResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BitMLEngineResult BitMLExecute(
    const BitMLClientID clientId,
    const BitMLTensor* inputTensor,
    BitMLTensor* outputTensor);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BitMLQueryNetwork
///
/// @brief Checks to see if network associated with client ID is ready for execution
///
/// @param  clientId input, ID of client to query
///
/// @return corresponding BitMLEngineResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BitMLEngineResult BitMLQueryNetwork(
    const BitMLClientID clientId);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BitMLQueryTensorSize
///
/// @brief Returs expected input and output tensor size for a given client
///
/// @param  clientId    input, ID of client to query
/// @param  inBatch     output, input tensor batch
/// @param  inDepth     output, input tensor depth
/// @param  inHeight    output, input tensor height
/// @param  inWidth     output, input tensor width
/// @param  inSize      output, input tensor size in bytes
/// @param  outBatch    output, output tensor batch
/// @param  outDepth    output, output tensor depth
/// @param  outHeight   output, output tensor height
/// @param  outWidth    output, output tensor width
/// @param  outSize     output, output tensor size in bytes
///
/// @return corresponding BitMLEngineResult
///         output will be valid only when return value is BitMLEngineSuccess
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BitMLEngineResult BitMLQueryTensorSize(
    const BitMLClientID clientId,
    UINT32* inBatch,
    UINT32* inDepth,
    UINT32* inHeight,
    UINT32* inWidth,
    UINT32* inSize,
    UINT32* outBatch,
    UINT32* outDepth,
    UINT32* outHeight,
    UINT32* outWidth,
    UINT32* outSize);

#ifdef __cplusplus
}
#endif // __cplusplus
CAMX_NAMESPACE_END

#endif // end of #ifndef BITMLENGINEINTERFACE_H
