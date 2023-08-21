////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslvfedefs.h
/// @brief VFE Hardware Interface Definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLVFEDEFS_H
#define CAMXCSLVFEDEFS_H
CAMX_NAMESPACE_BEGIN

// VFE output resource type
static const UINT32 VFEOutputEncode        = 0x1000;
static const UINT32 VFEOutputView          = 0x1001;
static const UINT32 VFEOutputVideo         = 0x1002;
static const UINT32 VFEOutputRDI0          = 0x1003;
static const UINT32 VFEOutputRDI1          = 0x1004;
static const UINT32 VFEOutputRDI2          = 0x1005;
static const UINT32 VFEOutputRDI3          = 0x1006;
static const UINT32 VFEOutputStatsAEC      = 0x1007;
static const UINT32 VFEOutputStatsAF       = 0x1008;
static const UINT32 VFEOutputStatsAWB      = 0x1009;
static const UINT32 VFEOutputStatsRS       = 0x1010;
static const UINT32 VFEOutputStatsCS       = 0x1011;
static const UINT32 VFEOutputStatsIHIST    = 0x1012;
static const UINT32 VFEOutputStatsSkin     = 0x1013;
static const UINT32 VFEOutputStatsBG       = 0x1014;
static const UINT32 VFEOutputStatsBF       = 0x1015;
static const UINT32 VFEOutputStatsBE       = 0x1016;
static const UINT32 VFEOutputStatsBHIST    = 0x1017;
static const UINT32 VFEOutputStatsBFScale  = 0x1018;
static const UINT32 VFEOutputStatsHDRBE    = 0x1019;
static const UINT32 VFEOutputStatsHDRBHIST = 0x1020;
static const UINT32 VFEOutputStatsAECBG    = 0x1021;
static const UINT32 VFEOutputCAMIFRaw      = 0x1022;
static const UINT32 VFEOutputIdealRaw      = 0x1023;

// VFE input resource type
static const UINT32 VFEInputTestGen        = 0x2000;
static const UINT32 VFEInputPHY0           = 0x2001;
static const UINT32 VFEInputPHY1           = 0x2002;
static const UINT32 VFEInputPHY2           = 0x2003;
static const UINT32 VFEInputPHY3           = 0x2004;
static const UINT32 VFEInputFE             = 0x2005;

CAMX_NAMESPACE_END
#endif // CAMXCSLVFEDEFS_H
