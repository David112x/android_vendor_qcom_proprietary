////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxifeproperty.h
/// @brief Define ife properties per usecase and per frame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIFEPROPERTY_H
#define CAMXIFEPROPERTY_H

#include "camxdefs.h"
#include "camxtypes.h"
#include "camxifestripinginterface.h"

CAMX_NAMESPACE_BEGIN

/// @todo (CAMX-1777) Need to update the below structures inline with standard HAL structures

// Maximum number of Gamma16 entries
static const UINT32 MaxGamma16Entries = 65;

// Static const define for Gamma15
static const UINT32 Gamma15TableSize                = 257;   ///< A CDF with first element=0 and last element=1023
static const UINT32 MaxGammaLUTNum                  = 3;     ///< Maximum number of Gamma Channels
static const UINT32 Gamma15LUTNumEntriesPerChannel  = 256;   ///< Maximum number of Gamma15 entries
static const UINT32 MaxGamma15Entries               = Gamma15LUTNumEntriesPerChannel * MaxGammaLUTNum;

/// @brief This structures encapsulate IFE crop info to IPE for Digital Zoom
struct CropWindow
{
    INT32    left;     ///< starting pixel to crop on IFE output
    INT32    top;      ///< starting line o crop on IFE output
    INT32    width;    ///< width of the crop window
    INT32    height;   ///< height of the crop window
};

/// @brief This structures encapsulate IFE /BPS Gamma G Channel Output
struct GammaInfo
{
    BOOL    isGammaValid;              ///< Is Valid Gamm Entries
    UINT32  gammaG[MaxGamma16Entries]; ///< Gamm Green Entries
};

/// @brief This structures encapsulate IFE Input
struct IFEInputDimensions
{
    UINT32    width;    ///< width of the Input IFE window
    UINT32    height;   ///< height of the Input IFE window
};

/// @brief This structures encapsulate IFE Input
struct IFEOutputDimensions
{
    UINT32    width;    ///< width of the Output IFE window
    UINT32    height;   ///< height of the Output IFE window
};

/// @brief This structures encapsulate Intermediate Dimension
struct IntermediateDimensions
{
    UINT32 width;   ///< intermediate width
    UINT32 height;  ///< intermediate height
    FLOAT  ratio;   ///< intermediate ratio
};

/// @brief This structures encapsulate IFE Downscale Factor
struct IFEInputResolution
{
    FLOAT               horizontalFactor;    ///< width of the Input IFE window
    FLOAT               verticalFactor;      ///< height of the Input IFE window
    IFEInputDimensions  resolution;          ///< Information about resolution
    CropWindow          CAMIFWindow;         ///< Information about CAMIF window
};

struct IFEOutputResolution
{
    BOOL                fullPortEnable;       ///< Enable Flag for full port
    BOOL                displayPortEnable;    ///<  Enable Flag for display port
    IFEOutputDimensions fullPortDimension;    ///< Information about full port demension
    IFEOutputDimensions displayPortDimension; ///< Information about Display port demension
};

/// @ brief Stream dimension
struct StreamDimension
{
    UINT32  width;   ///< Stream width in pixels
    UINT32  height;  ///< Stream Height in pixels
    UINT32  offset;  ///< Start offset in the output buffer
    UINT32  offsetX; ///< Start offset in the output buffer
    UINT32  offsetY; ///< Start offset in the output buffer
};

/// @ brief Structure to share MNDS output details with Crop module
struct IFEScalerOutput
{
    StreamDimension             dimension;       ///< MNDS out put width and height
    StreamDimension             input;           ///< MNDS input dimension
    FLOAT                       scalingFactor;   ///< MNDS input to output scaling
    BOOL                        preCropEnable;   ///< MNDS pre crop module Enable/Disable bit
    BOOL                        scalingLimitHit; ///< Scaling restricted to 105;

    union
    {
        struct
        {
            IFEStripeMNScaleDownInputV16  mnds_config_y;  ///< Y config from striping for MNDS version 1.6
            IFEStripeMNScaleDownInputV16  mnds_config_c;  ///< C config from striping for MNDS version 1.6
        } version16;

        // Precrop is a part of MNDS module from version 2.0 onward.
        struct
        {
            IFEStripeMNScaleDownInputV20  mnds_config_y;  ///< Y config from striping for MNDS version 2.0 or 2.1
            IFEStripeMNScaleDownInputV20  mnds_config_c;  ///< C config from striping for MNDS version 2.0 or 2.1
            IFEStripeCrop                 preMndsCrop_y;  ///< Y precrop for MNDS version 2.0 or 2.1
            IFEStripeCrop                 preMndsCrop_c;  ///< C precrop for MNDS version 2.0 or 2.1
        } version20;
    };
};

/// @brief Structure for IPE Gamma15 pre-calculation output
struct IPEGammaPreOutput
{
    BOOL    isPreCalculated;                    ///< Flag to indicate whether gamma15 is pre-calculated or not
    UINT32  packedLUT[MaxGamma15Entries];       ///< gamma LUT for packed data
};

/// @brief Represents if PD HW avilabilty based on target. IFE will publish PDHwAvailablity
struct PDHwAvailablity
{
    BOOL    isDualPDHwAvailable;    ///< Indicates if dual PD HW is available on the current target
    BOOL    isSparsePDHwAvailable;  ///< Indicates if sparse PD HW is available on the current target
    BOOL    isLCRHwAvailable;       ///< Indicates if LCR HW is available on the current target
};

CAMX_NAMESPACE_END

#endif // CAMXIFEPROPERTY_H
