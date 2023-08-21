////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2utils.h
/// @brief CHI feature utilities
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2UTILS_H
#define CHIFEATURE2UTILS_H

// NOWHINE FILE PR007b: Whiner incorrectly concludes as non-library files

#include <sstream>

#if defined (_LINUX) // This file for Linux build only
#include <hardware/gralloc1.h>
#endif // ANDROID

#include "chifeature2types.h"
#include "chitargetbuffermanager.h"
#include "chifeature2requestobject.h"

#include "chxusecaseutils.h"
#include "chiispstatsdefs.h"

// NOWHINE FILE CP006:  Used standard libraries for performance improvements

static const INT         DefaultStreamSizeQuotient      = 1;
static const UINT64      DefaultStreamFormat            = 0;
static const UINT64      DefaultStreamUsage             = 0;
static const INT         DefaultDS4StreamSizeQuotient   = 4;
static const INT         DefaultDS16StreamSizeQuotient  = 16;
static const INT         DefaultDS64StreamSizeQuotient  = 64;
static const INT         DefaultREGStreamSizeQuotient   = 3;
static const INT         DefaultStreamDirection         = -1;
static const UINT8       InvalidCameraIndex             = 0xFF;

// max resolution cap to output YUV from realtime feature
static const UINT        RTYUVOutputCap                 = 3840 * 2160;

static const UINT        MFSRDownscaleRatioShift        = 1;    ///< Currently the MFSR crop ratio is 2x,
                                                                ///  so we need to right shift 1 bit to achieve it
static const UINT        MaxIndexRegInputResolutionMap  = 7;

/// @brief The mapping table of registration input resolution from tuning XML
static const ChiDimension RegInputResolutionMap[]
{
    { 1920, 1440 },     ///<    Registration input resolution: 1440p
    { 1920, 1280 },     ///<    Registration input resolution: 1280p
    { 1920, 1080 },     ///<    Registration input resolution: 1080p
    { 1280, 960 },      ///<    Registration input resolution: 960p
    { 1280, 720 },      ///<    Registration input resolution: 720p
    { 960,  540 },      ///<    Registration input resolution: 540p
    { 480,  270 },      ///<    Registration input resolution: 270p
};

/// @brief Properties of incoming stream to match.
/// Incoming streams are compared against this data to get final reference stream per target.
struct ChiFeature2TargetStreamProperties
{
    CHISTREAMTYPE                direction;                    ///< Direction:Input or output
    std::vector<CHISTREAMFORMAT> format;                       ///< Stream Format
    UINT64                       usage;                        ///< Usage
    CHIDATASPACE                 dataspace;                    ///< A field that describes the contents of the buffer
    UINT32                       physicalCameraIndex;          ///< Match camera if values is not InvalidCameraId
};

/// @brief Values for internal streams.
/// This values are copied to internal stream on top of reference stream values.
struct ChiFeature2TargetStreamValues
{
    FLOAT                   widthQuotient;                     ///< Stream width Quotient compared to target
    FLOAT                   heightQuotient;                    ///< Stream height Quotient compared to target
    UINT64                  format;                            ///< Format value for internal stream
    INT32                   direction;                         ///< Direction value for internal stream
};

/// @brief Map info structure.
struct ChiFeature2TargetStreamMapInfo
{
    std::vector<const CHAR*>            pTargetName;             ///< Usecase XML target name.
                                                                 /// TARGET_BUFFER_<type_of_buffer>_<format>_<direction>
    ChiFeature2TargetStreamProperties   targetStream;            ///< reference stream property for this target
    ChiFeature2TargetStreamValues       targetStreamValues;      ///< Stream default values
};

/// @brief Initialize target and stream mapping structure.
const struct ChiFeature2TargetStreamMapInfo TargetStreamMap[] =
{
    /*
        Map is used to create internal stream for port/target based on incoming streams.
        Incoming stream means, list of streams configured during feature create.
        Default Values make internal stream values same as incoming stream values.
        "ChiFeature2TargetStreamProperties" is properties of incoming streams.
        "ChiFeature2TargetStreamValues" is value assigned to internal streams on top of targetStreamProperties.
    */
    /*
    {
        "Target Name from usecase descriptor/xml file",
        {Direction of incoming stream, {Possible formats of incoming stream}, Special Usage to differentiate},
        {Width Quotient w.r.t incoming stream, Height Quotient w.r.t incoming stream, StreamFormat, Stream Direction},
    }
    */
    {
        {"TARGET_BUFFER_RAW"},
        {ChiStreamTypeInput, {ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},

    },

    {
        {"TARGET_BUFFER_SWREMOSAIC_IN"},
        {ChiStreamTypeOutput, {ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, ChiStreamFormatRaw10, ChiStreamTypeInput},
    },

    {
        {"TARGET_BUFFER_CFA_FULLSIZE_RAW"},
        {ChiStreamTypeOutput, {ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, ChiStreamFormatRaw10, DefaultStreamDirection},

    },

    {
        {"TARGET_BUFFER_RAW_IN0"},
        {ChiStreamTypeInput, {ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},

    },

    {
        {"TARGET_BUFFER_RAW_IN1"},
        {ChiStreamTypeInput, {ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},

    },

    {
        {"TARGET_BUFFER_RAW_IN2"},
        {ChiStreamTypeInput, {ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},

    },

    {
        {"TARGET_BUFFER_YUV_HAL", "TARGET_BUFFER_YUV_HAL2" },
        {ChiStreamTypeOutput, {ChiStreamFormatYCbCr420_888},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_HEIC_YUV"},
        {ChiStreamTypeOutput, {ChiStreamFormatYCbCr420_888, ChiStreamFormatImplDefined},
        DefaultStreamUsage, DataspaceHEIF, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_CUSTOM_YUV", "TARGET_BUFFER_CUSTOM_YUV2" },
        {ChiStreamTypeInput, {ChiStreamFormatRaw10, ChiStreamFormatYCbCr420_888 },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, ChiStreamFormatYCbCr420_888, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_YUV", "TARGET_BUFFER_YUV_OUT", "TARGET_BUFFER_FD"},
        {ChiStreamTypeOutput, {ChiStreamFormatYCbCr420_888},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_IDEAL_RAW_OUT"},
        {ChiStreamTypeInput, {ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque}, DefaultStreamUsage},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, ChiStreamFormatRaw16, ChiStreamTypeOutput},
    },

    {
        {"TARGET_BUFFER_RAW_OUT"},
        {ChiStreamTypeOutput, {ChiStreamFormatRaw10, ChiStreamFormatRaw16},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_DISPLAY"},
        {ChiStreamTypeOutput, { ChiStreamFormatImplDefined, ChiStreamFormatYCbCr420_888 },
        GrallocUsageHwTexture|GrallocUsageHwRender, DataspaceUnknown, InvalidCameraId},
        {DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_SNAPSHOT", "TARGET_BUFFER_SNAPSHOT2", "TARGET_BUFFER_HEIC_BLOB"},
        {ChiStreamTypeOutput, {ChiStreamFormatBlob},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection },
    },

    {
        {"TARGET_BUFFER_VIDEO"},
        { ChiStreamTypeOutput, { ChiStreamFormatYCbCr420_888, ChiStreamFormatImplDefined },
        GrallocUsageHwVideoEncoder, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },

    // This is for YUV callback
    {
        { "TARGET_BUFFER_VIDEO" },
        { ChiStreamTypeOutput, { ChiStreamFormatYCbCr420_888 },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection },
    },

    {
        {"TARGET_BUFFER_FULLREF_UBWCTP10_OUT"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, ChiStreamFormatUBWCTP10, ChiStreamTypeOutput},
    },

    {
        {"TARGET_BUFFER_DS4REF_PD10_OUT"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultDS4StreamSizeQuotient, DefaultDS4StreamSizeQuotient, ChiStreamFormatPD10, ChiStreamTypeOutput},
    },

    {
        {"TARGET_BUFFER_DS16REF_PD10_OUT"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultDS16StreamSizeQuotient, DefaultDS16StreamSizeQuotient, ChiStreamFormatPD10, ChiStreamTypeOutput},
    },

    {
        {"TARGET_BUFFER_DS64REF_PD10_OUT"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultDS64StreamSizeQuotient, DefaultDS64StreamSizeQuotient, ChiStreamFormatPD10, ChiStreamTypeOutput},
    },

    {
        {"TARGET_BUFFER_REG_NV12_OUT"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultREGStreamSizeQuotient, DefaultREGStreamSizeQuotient, ChiStreamFormatYCbCr420_888, ChiStreamTypeOutput},
    },

    {
        {"TARGET_BUFFER_REG_UBWCNV124R_OUT"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultREGStreamSizeQuotient, DefaultREGStreamSizeQuotient, ChiStreamFormatUBWCNV124R, ChiStreamTypeOutput },
    },

    {
        {"TARGET_BUFFER_CVP_DMECONTEXT_OUT"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, ChiStreamFormatBlob, ChiStreamTypeOutput },
    },

    {
        {"TARGET_BUFFER_FULLREF_UBWCTP10_IN"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, ChiStreamFormatUBWCTP10, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_DS4REF_PD10_IN"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultDS4StreamSizeQuotient, DefaultDS4StreamSizeQuotient, ChiStreamFormatPD10, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_DS16REF_PD10_IN"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultDS16StreamSizeQuotient, DefaultDS16StreamSizeQuotient, ChiStreamFormatPD10, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_DS64REF_PD10_IN"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultDS64StreamSizeQuotient, DefaultDS64StreamSizeQuotient, ChiStreamFormatPD10, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_REG_NV12_IN"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultREGStreamSizeQuotient, DefaultREGStreamSizeQuotient, ChiStreamFormatYCbCr420_888, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_YUV0", "TARGET_BUFFER_YUV1", "TARGET_BUFFER_YUV2", "TARGET_BUFFER_YUV3", "TARGET_BUFFER_YUV4",
        "TARGET_BUFFER_YUV5", "TARGET_BUFFER_YUV6", "TARGET_BUFFER_YUV7", "TARGET_BUFFER_YUV_OUT"},
        { ChiStreamTypeInput, { ChiStreamFormatYCbCr420_888 }, DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection },
    },

    {
        {"TARGET_BUFFER_REG_UBWCNV124R_IN"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultREGStreamSizeQuotient, DefaultREGStreamSizeQuotient, ChiStreamFormatUBWCNV124R, DefaultStreamDirection },
    },

    {
        {"TARGET_BUFFER_CVP_DMECONTEXT_IN"},
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, ChiStreamFormatBlob, DefaultStreamDirection },
    },

    {
        {"TARGET_BUFFER_FUSION_INPUT0"},
        { ChiStreamTypeInput, {ChiStreamFormatYCbCr420_888},
        DefaultStreamUsage, DataspaceUnknown, 0},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_FUSION_INPUT1"},
        { ChiStreamTypeInput, {ChiStreamFormatYCbCr420_888},
        DefaultStreamUsage, DataspaceUnknown, 1},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_FUSION_INPUT2"},
        { ChiStreamTypeInput, {ChiStreamFormatYCbCr420_888},
        DefaultStreamUsage, DataspaceUnknown, 2},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },

    {
        {"TARGET_BUFFER_FUSION_INPUT3"},
        { ChiStreamTypeInput, {ChiStreamFormatYCbCr420_888},
        DefaultStreamUsage, DataspaceUnknown, 3},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },

    {
        {"TARGET_FUSION_SNAPSHOT"},
        { ChiStreamTypeOutput, {ChiStreamFormatYCbCr420_888},
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection},
    },
    {
        { "TARGET_BUFFER_IN",  },
        { ChiStreamTypeInput, { ChiStreamFormatYCbCr420_888 },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection },
    },
    {
        { "TARGET_BUFFER_OUT", },
        { ChiStreamTypeOutput, { ChiStreamFormatYCbCr420_888 },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection },
    },
    {
        { "TARGET_BUFFER_P010" },
        { ChiStreamTypeInput, { ChiStreamFormatRaw10, ChiStreamFormatRaw16, ChiStreamFormatRawOpaque },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, ChiStreamFormatP010, ChiStreamTypeOutput },
    },
    {
        { "TARGET_BUFFER_P010_UBWCTP10" },
        { ChiStreamTypeInput, { ChiStreamFormatUBWCTP10, ChiStreamFormatP010 },
        DefaultStreamUsage, DataspaceUnknown, InvalidCameraId},
        { DefaultStreamSizeQuotient, DefaultStreamSizeQuotient, DefaultStreamFormat, DefaultStreamDirection },
    },
};


static CHISTREAMPARAMS StreamParams
{
    0,
    0
};

static CHISTREAMPARAMS StreamParams1
{
    0,
    0
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of HDR Streams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHISTREAM HDRT1StreamsInput1
{
    ChiStreamTypeInput,
    4160,
    3120,
    ChiStreamFormatYCbCr420_888,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM HDRT1StreamsInput2
{
    ChiStreamTypeInput,
    4160,
    3120,
    ChiStreamFormatYCbCr420_888,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM HDRT1StreamsInput3
{
    ChiStreamTypeInput,
    4160,
    3120,
    ChiStreamFormatYCbCr420_888,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM HDRT1StreamsOutput1
{
    ChiStreamTypeOutput,
    4160,
    3120,
    ChiStreamFormatYCbCr420_888,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM* pHDRT1Streams[] =
{
    &HDRT1StreamsInput1,
    &HDRT1StreamsInput2,
    &HDRT1StreamsInput3,
    &HDRT1StreamsOutput1,
};

static CHISTREAMCONFIGINFO HDRT1StreamConfigInfo
{
    2,
    pHDRT1Streams,
    0,
    NULL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of Bayer2Yuv Streams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHISTREAM Bayer2YuvStreamsInput
{
    ChiStreamTypeInput,
    4608,
    3456,
    ChiStreamFormatRaw10,   // Raw10 not allocating enough buffer space for the image (needs 2 bytes per pixel).
    // Image might have 6 bytes of padding.
    GRALLOC1_PRODUCER_USAGE_CAMERA |
    GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM Bayer2YuvStreamsOutput
{
    ChiStreamTypeOutput,
    4160,
    3120,
    ChiStreamFormatYCbCr420_888,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM* pBayer2YuvStreams[] =
{
    &Bayer2YuvStreamsInput,
    &Bayer2YuvStreamsOutput
};

static CHISTREAMCONFIGINFO Bayer2YuvStreamConfigInfo
{
    2,
    pBayer2YuvStreams,
    0,
    NULL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of BPS Streams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHISTREAM BPSStreamsInput
{
    ChiStreamTypeInput,
    4608,
    3456,
    ChiStreamFormatRaw10,   // Raw10 not allocating enough buffer space for the image (needs 2 bytes per pixel).
    // Image might have 6 bytes of padding.
    GRALLOC1_PRODUCER_USAGE_CAMERA |
    GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM BPSStreamsOutput
{
    ChiStreamTypeOutput,
    4608,
    3456,
    ChiStreamFormatP010,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM* pBPSStreams[] =
{
    &BPSStreamsInput,
    &BPSStreamsOutput
};

static CHISTREAMCONFIGINFO BPSStreamConfigInfo
{
    2,
    pBPSStreams,
    0,
    NULL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of IPE Streams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHISTREAM IPEStreamsInput
{
    ChiStreamTypeInput,
    4608,
    3456,
    ChiStreamFormatP010,
    // Image might have 6 bytes of padding.
    GRALLOC1_PRODUCER_USAGE_CAMERA |
    GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM IPEStreamsOutput
{
    ChiStreamTypeOutput,
    4160,
    3120,
    ChiStreamFormatYCbCr420_888,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM* pIPEStreams[] =
{
    &IPEStreamsInput,
    &IPEStreamsOutput
};

static CHISTREAMCONFIGINFO IPEStreamConfigInfo
{
    2,
    pIPEStreams,
    0,
    NULL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of MFSR Streams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHISTREAM MFSRStreamsInput1
{
    ChiStreamTypeInput,
    4096,
    3072,
    ChiStreamFormatRaw10,   // Raw10 not allocating enough buffer space for the image (needs 2 bytes per pixel).
                            // Image might have 6 bytes of padding.
                            GRALLOC1_PRODUCER_USAGE_CAMERA |
                            GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM MFSRStreamsInput2
{
    ChiStreamTypeInput,
    4096,
    3072,
    ChiStreamFormatUBWCTP10,   // Raw10 not allocating enough buffer space for the image (needs 2 bytes per pixel).
                            // Image might have 6 bytes of padding.
                            GRALLOC1_PRODUCER_USAGE_CAMERA |
                            GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM MFSRStreamsInput3
{
    ChiStreamTypeInput,
    4096,
    3072,
    ChiStreamFormatYCbCr420_888,   // Raw10 not allocating enough buffer space for the image (needs 2 bytes per pixel).
                            // Image might have 6 bytes of padding.
                            GRALLOC1_PRODUCER_USAGE_CAMERA |
                            GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM MFSRStreamsInput4
{
    ChiStreamTypeInput,
    4096,
    3072,
    ChiStreamFormatPD10,   // Raw10 not allocating enough buffer space for the image (needs 2 bytes per pixel).
                                   // Image might have 6 bytes of padding.
                                   GRALLOC1_PRODUCER_USAGE_CAMERA |
                                   GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM MFSRStreamsOutput1
{
    ChiStreamTypeOutput,
    4096,
    3072,
    ChiStreamFormatYCbCr420_888,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM MFSRStreamsOutput2
{
    ChiStreamTypeOutput,
    4096,
    3072,
    ChiStreamFormatUBWCTP10,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM MFSRStreamsOutput3
{
    ChiStreamTypeOutput,
    4096,
    3072,
    ChiStreamFormatPD10,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM* pMFSRStreams[] =
{
    &MFSRStreamsInput1,
    &MFSRStreamsInput2,
    &MFSRStreamsInput3,
    &MFSRStreamsInput4,
    &MFSRStreamsOutput1,
    &MFSRStreamsOutput2,
    &MFSRStreamsOutput3
};

static CHISTREAMCONFIGINFO MFSRStreamConfigInfo
{
    7,
    pMFSRStreams,
    0,
    NULL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of JPEG Streams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHISTREAMPARAMS JPEGStreamParams
{
    0,
    0
};

static CHISTREAM YUV2JPEGStreamsInput
{
    ChiStreamTypeInput,
    4160,
    3120,
    ChiStreamFormatYCbCr420_888,
    GRALLOC1_PRODUCER_USAGE_CAMERA |
    GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    JPEGStreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM YUV2JPEGStreamsOutput
{
    ChiStreamTypeOutput,
    4000,
    3000,
    ChiStreamFormatBlob,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    JPEGStreamParams,
    { NULL, NULL, NULL },

};

static  CHISTREAM* pJPEGStreams[] =
{
    &YUV2JPEGStreamsInput,
    &YUV2JPEGStreamsOutput
};

static CHISTREAMCONFIGINFO YUV2JPEGStreamConfigInfo
{
    2,
    pJPEGStreams,
    0,
    NULL
};

/// @brief RDI Stream
static CHISTREAM RDIStream
{
    ChiStreamTypeOutput,
    4096,
    3072,
    ChiStreamFormatRaw10,
    GRALLOC1_PRODUCER_USAGE_CAMERA |
    GRALLOC1_PRODUCER_USAGE_CPU_READ,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams1,
    { NULL, NULL, NULL },
};

// Video stream
static CHISTREAM VideoStream
{
    ChiStreamTypeOutput,
    1920,
    1080,
    ChiStreamFormatImplDefined,
    GRALLOC_USAGE_HW_VIDEO_ENCODER,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams1,
    { NULL, NULL, NULL },
};

static CHISTREAM NZSLSnapshotRDIStream
{
    ChiStreamTypeOutput,
    5184,
    3880,
    ChiStreamFormatRaw10,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams1,
    { NULL, NULL, NULL },
};

static CHISTREAM NZSLBayer2YUVStreamsInput
{
    ChiStreamTypeInput,
    5184,
    3880,
    ChiStreamFormatRaw10,
    GRALLOC1_PRODUCER_USAGE_CAMERA |
    GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

static CHISTREAM NZSLBPSStreamsInput
{
    ChiStreamTypeInput,
    5184,
    3880,
    ChiStreamFormatRaw10,
    GRALLOC1_PRODUCER_USAGE_CAMERA |
    GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    StreamParams,
    { NULL, NULL, NULL },
};

/// @brief ChiFeature2Type
enum class ChiFeature2Type
{
    REALTIME,           ///< Realtime
    B2Y,                ///< Bayer to YUV
    BPS,                ///< BPS
    IPE,                ///< IPE
    JPEG,               ///< JPEG
    HDR,                ///< High Dynamic Range
    MFNR,               ///< MultiFrame Noise Reduction
    SWMF,               ///< Software MulitFrame
    RAWHDR,             ///< Software MulitFrame
    MFSR,               ///< MultiFrame Super Resolution
    QCFA,               ///< Quadra Color Filter Array
    FUSION,             ///< FUSION
    BOKEH,              ///< BOKEH
    ANCHORSYNC,         ///< ANCHORSYNC
    MFNR_HDR,           ///< MFNR + HDR
    MFSR_HDR,           ///< MFSR + HDR
    HDR_BOKEH,          ///< HDR  + BOKEH
    MFNR_BOKEH,         ///< MFNR + BOKEH
    MFSR_BOKEH,         ///< MFSR + BOKEH
    STUB_RT,            ///< STUB REALTIME
    STUB_B2Y,           ///< STUB B2Y
    FRAME_SELECT,       ///< Frame Select
    MEMCPY,             ///< Memcpy
    DEMUX,              ///< Demux
    SERIALIZER,         ///< Serializer
    FORMATCONVERTOR,    ///< Format Convertor
    MaxFeatureList      ///< Max
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsMultiCameraFeature
///
/// @brief  Method to check if the feature is multi camera special feature
///
/// @param  rFeatureType    The reference of feature type
///
/// @return TRUE if it is multi camera special feature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE BOOL IsMultiCameraFeature(
    ChiFeature2Type& rFeatureType)
{
    BOOL isMultiCameraFeature = FALSE;
    if ((rFeatureType == ChiFeature2Type::ANCHORSYNC) ||
        (rFeatureType == ChiFeature2Type::BOKEH)      ||
        (rFeatureType == ChiFeature2Type::FUSION))
    {
        isMultiCameraFeature = TRUE;
    }
    return isMultiCameraFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsFDStream
///
/// @brief  Method to check if the stream is FD stream
///
/// @param  pSrcCameraStream    camera stream to be cloned
///
/// @return TRUE if the stream is FD stream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE BOOL IsFDStream(
    ChiStream* pSrcCameraStream)
{
    BOOL      isFDStream = FALSE;
    ChiStream fdStream   = UsecaseSelector::GetFDOutStream();

    if (pSrcCameraStream->format == fdStream.format &&
        pSrcCameraStream->width  == fdStream.width &&
        pSrcCameraStream->height == fdStream.height)
    {
        isFDStream = TRUE;
    }

    return isFDStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsFusionStreamIncluded
///
/// @brief  Method to check whether streams include fusion stream
///
/// @param  pCameraStream  streams requested by the framework
///
/// @return FALSE if  stream include fusion stream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE BOOL IsFusionStreamIncluded(
    camera3_stream_configuration_t* pStreamConfig)
{
    BOOL isFusionStreamIncluded = FALSE;
    for (UINT32 i = 0; i < pStreamConfig->num_streams; i++)
    {
        // For YUV stream ,only fusion stream's physicalCameraId is NULL
        if ((pStreamConfig->streams[i]->format == HAL_PIXEL_FORMAT_YCbCr_420_888) &&
            (reinterpret_cast<CHISTREAM*>(pStreamConfig->streams[i])->physicalCameraId == NULL))
        {
            isFusionStreamIncluded = TRUE;
        }
    }

    return isFusionStreamIncluded;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Is4KYUVOut
///
/// @brief  Method to check if the chistream has YUV output of >= 4K
///
/// @param  pCameraStream    camera stream
///
/// @return TRUE if the stream is FD stream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE BOOL Is4KYUVOut(
    ChiStream* pCameraStream)
{
    BOOL is4KYUVOut = FALSE;
    if (TRUE == UsecaseSelector::IsYUVSnapshotStream(reinterpret_cast<camera3_stream_t*>(pCameraStream)))
    {
        if ((pCameraStream->height * pCameraStream->width) >= RTYUVOutputCap)
        {
            CHX_LOG_INFO("framework YUV out stream");
            is4KYUVOut = TRUE;
        }
    }

    return is4KYUVOut;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetBlobResolution
///
/// @brief  Method to clone stream
///
/// @param  pSrcCameraStream    camera stream to be cloned
///
/// @return Cloned stream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE CDKResult GetBlobResolution(
    CHISTREAMCONFIGINFO* pFwkStreamConfig,
    UINT32*              pBlobWidth,
    UINT32*              pBlobHeight,
    UINT32               physicalCameraId,
    BOOL                 isMultiCamera)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pFwkStreamConfig)
    {
        CHX_LOG_ERROR("invalid Arg! pFwkStreamConfig=NULL!");
        result = CDKResultEInvalidArg;
    }
    else
    {
        const UINT   numStreams   = pFwkStreamConfig->numStreams;
        CHISTREAM**  ppChiStreams = pFwkStreamConfig->pChiStreams;

        for (UINT streamIdx = 0; streamIdx < numStreams; streamIdx++)
        {

            if (ppChiStreams[streamIdx]->format == ChiStreamFormatBlob)
            {
                if ((NULL == ppChiStreams[streamIdx]->physicalCameraId) ||
                    (FALSE == isMultiCamera) || ((TRUE == isMultiCamera) &&
                    (NULL != ppChiStreams[streamIdx]->physicalCameraId) &&
                    (ChxUtils::GetCameraIdFromStream(ppChiStreams[streamIdx]) == physicalCameraId)))
                {
                    *pBlobWidth  = ppChiStreams[streamIdx]->width;
                    *pBlobHeight = ppChiStreams[streamIdx]->height;
                    break;
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SetMCCResult
///
/// @brief  Method to set MCC result to input metadata
///
/// @param  pVendorOps    vendor ops
/// @param  pAppSetting   input metadata
/// @param  pMCCResult    MCC result from multi camera controller
///
/// @return CDKResultSuccess if Set MCC result successfully
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE CDKResult SetMCCResult(
    CHITAGSOPS*               pVendorOps,
    const camera_metadata_t*  pAppSetting,
    Feature2ControllerResult* pMCCResult)
{
    CDKResult result = CDKResultSuccess;
    if ((NULL == pVendorOps) || (NULL == pAppSetting) || (NULL == pMCCResult))
    {
        CHX_LOG_ERROR("Invalid Arg! pVendorOps:%p, pAppSetting=%p, pMCCResult=%p",
            pVendorOps, pAppSetting, pMCCResult);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        UINT32 tag;
        pVendorOps->pQueryVendorTagLocation("com.qti.chi.feature2RequestInfo",
                                            "Feature2MccResult",
                                            &tag);
        // NOWHINE CP036a: Settings are from Android, so API is fixed
        result = pVendorOps->pSetMetaData(const_cast<camera_metadata_t*>(pAppSetting),
                                          tag,
                                          pMCCResult,
                                          sizeof(Feature2ControllerResult));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetMCCResult
///
/// @brief  Method to set MCC result to input metadata
///
/// @param  pVendorOps    vendor ops
/// @param  pAppSetting   input metadata
/// @param  pMCCResult    MCC result from multi camera controller
///
/// @return CDKResultSuccess if Set MCC result successfully
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE Feature2ControllerResult GetMCCResult(
    CHITAGSOPS*               pVendorOps,
    const camera_metadata_t*  pAppSetting,
    LogicalCameraInfo*        pCameraInfo)
{
    Feature2ControllerResult mccResult =
    {
        FALSE,                                      // isValid
        FALSE,                                      // isSnapshotFusion
        pCameraInfo->ppDeviceInfo[0]->cameraId,     // masterCameraId
        {{0}},                                      // activeCameras[MaxDevicePerLogicalCamera]
        0,                                          // numActiveCameras
        0x01                                        // activeMap
    };

    if (NULL != pAppSetting)
    {
        Feature2ControllerResult feature2ControllerResult;
        UINT32                   feature2RequestTag;

        pVendorOps->pQueryVendorTagLocation("com.qti.chi.feature2RequestInfo",
                                            "Feature2MccResult",
                                            &feature2RequestTag);

        CDKResult result = pVendorOps->pGetMetaData(
                // NOWHINE CP036a: Settings are from Android, so API is fixed
                reinterpret_cast<VOID*>(const_cast<camera_metadata_t*>(pAppSetting)),
                feature2RequestTag, &feature2ControllerResult, sizeof(Feature2ControllerResult));

        if (CDKResultSuccess == result)
        {
            ChxUtils::Memcpy(&mccResult, &feature2ControllerResult, sizeof(Feature2ControllerResult));
        }
    }

    CHX_LOG_INFO("MCC Result: isValid = %d, isFusion=%d, masterCameraId = %d, numActiveCameras = %d, activeMap = 0x%x",
                  mccResult.isValid, mccResult.isSnapshotFusion,
                  mccResult.masterCameraId, mccResult.numActiveCameras, mccResult.activeMap);
    return mccResult;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetMasterCameraIdx
///
/// @brief  Method to get master pipeline camera index
///
/// @param  pMCCResult    The pointer of MCC result
/// @param  pCameraInfo   The pointer of camera information
///
/// @return camera index of master
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE UINT8 GetMasterCameraIdx(
    const Feature2ControllerResult* pMCCResult,
    const LogicalCameraInfo*        pCameraInfo)
{
    UINT8 cameraIdx = InvalidCameraIndex;
    if ((NULL != pMCCResult) && (NULL != pCameraInfo))
    {
        for (UINT8 camIdx = 0; camIdx < pCameraInfo->numPhysicalCameras; ++camIdx)
        {
            if (pMCCResult->masterCameraId == pCameraInfo->ppDeviceInfo[camIdx]->cameraId)
            {
                cameraIdx = camIdx;
                break;
            }
        }
    }
    return cameraIdx;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetJPEGStream
///
/// @brief  Method to clone stream
///
/// @param  pSrcCameraStream    camera stream to be cloned
///
/// @return Cloned stream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE CHISTREAM* GetJPEGStream(
    CHISTREAMCONFIGINFO* pFwkStreamConfig,
    UINT32               physicalCameraId,
    BOOL                 isMultiCamera)
{
    CHISTREAM* pJPEGStream = NULL;

    if (NULL == pFwkStreamConfig)
    {
        CHX_LOG_ERROR("invalid Arg! pFwkStreamConfig=NULL!");
    }
    else
    {
        const UINT   numStreams   = pFwkStreamConfig->numStreams;
        CHISTREAM**  ppChiStreams = pFwkStreamConfig->pChiStreams;

        for (UINT streamIdx = 0; streamIdx < numStreams; streamIdx++)
        {

            if (ppChiStreams[streamIdx]->format == ChiStreamFormatBlob)
            {
                if ((FALSE == isMultiCamera) || ((TRUE == isMultiCamera) &&
                    ((NULL != ppChiStreams[streamIdx]->physicalCameraId) &&
                    (ChxUtils::GetCameraIdFromStream(ppChiStreams[streamIdx]) == physicalCameraId))))
                {
                    pJPEGStream = ppChiStreams[streamIdx];
                    break;
                }
            }
        }
    }

    return pJPEGStream;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetYUVSnapshotStream
///
/// @brief  Method to get YUV snapshot stream
///
/// @param  pFwkStreamConfig  stream from framework configure
///
/// @return YUV snapshot stream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE CHISTREAM* GetYUVSnapshotStream(
    CHISTREAMCONFIGINFO* pFwkStreamConfig)
{
    CHISTREAM* pYUVSnapshotStream = NULL;

    if (NULL == pFwkStreamConfig)
    {
        CHX_LOG_ERROR("invalid Arg! pFwkStreamConfig=NULL!");
    }
    else
    {
        const UINT   numStreams     = pFwkStreamConfig->numStreams;
        CHISTREAM**  ppChiStreams   = pFwkStreamConfig->pChiStreams;
        UINT64       maxResolution  = 0;

        for (UINT streamIdx = 0; streamIdx < numStreams; streamIdx++)
        {
            CHISTREAM* pStream = ppChiStreams[streamIdx];
            CHX_LOG_INFO("Negotiate stream = %d : Size = %d X %d Format = 0x%x Type = %d",
                         streamIdx, pStream->width, pStream->height, pStream->format, pStream->streamType);
            if ((ChiStreamTypeOutput == pStream->streamType)         &&
                (maxResolution < (pStream->width * pStream->height)) &&
                (pStream->format == ChiStreamFormatYCbCr420_888))
            {
                pYUVSnapshotStream  = pStream;
                maxResolution       = pStream->width * pStream->height;
            }
        }
    }

    return pYUVSnapshotStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CreateGlobalPortInstanceId
///
/// @brief  Creates a ChiFeature2GlobalPortInstanceId from its components
///
/// @param  featureId   The feature id from which to create the ChiFeature2GlobalPortInstanceId
/// @param  cameraId    The camera id from which to create the ChiFeature2GlobalPortInstanceId
/// @param  portId      The port id from which to create the ChiFeature2GlobalPortInstanceId
///
/// @return Newly constructed ChiFeature2GlobalPortInstanceId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE ChiFeature2GlobalPortInstanceId CreateGlobalPortInstanceId(
    const UINT32                    featureId,
    const ChiFeature2InstanceProps  instanceProps,
    const ChiFeature2Identifier     portId)
{
    return
    {
        featureId,
        instanceProps,
        portId
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CreateGlobalPortInstanceIdFromDesc
///
/// @brief  Creates a ChiFeature2GlobalPortInstanceId from a ChiFeature2InstanceDesc and a ChiFeature2PortDescriptor.
///
/// @param  rFeatureInstanceDesc  The ChiFeature2InstanceDesc from which to create the ChiFeature2GlobalPortInstanceId
/// @param  rPortDesc             The ChiFeature2PortDescriptor from which to create the ChiFeature2GlobalPortInstanceId
///
/// @return Newly constructed ChiFeature2GlobalPortInstanceId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE ChiFeature2GlobalPortInstanceId CreateGlobalPortInstanceIdFromDesc(
    const ChiFeature2InstanceDesc&      rFeatureInstanceDesc,
    const ChiFeature2PortDescriptor&    rPortDesc)
{
    return
    {
        rFeatureInstanceDesc.pFeatureDesc->featureId,
        *(rFeatureInstanceDesc.pInstanceProps),
        rPortDesc.globalId
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsGlobalPortInstanceIdEqual
///
/// @brief  Compares two ChiFeature2GlobalPortInstanceId values for equality
///
/// @param  rLHS The left-hand ChiFeature2GlobalPortInstanceId to compare
/// @param  rRHS The Right-hand ChiFeature2GlobalPortInstanceId to compare
///
/// @return TRUE if two ChiFeature2GlobalPortInstanceId values are the same
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsGlobalPortInstanceIdEqual(
    const ChiFeature2GlobalPortInstanceId& rLHS,
    const ChiFeature2GlobalPortInstanceId& rRHS)
{
    return ((rLHS.featureId                   == rRHS.featureId) &&
            (rLHS.instanceProps               == rRHS.instanceProps) &&
            (rLHS.portId.session              == rRHS.portId.session) &&
            (rLHS.portId.pipeline             == rRHS.portId.pipeline) &&
            (rLHS.portId.port                 == rRHS.portId.port) &&
            (rLHS.portId.portDirectionType    == rRHS.portId.portDirectionType));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsGlobalPortInstanceIdLess
///
/// @brief  Returns TRUE if ChiFeature2GlobalPortInstanceId rLHS is less then rRHS
///
/// @param  rLHS The left-hand ChiFeature2GlobalPortInstanceId to compare
/// @param  rRHS The Right-hand ChiFeature2GlobalPortInstanceId to compare
///
/// @return TRUE if ChiFeature2GlobalPortInstanceId rLHS is less then rRHS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsGlobalPortInstanceIdLess(
    const ChiFeature2GlobalPortInstanceId& rLHS,
    const ChiFeature2GlobalPortInstanceId& rRHS)
{
    BOOL isLHSLessThanRHS = FALSE;

    if (rLHS.featureId < rRHS.featureId)
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.featureId                == rRHS.featureId)                &&
             (rLHS.instanceProps.instanceId <  rRHS.instanceProps.instanceId))
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.featureId                == rRHS.featureId)                &&
             (rLHS.instanceProps.instanceId == rRHS.instanceProps.instanceId) &&
             (rLHS.instanceProps.cameraId   <  rRHS.instanceProps.cameraId))
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.featureId                == rRHS.featureId)                &&
             (rLHS.instanceProps.instanceId == rRHS.instanceProps.instanceId) &&
             (rLHS.instanceProps.cameraId   == rRHS.instanceProps.cameraId)   &&
             (rLHS.portId.session           <  rRHS.portId.session))
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.featureId                == rRHS.featureId)                &&
             (rLHS.instanceProps.instanceId == rRHS.instanceProps.instanceId) &&
             (rLHS.instanceProps.cameraId   == rRHS.instanceProps.cameraId)   &&
             (rLHS.portId.session           == rRHS.portId.session)           &&
             (rLHS.portId.pipeline          <  rRHS.portId.pipeline))
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.featureId                == rRHS.featureId)                &&
             (rLHS.instanceProps.instanceId == rRHS.instanceProps.instanceId) &&
             (rLHS.instanceProps.cameraId   == rRHS.instanceProps.cameraId)   &&
             (rLHS.portId.session           == rRHS.portId.session)           &&
             (rLHS.portId.pipeline          == rRHS.portId.pipeline)          &&
             (rLHS.portId.port              <  rRHS.portId.port))
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.featureId                == rRHS.featureId)                &&
             (rLHS.instanceProps.instanceId == rRHS.instanceProps.instanceId) &&
             (rLHS.instanceProps.cameraId   == rRHS.instanceProps.cameraId)   &&
             (rLHS.portId.session           == rRHS.portId.session)           &&
             (rLHS.portId.pipeline          == rRHS.portId.pipeline)          &&
             (rLHS.portId.port              == rRHS.portId.port)              &&
             (rLHS.portId.portDirectionType <  rRHS.portId.portDirectionType))
    {
        isLHSLessThanRHS = TRUE;
    }

    return isLHSLessThanRHS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IsVideoWithoutEIS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsVideoWithoutEIS(
    const CHAR* pPipelineName)
{
    BOOL bVideoWithoutEIS = FALSE;
    if (NULL != pPipelineName)
    {
        if (0 == CdkUtils::StrCmp(pPipelineName, "RealTimeFeatureZSLPreviewRawYUV"))
        {
            bVideoWithoutEIS = TRUE;
        }
        else
        {
            bVideoWithoutEIS = FALSE;
        }
    }

    return bVideoWithoutEIS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IsVideoEISV2Enabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsVideoEISV2Enabled(
    const CHAR* pPipelineName,
    UINT32 operationMode)
{
    BOOL bEISV2Enabled = FALSE;
    if (NULL != pPipelineName)
    {
        if ((0 == CdkUtils::StrCmp(pPipelineName, "VideoEIS2PreviewEIS2")) &&
            ((operationMode & StreamConfigModeQTIEISRealTime) == StreamConfigModeQTIEISRealTime))
        {
            bEISV2Enabled = TRUE;
        }
        else
        {
            bEISV2Enabled = FALSE;
        }
    }

    return bEISV2Enabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IsVideoEISV3Enabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsVideoEISV3Enabled(
    const CHAR* pPipelineName,
    UINT32 operationMode)
{
    BOOL bEISV3Enabled = FALSE;
    if (NULL != pPipelineName)
    {
        if ((0 == CdkUtils::StrCmp(pPipelineName, "VideoEIS3PreviewEIS2")) &&
            ((operationMode & StreamConfigModeQTIEISLookAhead) == StreamConfigModeQTIEISLookAhead))
        {
            bEISV3Enabled = TRUE;
        }
        else
        {
            bEISV3Enabled = FALSE;
        }
    }

    return bEISV3Enabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IsVideo4KEISV3Enabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsVideo4KEISV3Enabled(
    const CHAR* pPipelineName,
    UINT32 width,
    UINT32 height)
{
    BOOL b4kEISV3Enabled = FALSE;

    if ((0 == CdkUtils::StrCmp(pPipelineName, "Video4kEIS3PreviewEIS2")) &&
        ((UHDResolutionWidth == width) && (UHDResolutionHeight == height)))
    {
        b4kEISV3Enabled = TRUE;
    }
    else
    {
        b4kEISV3Enabled = FALSE;
    }

    return b4kEISV3Enabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IsPreviewPipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsPreviewPipeline(
    const CHAR* pPipelineName)
{
    BOOL bPreviewPipeline = FALSE;
    if (NULL != pPipelineName)
    {
        if (0 == CdkUtils::StrCmp(pPipelineName, "RealTimeFeatureZSLPreviewRaw"))
        {
            bPreviewPipeline = TRUE;
        }
        else
        {
            bPreviewPipeline = FALSE;
        }
    }

    return bPreviewPipeline;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IsVideoStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsVideoStream(
    UINT32 usage)
{
    BOOL isVideoStream = FALSE;

    if ((0 != (GRALLOC_USAGE_HW_VIDEO_ENCODER & usage)))
    {
        isVideoStream = TRUE;
    }

    return isVideoStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetMetaOwner
///
/// @brief  Obtain the camera ID of the metadata owner
///
/// @param  pResultMetadata     Result metadata
///
/// @return Camera ID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE UINT32 GetMetaOwner(
    ChiMetadata* pResultMetadata)
{
    INT32*      pMetaOwner       = NULL;
    UINT32      metaOwner        = INVALID_INDEX;
    CHITAGSOPS  vendorTagOps;
    UINT32      vendorTagMetaOwner;

    ExtensionModule::GetInstance()->GetVendorTagOps(&vendorTagOps);
    vendorTagOps.pQueryVendorTagLocation("com.qti.chi.metadataOwnerInfo",
                                         "MetadataOwner", &vendorTagMetaOwner);

    pMetaOwner = static_cast<INT32*>(pResultMetadata->GetTag(vendorTagMetaOwner));

    if (NULL != pMetaOwner)
    {
        metaOwner = *pMetaOwner;
    }

    return metaOwner;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsMasterCamera
///
/// @brief  Obtain the information about the current camera, to decide if is master or non-master sensor metadata
///
/// @param  pResultMetadata     Result metadata
/// @param  pMCCResult    MCC result from multi camera controller
///
/// @return True if current camera is master
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsMasterCamera(
    ChiMetadata*             pResultMetadata,
    const LogicalCameraInfo* pCameraInfo)
{
    BOOL        isMasterCamera  = TRUE;
    INT32*      pIsMaster       = NULL;
    CHITAGSOPS  vendorTagOps;
    UINT32      vendorTagMasterInfo;

    if (1 < pCameraInfo->numPhysicalCameras)
    {
        ExtensionModule::GetInstance()->GetVendorTagOps(&vendorTagOps);
        vendorTagOps.pQueryVendorTagLocation("com.qti.chi.multicamerainfo",
                                             "MasterCamera", &vendorTagMasterInfo);

        pIsMaster = static_cast<INT32*>(pResultMetadata->GetTag(vendorTagMasterInfo));
        if (NULL != pIsMaster)
        {
            isMasterCamera = *pIsMaster;
        }
        else
        {
            // Copy if multicamera information not available
            isMasterCamera = TRUE;
        }
    }
    else
    {
        // Always master for single camera usecase
        isMasterCamera = TRUE;
    }

    return isMasterCamera;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetCameraIndex
///
/// @brief  Helper Function to get Camera Index from camera ID
///
/// @param  cameraID      Camera ID
/// @param  pMCCResult    MCC result from multi camera controller
///
/// @return Camera Index
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE UINT32 GetCameraIndex(
    UINT32                   cameraID,
    const LogicalCameraInfo* pCameraInfo)
{
    UINT32 camIndex = INVALID_INDEX;
    for (UINT32 i = 0; i < pCameraInfo->numPhysicalCameras; i++)
    {
        if (pCameraInfo->ppDeviceInfo[i]->cameraId == cameraID)
        {
            camIndex = i;
            break;
        }
    }
    return camIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetMetadataBuffer
///
/// @brief  Get pointer to metadata buffer from target buffer info handle
///
/// @param  handle              Target buffer info handle containing the associated metadata buffer
/// @param  key                 Unique key identifying the metadata buffer
/// @param  phMetadataBuffer    Pointer to handle of metadata buffer
///
/// @return CDKResultSuccess if successful; error code otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE CDKResult GetMetadataBuffer(
    CHITARGETBUFFERINFOHANDLE   handle,
    UINT64                      key,
    CHIMETAHANDLE*              phMetadataBuffer)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == phMetadataBuffer)
    {
        CHX_LOG_ERROR("phMetaDataBuffer is NULL");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        CHITargetBufferManager* pTargetBufferManager = CHITargetBufferManager::GetTargetBufferManager(handle);
        if (NULL == pTargetBufferManager)
        {
            CHX_LOG_ERROR("Invalid pTargetBufferManager for handle=%pK", handle);
            result = CDKResultEInvalidPointer;
        }
        else
        {
            ChiMetadata* pChiMetadata = reinterpret_cast<ChiMetadata*>(pTargetBufferManager->GetTarget(handle, key));
            if (NULL == pChiMetadata)
            {
                CHX_LOG_ERROR("Unable to get buffer info for handle %pK", handle);
                result = CDKResultEInvalidPointer;
            }
            else
            {
                *phMetadataBuffer = pChiMetadata->GetHandle();
                if (NULL == (*phMetadataBuffer))
                {
                    CHX_LOG_ERROR("GetHandle() returned NULL");
                    result = CDKResultEInvalidPointer;
                }
            }
        }
    }

    return result;
}

/// @brief feature2 lib vendor type
enum class ChiFeature2VendorType
{
    Default = 0,  ///< Default vendor type, QTI
    OEM     = 1,  ///< Vendor type OEM.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetFeature2LibPath
///
/// @brief  Get the feature lib path
///
/// @param  vendor   QTI or oem
///
/// @return the full path of folders of feautre2 libs, NULL if in error.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE const CHAR* GetFeature2LibPath(
    ChiFeature2VendorType type)
{
#if defined(_LP64)
    const CHAR* pFeture2LibPath[]  =
    {
        // "/vendor/lib64/camera/components/feature2/qti",    // QTI default path
        "/vendor/lib64",                                   // QTI default path
        "/vendor/lib64/camera/components/feature2/oem",    // oem path
    };
#else // _LP64
    const CHAR* pFeture2LibPath[]  =
    {
        // "/vendor/lib/camera/components/feature2/qti",    // QTI default path
        "/vendor/lib",                                   // QTI default path
        "/vendor/lib/camera/components/feature2/oem",    // oem path
    };
#endif // _LP64

    const CHAR* pPath = NULL;

    if (ChiFeature2VendorType::Default == type)
    {
        pPath = pFeture2LibPath[0];
    }
    else if (ChiFeature2VendorType::OEM == type)
    {
        pPath = pFeture2LibPath[1];
    }

    return pPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ConfigureRegistrationSize
///
/// @brief  Configure the output size of registration stream
///
/// @param  ppTargetStream       pointer to target stream selected
/// @param  pResultStream        pointer to result stream. Can be NULL.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE VOID ConfigureRegistrationSize(
    CHISTREAM**     ppTargetStream,
    CHISTREAM*      pResultStream)
{
    UINT32 MFSRInputWidth  = (*ppTargetStream)->width  >> MFSRDownscaleRatioShift;
    UINT32 MFSRInputHeight = (*ppTargetStream)->height >> MFSRDownscaleRatioShift;
    UINT32 resolutionIndex = 0;

    for ( ; resolutionIndex < MaxIndexRegInputResolutionMap; ++resolutionIndex)
    {
        if ((MFSRInputWidth  >= RegInputResolutionMap[resolutionIndex].width)  &&
            (MFSRInputHeight >= RegInputResolutionMap[resolutionIndex].height))
        {
            break;
        }
    }

    if (resolutionIndex >= MaxIndexRegInputResolutionMap)
    {
        // If we cannot match any resolution in the map, then this index will be equal to MaxIndexRegInputResolutionMap.
        resolutionIndex = MaxIndexRegInputResolutionMap - 1;

        CHX_LOG_WARN("Cannot match any registration size, so use the smallest one: %u x %u",
                         pResultStream->width,
                         pResultStream->height);
    }

    pResultStream->width  = RegInputResolutionMap[resolutionIndex].width;
    pResultStream->height = RegInputResolutionMap[resolutionIndex].height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetTargetStream
///
/// @brief  Get Target Stream based on name and table entries
///
/// @param  pCameraInfo          Logical camera info
/// @param  pTargetName          Name to match
/// @param  ppSourceStreamsList  source stream
/// @param  numSourceStreams     number of SourceStreams
/// @param  ppTargetStream       pointer to target stream selected
/// @param  pResultStream        pointer to result stream. Can be NULL.
///                              Target properties as modified as per table update
///
/// @return result
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE CDKResult GetTargetStream(
    const LogicalCameraInfo* pCameraInfo,
    const CHAR*              pTargetName,
    CHISTREAM**              ppSourceStreamsList,
    UINT                     numSourceStreams,
    CHISTREAM**              ppTargetStream,
    CHISTREAM*               pResultStream)
{
    CDKResult   result           = CDKResultSuccess;
    UINT32      mapIndex         = 0;
    CHISTREAM*  pSecondaryStream = NULL;

    if ((NULL != pCameraInfo) &&
        (NULL != pTargetName) &&
        (NULL != ppSourceStreamsList) &&
        (NULL != ppTargetStream) &&
        (NULL != pResultStream))
    {
        for (mapIndex = 0; mapIndex < sizeof(TargetStreamMap)/sizeof(TargetStreamMap[0]); mapIndex++)
        {
            for (UINT32 targetIdx = 0; targetIdx < TargetStreamMap[mapIndex].pTargetName.size(); targetIdx++)
            {
                if (0 == CdkUtils::StrCmp(pTargetName, TargetStreamMap[mapIndex].pTargetName[targetIdx]))
                {
                    for (UINT32 streamIndex = 0; streamIndex < numSourceStreams; streamIndex++)
                    {
                        const ChiFeature2TargetStreamProperties& rTargetStream = TargetStreamMap[mapIndex].targetStream;
                        CHISTREAM* const                         pSourceStream = ppSourceStreamsList[streamIndex];

                        if (rTargetStream.direction == pSourceStream->streamType)
                        {
                            for (const CHISTREAMFORMAT targetFormat : TargetStreamMap[mapIndex].targetStream.format)
                            {
                                if (ppSourceStreamsList[streamIndex]->format == targetFormat)
                                {
                                    BOOL hasDataSpaceMatched = (DataspaceHEIF == pSourceStream->dataspace) &&
                                        (rTargetStream.dataspace == pSourceStream->dataspace);
                                    BOOL hasGrallocUsageMatched = (0 == rTargetStream.usage) ||
                                        (0 != (rTargetStream.usage & pSourceStream->grallocUsage));
                                    BOOL skipDataspaceMatching = (DataspaceHEIF != pSourceStream->dataspace);

                                    if ((TRUE == hasDataSpaceMatched) ||
                                        ((TRUE == skipDataspaceMatching) && (TRUE == hasGrallocUsageMatched)))
                                    {
                                        *ppTargetStream = ppSourceStreamsList[streamIndex];
                                    }
                                    else if (NULL == pSecondaryStream)
                                    {
                                        pSecondaryStream = ppSourceStreamsList[streamIndex];
                                    }

                                    if (InvalidCameraId != TargetStreamMap[mapIndex].targetStream.physicalCameraIndex)
                                    {
                                        UINT32 streamCameraId = ChxUtils::GetCameraIdFromStream(*ppTargetStream);
                                        UINT32 mapCameraIdx   = TargetStreamMap[mapIndex].targetStream.physicalCameraIndex;

                                        if (pCameraInfo->numPhysicalCameras > mapCameraIdx)
                                        {
                                            UINT32 physicalCameraId =
                                                pCameraInfo->ppDeviceInfo[mapCameraIdx]->cameraId;

                                            if ((InvalidCameraId == streamCameraId) ||
                                                (streamCameraId != physicalCameraId))
                                            {
                                                *ppTargetStream = NULL;
                                            }
                                        }
                                    }

                                    if (NULL != *ppTargetStream)
                                    {
                                        CHX_LOG_INFO("Target Stream: %s : Resolution: %d X %d Format = %d usage %x",
                                            pTargetName, (*ppTargetStream)->width, (*ppTargetStream)->height,
                                            (*ppTargetStream)->format,
                                            (*ppTargetStream)->grallocUsage);
                                        break;
                                    }
                                }
                            }
                        }

                        if (NULL != *ppTargetStream)
                        {
                            break;
                        }
                    }
                }

                if (NULL != *ppTargetStream)
                {
                    break;
                }
            }

            if (NULL != *ppTargetStream)
            {
                break;
            }
        }

        if (NULL == *ppTargetStream)
        {
            if (NULL != pSecondaryStream)
            {
                *ppTargetStream = pSecondaryStream;
                CHX_LOG_INFO("Target Stream: %s : Resolution: %d X %d Format = %d usage %x secondary",
                    pTargetName, (*ppTargetStream)->width, (*ppTargetStream)->height,
                    (*ppTargetStream)->format,
                    (*ppTargetStream)->grallocUsage);
            }
            else
            {
                result = CDKResultEUnsupported;
            }
        }

        if (CDKResultSuccess == result)
        {
            *pResultStream = **ppTargetStream;

            /// initialize stream values
            FLOAT         widthScale  = TargetStreamMap[mapIndex].targetStreamValues.widthQuotient;
            FLOAT         heightScale = TargetStreamMap[mapIndex].targetStreamValues.heightQuotient;
            UINT64        format      = TargetStreamMap[mapIndex].targetStreamValues.format;
            INT32         type        = TargetStreamMap[mapIndex].targetStreamValues.direction;

            if ((DefaultREGStreamSizeQuotient == widthScale) &&
                (DefaultREGStreamSizeQuotient == heightScale))
            {
                //  Configure the registration stream output size
                ConfigureRegistrationSize(ppTargetStream, pResultStream);
            }
            else
            {
                if (DefaultStreamSizeQuotient != widthScale)
                {
                    pResultStream->width  = ChxUtils::EvenCeilingUINT32(
                            ChxUtils::AlignGeneric32(pResultStream->width, widthScale) / widthScale);
                }

                if (DefaultStreamSizeQuotient != heightScale)
                {
                    pResultStream->height = ChxUtils::EvenCeilingUINT32(
                            ChxUtils::AlignGeneric32(pResultStream->height, heightScale) / heightScale);
                }
            }

            if (DefaultStreamFormat != format)
            {
                pResultStream->format     = static_cast<CHISTREAMFORMAT>(format);
            }

            if (DefaultStreamDirection != type)
            {
                pResultStream->streamType = static_cast<CHISTREAMTYPE>(type);
            }

            if (CDKResultSuccess == result)
            {
                CHX_LOG_INFO("Result Stream: Name = %s resolution = %dX%d Format = %d Direction = %d",
                    pTargetName, pResultStream->width, pResultStream->height,
                    pResultStream->format, pResultStream->streamType);
            }
        }
        else
        {
            CHX_LOG_ERROR("Could not match Port and Stream: %s", pTargetName);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetSensorOutputDimension
///
/// @brief  Get sensor output dimensions based on stream configurations
///
/// @param  physicalCameraId   Physical camera id
/// @param  pStreamConfig      Stream configuration
/// @param  instanceFlags      Feature2 instance flags, also needed in determine sensor mode and output dimensions
/// @param  pSensorWidth       Output sensor width
/// @param  pSensorHeight      Output sensor height
///
/// @return CDKResultSuccess if successful; error code otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE CDKResult GetSensorOutputDimension(
    UINT32                     physicalCameraId,
    const CHISTREAMCONFIGINFO* pStreamConfig,
    ChiFeature2InstanceFlags   instanceFlags,
    UINT32*                    pSensorWidth,
    UINT32*                    pSensorHeight)
{
    CDKResult   result = CDKResultSuccess;

    if ((NULL == pStreamConfig) ||
        (NULL == pSensorWidth)  ||
        (NULL == pSensorHeight))
    {
        CHX_LOG_ERROR("Invalid input parameters");
        result = CDKResultEInvalidPointer;
    }
    else
    {
        if ((TRUE == instanceFlags.isNZSLSnapshot) &&
            ((TRUE == instanceFlags.isSWRemosaicSnapshot) || (TRUE == instanceFlags.isHWRemosaicSnapshot)))
        {
            // for non-zsl remosaic snapshot of qcfa sensor, always get the max sensor output dimension,
            // whose mode caps is 'QuadCFA', instead of 'NORMAL'.
            UINT32             modeCount  = 0;
            ChiSensorModeInfo* pAllModes  = NULL;

            result = ExtensionModule::GetInstance()->GetPhysicalCameraSensorModes(physicalCameraId, &modeCount, &pAllModes);
            if (CDKResultSuccess == result)
            {
                for (UINT i = 0; i < modeCount; i++)
                {
                    CHIRECT* pSensorDimension = &(pAllModes[i].frameDimension);
                    if ((pSensorDimension->width  > *pSensorWidth) ||
                        (pSensorDimension->height > *pSensorHeight))
                    {
                        *pSensorWidth  = pSensorDimension->width;
                        *pSensorHeight = pSensorDimension->height;
                    }
                }
            }
        }
        else
        {
            // Determine if we have multiple aspect ratios in the request
            // If so, request full pixel array from previous blocks to avoid
            // double cropping an output stream.
            UINT32 adjustAspectRatio  = FALSE;
            FLOAT  prevAspectRatio    = 0.0f;
            for (UINT i = 0; i < pStreamConfig->numStreams; i++)
            {
                if ((NULL != pStreamConfig->pChiStreams[i]) &&
                    (0 != pStreamConfig->pChiStreams[i]->width) &&
                    (0 != pStreamConfig->pChiStreams[i]->height))
                {
                    FLOAT currentAspectRatio =
                        static_cast<FLOAT>((static_cast<FLOAT>(pStreamConfig->pChiStreams[i]->width) /
                                            static_cast<FLOAT>(pStreamConfig->pChiStreams[i]->height)));

                    CHX_LOG_INFO("currentAspectRatio %.3f phy camId %d, wxd: %dx%d",
                                 currentAspectRatio, physicalCameraId, pStreamConfig->pChiStreams[i]->width,
                                 pStreamConfig->pChiStreams[i]->height);

                    if ((0 != prevAspectRatio) &&
                        (FALSE == ChxUtils::FEqualCoarse(currentAspectRatio, prevAspectRatio)))
                    {
                        adjustAspectRatio = TRUE;
                        break;
                    }
                    prevAspectRatio = currentAspectRatio;
                }
                else
                {
                    result = CDKResultEInvalidArg;
                    CHX_LOG_ERROR("Invalid input stream configuration!");
                    break;
                }
            }

            if (CDKResultSuccess == result)
            {
                UsecaseSelector::getSensorDimension(physicalCameraId,
                                                    reinterpret_cast<const camera3_stream_configuration_t*>(pStreamConfig),
                                                    pSensorWidth,
                                                    pSensorHeight,
                                                    1,
                                                    adjustAspectRatio);
            }
        }

        CHX_LOG_INFO("Sensor output size: %dx%d", *pSensorWidth, *pSensorHeight);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DumpMetadata
///
/// @brief  Dump metadata
///
/// @param  pMetadata       Metadata to dump
/// @param  pName           Dump file name
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE VOID DumpMetadata(
    ChiMetadata* pMetadata,
    CHAR*        pName)
{
    pMetadata->BinaryDump(pName);
}

typedef size_t(*PFNCAMXMEMGETTOTALPLANESIZE)(uint32_t width, uint32_t height, uint32_t format, uint32_t usage);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DumpRawImage
///
/// @brief  Dump raw image
///
/// @param  pStreamBuffer   Stream buffer to dump.
/// @param  pName           Dump file name.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE VOID DumpRawImage(
    CHISTREAMBUFFER* pStreamBuffer,
    CHAR*            pName)
{
    CDKResult       result       = CDKResultSuccess;
    INT             fileDesc     = CHIBufferManager::GetFileDescriptor(&pStreamBuffer->bufferInfo);
    size_t          bufferLength = 0;

    OSLIBRARYHANDLE             hLibrary                  = ExtensionModule::GetInstance()->GetLibrary();
    PFNCAMXMEMGETTOTALPLANESIZE pCamXMemGetTotalPlaneSize =
        reinterpret_cast<PFNCAMXMEMGETTOTALPLANESIZE>(ChxUtils::LibGetAddr(hLibrary, "CamxMemGetTotalPlaneSize"));

    if (NULL != pCamXMemGetTotalPlaneSize)
    {
        bufferLength = (*pCamXMemGetTotalPlaneSize)(static_cast<uint32_t>(pStreamBuffer->pStream->width),
                                                    static_cast<uint32_t>(pStreamBuffer->pStream->height),
                                                    static_cast<uint32_t>(pStreamBuffer->pStream->format),
                                                    static_cast<uint32_t>(pStreamBuffer->pStream->grallocUsage));
    }
    else
    {
        CHX_LOG_ERROR("Fail to load CamXMemGetTotalPlaneSize.");
        result = CDKResultEFailed;
    }

    if ((0 != fileDesc) && (CDKResultSuccess == result) && (0 != bufferLength))
    {
        VOID* pHostptr = NULL;
        pHostptr = ChxUtils::MemMap(fileDesc, bufferLength, 0);
        if (0 != pHostptr)
        {
            FILE* pFile = CdkUtils::FOpen(pName, "wb");
            if (NULL != pFile)
            {
                CdkUtils::FWrite(pHostptr, bufferLength, 1, pFile);
                CdkUtils::FClose(pFile);
            }
            ChxUtils::MemUnmap(pHostptr, bufferLength);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DumpTuningMode
///
/// @brief  Dump tuning mode to file
///
/// @param  pMetadata            Metadata to use
/// @param  pTuningModeDataName  Dump file name.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE VOID DumpTuningMode(
    ChiMetadata* pMetadata,
    CHAR*        pTuningModeDataName)
{
    CDKResult               result      = CDKResultSuccess;
    ChiTuningModeParameter* pTuningMode = NULL;

    if ((NULL == pMetadata) || (NULL == pTuningModeDataName))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pTuningMode = static_cast<ChiTuningModeParameter*>(pMetadata->GetTag("org.quic.camera2.tuning.mode", "TuningMode"));
    }

    if (NULL != pTuningMode)
    {
        std::ostringstream dArray;
        for (UINT i = 0; i < pTuningMode->noOfSelectionParameter; i++)
        {
            if (0 != i)
            {
                dArray << ".";
            }
            dArray << pTuningMode->TuningMode[i].subMode.value;
        }

        FILE* pFile = CdkUtils::FOpen(pTuningModeDataName, "w");
        if (NULL != pFile)
        {
            CdkUtils::FWrite(dArray.str().c_str(), 1, dArray.tellp(), pFile);
            CdkUtils::FClose(pFile);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DumpStats
///
/// @brief  Dump tuning mode to file
///
/// @param  pMetadata   Metadata to use
/// @param  index       Index of dump file
/// @param  pBaseName   Base name to be used for determining the output dump file name
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE VOID DumpStats(
    ChiMetadata* pMetadata,
    UINT         index,
    CHAR*        pBaseName)
{
    auto DumpStatsToFile = [](const CHAR* pFileName,
                              VOID*       pStats,
                              UINT        size,
                              UINT        tagID)->VOID
    {
        CHAR   dumpFilename[256];

        CdkUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "/data/vendor/camera/%s", pFileName);

        FILE* pFile = CdkUtils::FOpen(dumpFilename, "wb");
        if (NULL != pFile)
        {
            SIZE_T len   = 0;
            UINT   tagId = tagID;

            len += CdkUtils::FWrite(&tagId, 1, sizeof(tagId), pFile);
            len += CdkUtils::FWrite(pStats, 1, size, pFile);

            CHX_LOG_INFO("Dump stats %zu bytes to %s", len, dumpFilename);
            CdkUtils::FClose(pFile);
        }
        else
        {
            CHX_LOG_ERROR("Cannot dump Stats file name %s", dumpFilename);
        }
    };

    CDKResult result = CDKResultSuccess;

    if (NULL == pMetadata)
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        static const UINT            PropertyIDParsedTintlessBGStatsOutput = 0x38000032;
        ParsedTintlessBGStatsOutput* pParsedTintlessBGStats                =
            static_cast<ParsedTintlessBGStatsOutput*>(pMetadata->GetTag(PropertyIDParsedTintlessBGStatsOutput));

        if (NULL != pParsedTintlessBGStats)
        {
            CHAR filename[256] = { 0 };
            CdkUtils::SNPrintF(filename,
                               sizeof(filename),
                               "%s_%d.tint",
                               pBaseName,
                               index);
            DumpStatsToFile(filename,
                            pParsedTintlessBGStats,
                            sizeof(ParsedTintlessBGStatsOutput),
                            PropertyIDParsedTintlessBGStatsOutput);
        }
        else
        {
            CHX_LOG_ERROR("ParsedTintlessBGStats not available !");
            result = CDKResultENoSuch;
        }
    }

    if (CDKResultSuccess == result)
    {
        static const UINT       PropertyIDParsedAWBBGStatsOutput = 0x3800002A;
        ParsedAWBBGStatsOutput* pParsedAWBBGStatsOutput          =
            static_cast<ParsedAWBBGStatsOutput*>(pMetadata->GetTag(PropertyIDParsedAWBBGStatsOutput));

        if (NULL != pParsedAWBBGStatsOutput)
        {
            CHAR filename[256] = { 0 };
            CdkUtils::SNPrintF(filename,
                               sizeof(filename),
                               "%s_%d.awbbg",
                               pBaseName,
                               index);
            DumpStatsToFile(filename,
                            pParsedAWBBGStatsOutput,
                            sizeof(ParsedAWBBGStatsOutput),
                            PropertyIDParsedAWBBGStatsOutput);
        }
        else
        {
            CHX_LOG_ERROR("ParsedAWBBGStatsOutput not available !");
            result = CDKResultENoSuch;
        }
    }

    if (CDKResultSuccess == result)
    {
        static const UINT       PropertyIDParsedBHistStatsOutput = 0x3800002C;
        ParsedBHistStatsOutput* pParsedBHistStats                =
            static_cast<ParsedBHistStatsOutput*>(pMetadata->GetTag(PropertyIDParsedBHistStatsOutput));

        if (NULL != pParsedBHistStats)
        {
            CHAR filename[256] = { 0 };
            CdkUtils::SNPrintF(filename,
                               sizeof(filename),
                               "%s_%d.bhist",
                               pBaseName,
                               index);
            DumpStatsToFile(filename,
                            pParsedBHistStats,
                            sizeof(ParsedBHistStatsOutput),
                            PropertyIDParsedBHistStatsOutput);
        }
        else
        {
            CHX_LOG_ERROR("ParsedBHistStatsOutput not available !");
            result = CDKResultENoSuch;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsValidRect
///
/// @brief  Check if rect is valid
///
/// @param  CHIRectangle    rectangle to validate
///
/// @return True if valid, false otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHX_INLINE static BOOL IsValidRect(
        CHIRectangle rect)
{
    return ((rect.top == 0) && (rect.left == 0) && (rect.width == 0) && (rect.height == 0)) ? FALSE : TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CheckBufferSkippedForPort
///
/// @brief  Check if buffer is skipped for port
///
/// @param  pRequestObject   Feature request object
/// @param  Chi       Index of dump file
/// @param  pBaseName   Base name to be used for determining the output dump file name
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL CheckBufferSkippedForPort(
    ChiFeature2RequestObject*       pRequestObject,
    const ChiFeature2Identifier*    pPortId,
    UINT                            requestId)
{
    CDKResult result                               = CDKResultSuccess;
    BOOL      skipped                              = FALSE;

    ChiFeature2BufferMetadataInfo* pBufferMetaInfo = NULL;

    if (NULL != pRequestObject && NULL != pPortId)
    {
        result = pRequestObject->GetFinalBufferMetadataInfo(*pPortId, &pBufferMetaInfo, requestId);

        if ((CDKResultSuccess == result) && (NULL != pBufferMetaInfo))
        {
            if (pBufferMetaInfo->bufferSkipped == TRUE)
            {
                CHX_LOG_INFO("Buffer marked as skipped for port %d %d %d", pPortId->session, pPortId->pipeline, pPortId->port);
                skipped = TRUE;
            }
        }
    }
    return skipped;
}
#endif // CHIFEATURE2UTILS_H
