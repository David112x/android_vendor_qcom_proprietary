////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BAYER2YUVINPUTDATA_H
#define BAYER2YUVINPUTDATA_H

#include "chi.h"
#include "chifeature2types.h"

namespace tests // Namespace to prevent redefinition errors
{
    static const char* BPS_IDEALRAW_UHD = "Bayer2Yuv_image_4656x3496_0.raw";

    static CHISTREAMPARAMS InputStreamParams
    {
        5824, // 4656 * 10 / 8 = 5820, aligned to 16
        3496
    };

    static CHISTREAM Bayer2YuvStreamsInput
    {
        ChiStreamTypeInput,
        4656,
        3496,
        ChiStreamFormatRaw10,
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
        InputStreamParams,
        { NULL, NULL, NULL },
    };

    static CHISTREAMPARAMS OutputStreamParams
    {
        4672, // 4656 aligned to 64
        3520  // 3496 aligned to 32
    };

    static CHISTREAM Bayer2YuvStreamsOutput
    {
        ChiStreamTypeOutput,
        4656,
        3496,
        ChiStreamFormatYCbCr420_888,
        0,
        8,
        NULL,
        DataspaceUnknown,
        StreamRotationCCW0,
        NULL,
    #if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
        NULL,
    #endif
        OutputStreamParams,
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
}

extern const ChiFeature2Descriptor Bayer2YuvFeatureDescriptor;

#endif // BAYER2YUVINPUTDATA_H
