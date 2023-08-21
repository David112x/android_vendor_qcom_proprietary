////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  genericbuffermanager.h
/// @brief Declarations for generic buffer manager.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GENERICBUFFERMANAGER_H
#define GENERICBUFFERMANAGER_H

#include <queue>
#include <string>
#include <sstream>
#include <map>
#include <regex>

#ifdef _LINUX
#include <sys/stat.h> // mkdir
#include <ui/GraphicBuffer.h>
#else
#include <direct.h> // _mkdir
#endif

#include "camxpresilmem.h"
#include "camera3.h"
#include "chxutils.h"
#include "cdkutils.h"
#include "chi.h"
#include "chifeature2test.h"
#include "spectraconfigparser.h"

const std::string inputImagePath  = "/data/vendor/camera/";
const std::string outputImagePath = "/data/vendor/camera/output/";

typedef camera3_stream_t        NativeHalStream;
typedef camera3_stream_buffer_t NativeHalBuffer;
typedef CHISTREAM               NativeChiStream;
typedef CHISTREAMBUFFER         NativeChiBuffer;
typedef CHIBUFFERHANDLE         NativeChiBufferHandle;

struct GenericStream
{
    union
    {
        NativeHalStream* pHalStream;
        NativeChiStream* pChiStream;
    };
    GenericStream() {};
    GenericStream(NativeHalStream* pStream) : pHalStream(pStream) {};
    GenericStream(NativeChiStream* pStream) : pChiStream(pStream) {};
};

struct GenericBuffer
{
    union
    {
        NativeHalBuffer* pHalBuffer;
        NativeChiBuffer* pChiBuffer;
    };
    GenericBuffer() {};
    GenericBuffer(NativeHalBuffer* pBuffer) : pHalBuffer(pBuffer) {};
    GenericBuffer(NativeChiBuffer* pBuffer) : pChiBuffer(pBuffer) {};
};

typedef enum ChiStreamPrivateFormat
{
    PRIVATE_PIXEL_FORMAT_RAW64      = 0x00000027,
    PRIVATE_PIXEL_FORMAT_NV12HEIF   = 0x00000116,
    PRIVATE_PIXEL_FORMAT_P010       = 0x7FA30C0A,
    PRIVATE_PIXEL_FORMAT_UBWCTP10   = 0x7FA30C09,
    PRIVATE_PIXEL_FORMAT_PD10       = 0x7FA30C08,
    PRIVATE_PIXEL_FORMAT_UBWCNV12   = 0x7FA30C06
} CHISTREAMPRIVATEFORMAT;

class GenericBufferManager
{

public:

    static int LoadBufferLibs(void* pLib);
    GenericBufferManager* Initialize(int cameraId, GenericStream* pConfiguredStream, int streamId,
        const char* testCaseName, const char* inputFileName, bool multiFrame = FALSE);
    void* GetOutputBufferForRequest();
    void* GetInputBufferForRequest();

    virtual int   ProcessBufferFromResult(const int frameNumber, GenericBuffer* pResultBuffer, bool dump);
    virtual void  DestroyBuffers();

    virtual int GetStatusFromBuffer(GenericBuffer* pBuffer) = 0;

protected:

#ifdef _LINUX
    std::map<native_handle_t*, android::sp<android::GraphicBuffer>> m_GBmap;
#endif

    static CamxMemAllocFunc              m_pCamxMemAlloc;
    static CamxMemReleaseFunc            m_pCamxMemRelease;
    static CamxMemGetImageSizeFunc       m_pCamxMemGetImageSize;
    static CamxMemGetImageSizeStrideFunc m_pCamxMemGetImageSizeStride;

    int GenerateBuffers(int cameraId, GenericStream* pConfiguredStream, int streamId, const char* testCaseName,
        const char* inputFileName, bool multiFrame = FALSE);

    // Static helpers
    static std::string GetFileExt(int format);
    static Size GetGrallocSize(uint32_t width, uint32_t height, int format, int dataspace, void* pNativeBuffer = NULL);
    static uint32_t GetExactJpegBufferSize(void* pNativeBuffer);

    GenericBufferManager();
    virtual ~GenericBufferManager();

private:

    std::deque<GenericBuffer*>*  m_pInputImageQueue;
    std::queue<GenericBuffer*>*  m_pEmptyQueue;
    std::queue<GenericBuffer*>*  m_pFilledQueue;

    Mutex*              m_pQueueMutex;
    Condition*          m_pQueueCond;
    bool                m_bBufferAvailable;
    std::string         m_outFileName;
    int                 m_streamType;

    const int TIMEOUT_RETRIES = 10;

    virtual int GenerateOutputBuffer(GenericBuffer* pBuffer, GenericStream* pStream) = 0;
    virtual int GenerateInputBuffer(GenericBuffer* pBuffer, GenericStream* pStream, const char* filename) = 0;
    virtual int SaveImageToFile(GenericBuffer* resultbuffer, std::string filename) = 0;

    std::string ConstructOutputFileName(int cameraId, const char* testCaseName);

    /* Buffer Allocation Helpers */
    virtual GenericBuffer* CreateGenericBuffer() = 0;
    virtual void DestroyGenericBuffer(GenericBuffer* pBuffer) = 0;

    /* Stream Getters */
    virtual int GetTypeFromStream(GenericStream* pStream) = 0;
    virtual uint32_t GetMaxBuffersFromStream(GenericStream* pStream) = 0;

    /* Buffer Getters */
    virtual void* GetNativeBufferFromGeneric(GenericBuffer* pBuffer) = 0;
    virtual NativeChiBufferHandle GetHandleFromBuffer(GenericBuffer* pBuffer) = 0;
    virtual int GetReleaseFenceFromBuffer(GenericBuffer* pBuffer) = 0;
    virtual void SetAcquireFenceInBuffer(GenericBuffer* pBuffer, int setVal) = 0;
    virtual uint32_t GetMaxBuffersFromBuffer(GenericBuffer* pBuffer) = 0;

    /// Do not allow the copy constructor or assignment operator
    GenericBufferManager(const GenericBufferManager& )             = delete;
    GenericBufferManager& operator= (const GenericBufferManager& ) = delete;
};

#endif // GENERICBUFFERMANAGER_H
