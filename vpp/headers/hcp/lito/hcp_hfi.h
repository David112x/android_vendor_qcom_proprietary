/**
 Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
**/

/*==============================================================================
 * @file            hcp_hfi.h
 *
 * @services
 *
 * @description     Header file containing definitions for command and
*                      message packets.
==============================================================================*/

#ifndef HCP_HFI_H
#define HCP_HFI_H

#ifdef _ANDROID_
#include "AEEStdErr.h"
#include "AEEStdDef.h"
#endif//_ANDROID_

/*===========================================================================
 Constants, macros
===========================================================================*/
#define MAX_NUM_BUFFS_IN_GRP        2
#define MAX_NUM_PLANES              4
#define MEAS_BUF_ALIGNMENT          256
#define INT_PIX_BUF_ALIGNMENT       256
#define HCP_FRAMERATE_MIN           24

#define SESSION_ID_INVALID          ((int32)0xFFFFFFFF)
#define SESSION_ID_ALL              ((int32)0xFFFFFFFE)

#define LOCK_HOST                   0
#define UNLOCK_HOST                 1

/* Fields to enable logging at different levels */
#define HCP_LOGFLD_DEBUG            (1<<4)
#define HCP_LOGFLD_LOW              HCP_LOGFLD_DEBUG
#define HCP_LOGFLD_MEDIUM           (1<<3)
#define HCP_LOGFLD_HIGH             (1<<2)
#define HCP_LOGFLD_ERROR            (1<<1)
#define HCP_LOGFLD_FATAL            (1<<0)

//#define PACK_STRUCTS
#ifdef PACK_STRUCTS//Pack structs
#define PACKING_ATTR __attribute__((__packed__))
#else
#define PACKING_ATTR
#endif //PACK_STRUCTS


/*===========================================================================
 Type definitions
===========================================================================*/
/**
* @enum     t_EHcpErrId
* @brief    System/session error IDs.
*/
typedef enum{
    ERR_ID_SUCCESS = 0,
    ERR_ID_FAIL,/*< Generic  */
    ERR_ID_NULLPTR,/*< NULL Ptr  */
    ERR_ID_MEM_FAIL,/*< Memory failure: failed to allocate or failed to access  */
    ERR_ID_INVALID_PARAM,/*< Invalid parameter  */
    ERR_ID_INVALID_BUFSIZE,/*< Size of buffer is not what is expected  */
    ERR_ID_INVALID_STATE,/*< Resource is in invalid state  */
    ERR_ID_NO_RESOURCE,/*< Resources unavailable  */
    ERR_ID_INCOMPLETE_CONFIG,/*< Current configuration is not complete/valid  */
    ERR_ID_INACTIVE_SESS, /*< Indicates that his session is not active.  */
    ERR_ID_BUFLIST_FULL, /*< Indicate that internal buf list is full.  */
    ERR_ID_CLOSE_SESS_FAILED, /*< Failed to close session  */
    ERR_ID_INTERRUPTED,/*< Operation interrupted  */
    /////////////////////////////////////////////////////////
    ERR_ID_NUM
}t_EHcpErrId;

/**
* @enum     t_EHcpDiagId
* @brief    Setting Id.
*/
typedef enum{
    DIAG_CONTENT_LOG,/*< Configure logflag */
    DIAG_CONTENT_DIAGCOLLECT,/*< Configure diagnostic collection */
    /////////////////////////////////////////////////////////
    DIAG_CONTENT_NUM
}t_EHcpDiagContent;

/**
* @enum     t_EHcpDiagFlagCfgShift
* @brief    Flag mask bitshifts: use to enable content.
*/
typedef enum{
    DIAG_FLAGCFGSHIFT_TRACE,/*< Update trace fields in record */
    DIAG_FLAGCFGSHIFT_REGS,/*< Update register fields in record */
    DIAG_FLAGCFGSHIFT_PERF,/*< Update performance fields in record */
    /////////////////////////////////////////////////////////
    DIAG_FLAGCFGSHIFT_NUM
}t_EHcpDiagFlagCfgShift;

/**
* @struct     t_StDiagCollect
* @brief    Diagnostic collection settings.
*/
typedef struct PACKING_ATTR stDiagCollect {
    uint32 u32En; /*< Enable/Disable */
    uint32 u32CfgFlag; /*< Configuration flag. See: t_EHcpDiagFlagCfgShift */
}t_StDiagCollect;

/**
* @struct     t_StSysDiag
* @brief    Logging and diagnostics.
*/
typedef struct PACKING_ATTR stSysDiag {
    uint32 u32Content;/*< Id: see t_EHcpDiagId */
    union {
        uint32 u32LogFlag;/*< Logging */
        t_StDiagCollect stDiagCollect;/*< Diagnostics collection */
    };
}t_StSysDiag;

/**
* @struct     t_StSysTuneParam
* @brief    Tuning parameters
*/
typedef struct PACKING_ATTR StSysTuneParam {
    struct{
        uint32 u32Id; /*< Tuning Id. See t_EHcpHfiTuningId. */
        uint32 u32Len;/*< How many uint32 elements in u32Bdy array*/
    } hdr;
    uint32 u32Bdy[1];/*< */
}t_StSysTuneParam;

/**
* @struct     t_StSysTune
* @brief    Tuning parameters
*/
typedef struct PACKING_ATTR StSysTune {
    uint32 u32NumTuneParams; /*< How many tuning parameters are contanied within */
    uint32 u32Bdy[1];/*< Variable body payload starting at u32Bdy[0]. Contains an array of t_StSysTuneParam. */
}t_StSysTune;

/**
* @enum     t_EHcpHfiOpMode
* @brief    Operation mode.
*/
typedef enum{
    //! Drop all out pixels, only get measurement values
    OPMODE_MEASURE = 0,
    //! Reconstruct pixels into external memory
    OPMODE_PROCESS= 1,
    OPMODE_MAX
}t_EHcpHfiOpMode;

/**
* @enum     t_EHcpHfiCP
* @brief    Content protection attribute.
*/
typedef enum{
    //! Non-Content Protected pixels (not in CPZ)
    CP_NO,
    //! Content Protected pixels (in CPZ)
    CP_YES,
    CP_MAX
}t_EHcpHfiCP;

/**
* @struct     t_StBufReqs
* @brief     Buffer requirements for session
*/
typedef struct PACKING_ATTR StSessBufReqs {
    uint32 u32InBuffsNeeded;     /*< Number of input buffers needed per transaction */
    uint32 u32OutBuffsNeeded;    /*< Number of output buffers needed per transaction.
                                This can be pixel or results buffer (depending on opmode)*/
    uint32 u32MeasBuffSize;      /*< Size of results buffer */
    uint32 u32IntPixBuffSize;   /*< Size of intermediate pixel buffer */
}t_StSessBufReqs;


/**
* @struct     t_StSessResourceHints
* @brief     Resource hints: Used by HCP to calculate Clock/Bandwidth requirements
*/
typedef struct PACKING_ATTR stSessResourceHints {
    uint32 u32ImgWidth; /*< total image width in pixels */
    uint32 u32ImgHeight; /*< total image height in lines */
    uint32 eFmtId; /*< See: t_EHcpHfiFmts for buffer format */
    uint32 u32FrameRate; /*< frame rate (Hz) */
}t_StSessResourceHints;

/*!
 * Codec type
 */
typedef enum {
    //! ITU-T Rec. H.264; ISO/IEC 14496-10
    CODECTYPE_H264,
    //! ITU-T Rec. H.263
    CODECTYPE_H263,
    //! ISO/IEC 11172-2
    CODECTYPE_MPEG1,
    //! ITU-T Rec. H.262; ISO/IEC 13818-2
    CODECTYPE_MPEG2,
    //! ISO/IEC_14496-2
    CODECTYPE_MPEG4,
    //! DivX 3.11
    CODECTYPE_DIVX_311,
    //! DivX 4, 5 or 6
    CODECTYPE_DIVX,
    //! SMPTE 421M
    CODECTYPE_VC1,
    //! Sorenson Spark
    CODECTYPE_SPARK,
    //! On2 VP8
    CODECTYPE_VP8,
    //! HEVC ITU-T Rec. H.265 ISO/IEC 23008-2 MPEG-H Part-2
    CODECTYPE_HEVC,
    //! VP9
    CODECTYPE_VP9,
    CODECTYPE_MAX
} t_EHcpHfiCodecType;

/*!
 * Picture/frame type
 */
typedef enum {
    //! I-frame type
    PICTURETYPE_I,
    //! P-frame type
    PICTURETYPE_P,
    //! B-frame type
    PICTURETYPE_B,
    PICTURETYPE_MAX
} t_EHcpHfiPictureType;

/*!
 * Dynamic range
 */
typedef enum {
    //! SDR
    DYNRANGE_SDR,
    //! HDR
    DYNRANGE_HDR,
///////////////////////
    DYNRANGE_MAX
} t_EHcpHfiDynRange;

/**
* @struct   t_StDynRangeParam
* @brief    Dynamic range parameters.
*/
typedef struct PACKING_ATTR StDynRange  {
    uint32 eDynRange;/*< : t_EHcpHfiDynRange */
} t_StDynRangeParam;

/**
* @struct     t_StSessVdecParam
* @brief     Video codec parameters for session
*/
typedef struct PACKING_ATTR StSessVdecParam  {
    //! LCU size
    uint32 eLcu; /*< :t_EHcpHfiLcuSize */
    //! Codec standard
    uint32 eCodec;/*< :t_EHcpHfiCodecType */
    //! frame rate
    uint32 nFrameRate;
    //! overall bit rate
    uint32 nOverallBitRate;
    //! frame/picture type
    uint32 ePicType;/*< :t_EHcpHfiPictureType */
    //! per-frame bit rate
    uint32 nPerFrameBitRate;
    //! _SUM_QP
    uint32 nQPSum;
    //! _SUM_QP_COUNT
    uint32 nQPSumCnt;
    //! _SKIP_SUM_QP
    uint32 nSkipQPSum;
    //! _SKIP_SUM_QP_COUNT
    uint32 nSkipQPSumCnt;
    //! Dynamic range parameters
    t_StDynRangeParam stDynRangeParam;
} t_StSessVdecParam;

/**
* @struct     t_StUbwcStats
* @brief     UBWC Stats
*/
typedef struct PACKING_ATTR StUbwcStats  {
    uint32 eValid; /*< 0=>Content is invalid */
    uint32 u32Num64BComp;
    uint32 u32Num128BComp;
    uint32 u32Num192BComp;
    uint32 u32Num256BComp;
    uint32 u32Num32BComp;
    uint32 u32Num96BComp;
    uint32 u32Num160BComp;
} t_StUbwcStats;

/**
* @enum     t_EHcpSplitScreenRegion
* @brief    Screen region.
*/
typedef enum {
    HCP_REGION_LEFT,
    HCP_REGION_RIGHT,
    HCP_REGION_TOP,
    HCP_REGION_BOT,
/////////////////////////////////
    HCP_REGION_NUM
} t_EHcpSplitScreenRegion;

/**
* @struct   t_uSplitScreenCtrl
* @brief    Split screen control
*/
union t_uSplitScreenCtrl{
    uint32 u32;
    struct {
        uint32 eProcessedRegion : 8; /*< t_EHcpSplitScreenRegion: determines what region contains processed pixels. */
        uint32 u32ProcessedPercent : 8; /*< Determines what percent of the overall image the processed region takes up. */
    }st;
};

/**
* @enum     t_EHcpHfiLtmMode
* @brief    LTM Mode ID.
*/
typedef enum {
    LTM_MODE_MANUAL=0,  /*< Manual mode.*/
    LTM_MODE_AUTO,      /*< Auto mode.*/
/////////////////////////////////
    LTM_MODE_NUM
} t_EHcpHfiLtmMode;

/**
* @struct   t_StHcpHfiCustomKnob
* @brief    Custom knob container.
*/
typedef struct {
    uint32 u32BdySizeBytes; /*< Size of body in bytes. */
	uint32 u32Bdy[1];       /*< Variable body payload starting at u32Bdy[0] */
} t_StHcpHfiCustomKnob;

/**
* @enum     t_EHcpHfiKnobId
* @brief    Control knob identifiers.
*/
typedef enum {
    /*> EAR mode: mode={0, 5, app dflt=4}*/
    KNOB_ID_EAR_NMODE,
    /*> DE gains: level={0, 100, app dflt=25}*/
    KNOB_ID_LTM_FDEGAIN,
    /*> LTM Gain for saturation: level={0, 100, app dflt=50}*/
    KNOB_ID_LTM_NSAT_GAIN,
    /*> ACE contrast enhancement strength: level={0, 100, app dflt=50}*/
    KNOB_ID_LTM_NACESTRCON,
    /*> Split screen demo mode: mode={0, 100, app dflt=0}.
    The processed part is: right side for 0, 20, 40... bottom side for 10, 30, 50....
    Linewidth:based on resolution*/
    KNOB_ID_APP_SPLITSCREEN,
    /*> LTM Offset for saturation: level={0, 100, app dflt=50}*/
    KNOB_ID_LTM_NSAT_OFF,
    /*> ACE brightness enhancement strength for dark regions: level={0, 100, app dflt=25}*/
    KNOB_ID_LTM_NACESTRBRIL,
    /*> ACE brightness enhancement strength for bright regions: level={0, 100, app dflt=25}*/
    KNOB_ID_LTM_NACESTRBRIH,
    /*> LTM Mode. See t_EHcpHfiLtmMode.*/
    KNOB_ID_LTM_MODE,
    /*< Set config for custom functionality.*/
    KNOB_ID_CUSTOM,
/////////////////////////////////
    KNOB_ID_NUM
} t_EHcpHfiKnobId;

/**
* @struct     t_StSessKnobs
* @brief     Knobs for session
*/
typedef struct PACKING_ATTR StSessKnobs {
    uint32 eKnobId; /*<t_EHcpHfiKnobId */
    union{
        uint32 u32EarMode;
        uint32 u32DeGain;
        uint32 u32SatGain;
        uint32 u32AceStrCon;
        uint32 u32SatOff;
        uint32 u32AceStrBriL;
        uint32 u32AceStrBriH;
        uint32 u32LtmMode;
        union t_uSplitScreenCtrl uSplitScreen;
        t_StHcpHfiCustomKnob stCustom;
    };
}t_StSessKnobs;

/**
* @enum     t_EHcpHfiBufDir
* @brief    Buffer direction
*/
typedef enum {
    BUFDIR_INPUT = 0,
    BUFDIR_OUTPUT,
    BUFDIR_INOUTPUT,
////////////////////////////////
    BUFDIR_NUM
} t_EHcpHfiBufDir;


/*!
 * LCU sizes
 */
typedef enum {
    //! 16x16
    LCUSIZE_16X16,
    //! 32X32
    LCUSIZE_32X32,
    //! 64X64
    LCUSIZE_64X64,
    LCUSIZE_MAX
} t_EHcpHfiLcuSize;

/*!
 * Format list
 */
typedef enum {
    FMT_NV12_UBWC,
    FMT_NV21_UBWC,
    FMT_TP10_UBWC,
    FMT_NV12,
    FMT_NV21,
    FMT_P010,
    FMT_INVALID,
    FMT_MAX
} t_EHcpHfiFmts;

/*!
 * pixel specification
 */
typedef struct {
    //! total image width in pixels
    uint32 u32ImgWidth;
    //! total image height in lines
    uint32 u32ImgHeight;

#if 0
    //! Horizontal active start position
    uint32 u32ActiveWidthStart;
    //! Vertical active start position
    uint32 u32ActiveHeightStart;
    //! Horizontal active width
    uint32 u32ActiveWidth;
    //! Vertical active height
    uint32 u32ActiveHeight;

    //! video scan mode
    uint32 eVideoScan; /* t_EDvpVideoScan */
#endif
    //! buffer format
    uint32 eFmtId; /* t_EHcpHfiFmts */
} t_StHcpHfiVideo;

/*!
 *Plane identifier
 */
typedef enum {
    PLANEID_UBWCNV12_LUMAMD = 0,
    PLANEID_UBWCNV12_LUMA,
    PLANEID_UBWCNV12_CHROMAMD,
    PLANEID_UBWCNV12_CHROMA,

    PLANEID_UBWCTP10_LUMAMD = 0,
    PLANEID_UBWCTP10_LUMA,
    PLANEID_UBWCTP10_CHROMAMD,
    PLANEID_UBWCTP10_CHROMA,

    PLANEID_NV12_LUMA = 0,
    PLANEID_NV12_CHROMA,

    PLANEID_P010_LUMA = 0,
    PLANEID_P010_CHROMA

///////////////////////////////////////
} t_EHcpHfiUbwcPlaneId;

/**
* @struct     t_StHcpHfiPlaneInfo
* @brief     Plane information
*/
typedef struct PACKING_ATTR stHcpHfiPlaneInfo {
   uint32 u32Stride;
   uint32 u32Sizebytes;
}t_StHcpHfiPlaneInfo;

/**
* @struct     t_StHcpHfiPixBufAttr
* @brief     Pixel buffer attributes
*/
typedef struct PACKING_ATTR StHcpHfiPixBufAttr {
   uint32 offset;                            /*< If all buffers exist in one contig memory region,
                                                    this identifies the offset of this particular buffer
                                                    from the start of the memory region*/
   uint32 eDirection; /*< :t_EHcpHfiBufDir*/
   t_StHcpHfiVideo stVideoParam;
   t_StSessVdecParam stVdecParam; /*< metadata and decoder info */
   uint32 u32NumPlanes;
   t_StHcpHfiPlaneInfo stPlaneInfo[MAX_NUM_PLANES];
   t_StUbwcStats stUbwcStats;
}t_StHcpHfiPixBufAttr;

/**
* @struct     t_StHcpHfiMeasBufContent
* @brief     Result (measurement) buffer content (for MEASURE mode only)
*/
typedef struct PACKING_ATTR stHcpHfiMeasBufContent {
    uint32 u32LtmZoneX; /*<  LTM number of horizontal zones*/
    uint32 u32LtmZoneY; /*<  LTM number of vertical zones*/

    uint32 u32LtmHistoOffset; /*< Start of Ltm Histogram in pMeasBuf*/
    uint32 u32EarHistoOffset; /*< Start of Ear Histogram in pMeasBuf*/
    uint32 u32NyqOffset;/*< Start of Nyq counts inpMeasBuf */

    uint32 u32MeasBufSize; /*< Buffer size in bytes */
    uint32 align[MEAS_BUF_ALIGNMENT-1];/*< Align buffer to 256B. */
    uint32 pMeasBuf[1];/*< Must be aligned to 256. */
}t_StHcpHfiMeasBufContent;

/**
* @struct     t_StHcpHfiMeasNyqResults
* @brief     Nyquist results
*/
typedef struct PACKING_ATTR stHcpHfiMeasNyqResults {
    uint32 u32Hnyq_counts; /*< Count of values exceeding threshold */
    uint32 u32Vnyq_counts;/*< Count of values exceeding threshold */
    uint32 u32Hnyq_maxvalue;/*< Maxvalue of Horizontal Nyquist */
    uint32 u32Vnyq_maxvalue; /*< Maxvalue of Vertical Nyquist */
}t_StHcpHfiMeasNyqResults;

/**
* @struct     t_StHcpHfiMeasBufAttr
* @brief     Result (measurement) buffer attributes
*/
typedef struct PACKING_ATTR StHcpHfiMeasBufAttr {
   uint32 u32Size; /*< Buffer size in bytes */
}t_StHcpHfiMeasBufAttr;

/**
* @struct     t_StHcpHfiIntermPixBufAttr
* @brief     Intermediate buffer attributes
*/
typedef struct PACKING_ATTR StHcpHfiIntermBufAttr {
   uint32 u32Size; /*< Buffer size in bytes */
   uint32 eCpStatus; /*< Protection attributes: t_EDvpCP */
}t_StHcpHfiIntermPixBufAttr;

/**
* @enum     t_EHcpHfiGenBufUsage
* @brief    Buffer usage
*/
typedef enum {
   BUFF_USAGE_DIAG, /*< Use this buffer for diagnostics */
   BUFF_USAGE_TUNE, /*< Use this buffer for tuning */
/////////////////////////
    BUFF_USAGE_NUM
}t_EHcpHfiGenBufUsage;

/**
* @struct     t_StHcpHfiGenBufAttr
* @brief     Generic (non-pixel) buffer attributes
*/
typedef struct PACKING_ATTR StHcpHfiGenBufAttr {
   uint32 u32Size; /*< Buffer size in bytes */
   uint32 eDirection; /*< :t_EHcpHfiBufDir*/
   uint32 eUsage; /* < t_EHcpHfiGenBufUsage */
}t_StHcpHfiGenBufAttr;

/**
* @enum     t_EHcpHfiBufType
* @brief    Buffer type
*/
typedef enum {
   BUFF_TYPE_GENERIC,   /*< Custom buffers*/
   BUFF_TYPE_PIXEL,     /*< Pixel buffers*/
   BUFF_TYPE_MEAS,   /*< Measurement buffers*/
   BUFF_TYPE_INTERMEDIATE_PIXEL,  /*< Intermediate pixel buffers*/
/////////////////////////
    BUFF_TYPE_NUM
}t_EHcpHfiBufType;

/**
* @enum     t_EHcpHfiBufOp
* @brief    What operation was applied on this buffer
*/
typedef enum {
   BUFF_OP_PROCESSED,/*< Buffer was processed in HQV */
   BUFF_OP_BYPASSED,/*< Buffer was bypassed in HQV */
   BUFF_OP_HWFAILED,/*< HQV HW failed to process buffer */
/////////////////////////
    BUFF_OP_NUM
}t_EHcpHfiBufOp;

/**
* @struct     t_StHcpHfiBufAttr
* @brief     Buffer attributes
*/
typedef struct PACKING_ATTR StHcpHfiBufAttr
{
    uint32 eBufType; /* :t_EHcpHfiBufType */
    uint32 u32Cookie_h;  /*<  */
    uint32 u32Cookie_l;  /*<  */
    uint32 u32Operation; /*< :t_EHcpHfiBufOp */
    uint32 u32Pa;
    uint32 u32IrqStatus;
    uint32 u32ErrIrqStatus;
    uint32 u32Counts;
    //Anonymous union to store buf attr
    union{
        t_StHcpHfiGenBufAttr stGenBufAttr;
        t_StHcpHfiPixBufAttr stPixBufAttr;
        t_StHcpHfiMeasBufAttr stMeasBufAttr;
        t_StHcpHfiIntermPixBufAttr stIntermPixBufAttr;
        uint32 padding[48];/*To avoid interface mismatches
                        with HLOS if this needs to change*/
    };
}t_StHcpHfiBufAttr;

/**
* @struct     t_StHcpHfiBufContent
* @brief     Buffer content: address or descriptor.
*/
typedef struct PACKING_ATTR StHcpHfiBufContent {
   int32 fd;
   uint32 u32Q6Paddr_l;
   uint32 u32Q6Paddr_h;
   uint32 u32Q6Vaddr_l;
   uint32 u32Q6Vaddr_h;
   uint32 u32Size;
   uint32 u32Offset;
}t_StHcpHfiBufContent;

/**
* @struct     t_StHcpHfiSnglBufDesc
* @brief     Single buffer descriptor
*/
typedef struct PACKING_ATTR StHcpHfiSnglBufDesc {
   t_StHcpHfiBufAttr bufAttr;
   t_StHcpHfiBufContent bufContent;
}t_StHcpHfiSnglBufDesc;

/**
* @struct     t_StHcpHfiBufGrpDesc
* @brief     Buffer group descriptor
*/
typedef struct PACKING_ATTR StHcpHfiBufGrpDesc {
   uint32 u32NumBuffs;    /*< How many buffers are contained in this group */
   t_StHcpHfiBufAttr bufAttr[MAX_NUM_BUFFS_IN_GRP];
   t_StHcpHfiBufContent bufContent[MAX_NUM_BUFFS_IN_GRP];
}t_StHcpHfiBufGrpDesc;

/**
* @struct     t_StBufTimestamp
* @brief     Buffer timestamp
*/
typedef struct PACKING_ATTR StBufTimestamp {
   uint32 u32SequenceNum;
}t_StBufTimestamp;


/*******************  Command/Message Data Structures ***************/
/**
* @enum     t_EHcpHfiCmdId
* @brief    System and session command IDs.
*/
typedef enum{
    CMD_ID_SET_PROPERTY,        /*< Set system property: t_StHcpHfiCmdSetProp */
    CMD_ID_GET_PROPERTY,        /*< Get system property: t_StHcpHfiCmdGetProp */
    CMD_ID_SYS_SET_BUFFERS,     /*< Set system buffers */
    CMD_ID_SESS_SET_BUFFERS,    /*< Set buffers: TODO: reference buf_attr*/
    CMD_ID_SESS_CLOSE,          /*< Close session.*/
    CMD_ID_NOOP,         /*< NOOP Command: use for client synchronization*/
    /////////////////////////////////////////////////////////
    CMD_ID_NUM
}t_EHcpHfiCmdId;

/**
* @enum     t_EHcpHfiMsgId
* @brief    System and session message IDs.
*/
typedef enum{
    MSG_ID_SET_PROPERTY_DONE = CMD_ID_NUM,      /*< Returned as a result of CMD_ID_SET_PROPERTY: t_StHcpHfiMsgSetProp */
    MSG_ID_PROPERTY,                            /*< Returned as a result of CMD_ID_GET_PROPERTY: t_StHcpHfiMsgGetProp */
    MSG_ID_BUFF_RELEASED,                   /*< Returned after CMD_ID_SESS_SET_BUFFERS with a released buffer */
    MSG_ID_CLOSE_DONE,
    MSG_ID_EVENT_NOTIFY,                        /* Async SYS event notification: t_StHcpHfiMsgEvt */
    MSG_ID_NOOP,                            /*< Returned as a result of CMD_ID_NOOP*/
    /////////////////////////////////////////////////////////
    MSG_ID_NUM
}t_EHcpHfiMsgId;

/**
* @enum     t_EHcpHfiPropId
* @brief    System/session property IDs.
*/
typedef enum{
    ///System properties
    PROP_ID_SYS_DIAG,     /*< Enable/disable/configure diagnostics */
    ///Session properties
    PROP_ID_SESS_RESOURCE_HINTS,/*< Info needed to determine necessary resources */
    PROP_ID_SESS_BUFREQ,/*< Set/get buffer requirements */
    PROP_ID_SESS_KNOB,         /*< Set/get knob */
    PROP_ID_SESS_OPMODE,    /*< Set/get operational mode */
    PROP_ID_SESS_CP_STATUS,    /*< Set/get content protection status */
    /////////////////////////////////////////////////////////
    PROP_ID_NUM
}t_EHcpHfiPropId;

/**
* @enum     t_EHcpHfiEvtId
* @brief    System/session event IDs.
*/
typedef enum{
    /*--- System-wide events --- */
    EVT_ID_SYS_BASE                 = 0x1,
    EVT_ID_SYS_HW_FATAL             = EVT_ID_SYS_BASE,

    /*--- Session-specific events --- */
    EVT_ID_SESS_BASE                = 0x10000000,
    EVT_ID_SESS_SET_BUFFERS_FAILED   = EVT_ID_SESS_BASE, /*< SESS_SET_BUFFERS command failed. The client can remove the buffers */

    /*--- Custom events --- */
    EVT_ID_CUSTOM_BASE              = 0x20000000

}t_EHcpHfiEvtId;

/**
 * @struct     t_StHcpHfiCmdSetProp
 * @brief "Set property" command.
 */
 typedef struct PACKING_ATTR StHcpHfiCmdSetProp{
    uint32 propId; /*< :t_EHcpHfiPropId */
    union{
    //place individual structures here
    t_StSysDiag stSysDiag;              //PROP_ID_SYS_DIAG

    t_StSessResourceHints stResourceHints;  //PROP_ID_SESS_RESOURCE_HINTS
    t_StSessBufReqs stBufReqs;          //PROP_ID_SESS_BUFREQ
    t_StSessKnobs stKnobs;              //PROP_ID_SESS_KNOB
    uint32 eOpMode;                     //PROP_ID_SESS_OPMODE: t_EHcpHfiOpMode
    uint32 eCpStatus;                   //PROP_ID_SESS_CP_STATUS: t_EHcpHfiCP
    };
 }t_StHcpHfiCmdSetProp;

 /**
 * @struct     t_StHcpHfiCmdGetProp
 * @brief "Get property" command.
 */
 typedef struct PACKING_ATTR StHcpHfiCmdGetProp{
    uint32 propId; /*< t_EHcpHfiPropId*/
    union{
    /*place individual identifiers here*/
        uint32 u32TuneId; //PROP_ID_SYS_TUNING
        uint32 eKnobId; //PROP_ID_SESS_KNOB
    };
 }t_StHcpHfiCmdGetProp;

/**
 * @struct     t_StHcpHfiMsgSetProp
 * @brief "Set property" message.
 */
 typedef struct PACKING_ATTR StHcpHfiMsgSetProp{
    uint32 propId; /*< :t_EHcpHfiPropId */
 }t_StHcpHfiMsgSetProp;

 /**
 * @struct     t_StHcpHfiMsgGetProp
 * @brief "Get property" message
 */
 typedef struct PACKING_ATTR StHcpHfiMsgGetProp{
     uint32 propId; /*< :t_EHcpHfiPropId */
    union{
    /*place individual structures here*/
    t_StSysDiag stSysDiag;              //PROP_ID_SYS_DIAG

    t_StSessBufReqs stBufReqs;          //PROP_ID_SESS_BUFREQ
    t_StSessKnobs stKnobs;              //PROP_ID_SESS_KNOB
    uint32 eOpMode;            //PROP_ID_SESS_OPMODE: t_EDvpOpMode
    uint32 eCpStatus;        //PROP_ID_SESS_CP_STATUS: t_EDvpCP
    };
 }t_StHcpHfiMsgGetProp;

/**
 * @struct     t_StHcpHfiCmdBuf
 * @brief "Set buffers" command
 */
 typedef struct PACKING_ATTR StHcpHfiCmdBuf{
    t_StHcpHfiBufGrpDesc stBufGrpDesc;
 }t_StHcpHfiCmdBuf;

 /**
 * @struct     t_StHcpHfiMsgBuf
 * @brief "Release buffers" message: returned when buffer is released
 */
 typedef struct PACKING_ATTR StHcpHfiMsgBuf{
    t_StHcpHfiSnglBufDesc stBufDesc;
 }t_StHcpHfiMsgBuf;

  /**
 * @struct     t_StHcpHfiMsgEvt
 * @brief "Event" message
 */
 typedef struct PACKING_ATTR StHcpHfiMsgEvt{
    uint32 eId;
 }t_StHcpHfiMsgEvt;

 /**
 * @struct     t_StHcpHfiCmdPktHdr
 * @brief Commad packet header
 */
 typedef struct PACKING_ATTR StHcpHfiCmdPktHdr{
    uint32 eCmdId;      /*< Command Identifier: t_EHcpHfiCmdId */
    int32 s32SessId;     /*< Sess Id (will be ignored if eCmdId is SYS CMD */
    uint32 u32Size;       /* Size of the packet (hdr + bdy) in bytes*/
 }t_StHcpHfiCmdPktHdr;

/**
 * @struct     t_StHcpHfiMsgPktHdr
 * @brief Message packet header
 */
 typedef struct PACKING_ATTR StHcpHfiMsgPktHdr{
    uint32 eMsgId;      /*< Message Identifier: t_EHcpHfiMsgId*/
    int32 s32SessId;     /*< Sess Id */
    uint32 u32Size;       /* Size of the packet (hdr + bdy) in bytes*/
    int32 s32Err;         /*< Error code */
 }t_StHcpHfiMsgPktHdr;

/**
 * @struct     t_StHcpHfiCmdPkt
 * @brief Commad packet
 */
typedef struct PACKING_ATTR StHcpHfiCmdPkt{
    t_StHcpHfiCmdPktHdr stHdr;
    uint32 u32Bdy[1];/*< Variable body payload starting at u32Bdy[0] */
}t_StHcpHfiCmdPkt;

  /**
 * @struct     t_StHcpHfiMsgPkt
 * @brief Message packet
 */
 typedef struct PACKING_ATTR StHcpHfiMsgPkt{
    t_StHcpHfiMsgPktHdr  stHdr;
    union{
        t_StHcpHfiMsgGetProp stMsgGetProp;
        t_StHcpHfiMsgSetProp stMsgSetProp;
        t_StHcpHfiSnglBufDesc stMsgBuf;
        t_StHcpHfiMsgEvt stMsgEvt;
        uint32 u32Bdy[1];/*< Variable body payload starting at u32Bdy[0] */
    };
 }t_StHcpHfiMsgPkt;

   /**
 * @enum
 * @brief Tuning Ids
 */

typedef enum {
    //! LTM MIN LUMA VALUE TO BE CORRECTED {0, 63, [DFLT=12]}
    DDTUNE_SAT_THR,

    //! SPLITSCREEN Y MSB VALUE OF SPLIT INDICATOR LINE {0, 15, [DFLT=9]}
    //! GREEN=149/43/21, RED=76/84/255, BLUE=29/255/107
    DDTUNE_PPSPLIT_LINE_Y,

    //! SPLITSCREEN CB/U MSB VALUE OF SPLIT INDICATOR LINE {0, 15, [DFLT=2]}
    DDTUNE_PPSPLIT_LINE_CB,

    //! SPLITSCREEN CR/V MSB VALUE OF SPLIT INDICATOR LINE {0, 15, [DFLT=1]}
    DDTUNE_PPSPLIT_LINE_CR,

    DDTUNE_MAX,

    //! Table to map an EAR level (LOW/MED/HIGH) to one of the 8 available levels in the Blending LUT and Variance Gain LUT.
    //! LEN=4. VAL[0-3]={0, 7, []}
    EARTUNE_REG_MAP_H264,
    EARTUNE_REG_MAP_H263,
    EARTUNE_REG_MAP_MPEG1,
    EARTUNE_REG_MAP_MPEG2,
    EARTUNE_REG_MAP_MPEG4,
    EARTUNE_REG_MAP_DIVX_311,
    EARTUNE_REG_MAP_DIVX,
    EARTUNE_REG_MAP_VC1,
    EARTUNE_REG_MAP_SPARK,
    EARTUNE_REG_MAP_VP8,
    EARTUNE_REG_MAP_HEVC,
    EARTUNE_REG_MAP_VP9,

    //! Use BAR Detection enable {0, 1, [dflt=1]} dis=0, auto=1
    EARTUNE_NBARDETEN,

    EARTUNE_MAX,

    //! SWITCH LTM ZONE LEVEL MODE {0, 1, [DFLT=1]}
    LTMTUNE_NLTMZONELEVEL,

    //! REMAP THE LTM X AND Y ZONE PARTITION TABLE. LEN=20.
    //! First 5 pairs used for low power level; last 5 pairs used for high power/performance level.
    //! VAL[0,2,4,6,8,10,12,14,16,18]={0, 7, []}. VAL[1,3,5,7,9,11,13,15,17,19]={0, 5, []}
    LTMTUNE_ANLTMZONEMAP,

    //! REMAP DE GAIN FACTORS TABLE, APPLIED WITH NYQ RESULTS. LEN=9. IN F16_16
    //! VAL[0-8]={0, 0x20000, []}
    LTMTUNE_ANNYQDEGAINFACTOR,

    //! REMAP DE GAIN FACTORS TABLE, APPLIED WITH NYQ RESULTS. LEN=9. IN F16_16
    //! VAL[0-8]={0, 0x20000, []}
    LTMTUNE_ANEARDEGAINFACTOR,

    //! DE OFFSET GAINS {-1.0, 7.0, [DFLT=0.0]} IN F16_16
    LTMTUNE_FDEOFF,

    //! DE CORING MIN PER HUE {0, 63, [DFLT=4]}
    LTMTUNE_NDEMIN,

    //! DE CORING MAX PER HUE {0, 63, [DFLT=16]}
    LTMTUNE_NDEMAX,

    //! DE NOISE CORING POWER {0, 8, [DFLT=2]}
    LTMTUNE_NDEPOW1,

    //! DE OVERSHOOT CORING POWER {0.0, 8, [DFLT=6]}
    LTMTUNE_NDEPOW2,

    //! Nyquist detection threshold over which to apply NYQ_GAIN {0, 1023, [DFLT=32]}
    LTMTUNE_NDENYQTHRESH,

    //! Nyquist gain coefficient for Nyquist adaptive DE {0, 255, [dflt=32]}
    LTMTUNE_NDENYQGAIN,

    //! Blending gain for the combined unsharp mask gain and 3x3 LPF gain {0, 31, [dflt=23]}
    LTMTUNE_NDELPFBLEND,

    //! LTM IIR filter coefficient for histograms accumulation {0.0, 1.0, [dflt=0.25]}
    LTMTUNE_FLUTCOEFFIIR,

    //! LTM Number of blended LUTs {1, 63, [dflt=16]}
    LTMTUNE_NLUTCNTBLEND,

    //! HST HISTOGRAM DIFFERENCE THRESHOLD FOR SCENE DETECTION {0.0, 1.0, [DFLT=0.2]} IN F16_16
    LTMTUNE_FHSTTHRDIFF,

    //! HST IIR filter coefficient for histograms accumulation {0.0, 1.0, [dflt=0.25]} in F16_16
    LTMTUNE_FHSTCOEFFIIR,

    //! ACE maximum black limit {0, 255, [dflt=16]}
    LTMTUNE_NBLACKMAX,

    //! ACE minimum white limit {0, 255, [dflt=240]}
    LTMTUNE_NWHITEMIN,

    //! ACE quantile for black and white points {0, 255, [dflt=12]}
    LTMTUNE_NQUANTILE,

    //! ACE Difference between max and min quantile values in histogram to be enhanced {0, 255, [dflt=64]}
    LTMTUNE_NRNGLIM,

    //! ACE Histogram value to be enhanced {0, 255, [dflt=64]}
    LTMTUNE_NHSTLIM,

    //! LTM Luma clippimg gap: {0, 31, [dflt=8], 0 means no clipping}
    LTMTUNE_NCLPGAP,

    //! DE IIR filter coefficient for coring table: {0.0, 1.0, [dflt=0.25]}
    LTMTUNE_FDECOEFFIIR,

    //! HST percentage of updated tiles for DE gain to be corrected: {0,128, [dflt=64 (50%)}
    LTMTUNE_NHSTUPDDEL,

    //! LUT IIR filter coefficient for LUTs accumulation: {0.0,1.0, [dflt=0.0]}
    LTMTUNE_FLUTCOEFFMINIIR,

    //! LUT IIR threshold for filter coefficient: {0.0,1.0, [dflt=0.1]}
    LTMTUNE_FLUTTHRESHIIR,

    //! LUT LPF filter coefficient of LUTs blend in transition between scenes: {0 ,256, [dflt=256]}
    LTMTUNE_NLUTCOEFFLPF,

    //! LUT percentage of plain tiles for DE gain to be set to zero: {0,128, [dflt=64 (50%)]} : 0 - disable the feature
    LTMTUNE_NLUTUPDDEL,

    //! TM coefficients: {0.0,10.0, [dflt=1.0]} : 1.0 - no mapping
    LTMTUNE_FTMCOEFF,

    //! TM function order: {0, 1, [dflt=0]} : 0 - after HE, 1 - before HE
    LTMTUNE_NTMORDER,

    //! Auto table for contrast: {0.0, 1.0, []}, 4 entries.
    LTMTUNE_AFAUTOACECON,

    //! Auto table for brightness in dark areas: {0.0, 1.0, []}, 4 entries.
    LTMTUNE_AFAUTOACEBRIL,

    //! Auto table for brightness in bright areas: {0.0, 1.0, []}, 4 entries.
    LTMTUNE_AFAUTOACEBRIH,

    //! Auto table for saturation gain: {0.0, 2.0, []}, 4 entries.
    LTMTUNE_AFAUTOSATSAT,

    //! Auto table for saturation offset: {0.0, 2.0, []}, 4 entries.
    LTMTUNE_AFAUTOSATOFF,

    //! Auto table for DE gain: {-1.0, 7.0, []}, 4 entries.
    LTMTUNE_AFAUTODEGAIN,

    //! Auto table for DE offset: {-1.0, 7.0, []}, 4 entries.
    LTMTUNE_AFAUTODEOFF,

    //! Enable/disable custom LUT generation for SDR content: {0, 1, [dflt=0]}:
    //! 0 - use default LUT generation, 1 - use custom LUT generation
    LTMTUNE_NCUST_SDR_LUT_EN,

    //! Enable/disable custom LUT generation for HDR content: {0, 1, [dflt=0]}:
    //! 0 - use default LUT generation, 1 - use custom LUT generation
    LTMTUNE_NCUST_HDR_LUT_EN,

    LTMTUNE_MAX,

    //! USE LTM MAP {0, 1, [DFLT=1]}
    APPTUNE_NLTMMAPEN,

    //! USE LTM SAT {0, 1, [DFLT=1]}
    APPTUNE_NLTMSATEN,

    //! USE DITHER MODE 1:TRUNCATE 2:ROUND 3:NOISE {1, 3, [DFLT=1]}
    APPTUNE_NDITMODE,

    //! SPLITSCREEN SPLIT INDICATOR LINE WIDTH MAP TABLE. LEN=3
    //! VAL[0-2]={0, 3, [DFLT={1,2,3}]}.
    APPTUNE_ANPPSPLITWIDTHMAP,

    APPTUNE_MAX,
/////////////////////////////////////////////////////////////////
    HCP_HFI_TUNE_NUM

}t_EHcpHfiTuningId;

 /*===========================================================================
 Function Prototypes
===========================================================================*/

#endif //HCP_HFI_H

