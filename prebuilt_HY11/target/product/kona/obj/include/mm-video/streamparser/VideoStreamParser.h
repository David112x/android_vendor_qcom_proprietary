/*-------------------------------------------------------------------
Copyright (c) 2014-2018 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
--------------------------------------------------------------------*/

#ifndef _VIDEOSTREAMPARSER_H
#define _VIDEOSTREAMPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VideoStreamParserInterface.h>
#include <stdlib.h>
#include <linux/videodev2.h>
#include <VideoComDef.h>
#include <list.h>
#include <media/msm_media_info.h>

#define FORWARD 1
#define BACKWARD 0

/*  Four-character-code (FOURCC) */
#define v4l2_fourcc(a, b, c, d)\
((__u32)(a) | ((__u32)(b) << 8) | ((__u32)(c) << 16) | ((__u32)(d) << 24))

#define V4L2_PIX_FMT_DIVX      v4l2_fourcc('D', 'I', 'V', 'X') /* DIVX*/
#define V4L2_PIX_FMT_DIVX_311  v4l2_fourcc('D', 'I', 'V', '3') /* DIVX311*/

typedef enum ParserFrameType{
  PARSER_FRAME_TYPE_SEQ_HEADER = 0x00,
  PARSER_FRAME_TYPE_SYNC = 0x01,
  PARSER_FRAME_TYPE_NON_SYNC = 0x02,
  PARSER_FRAME_TYPE_VC1_RCV_STRCUT_C = 0x03
}ParserFrameType;

/* use the same string as exposed through xml for kernel and omx test-app */
static ParseCfgEnum pCodecTypeMap[] =
{
    { "VIDEO_CodingMPEG4",    V4L2_PIX_FMT_MPEG4 },
    { "VIDEO_CodingH263",   V4L2_PIX_FMT_H263 },
    { "VIDEO_CodingAVC",   V4L2_PIX_FMT_H264 },
    { "VIDEO_CodingVP8",    V4L2_PIX_FMT_VP8 },
    { "VIDEO_CodingVC1",    V4L2_PIX_FMT_VC1_ANNEX_G },
    { "VIDEO_CodingVC1RCV",    V4L2_PIX_FMT_VC1_ANNEX_L },
    { "VIDEO_CodingDIVX",   V4L2_PIX_FMT_DIVX },
    { "VIDEO_CodingMPEG2",  V4L2_PIX_FMT_MPEG2 },
    { "VIDEO_CodingHEVC",   V4L2_PIX_FMT_HEVC },
    { "VIDEO_CodingImageHEIC",  V4L2_PIX_FMT_HEVC},
    { "VIDEO_CodingVP9",    V4L2_PIX_FMT_VP9 },
    { 0, 0 }
};

typedef struct FileOffset
{
    ParserFrameType eFrameType;
    u64 nOffset;     //Buffer data read start offset
    u64 nLength;     //The length of the data in the buffer
    uint32 isLastFrame;
}FileOffset;

typedef struct FileOffsetMap
{
    uint32     nFrameNum;
    FileOffset val;
    LIST_NODE list;
}FileOffsetMap;

//Structure for saving buffer filled length for skipping mode in decoders
typedef struct LoadedBufferInfo
{
    uint8*    pbuffer;
    u64    filledLength;
    LIST_NODE list;
}LoadedBufferInfo;

typedef signed char        Int8;
typedef unsigned char      UInt8;
typedef signed short  int  Int16;
typedef unsigned short     UInt16;
typedef signed long           Int32;
typedef unsigned long         UInt32;
typedef long long          Int64;
typedef unsigned long long UInt64;    /** Unsigned 64 bit type */

#define VENUS_CHROMA_FORMAT_IDC400 0
#define VENUS_CHROMA_FORMAT_IDC420 1
#define VENUS_CHROMA_FORMAT_IDC422 2
#define VENUS_CHROMA_FORMAT_IDC444 3


#ifdef __cplusplus
}
#endif

#endif // #ifndef _VIDEOSTREAMPARSER_H
