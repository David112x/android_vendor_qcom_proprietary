////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3defs.h
/// @brief Definitions of HAL macros and constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXHAL3DEFS_H
#define CAMXHAL3DEFS_H

#include "camxdefs.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constant Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The maximum number of external component supported by the HAL
static const UINT32 MaxExternalComponents = 40;

/// The maximum number of image sensors simultaneously supported by the HAL
static const UINT32 MaxNumImageSensors = 16;

/// The maximum number of concurrent devices supported by the HAL
static const UINT32 MaxConcurrentDevices = 16;

/// Maximum camera pipeline depth
static const UINT32 RequestQueueDepth = 38;

/// Default camera pipeline depth
static const UINT32 DefaultRequestQueueDepth = 8;

/// This is the requestId that the HAL will associate the very first incoming frame. All proceeding frames will have a
/// sequentially incrementing requestId. This value *MUST* be greater than 0.
static const UINT64 FirstValidRequestId = 1;

/// Maximum partial metadata results that can be sent from topology/HAL to framework (android.request.partialResultCount)
static const UINT32 MaxPartialMetadataHAL = 2;

/// Maximum metadata buffers that can be sent to the CHI
static const UINT32 MaxMetadataCHI = MaxPartialMetadataHAL + 1;

/// Maximum number of input buffers in a request. Currently the HAL API only supports 1
static const UINT32 MaxNumInputBuffers = 1;

/// Maximum number of output buffers in a request/result. This is also the maximum number of streams supported by the HAL
static const UINT32 MaxNumOutputBuffers = 16;

/// Maximum streams
static const UINT32 MaxNumStreams = 16;

/// Maximum buffer composite ID, based on the maximum output ports that a node supports
static const UINT32 MaxBufferComposite = 30;

/// Maximum errors (non-device and non-request type) in a result
static const UINT32 MaxResultErrors = 3;

CAMX_NAMESPACE_END

#endif // CAMXHAL3DEFS_H
