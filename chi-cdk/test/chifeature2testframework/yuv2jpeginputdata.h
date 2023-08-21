////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef YUVTOJPEGINPUTDATA_H
#define YUVTOJPEGINPUTDATA_H

#include "chi.h"
#include "chifeature2types.h"

namespace tests // Namespace to prevent redefinition errors
{
    static const char* IPE_COLORBAR_VGA = "JPEG_image_4656x3496_0.yuv";

    static CHISTREAMPARAMS JPEGInputStreamParams
    {
        4672, // 4656 aligned to 64
        3520  // 3496 aligned to 32
    };

    static CHISTREAM YUV2JPEGStreamsInput
    {
        ChiStreamTypeInput,
        4656,
        3496,
        ChiStreamFormatYCbCr420_888,
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
        JPEGInputStreamParams,
        { NULL, NULL, NULL },
    };

    static CHISTREAMPARAMS JPEGOutputStreamParams
    {
        0,
        0
    };

    static CHISTREAM YUV2JPEGStreamsOutput
    {
        ChiStreamTypeOutput,
        4160,
        3120,
        ChiStreamFormatBlob,
        0,
        8,
        NULL,
        DataspaceUnknown,
        StreamRotationCCW0,
        NULL,
    #if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
        NULL,
    #endif
        JPEGOutputStreamParams,
        { NULL, NULL, NULL },
    };

    static CHISTREAM* pJPEGStreams[] =
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
}

extern const ChiFeature2Descriptor JPEGFeatureDescriptor;

#endif // YUVTOJPEGINPUTDATA_H
