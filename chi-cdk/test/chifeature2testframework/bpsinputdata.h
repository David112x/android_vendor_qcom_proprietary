////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BPSINPUTDATA_H
#define BPSINPUTDATA_H

#include "chi.h"
#include "chifeature2types.h"

namespace tests // Namespace to prevent redefinition errors
{
    static const char* BPS_TEST_IMAGE = "Bayer2Yuv_image_4656x3496_0.raw";

    static CHISTREAMPARAMS BPSInputStreamParams
    {
        5824, // 4656 * 10 / 8 = 5760, aligned to 16
        3496
    };

    static CHISTREAM BPSStreamsInput
    {
        ChiStreamTypeInput,
        4656,
        3496,
        ChiStreamFormatRaw10,   // Raw10 not allocating enough buffer space for the image (needs 2 bytes per pixel). Image might have 6 bytes of padding.
        GRALLOC1_PRODUCER_USAGE_CAMERA |
        GRALLOC1_PRODUCER_USAGE_CPU_READ |
        GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
        8,
        NULL,
        DataspaceUnknown,
        StreamRotationCCW0,
        NULL,
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
        NULL,
#endif
        BPSInputStreamParams,
        { NULL, NULL, NULL },
    };

    static CHISTREAMPARAMS BPSOutputStreamParams
    {
        9472, // 4656 * 2 = 9312, aligned to 256
        3520  // 3496, aligned to 32
    };

    static CHISTREAMPARAMS BPSDS4OutputStreamParams
    {
        4864, // 1164 * 4 = 4656, aligned to 192, aligned to 256
        448   //  874 / 2 =  437, aligned to 32
    };

    static CHISTREAMPARAMS BPSDS16OutputStreamParams
    {
        1536, // 292 * 4 = 1168, aligned to 192 = 1344, aligned to 256
        128   // 220 / 2 =  110, aligned to 32
    };

    static CHISTREAMPARAMS BPSDS64OutputStreamParams
    {
        512, // 72 * 4 = 288, aligned to 192 = 384, aligned to 256
        32   // 56 / 2 =  28, aligned to 32
    };

    static CHISTREAM BPSStreamsOutput
    {
        ChiStreamTypeOutput,
        4656,
        3496,
        ChiStreamFormatP010,
        0,
        8,
        NULL,
        DataspaceUnknown,
        StreamRotationCCW0,
        NULL,
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
        NULL,
#endif
        BPSOutputStreamParams,
        { NULL, NULL, NULL },
    };

    static CHISTREAM BPSStreamsOutputDS4
    {
        ChiStreamTypeOutput,
        1164,
        874,
        ChiStreamFormatPD10,
        0,
        8,
        NULL,
        DataspaceUnknown,
        StreamRotationCCW0,
        NULL,
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
        NULL,
#endif
        BPSDS4OutputStreamParams,
        { NULL, NULL, NULL },
    };

    static CHISTREAM BPSStreamsOutputDS16
    {
        ChiStreamTypeOutput,
        292,
        220,
        ChiStreamFormatPD10,
        0,
        8,
        NULL,
        DataspaceUnknown,
        StreamRotationCCW0,
        NULL,
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
        NULL,
#endif
        BPSDS16OutputStreamParams,
        { NULL, NULL, NULL },
    };

    static CHISTREAM BPSStreamsOutputDS64
    {
        ChiStreamTypeOutput,
        72,
        56,
        ChiStreamFormatPD10,
        0,
        8,
        NULL,
        DataspaceUnknown,
        StreamRotationCCW0,
        NULL,
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
        NULL,
#endif
        BPSDS64OutputStreamParams,
        { NULL, NULL, NULL },
    };

    static CHISTREAM* pBPSStreams[] =
    {
        &BPSStreamsInput,
        &BPSStreamsOutput,
        &BPSStreamsOutputDS4,
        &BPSStreamsOutputDS16,
        &BPSStreamsOutputDS64
    };

    static CHISTREAMCONFIGINFO BPSStreamConfigInfo
    {
        5,
        pBPSStreams,
        0,
        NULL
    };
}

extern const ChiFeature2Descriptor BPSFeatureDescriptor;

#endif // BPSINPUTDATA_H
