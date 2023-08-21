/*-------------------------------------------------------------------
Copyright (c) 2014-2020 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
--------------------------------------------------------------------*/

#ifndef _VIDEOSTREAMPARSERINTERFACE_H
#define _VIDEOSTREAMPARSERINTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <linux/videodev2.h>
#include <VideoComDef.h>
#include <list.h>
#include <media/msm_media_info.h>

#if 0
#define BOOL uint32
#define TRUE 1
#define FALSE 0
#endif

typedef struct File File;

typedef enum VidcDecHdrMode
{
    V4L2_DEC_S_ONLY = 1,
    V4L2_DEC_SF = 2
}VidcDecHdrMode;

typedef struct VideoSessionStaticConfig
{
    uint32  nFrameCnt;
    uint32  nSkipReadAfterNFrames;
    uint32  nSuperFrame;
    char*   sCodecType;
    char*   sBufferFormat;
}VideoSessionStaticConfig;
typedef struct VideoFrameRate
{
    uint8 nFrameNum;
    uint8 nFrameDen;
}VideoFrameRate;

/*
    This structure is used to supply information to the parser and parser uses to parse the frame.
    In return parser fills the necessory information. It also filles the buffer with the parsed data.
*/
typedef struct ParserFrameInfoType
{
   uint8 *pBuffer;        //Buffer pointer where data will be filled in
   uint32 nOffset;        //Buffer data read start offset
   u64 nFilledLen;        //The length of filled data in the buffer
   uint32 nAllocLen;      //Allocated buffer length
   u64 nTimeStamp;        //Timestamp in microsec
   uint32 nFlags;         //Any flag to pass on
   int nIonfd;            //Ion fd needed for secure mode operation
   uint32 nIonHeap;       //Heap id mask for this ion allocation
   BOOL   bLastFrame;     //is this last frame read ?
   uint32 nFrameCounter;
   BOOL   bSeqHdr;
}ParserFrameInfoType;

typedef struct ParseCfgEnum {
    const char* pEnumName;
    uint32 eEnumVal;
}ParseCfgEnum;

/* use the same string as exposed through xml for kernel and omx test-app */
static ParseCfgEnum pYUVFormatMap[] =
{
    { "NV12",   V4L2_PIX_FMT_NV12 },
    { "NV21",   V4L2_PIX_FMT_NV21 },
    { "UBWC_NV12_8bit",    V4L2_PIX_FMT_NV12_UBWC },
    { "UBWC_10bit",   V4L2_PIX_FMT_NV12_TP10_UBWC },
    { "RGBA8888",          V4L2_PIX_FMT_RGB32 },
    { "UBWC_RGBA8888",     V4L2_PIX_FMT_RGBA8888_UBWC },
    { "P010",   V4L2_PIX_FMT_SDE_Y_CBCR_H2V2_P010_VENUS },
    { "NV12_512",   V4L2_PIX_FMT_NV12 },
    { "NV12_LINEAR_FLEX", V4L2_PIX_FMT_NV12 },
    { "NV12_UBWC_FLEX", V4L2_PIX_FMT_NV12_UBWC },
    { 0, 0 }
};

typedef struct VideoStreamParser VideoStreamParser;
typedef struct VideoStreamWriter VideoStreamWriter;

typedef VidcStatus(*ReadBuffer)(VideoStreamParser* pParser, ParserFrameInfoType *pFrameInfo);
typedef BOOL(*IsNextFrameSlice)(VideoStreamParser* pParser, uint8 **pBuffer, uint32 *nCode, uint32 *nReadOffset);
typedef BOOL(*IsHeaderStartCode)(VideoStreamParser* pParser, uint8 *pBuffer, uint32 nFilledLen);
typedef struct UBWCInfo
{
    uint32 metaY_stride;
    uint32 metaY_height;
    uint32 metaC_stride;
    uint32 metaC_height;
    uint32 metaY_size;
    uint32 metaC_size;
    uint32 nStrideWidth_Y;
    uint32 nStrideHeight_Y;
    uint32 nStrideWidth_UV;
    uint32 nStrideHeight_UV;
    uint32 nAlignWidth;
    uint32 nAlignHeight;
    uint32 epbBuffSize;
    uint32 encMetaDataLumaSz;
    uint32 encMetaDataChromaSz;
    uint32 epbMetaDataOffset;
    uint32 epbBuffChromaOffset;
    uint32 ubwc_EncEn_EncData;
}UBWCInfo;

typedef unsigned long         UInt32;

//Common formats between IP cores
#define FORMAT_RGBA8888       0
#define FORMAT_NV12_NV21      1
#define FORMAT_P010           2
#define FORMAT_YUYV           3
#define FORMAT_Y210           4
#define FORMAT_YUVA           5
#define FORMAT_RGBA1010102    6
#define FORMAT_Y410           7
#define FORMAT_YUV420_TP10    8
#define FORMAT_YUV422_TP10    9
#define FORMAT_P016           10
#define FORMAT_Y216           11
#define FORMAT_RGB565         12

struct VideoStreamParser
{
   uint32   m_nFrames;              //Total Frame to parse.
   uint32   m_nFrameWidth;          //Resolution Width
   uint32   m_nFrameHeight;         //Resolution Height
   uint32   m_nStrideWidth;          //Resolution Width
   uint32   m_nSliceHeight;         //Resolution Height
   uint32   m_nBufSize;             //Size of each buffer
   uint32   m_eCodecFormat;         //Codec type
   uint32   m_nStartCodeLen;
   uint32   m_nConcealColor;
   float    m_nTimeStamp;
   float    m_nTimeStampDelta;
   u64   m_nFileSize;  //TODO: based on what ftell returns
   uint32   m_nTotalInputFrames; //Total discovered input frame based on a clip. These many ETBs will be sent
   uint32   m_nSuperFrame;            // indicates how many AUs needs to fill up in each ETB
   uint32   m_nMaxInputFrameSize;     // Store max frame size once indexing has been done

   BOOL     m_bSecureSession;
   BOOL     m_bIsProfileMode;
   BOOL     m_bAutoRestart;
   BOOL     m_bEosReached;
   BOOL     m_bRCVFullSeqLayer;
   BOOL     m_bRCVIsV2Format;
   uint32   m_eIVFType;
   BOOL     m_bIsGetSeqHdr;
   BOOL     m_bIsInterlace;
   BOOL     inputDump;
   void*    m_pIntBuf;

   File*    m_InputFile;
   FILE*    m_bitsDump;
   FILE*    m_InputBitsOffsetFile;
   FILE*    m_InputOffsetFile;
   uint32   m_bReadDirection;       // YUV Parse Direction: Forward:1 Backward:0
   uint32   m_nInputFrameSize;      // Size of Frame to be READ from Input file.
   u64   m_nCurFilePos;          // Current position of InputFile pointer.
   uint32   m_bIsDivxPacked;
   uint32   nMinFrameDelay;
   uint32   nOffsetDump;
   VideoFrameRate   m_nFrameRate;

   ReadBuffer m_fReadBuffer;   //Function pointer to a codec based parser

   IsNextFrameSlice  m_fIsNextFrameSlice;
   IsHeaderStartCode m_fIsHeaderStartCode;

   uint32 m_eBufferFormat;
   LIST_NODE m_FrameOffsetTable; //Map of <FrameNum,FileOffset> for seeking in file

   uint8*   m_pYuvScratch; //Used for planer to UBWC conversion.
   UBWCInfo m_ubwcInfo;

   VidcDecHdrMode  m_nPrefixHeaderMode; // 0 - S mode, 1 - SF mode, 2 - S+SF mode
   uint32 nHdrLengthCopied;

   uint32 m_nSkipReadAfterNFrames;
   uint32 m_nReverseReadingBasis;
   uint32 m_nlogMask;
   LIST_NODE m_LoadedInputBufferList;//List of <pBuffer, FilledLength> for saving loaded buffer info for skipping reading in decoders

   void       (*Close)(VideoStreamParser* pParser);
   VidcStatus (*Configure)(VideoStreamParser* pParser, VideoSessionStaticConfig *pConfig,
                string sInRoot, BOOL inputDump, string sOutRoot,
                string sInFileName, string sTestID, BOOL bSecure);
   VidcStatus (*GetNextFrame)(VideoStreamParser* pParser, ParserFrameInfoType *pFrameInfo);
   VidcStatus (*SeekToFrame)(VideoStreamParser* pParser, ParserFrameInfoType *pFrameInfo, uint32 frameNum, uint32 isSyncRequest);
   VidcStatus (*SetResolution)(VideoStreamParser* pParser, VideoResolution res, uint32 size);
   VidcStatus (*SetConcealColor)(VideoStreamParser* pParser, uint32 color);
   void       (*SetFrameRate)(VideoStreamParser* pParser, uint32 frameRatenum, uint32 frameRateDen);
   void       (*ReadDivx311StreamInfo)(VideoStreamParser* pParser, uint32 *nWidth, uint32 *nHeight);
   void       (*ReadVC1RCVStreamInfo)(VideoStreamParser* pParser, uint32 *nWidth, uint32 *nHeight);
   VidcStatus (*BuildOffsetTable)(VideoStreamParser* pParser);
   uint32     (*CountSlicesInBuffer)(uint8* pFrameBuffer, uint32 FrameSize);
   unsigned int (*GetUBWC3FrameSize)(uint32 width, uint32 height, uint32 bufferformat);
};

VidcStatus VideoStreamParserInit(VideoStreamParser* pParser);

struct VideoStreamWriter
{
   uint32 m_nNalHeaderLength;
   void* m_pIntBuf;
   BOOL  m_bSecureSession;
   VidcStatus (*bitstreamWriter)(VideoStreamWriter* pWriter, void* pFile,
                ParserFrameInfoType* pFrm);
   VidcStatus (*CountSlicesInBuffer)(VideoStreamWriter* pWriter,
                ParserFrameInfoType *pFrm, uint32 *pNumSlices);
   void (*Close)(VideoStreamWriter*);
};

VidcStatus VideoStreamWriterInit(VideoStreamWriter*, uint32, BOOL);

typedef enum VideoCryptoCopyDir
{
    COPY_NONSECURE_TO_SECURE,
    COPY_SECURE_TO_NONSECURE
}VideoCryptoCopyDir;

VidcStatus VideoCryptoLoadLib(void);
VidcStatus VideoCryptoUnloadLib(void);
VidcStatus VideoCryptoCopyBuffer(ParserFrameInfoType, unsigned);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _VIDEOSTREAMPARSERINTERFACE_H
