////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MFXRINPUTDATA_H__
#define __MFXRINPUTDATA_H__

#include "chi.h"
#include "chifeature2types.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files
namespace tests // Namespace to prevent redefinition errors
{
    static const char* MFXR_IDEALRAW_UHD = "MFXR_image_4656x3496.raw";

    /* Input data */
    static CHISTREAMPARAMS InputStreamParams
    {
        5824,  // 4656 * 10 / 8 = 5820, align to 16
        3496
    };

    static CHISTREAM MfxrStreamsInput
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

    /* Output data */
    static CHISTREAMPARAMS OutputStreamParams
    {
        4672, // 4656 aligned to 64
        3520  // 3496 aligned to 32
    };

    static CHISTREAM MfxrStreamsOutput
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

    static CHISTREAM* pMfxrStreams[] =
    {
        &MfxrStreamsInput,
        &MfxrStreamsOutput
    };

    static CHISTREAMCONFIGINFO MfxrStreamConfigInfo
    {
        2,
        pMfxrStreams,
        0,
        NULL
    };
}

extern const ChiFeature2Descriptor MFXRFeatureDescriptor;
#endif // __MFXRINPUTDATA_H__
