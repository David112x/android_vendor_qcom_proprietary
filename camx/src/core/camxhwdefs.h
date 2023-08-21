////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhwdefs.h
/// @brief Hardware definitions for all platforms
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXHWDEFS_H
#define CAMXHWDEFS_H

#include "camxdefs.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

/// @brief List of software Nodes
const UINT Sensor                       = 0;
const UINT StatsProcessing              = 1;
const UINT SinkBuffer                   = 2;
const UINT SinkNoBuffer                 = 3;
const UINT SourceBuffer                 = 4;
const UINT AutoFocus                    = 5;
const UINT JPEGAggregator               = 6;
const UINT FDSw                         = 7;
const UINT FDManager                    = 8;
const UINT StatsParse                   = 9;
const UINT OfflineStats                 = 10;
const UINT Torch                        = 11;
const UINT AutoWhiteBalance             = 12;
const UINT HistogramProcess             = 13;
const UINT Tracker                      = 14;

/// @brief List of Hw Nodes
const UINT HwNodeIDStart                = 1 << 16;

const UINT IFE                          = HwNodeIDStart + 0;
const UINT JPEG                         = HwNodeIDStart + 1;
const UINT IPE                          = HwNodeIDStart + 2;
const UINT BPS                          = HwNodeIDStart + 3;
const UINT FDHw                         = HwNodeIDStart + 4;
const UINT LRME                         = HwNodeIDStart + 5;
const UINT RANSAC                       = HwNodeIDStart + 6;
const UINT CVP                          = HwNodeIDStart + 7;

/// @brief Sensor output datatype
const UINT PortSrcTypeUndefined         = 0;
const UINT PortSrcTypePixel             = 1;
const UINT PortSrcTypePDAF              = 2;
const UINT PortSrcTypeHDR               = 3;
const UINT PortSrcTypeMeta              = 4;

/// @brief Sensor output port ID's
const UINT SensorInstanceName0          = 0;
const UINT SensorOutputPort0            = 0;
const UINT SensorOutputPort1            = 1;
const UINT SensorOutputPort2            = 2;
const UINT SensorOutputPort3            = 3;
const UINT SensorOutputPort4            = 4;

const UINT TorchInstanceName0           = 0;
const UINT TorchOutputPort0             = 0;

const UINT StatsProcessingInstanceName0 = 0;
const UINT StatsParseInstanceName0      = 0;
const UINT StatsParseOutputPort0        = 0;

/// @brief Stats Processing node output port names
const UINT StatsProcessingOutputPort0   = 0;

/// @brief Stats Processing node input ports, number values must match XML configuration
const UINT StatsInputPortHDRBE          = 0;
const UINT StatsInputPortAWBBG          = 1;
const UINT StatsInputPortHDRBHist       = 2;
const UINT StatsInputPortBHist          = 3;
const UINT StatsInputPortBF             = 4;
const UINT StatsInputPortIHist          = 5;
const UINT StatsInputPortCS             = 6;
const UINT StatsInputPortRS             = 7;
const UINT StatsInputPortRDIPDAF        = 8;
const UINT StatsInputPortTintlessBG     = 9;
const UINT StatsInputPortPDAFType3      = 10;
const UINT StatsInputPortRDIRaw         = 11;
const UINT StatsInputPortDualPDHWPDAF   = 12;
const UINT StatsInputPortRDIStats       = 13;
const UINT StatsInputPortLCRHW          = 14;
const UINT StatsInputPortBPSAWBBG       = 15;
const UINT StatsInputPortBPSRegYUV      = 16;
const UINT StatsInputPortMaxCount       = 17;


/// @brief Stats Parse node input ports, number values must match XML configuration
const UINT StatsParseInputPortHDRBE          = 0;
const UINT StatsParseInputPortAWBBG          = 1;
const UINT StatsParseInputPortHDRBHist       = 2;
const UINT StatsParseInputPortBHist          = 3;
const UINT StatsParseInputPortBF             = 4;
const UINT StatsParseInputPortIHist          = 5;
const UINT StatsParseInputPortCS             = 6;
const UINT StatsParseInputPortRS             = 7;
const UINT StatsParseInputPortRDIPDAF        = 8;
const UINT StatsParseInputPortTintlessBG     = 9;
const UINT StatsParseInputPortPDAFType3      = 10;
const UINT StatsParseInputPortRDIRaw         = 11;
const UINT StatsParseInputPortDualPDHWPDAF   = 12;
const UINT StatsParseInputPortRDIStats       = 13;
const UINT StatsParseInputPortLCRHW          = 14;
const UINT StatsParseInputPortBPSAWBBG       = 15;
const UINT StatsParseInputPortBPSRegYUV      = 16;
const UINT StatsParseInputPortMaxCount       = 17;

/// @brief Histogram Process node input ports, number values must match XML configuration
const UINT HistogramProcessInputPortFD  = 0;

/// @brief Autofocus node instance names
const UINT AutoFocusInstanceName0       = 0;

/// @todo  (CAMX-1349): Output port for sink node does not make much sense. However, topology manager only walks the AF node
/// if output port exists and is linked to sinknobuffer. Revisit this again.
/// @brief Auto Focus Node Output port names
const UINT AutoFocusOutputPort0         = 0;

/// @brief AWB node instance names
const UINT AWBInstanceName0 = 0;

/// @brief AWB Node Output port names
const UINT AWBOutputPort0 = 0;

/// @brief HistogramProcess node instance names
const UINT HistogramProcessInstanceName0 = 0;

/// @brief HistogramProcess Node Output port names
const UINT HistogramProcessOutputPort0   = 0;

const UINT SinkInstanceName0            = 0;
const UINT SinkInstanceName1            = 1;
const UINT SinkInstanceName2            = 2;
const UINT SinkInstanceName3            = 3;

/// @brief JPEG aggregator node instance names
const UINT JPEGAggregatorInstanceName0  = 0;

/// @brief JPEG Aggregator Node port names
const UINT JPEGAggregatorInputPort0     = 0;
const UINT JPEGAggregatorInputPort1     = 1;
const UINT JPEGAggregatorOutputPort0    = 2;

/// @brief FD manager Node port names
const UINT FDManagerInputPortHwResults          = 0;
const UINT FDManagerInputPortHwRawResults       = 1;
const UINT FDManagerInputPortHwPyramidBuffer    = 2;
const UINT FDManagerInputPortImage              = 3;
const UINT FDManagerMaxInputPorts               = 4;
const UINT FDManagerMaxOutputPorts              = 1;

/// @brief Tracker node input port names
const UINT TrackerInputPortImage                = 0;

const UINT OfflineStatsInputPort0               = 0;
const UINT OfflineStatsOutputPort0              = 0;
const UINT OfflineStatsInstanceName0            = 0;


static const UINT32 CamxCommandBufferAlignmentInBytes   = 32;   ///< Command buffer alignment in bytes
static const UINT32 CamxPacketAlignmentInBytes          = 8;    ///< Packet alignment in bytes

CAMX_NAMESPACE_END

#endif // CAMXHWDEFS_H
