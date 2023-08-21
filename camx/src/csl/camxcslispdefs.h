////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslispdefs.h
/// @brief ISP Hardware Interface Definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLISPDEFS_H
#define CAMXCSLISPDEFS_H

#include "camxdefs.h"

CAMX_NAMESPACE_BEGIN

// ISP Resource Type
static const UINT32 ISPResourceIdPort      = 0;
static const UINT32 ISPResourceIdClk       = 1;

// ISP Usage Type
static const UINT32 ISPResourceUsageSingle = 0;
static const UINT32 ISPResourceUsageDual   = 1;

// ISP Color Pattern
static const UINT32 ISPPatternBayerRGRGRG  = 0;
static const UINT32 ISPPatternBayerGRGRGR  = 1;
static const UINT32 ISPPatternBayerBGBGBG  = 2;
static const UINT32 ISPPatternBayerGBGBGB  = 3;
static const UINT32 ISPPatternYUVYCBYCR    = 4;
static const UINT32 ISPPatternYUVYCRYCB    = 5;
static const UINT32 ISPPatternYUVCBYCRY    = 6;
static const UINT32 ISPPatternYUVCRYCBY    = 7;

// ISP Input Format
static const UINT32 ISPFormatMIPIRaw6      = 1;
static const UINT32 ISPFormatMIPIRaw8      = 2;
static const UINT32 ISPFormatMIPIRaw10     = 3;
static const UINT32 ISPFormatMIPIRaw12     = 4;
static const UINT32 ISPFormatMIPIRaw14     = 5;
static const UINT32 ISPFormatMIPIRaw16     = 6;
static const UINT32 ISPFormatMIPIRaw20     = 7;
static const UINT32 ISPFormatRaw8Private   = 8;
static const UINT32 ISPFormatRaw10Private  = 9;
static const UINT32 ISPFormatRaw12Private  = 10;
static const UINT32 ISPFormatRaw14Private  = 11;
static const UINT32 ISPFormatPlain8        = 12;
static const UINT32 ISPFormatPlain168      = 13;
static const UINT32 ISPFormatPlain1610     = 14;
static const UINT32 ISPFormatPlain1612     = 15;
static const UINT32 ISPFormatPlain1614     = 16;
static const UINT32 ISPFormatPlain1616     = 17;
static const UINT32 ISPFormatPlain3220     = 18;
static const UINT32 ISPFormatPlain64       = 19;
static const UINT32 ISPFormatPlain128      = 20;
static const UINT32 ISPFormatARGB          = 21;
static const UINT32 ISPFormatARGB10        = 22;
static const UINT32 ISPFormatARGB12        = 23;
static const UINT32 ISPormatARGB14         = 24;
static const UINT32 ISPFormatDPCM10610     = 25;
static const UINT32 ISPFormatDPCM10810     = 26;
static const UINT32 ISPFormatDPCM12612     = 27;
static const UINT32 ISPFormatDPCM12812     = 28;
static const UINT32 ISPFormatDPCM14814     = 29;
static const UINT32 ISPFormatDPCM141014    = 30;
static const UINT32 ISPFormatNV21          = 31;
static const UINT32 ISPFormatNV12          = 32;
static const UINT32 ISPFormatTP10          = 33;
static const UINT32 ISPFormatYUV422        = 34;
static const UINT32 ISPFormatPD8           = 35;
static const UINT32 ISPFormatPD10          = 36;
static const UINT32 ISPFormatUBWCNV12      = 37;
static const UINT32 ISPFormatUBWCNV124R    = 38;
static const UINT32 ISPFormatUBWCTP10      = 39;
static const UINT32 ISPFormatUBWCP010      = 40;
static const UINT32 ISPFormatPlain8Swap    = 41;
static const UINT32 ISPFormatPlain810      = 42;
static const UINT32 ISPFormatPlain810Swap  = 43;
static const UINT32 ISPFormatYV12          = 44;
static const UINT32 ISPFormatY             = 45;
static const UINT32 ISPFormatUndefined     = 0xFFFFFFFF;

// ISP output resource type
static const UINT32 ISPOutputEncode        = 1000;
static const UINT32 ISPOutputView          = 1001;
static const UINT32 ISPOutputVideo         = 1002;
static const UINT32 ISPOutputRDI0          = 1003;
static const UINT32 ISPOutputRDI1          = 1004;
static const UINT32 ISPOutputRDI2          = 1005;
static const UINT32 ISPOutputRDI3          = 1006;
static const UINT32 ISPOutputStatsAEC      = 1007;
static const UINT32 ISPOutputStatsAF       = 1008;
static const UINT32 ISPOutputStatsAWB      = 1009;
static const UINT32 ISPOutputStatsRS       = 1010;
static const UINT32 ISPOutputStatsCS       = 1011;
static const UINT32 ISPOutputStatsIHIST    = 1012;
static const UINT32 ISPOutputStatsSkin     = 1013;
static const UINT32 ISPOutputStatsBG       = 1014;
static const UINT32 ISPOutputStatsBF       = 1015;
static const UINT32 ISPOutputStatsBE       = 1016;
static const UINT32 ISPOutputStatsBHIST    = 1017;
static const UINT32 ISPOutputStatsBFScale  = 1018;
static const UINT32 ISPOutputStatsHDRBE    = 1019;
static const UINT32 ISPOutputStatsHDRBHIST = 1020;
static const UINT32 ISPOutputStatsAECBG    = 1021;
static const UINT32 ISPOutputCAMIFRaw      = 1022;
static const UINT32 ISPOutputIdealRaw      = 1023;

// ISP input resource type
static const UINT32 ISPInputTestGen        = 1500;
static const UINT32 ISPInputPHY0           = 1501;
static const UINT32 ISPInputPHY1           = 1502;
static const UINT32 ISPInputPHY2           = 1503;
static const UINT32 ISPInputPHY3           = 1504;
static const UINT32 ISPInputFE             = 1505;

// ISP input resource Lane Type
static const UINT32 ISPLaneTypeDPHY        = 0;
static const UINT32 ISPLaneTypeCPHY        = 1;

/* ISP Resurce Composite Group ID */
static const UINT32  ISPOutputGroupIdNONE   = 0;
static const UINT32  ISPOutputGroupId0      = 1;
static const UINT32  ISPOutputGroupId1      = 2;
static const UINT32  ISPOutputGroupId2      = 3;
static const UINT32  ISPOutputGroupId3      = 4;
static const UINT32  ISPOutputGroupId4      = 5;
static const UINT32  ISPOutputGroupId5      = 6;
static const UINT32  ISPOutputGroupId6      = 7;
static const UINT32  ISPOutputGroupIdMAX    = 8;

static const UINT16  ISPAcquireCommonVersion1 = 0x1000;     ///< IFE Common Resource structure Version
static const UINT16  ISPAcquireInputVersion1  = 0x0100;     ///< IFE Input Resource structure Version
static const UINT16  ISPAcquireOutputVersion1 = 0x0100;     ///< IFE Output Resource structure Version

static const UINT16  ISPAcquireCommonVersion2 = 0x2000;     ///< IFE Common Resource structure Version
static const UINT16  ISPAcquireInputVersion2  = 0x0200;     ///< IFE Input Resource structure Version
static const UINT16  ISPAcquireOutputVersion2 = 0x0200;     ///< IFE Output Resource structure Version

static const UINT32 ISPMaxVCConfig = 4;
static const UINT32 ISPMaxDTConfig = 4;

static const UINT32 ISPMaxOutputPorts = 30; ///< Max IFE Output Ports

// IFE HW Mask same as KMD define
static const UINT32 IFE0HWMask     = 0x1;
static const UINT32 IFE1HWMask     = 0x2;
static const UINT32 IFE0LiteHWMask = 0x4;
static const UINT32 IFE1LiteHWMask = 0x8;
static const UINT32 IFE2LiteHWMask = 0x10;
static const UINT32 IFE3LiteHWMask = 0x20;
static const UINT32 IFE4LiteHWMask = 0x40;
static const UINT32 IFE2HWMask     = 0x100;

static const UINT32 IFEHWMasks = (IFE0HWMask |IFE1HWMask | IFE2HWMask) |
                                 (IFE0LiteHWMask |IFE1LiteHWMask |IFE2LiteHWMask | IFE3LiteHWMask | IFE4LiteHWMask);

// Max IFE HW numbers and Max split HW numbers, same as KMD define
static const UINT32 IFEHWMaxCounts =   5;
static const UINT32 IFEHWMaxSplits =   3;

static const UINT32 IFEWMModeLineBased  = 0x0;  ///< WM Line based Mode
static const UINT32 IFEWMModeFrameBased = 0x1;  ///< WM Frame based Mode
static const UINT32 IFEWMModeIndexBased = 0x2;  ///< WM Index based Mode

// IFE Path Mask same as KMD define
static const UINT32  IFEPXLPathMask  = 0x1;
static const UINT32  IFEPPDPathMask  = 0x2;
static const UINT32  IFELCRPathMask  = 0x4;
static const UINT32  IFERDI0PathMask = 0x8;
static const UINT32  IFERDI1PathMask = 0x10;
static const UINT32  IFERDI2PathMask = 0x20;
static const UINT32  IFERDI3PathMask = 0x40;

static const UINT32  IFEPathMasks = IFEPXLPathMask |IFEPPDPathMask |IFELCRPathMask| IFERDI0PathMask|
                                        IFERDI1PathMask |IFERDI2PathMask |IFERDI3PathMask;

/// @brief IFE output Resource Information
struct ISPOutResourceInfo
{
    UINT32 resourceType;       ///< Resource type
    UINT32 format;             ///< Format of the output
    UINT32 width;              ///< Width of the output image
    UINT32 height;             ///< Height of the output image
    UINT32 compositeGroupId;   ///< Composite Group id of the group
    UINT32 splitPoint;         ///< Split point in pixels for Dual ISP case
    UINT32 secureMode;         ///< Output port to be secure or non-secure
    UINT32 reserved;           ///< Reserved field
};

/// @brief ISP output Resource Information
struct ISPOutResourceInfoVer2
{
    UINT32 resourceType;       ///< Resource type
    UINT32 format;             ///< Format of the output
    UINT32 width;              ///< Width of the output image
    UINT32 height;             ///< Height of the output image
    UINT32 compositeGroupId;   ///< Composite Group id of the group
    UINT32 splitPoint;         ///< Split point in pixels for Dual ISP case
    UINT32 secureMode;         ///< Output port to be secure or non-secure
    UINT32 wmMode;             ///< WM mode
    UINT32 outPortreserved1;   ///< Reserved field
    UINT32 outPortreserved2;   ///< Reserved field
};

/// @brief ISP input resource information
struct ISPInResourceInfo
{
    UINT32             resourceType;      ///< Resource type
    UINT32             laneType;          ///< Lane type (dphy/cphy)
    UINT32             laneNum;           ///< Active lane number
    UINT32             laneConfig;        ///< Lane configurations: 4 bits per lane
    UINT32             VC;                ///< Virtual Channel
    UINT32             DT;                ///< Data Type
    UINT32             format;            ///< Input Image Format
    UINT32             testPattern;       ///< Test Pattern for the TPG
    UINT32             usageType;         ///< Single ISP or Dual ISP
    UINT32             leftStart;         ///< Left input start offset in pixels
    UINT32             leftStop;          ///< Left input stop offset in pixels
    UINT32             leftWidth;         ///< Left input Width in pixels
    UINT32             rightStart;        ///< Right input start offset in pixels
    UINT32             rightStop;         ///< Right input stop offset in pixels
    UINT32             rightWidth;        ///< Right input Width in pixels
    UINT32             lineStart;         ///< Start offset of the pixel line
    UINT32             lineStop;          ///< Stop offset of the pixel line
    UINT32             height;            ///< Input height in lines
    UINT32             pixleClock;        ///< Sensor output clock
    UINT32             batchSize;         ///< Batch size for HFR mode
    UINT32             DSPMode;           ///< DSP Mode
    UINT32             HBICount;          ///< HBI Count
    UINT32             reserved;          ///< Reserved field
    UINT32             numberOutResource; ///< Number of the output resource associated
    ISPOutResourceInfo pDataField[1];     ///< Output Resource starting point
};


/// @brief ISP input resource information
struct ISPInResourceInfoVer2
{
    UINT32                 resourceType;       ///< Resource type
    UINT32                 laneType;           ///< Lane type (dphy/cphy)
    UINT32                 laneNum;            ///< Active lane number
    UINT32                 laneConfig;         ///< Lane configurations: 4 bits per lane
    UINT32                 VC[ISPMaxVCConfig]; ///< Virtual Channel
    UINT32                 DT[ISPMaxDTConfig]; ///< Data Type
    UINT32                 numValidVCDT;       ///< Number of valid VC and DT
    UINT32                 format;             ///< Input Image Format
    UINT32                 testPattern;        ///< Test Pattern for the TPG
    UINT32                 usageType;          ///< Single ISP or Dual ISP
    UINT32                 leftStart;          ///< Left input start offset in pixels
    UINT32                 leftStop;           ///< Left input stop offset in pixels
    UINT32                 leftWidth;          ///< Left input Width in pixels
    UINT32                 rightStart;         ///< Right input start offset in pixels
    UINT32                 rightStop;          ///< Right input stop offset in pixels
    UINT32                 rightWidth;         ///< Right input Width in pixels
    UINT32                 lineStart;          ///< Start offset of the pixel line
    UINT32                 lineStop;           ///< Stop offset of the pixel line
    UINT32                 height;             ///< Input height in lines
    UINT32                 pixleClock;         ///< Sensor output clock
    UINT32                 batchSize;          ///< Batch size for HFR mode
    UINT32                 DSPMode;            ///< DSP Mode
    UINT32                 HBICount;           ///< HBI Count
    UINT32                 customNode;         ///< Custom Node (SBI)
    UINT32                 numberOutResource;  ///< Number of the output resource associated
    UINT32                 offlineMode;        ///< Offline mode
    UINT32                 HorizontalBinning;  ///< Horiz binning
    UINT32                 QCFABinning;        ///< Quad CFA binning
    UINT32                 CSIDreserved1;      ///< Reserved field
    UINT32                 CSIDreserved2;      ///< Reserved field
    UINT32                 IFEReserved1;       ///< Reserved field
    UINT32                 IFEreserved2;       ///< Reserved field
    ISPOutResourceInfoVer2 pDataField[1];      ///< Output Resource starting point
};

/// @brief ISP resource acquire Information
struct ISPAcquireHWInfo
{
    UINT16 commonInfoVersion;             ///< Common Info Version represents the IFE common resource structure version
                                          ///< currently we dont have any common reosurce strcuture, It is reserved for future
                                          ///< HW changes in KMD
    UINT16 commonInfoSize;                ///< Common Info Size represents IFE common resource strcuture Size
    UINT16 commonInfoOffset;              ///< Common Info Offset represnts the Offset from where the IFE common resource
                                          ///< structure is stored in data
    UINT32 numInputs;                     ///< Number of IFE inpiut resources
    UINT32 inputInfoVersion;              ///< Input Info Strcure Version
    UINT32 inputInfoSize;                 ///< Size of the Input resource
    UINT32 inputInfoOffset;               ///< Offset where the Input resource structure starts in data
    UINT64 data;                          ///< IFE resource data
};

/// @brief IFE acquried Information, it same as KMD return structure
struct IFEAcquiredHWInfo
{
    UINT32    acquired_hw_id[IFEHWMaxCounts];                   ///< IFE HW mask list to show vaild HW used
    UINT32    acquired_hw_path[IFEHWMaxCounts][IFEHWMaxSplits]; ///< Path mask list per every IFEs
                                                                ///  Dual IFE using [0] and [1], others using [0]
    UINT32    valid_acquired_hw;                                ///< it shows how many hw be required
};

/// @brief IFE Core Config
struct IFECoreConfig
{
    UINT32                 version;               ///< Version
    UINT32                 videoDS16R2PDEnable;   ///< Video DS16 R2PD
    UINT32                 videoDS4R2PDEnable;    ///< Video DS4 R2PD
    UINT32                 displayDS16R2PDEnable; ///< Display DS16 R2PD
    UINT32                 displayDS4R2PDEnable;  ///< Display DS4 R2PD
    UINT32                 DSPStreamingTapPoint;  ///< DSP Tap Point
    UINT32                 IHistSrcSel;           ///< IHist Src/tap-out Selection
    UINT32                 HDRBESrcSel;           ///< HDR BE Src/tap-out Selection
    UINT32                 HDRBHistSrcSel;        ///< HDR Bhist Src/tap-out Selection
    UINT32                 inputMuxSelPDAF;       ///< Input Mux selector PDAF
    UINT32                 inputMuxSelPP;         ///< Input Mux selector PP
    UINT32                 reserved;              ///< Reserved
};

/// @brief IFE WM Config
struct IFEWMConfig
{
    UINT32 portID;         ///< unique ID of output port
    UINT32 mode;           ///< Wm Mode
    UINT32 hInit;          ///< Starting Pixel offset of the current WM stripe. Its in pixels
    UINT32 height;         ///< Height
    UINT32 width;          ///< Image width of the WM port
    UINT32 virtualFrameEn; ///< Virtual Frame enbale bit for WM port
    UINT32 stride;         ///< Stride
    UINT32 offset;         ///< Offset
    UINT32 reserved1;      ///< Reserved field for future use
    UINT32 reserved2;      ///< Reserved field for future use
    UINT32 reserved3;      ///< Reserved filed for future use
    UINT32 reserved4;      ///< Reserved field for future use
};

/// @brief IFE Out Config
struct IFEOutConfig
{
    UINT32 numOutputPorts;                                  ///< Number Of Output Ports
    UINT32 reserved;                                        ///< Reserved Field for future use
    struct IFEWMConfig outputPortConfig[ISPMaxOutputPorts]; ///< OutputPortConfig
};

CAMX_NAMESPACE_END
#endif // CAMXCSLISPDEFS_H
