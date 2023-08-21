////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016, 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpropertydefs.h
/// @brief Definitions for Property Pool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXPROPERTYDEFS_H
#define CAMXPROPERTYDEFS_H

#include "camxdefs.h"
#include "camxtypes.h"
#include "camxhal3defs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constant Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-28) - Replace all of the below controls with setting

// Max per-frame property pool window size - RequestQueueDepth for max requests in pipeline
// and another RequestQueueDepth for the max requests that may have queued up
static const UINT32 MaxPerFramePoolWindowSize       = RequestQueueDepth * 2;

// Default per-frame property pool window size - DefaultRequestQueueDepth for default amount of requests in pipeline
// and another DefaultRequestQueueDepth for another set of requests that may have queued up
static const UINT32 DefaultPerFramePoolWindowSize   = DefaultRequestQueueDepth * 2;

// Max number of clients that can subscribe to the pool
static const UINT32 MaxPropertyPoolClients          = 4;

// Max number of properties in camx that can be subscribed to
static const UINT32 MaxProperties                   = 256;

/// Maximum number of dependency lists that a node can report in a ProcessRequest
static const UINT32 MaxDependencies                 = 7;

/// The maximum number of input ports per node and hence the maximum number of input fences that a node is dependent on.
/// There is an assert to catch this and node creation will fail if a node can have more than these many number of fence
/// dependencies.
static const UINT32 MaxDependentFences              = 32;

// Max number of metadata tags in camx that can be subscribed to
static const UINT32 MaxMetadataTags                 = 1000;

// Max number of sensor output stream configurations that are available
static const UINT32 MaxSensorStreamConfigurations   = 5;

// roll of table size for LSC calibration data
static const UINT32 HWRollOffTableSize              = 221;

// Max number of PDAF knot X values
static const UINT16 MaxPDAFKnotX                    = 16;

// Max number of PDAF knot Y values
static const UINT16 MaxPDAFKnotY                    = 12;

// Max PDAF window width
static const UINT16 MaxPDAFWidth                    = 25;

// Max PDAF window height
static const UINT16 MaxPDAFHeight                   = 19;

// Max fixed PDAF window number
static const UINT16 MaxPDAFWindow                   = 200;

// Max custom elements can be configured in the EEPROM custom info
static const UINT16 MaxEEPROMCustomInfoCount        = 50;

// Max AF calibration distance can be configured in EEPROM
static const UINT16 MaxAFCalibrationDistances       = 10;

// Max string length value
static const UINT16 MaximumStringLength             = 256;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Limit constants for variable length request and response metadata tags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Max tone map curve points
static const UINT MaxCurvePoints    = 512;

// Max ROI
static const UINT MaxROI            = 10;

// Max Shading
static const UINT ShadingV          = 17;
static const UINT ShadingH          = 13;

CAMX_NAMESPACE_END

#endif // CAMXPROPERTYDEFS_H
