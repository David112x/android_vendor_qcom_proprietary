// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

#ifndef GEOLIBUTILS_H
#define GEOLIBUTILS_H

#include "GeoLib.h"

void Dump_GeoLibVideoStreamInput(
    const GeoLibVideoFlowMode       flowMode,
    const GeoLibImageSize*          sensorSize,
    const GeoLibVideoStreamIfePort  ifeOutPort,
    const GeoLibImageSize*          fdOutSize,
    const NcLibIcaGrid*             ldFullOutToIn,
    const NcLibIcaGrid*             ldFullInToOut,
    const GeoLibImageSize*          ldFullOutSize,
    const GeoLibImageSize*          videoOutSize,
    const GeoLibImageSize*          displayOutSize,
    const float                     maxVsrScaling,
    const GeoLibEisParams*          eisParams,
    FILE*                           dumpFile);

void Dump_GeoLibVideoStreamConfig(
    const GeoLibVideoStreamConfig*  streamConfig,
    FILE*                           dumpFile);

void Dump_GeoLibVideoFrameConfig(
    const GeoLibVideoFrameConfig* geoLibStruct,
    const GeoLibROI*                zoomWindow,
    const GeoLibWindowSize*         satWindowSize,
    const float                     cvpDsRatio,
    const float                     motionIndication,
    FILE* dumpRegFile);

void Dump_GeoLibStillFrameInput(
    const GeoLibStillFlowMode       flowMode,
    const GeoLibMultiFrameMode      mfMode,
    const GeoLibImageSize*          sensorSize,
    const NcLibIcaGrid*             ldFullOutToIn,
    const NcLibIcaGrid*             ldFullInToOut,
    const GeoLibImageSize*          outSize,
    const GeoLibImageSize*          dispSize,
    const GeoLibImageSize*          regOutSize,
    const GeoLibROI*                zoomWindow,
    FILE*                           dumpFile);

void Dump_GeoLibStillFrameConfig(
    const GeoLibStillFrameConfig*   frameCfg,
    FILE*                           dumpFile);

bool Validate_GeoLibVideoStreamInit(
    const GeoLibVideoFlowMode       flowMode,
    const GeoLibImageSize*          sensorSize,
    GeoLibVideoStreamIfePort        ifeOutPort,
    const GeoLibImageSize*          fdOutSize,
    const NcLibIcaGrid*             ldFullOutToIn,
    const NcLibIcaGrid*             ldFullInToOut,
    const GeoLibImageSize*          ldFullOutSize,
    const GeoLibImageSize*          videoOutSize,
    const GeoLibImageSize*          displayOutSize,
    const float                     maxVsrScaling,
    const GeoLibEisParams*          eisParams,
    const GeoLibVideoStreamConfig*  streamConfig);

bool Validate_GeoLibVideoCalcFrame(
    const GeoLibVideoFlowMode       flowMode,
    const float                     digitalZoom,
    const bool                      isSatEnabled,
    const GeoLibVideoFrameConfig*   frameConfig);

bool Validate_GeoLibStillCalcFrameInput(
    const GeoLibStillFlowMode      flowMode,
    const GeoLibMultiFrameMode     mfMode,
    const GeoLibImageSize*         sensorSize,
    const NcLibIcaGrid*            ldFullOutToIn,
    const NcLibIcaGrid*            ldFullInToOut,
    const GeoLibImageSize*         ldFullOutSize,
    const GeoLibImageSize*         stillOutSize,
    const GeoLibImageSize*         dispOutSize,
    const GeoLibImageSize*         regOutSize,
    const float                    digitalZoom);

bool Validate_GeoLibStillCalcFrameOutput(
    const GeoLibStillFlowMode       flowMode,
    const GeoLibMultiFrameMode      mfMode,
    const GeoLibImageSize*          dispSize,
    const GeoLibStillFrameConfig*   frameConfig
);

void SetDefaultsForGeoLibIcaPassMapping(GeoLibIcaPassMapping* icaPassMapping, uint32_t numOfPasses);

#endif // GEOLIBUTILS_H
