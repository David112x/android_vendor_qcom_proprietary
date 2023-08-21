////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IPEINPUTDATA_H
#define IPEINPUTDATA_H

#include "chi.h"
#include "chifeature2types.h"

namespace tests // Namespace to prevent redefinition errors
{
    static const char* IPE_TEST_IMAGE = "IPE_image_4656x3496_0.p010";

    static CHISTREAMPARAMS IPEInputStreamParams
    {
        9472, // 4656 * 2 = 9312, aligned to 256
        3520, // 3496 aligned to 32
    };

    static CHISTREAM IPEStreamsInput
    {
        ChiStreamTypeInput,
        4656,
        3496,
        ChiStreamFormatP010,
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
        IPEInputStreamParams,
        { NULL, NULL, NULL },
    };

    static CHISTREAMPARAMS IPEOutputStreamParams
    {
        4672, // 4656 aligned to 64
        3520  // 3496 aligned to 32
    };

    static CHISTREAM IPEStreamsOutput
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
        IPEOutputStreamParams,
        { NULL, NULL, NULL },
    };

    static CHISTREAM* pIPEStreams[] =
    {
        &IPEStreamsInput,
        &IPEStreamsOutput,
    };

    static CHISTREAMCONFIGINFO IPEStreamConfigInfo
    {
        2,
        pIPEStreams,
        0,
        NULL
    };
}

extern const ChiFeature2Descriptor IPEFeatureDescriptor;

#endif // IPEINPUTDATA_H
