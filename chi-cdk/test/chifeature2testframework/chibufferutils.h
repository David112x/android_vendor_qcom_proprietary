////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chibufferutils.h
/// @brief Declarations for buffer utilities.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIBUFFERUTILS_H
#define CHIBUFFERUTILS_H

//#include "camera3stream.h"
#include "chi.h"
#include "chxutils.h"
#include "chxusecaseutils.h"
#include "cdkutils.h"
#include "feature2buffermanager.h"

//#include <gralloc_priv.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

//static const CHAR inputImagePath[] = "/data/vendor/camera/";
//static const CHAR outputImagePath[] = "/data/vendor/camera/chifeature2test/";
static CHIBufferManager*        m_pBufferManager;

//typedef camera3_stream_t        NativeHalStream;
//typedef camera3_stream_buffer_t NativeHalBuffer;
//typedef CHISTREAM               NativeChiStream;
//typedef CHISTREAMBUFFER         NativeChiBuffer;
//typedef CHIBUFFERHANDLE         NativeChiBufferHandle;
//
//struct GenericStream
//{
//    union
//    {
//        NativeHalStream* pHalStream;
//        NativeChiStream* pChiStream;
//    };
//    GenericStream() {};
//    GenericStream(NativeHalStream* stream) : pHalStream(stream) {};
//    GenericStream(NativeChiStream* stream) : pChiStream(stream) {};
//};
//
//struct GenericBuffer
//{
//    union
//    {
//        NativeHalBuffer* pHalBuffer;
//        NativeChiBuffer* pChiBuffer;
//    };
//    GenericBuffer() {};
//    GenericBuffer(NativeHalBuffer* buffer) : pHalBuffer(buffer) {};
//    GenericBuffer(NativeChiBuffer* buffer) : pChiBuffer(buffer) {};
//};


/* DEPRECATED FOR FEATURE2BUFFERMANAGER */
class ChiFeature2BufferUtils
{
public:

//    static GenerateOutputBuffer(
//        GenericBuffer* buffer,
//        GenericStream* stream);

    static CDKResult GenerateInputBuffer(
        GenericBuffer* buffer,
        GenericStream* stream,
        const char* filename);

    static CHIFENCEINFO CreateFence(bool isValid);

    static GenericBuffer* CreateGenericBuffer();
    static void DestroyGenericBuffer(GenericBuffer* buffer);
    static void DestroyFence(CHIFENCEINFO fenceInfo);
    static void Destroy();

private:

    static void DestroyBufferManager();
    static void CreateBufferManager(GenericStream* inputStream);
};

#endif // CHIBUFFERUTILS_H
