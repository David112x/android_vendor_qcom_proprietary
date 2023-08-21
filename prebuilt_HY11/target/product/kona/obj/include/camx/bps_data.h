//-------------------------------------------------------------------------
// Copyright (c) 2015-2017, 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// @file  bps_data.h
// @brief BPS (Bayer Processing) data types.
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// @Edit History
//
// when              who           what,where,why
// -----             ---           -------------
//
// 10/29/2015        opolonsk      Initial version
//------------------------------------------------------------------------
#ifndef _BPS_DATA_H_
#define _BPS_DATA_H_

#include "ipdefs.h"
#include "fwdefs.h"

//-----------------------------------------------------------------------------
// Type Declarations
//-----------------------------------------------------------------------------
#pragma pack(push)
#pragma pack(1)

typedef enum _BPS_IO_IMAGES
{
    BPS_INPUT_IMAGE,
    BPS_OUTPUT_IMAGE_FULL,
    BPS_OUTPUT_IMAGE_DS4,
    BPS_OUTPUT_IMAGE_DS16,
    BPS_OUTPUT_IMAGE_DS64,
    BPS_OUTPUT_IMAGE_STATS_BG,
    BPS_OUTPUT_IMAGE_STATS_BHIST,
    BPS_OUTPUT_IMAGE_REG1,
    BPS_OUTPUT_IMAGE_REG2,

    BPS_OUTPUT_IMAGE_FIRST = BPS_OUTPUT_IMAGE_FULL,
    BPS_OUTPUT_IMAGE_LAST = BPS_OUTPUT_IMAGE_REG2,

    BPS_IO_IMAGES_MAX // Must be the last member

} BPS_IO_IMAGES;

typedef struct _BpsConfigIoData
{
    ImageDescriptor images[BPS_IO_IMAGES_MAX];

    // An indication whether the BPS is working in secure mode
    uint32_t               secureMode;

} BpsConfigIoData;

typedef struct _BpsConfigIoResult
{
    uint32_t            cdmBufferSize;
} BpsConfigIoResult;

typedef struct _BPS_PEDESTAL_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  SCALE_BYPASS; /* 8:8 */
    uint32_t  NUM_SUBBLOCKS; /* 13:12 */
} BPS_PEDESTAL_MODULE_CFG;

typedef struct _PedestalParameters
{
    BPS_PEDESTAL_MODULE_CFG moduleCfg;
    uint32_t subgridWidth;
    uint32_t meshGridBwidth;
    uint32_t startBlockIndex;
    uint32_t startSubgridIndex;
    uint32_t topLeftCoordinate;

    uint32_t initBlockY;
    uint32_t initSubblockY;
    uint32_t initPixelY;
} PedestalParameters;

typedef struct _LINEARIZATION_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} LINEARIZATION_MODULE_CFG;

typedef struct _LinearizationParameters
{
    LINEARIZATION_MODULE_CFG moduleCfg;
} LinearizationParameters;

typedef struct _BPC_PDPC_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  PDAF_PDPC_EN; /* 8:8 */
    uint32_t  BPC_EN; /* 9:9 */
    uint32_t  HOT_PIXEL_CORRECTION_DISABLE; /* 12:12 */
    uint32_t  COLD_PIXEL_CORRECTION_DISABLE; /* 13:13 */
    uint32_t  USING_CROSS_CHANNEL_EN; /* 14:14 */
    uint32_t  REMOVE_ALONG_EDGE_EN; /* 15:15 */
    uint32_t  BAYER_PATTERN; /* 17:16 */
    uint32_t  PDAF_HDR_SELECTION; /* 20:18 */
    uint32_t  PDAF_ZZHDR_FIRST_RB_EXP; /* 21:21 */
    uint32_t  CHANNEL_BALANCE_EN; /* 22:22 */
} BPC_PDPC_MODULE_CFG;

typedef struct _PDPCParameters
{
    BPC_PDPC_MODULE_CFG moduleCfg;
    int32_t pdafGlobalOffsetX;
    int32_t pdafTableOffsetX;
    int32_t pdafGlobalOffsetY;
    int32_t pdafTableOffsetY;
    int32_t pdafXEnd;
    int32_t pdafYEnd;
} PDPCParameters;

typedef struct _BPC_PDPC_MODULE_CFG_480
{
    uint32_t  EN;             /* 0:0 */
    uint32_t  PDAF_PDPC_EN;         /* 8:8 */
    uint32_t  BPC_EN;             /* 9:9 */
    uint32_t  USESAMECHANNEL_ONLY;     /* 12:12 */
    uint32_t  SINGLEBPC_ONLY;         /* 13:13 */
    uint32_t  FLAT_DETECTION_EN;     /* 14:14 */
    uint32_t  DBPC_EN;         /* 15:15 */
    uint32_t  BAYER_PATTERN;         /* 17:16 */
    uint32_t  PDAF_HDR_SELECTION;     /* 20:18 */
    uint32_t  PDAF_ZZHDR_FIRST_RB_EXP;    /* 21:21 */
    uint32_t  CHANNEL_BALANCE_EN;     /* 22:22 */
} BPC_PDPC_MODULE_CFG_480;

typedef struct _PDPCParameters_480
{
    BPC_PDPC_MODULE_CFG_480 moduleCfg;
    int32_t pdafGlobalOffsetX;
    int32_t pdafTableOffsetX;
    int32_t pdafGlobalOffsetY;
    int32_t pdafTableOffsetY;
    int32_t pdafXEnd;
    int32_t pdafYEnd;
} PDPCParameters_480;

typedef struct _BPC_PDPC_MODULE_CFG_680
{
    uint32_t  EN;                         /* 0:0 */
    uint32_t  PDAF_PDPC_EN;             /* 8:8 */
    uint32_t  BPC_EN;                     /* 9:9 */
    uint32_t  USESAMECHANNEL_ONLY;         /* 12:12 */
    uint32_t  SINGLEBPC_ONLY;             /* 13:13 */
    uint32_t  DBPC_EN;                     /* 15:15 */
} BPC_PDPC_MODULE_CFG_680;

typedef struct _PDPCParameters_680
{
    BPC_PDPC_MODULE_CFG_680 moduleCfg;
    int32_t pdafGlobalOffsetX;
    int32_t pdafTableOffsetX;
    int32_t pdafGlobalOffsetY;
    int32_t pdafTableOffsetY;
    int32_t pdafXEnd;
    int32_t pdafYEnd;
} PDPCParameters_680;

typedef struct _HDR_RECON_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} HDR_RECON_MODULE_CFG;

typedef struct _HDRReconParameters
{
    HDR_RECON_MODULE_CFG moduleCfg;
    uint32_t hdrMode; // 0 iHdr, 1 ZZ hdr
    uint32_t zzHdrShift;
    uint32_t zzHdrPattern; // 0=R1G1G2B1, 1=R1G1G2B2, 2=R1G2G1B1, 3=R1G2G1B2
    uint32_t zzHdrPrefilterTap;
    int32_t  linearMode;
    int32_t  hdrExpRatio;
} HDRReconParameters;

typedef struct _HDR_MAC_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} HDR_MAC_MODULE_CFG;

typedef struct _HDRMacParameters
{
    HDR_MAC_MODULE_CFG moduleCfg;
    int32_t  dilation;
    uint32_t smoothFilterEnable;
    int32_t  linearMode;
} HDRMacParameters;

typedef struct _HDR_BINCORRECT_MODULE_CFG_480
{
    uint32_t  EN; /* 0:0 */
} HDR_BINCORRECT_MODULE_CFG_480;

typedef struct _HDRBinCorrectParameters_480
{
    HDR_BINCORRECT_MODULE_CFG_480 moduleCfg;
    int32_t  hdrExpRatio;
    uint32_t hdrZrecFirstRbExp;
    uint32_t hdrZrecPattern;
    uint32_t hdrLsbAligh;
    int32_t  hdrLinearMode;
    uint32_t hdrEnable;
    int32_t  hdrMacDilation;
    int32_t  hdrMacLinearMode;
    uint32_t binCorrEn;
} HDRBinCorrectParameters_480;

typedef struct _GIC_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  PNR_EN; /* 8:8 */
    uint32_t  GIC_EN; /* 9:9 */
} GIC_MODULE_CFG;

typedef struct _GicParameters
{
    GIC_MODULE_CFG moduleCfg;
} GicParameters;

typedef struct _GIC_MODULE_CFG_680
{
    uint32_t  EN;                      /* 0:0 */
    uint32_t  GIC_EN;                  /* 9:9 */
} GIC_MODULE_CFG_680;

typedef struct _GicParameters_680
{
    GIC_MODULE_CFG_680 moduleCfg;
} GicParameters_680;

typedef struct _BINCORRECT_MODULE_CFG
{
    uint32_t EN;    /* 0:0 */
} BINCORRECT_MODULE_CFG;

typedef struct _BinCorrectParameters
{
    BINCORRECT_MODULE_CFG moduleCfg;
} BinCorrectParameters;

typedef struct _BAYER_GTM_MODULE_CFG
{
    uint32_t EN;    /* 0:0 */
} BAYER_GTM_MODULE_CFG;

typedef struct _BayerGtmParameters
{
    BAYER_GTM_MODULE_CFG moduleCfg;
} BayerGtmParameters;

typedef struct _ABF_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  FILTER_EN; /* 8:8 */
    uint32_t  ACT_ADJ_EN; /* 9:9 */
    uint32_t  DARK_SMOOTH_EN; /* 10:10 */
    uint32_t  DARK_DESAT_EN; /* 11:11 */
    uint32_t  DIR_SMOOTH_EN; /* 12:12 */
    uint32_t  MINMAX_EN; /* 13:13 */
    uint32_t  CROSS_PLANE_EN; /* 14:14 */
    uint32_t  BLS_EN; /* 15:15 */
    uint32_t  PIX_MATCH_LEVEL_RB; /* 18:16 */
    uint32_t  PIX_MATCH_LEVEL_G; /* 22:20 */
    uint32_t  BLOCK_MATCH_PATTERN_RB; /* 25:24 */
} ABF_MODULE_CFG;

typedef struct _AbfParameters
{
    ABF_MODULE_CFG moduleCfg;
} AbfParameters;

typedef struct _ABF_MODULE_CFG_680
{
    uint32_t  EN;                /* 0:0 */
    uint32_t  BPC_EN;            /* 8:8 */
    uint32_t  ABF_EN;            /* 9:9 */
    uint32_t  SINGLE_BPC_EN; /* 10:10 */
} ABF_MODULE_CFG_680;

typedef struct _AbfParameters_680
{
    ABF_MODULE_CFG_680 moduleCfg;
} AbfParameters_680;

typedef struct _LENS_ROLLOFF_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  NUM_SUBBLOCKS; /* 13:12 */
} LENS_ROLLOFF_MODULE_CFG;

typedef struct _LensRollOffParameters
{
    LENS_ROLLOFF_MODULE_CFG moduleCfg;
    uint32_t subgridWidth;
    uint32_t meshGridBwidth;
    uint32_t startBlockIndex;
    uint32_t startSubgridIndex;
    uint32_t topLeftCoordinate;
    uint32_t numHorizontalMeshGains;

    uint32_t initBlockY;
    uint32_t initSubblockY;
    uint32_t initPixelY;
} LensRollOffParameters;

typedef struct _LENS_ROLLOFF_MODULE_CFG_480
{
    uint32_t  EN; /* 0:0 */
    uint32_t  ALSC_EN; /* 1:1 */
    uint32_t  NUM_SUBBLOCKS; /* 13:12 */
} LENS_ROLLOFF_MODULE_CFG_480;

typedef struct _LensRollOffParameters_480
{
    LENS_ROLLOFF_MODULE_CFG_480 moduleCfg;
    uint32_t subgridWidth;
    uint32_t meshGridBwidth;
    uint32_t startBlockIndex;
    uint32_t startSubgridIndex;
    uint32_t topLeftCoordinate;
    uint32_t numHorizontalMeshGains;

    uint32_t initBlockY;
    uint32_t initSubblockY;
    uint32_t initPixelY;
} LensRollOffParameters_480;

typedef struct _DEMOSAIC_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  COSITED_RGB_EN; /* 10:10 */
    uint32_t  DIR_G_INTERP_DIS; /* 12:12 */
    uint32_t  DIR_RB_INTERP_DIS; /* 13:13 */
    uint32_t  DYN_G_CLAMP_EN; /* 14:14 */
    uint32_t  DYN_RB_CLAMP_EN; /* 15:15 */
} DEMOSAIC_MODULE_CFG;

typedef struct _DemosaicParameters
{
    DEMOSAIC_MODULE_CFG moduleCfg;
} DemosaicParameters;

typedef struct _STATS_BG_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  SAT_STATS_EN; /* 8:8 */
    uint32_t  QUAD_SYNC_EN; /* 9:9 */
    uint32_t  SHIFT_BITS; /* 12:10 */
} STATS_BG_MODULE_CFG;

typedef struct _BayerGridParameters
{
    STATS_BG_MODULE_CFG moduleCfg;
    int32_t  bgRgnWidth;
    int32_t  bgRgnHeight;
    int32_t  bgRgnVNum;   //for stats packing only
    int32_t  bgRgnHNum;
    int32_t  bgRegionSampling;
    int32_t  bgRoiHOffset;
    int32_t  bgRoiVOffset;
    uint32_t bgYOutputEnable;
    uint32_t greenChannelSelect;
    uint32_t a0Coefficient;
    uint32_t a1Coefficient;
    uint32_t a2Coefficient;
    uint32_t rMax;
    uint32_t rMin;
    uint32_t grMax;
    uint32_t grMin;
    uint32_t gbMax;
    uint32_t gbMin;
    uint32_t bMax;
    uint32_t bMin;
    uint32_t yMax;
    uint32_t yMin;
} BayerGridParameters;

typedef struct _STATS_HDR_BHIST_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  HDR_BHIST_CHAN_SEL; /* 8:8 */
                                  /* 18:16 */ // bits updated in Waipio Bhist v1.6
    // Obsolete item, BPS BHist HW doesn't have this bit
    uint32_t  HDR_BHIST_FIELD_SEL; /* 10:9 */
    // Obsolete item, BPS BHist HW doesn't have this bit
    uint32_t  HDR_BHIST_SITE_SEL; /* 11:11 */
} STATS_HDR_BHIST_MODULE_CFG;

typedef struct _BayerHistogramParameters
{
    STATS_HDR_BHIST_MODULE_CFG moduleCfg;
    int32_t  bihistRoiHOffset;
    int32_t  bihistRoiVOffset;
    int32_t  bihistRgnHNum;
    int32_t  bihistRgnVNum;
} BayerHistogramParameters;

typedef struct _BPS_COLOR_CORRECT_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} BPS_COLOR_CORRECT_MODULE_CFG;

typedef struct _BpsColorCorrectParameters
{
    BPS_COLOR_CORRECT_MODULE_CFG moduleCfg;
} BpsColorCorrectParameters;

typedef struct _BPS_GLUT_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} BPS_GLUT_MODULE_CFG;

typedef struct _BpsGlutParameters
{
    BPS_GLUT_MODULE_CFG moduleCfg;
} BpsGlutParameters;

typedef struct _BPS_COLOR_XFORM_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} BPS_COLOR_XFORM_MODULE_CFG;

typedef struct _ColorXformParameters
{
    BPS_COLOR_XFORM_MODULE_CFG moduleCfg;
} ColorXformParameters;

typedef struct _ChromaSubsampleParameters
{
    uint32_t horizontalRoundingOption;
    uint32_t verticalRoundingOption;
} ChromaSubsampleParameters;

typedef struct _RegistrationScalerParameters
{
    uint32_t horizontalRoundingOption;
    uint32_t verticalRoundingOption;
} RegistrationScalerParameters;

typedef struct _FourToOneDownscalerParameters
{
    uint32_t coefficient7;
    uint32_t coefficient16;
    uint32_t coefficient25;
} FourToOneDownscalerParameters;

// The following block is specified explicitly since the round and clamp
// are coupled with the crop parameters which are striping dependent
typedef struct _ClampParameters
{
    uint32_t clampMin;
    uint32_t clampMax;
} ClampParameters;

typedef struct _RoundParameters
{
    uint32_t roundingPattern;
} RoundParameters;

typedef struct _RoundAndClampParameters
{
    ClampParameters lumaClampParameters;
    ClampParameters chromaClampParameters;
    RoundParameters lumaRoundParameters;
    RoundParameters chromaRoundParameters;
} RoundAndClampParameters;

typedef struct _WindowRegion BpsFetchCropWindow;

typedef struct _BPS_COMPDECOMP_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  ROUND_EN; /*3:2*/
} BPS_COMPDECOMP_MODULE_CFG;
typedef struct _BpsCompDecompParameters
{
    BPS_COMPDECOMP_MODULE_CFG moduleCfg;
} BpsCompDecompParameters;
typedef struct _BPS_WB_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} BPS_WB_MODULE_CFG;
typedef struct _BpsWbParameters
{
    BPS_WB_MODULE_CFG moduleCfg;
    uint32_t          channelRecovery;
    uint32_t          recoveryInterpRange;
} BpsWbParameters;
typedef struct _BPS_LCAC_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} BPS_LCAC_MODULE_CFG;
typedef struct _BpsLcacParameters
{
    BPS_LCAC_MODULE_CFG moduleCfg;
    uint32_t            startStepXMSB;
    uint32_t            startStepXLSB;
    uint32_t            startR2MSB;
    uint32_t            startR2LSB;
    uint32_t            StepXMSB;
    uint32_t            StepXLSB;
    uint32_t            OpticalCenterX;
} BpsLcacParameters;

/* Note: DC is disabled for BPS Bayer LTM */
typedef struct _BAYER_LTM_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  GAMMA_EN; /*8:8*/
    uint32_t  IP_EN; /* 11:11 */
    uint32_t  LA_EN; /* 12:12 */
    uint32_t  TABLE_SEL; /* 23:21 */
} BAYER_LTM_MODULE_CFG;

typedef struct _BayerLtmParameters // Local tone mapping
{
    BAYER_LTM_MODULE_CFG moduleCfg;

} BayerLtmParameters;

typedef struct _BpsIQSettings
{
    PedestalParameters            pedestalParameters;
    LinearizationParameters       linearizationParameters;
    PDPCParameters                pdpcParameters;
    PDPCParameters_480            pdpcParameters_480;
    PDPCParameters_680            pdpcParameters_680;
    HDRReconParameters            hdrReconParameters;
    HDRMacParameters              hdrMacParameters;
    HDRBinCorrectParameters_480   hdrBinCorrectParameters_480;
    GicParameters                 gicParameters;
    GicParameters_680             gicParameters_680;
    AbfParameters                 abfParameters;
    AbfParameters_680              abfParameters_680;
    LensRollOffParameters         lensRollOffParameters;
    LensRollOffParameters_480     lensRollOffParameters_480;
    DemosaicParameters            demosaicParameters;
    BayerGridParameters           bayerGridParameters;
    BayerHistogramParameters      bayerHistogramParameters;
    BpsColorCorrectParameters     colorCorrectParameters;
    GtmParameters                 gtmParameters;
    BpsGlutParameters             glutParameters;
    ColorXformParameters          colorXformParameters;
    ChromaSubsampleParameters     chromaSubsampleParameters;
    HnrParameters                 hnrParameters;
    RegistrationScalerParameters  registration1ScalerParameters;
    RegistrationScalerParameters  registration2ScalerParameters;
    FourToOneDownscalerParameters quarterScalingParameters;
    FourToOneDownscalerParameters sixteenthScalingParameters;
    FourToOneDownscalerParameters sixtyFourthScalingParameters;
    RoundAndClampParameters       fullRoundAndClampParameters;
    RoundAndClampParameters       quarterRoundAndClampParameters;
    RoundAndClampParameters       sixteenthRoundAndClampParameters;
    RoundAndClampParameters       sixtyFourthRoundAndClampParameters;
    RoundAndClampParameters       registration1RoundAndClampParameters;
    RoundAndClampParameters       registration2RoundAndClampParameters;

    // Output zoom window.
    uint32_t                      fetchCropEN;
    BpsFetchCropWindow            fetchCropWindow;
    float                         minDownscaleRatio;
    UVGammaParameters             uvGammaParameters;
    BpsCompDecompParameters       compdecompParameters;
    BpsWbParameters               wbParameters; //Note: WB module is decoupled from Demosaic in Titan 680
    BpsLcacParameters             lcacParameters;
    BayerGtmParameters            bayerGtmParameters;
    BinCorrectParameters          binCorrectParameters;
    BayerLtmParameters            bayerLtmParameters;
} BpsIQSettings;

// In HFR, a batch of multiple frames are processed using the same settings.
typedef struct _FrameSetBps
{
    // Pointers to the buffers. The size and format
    // should already be known. Configured by SW.
    FrameBuffers        frameBuffers[BPS_IO_IMAGES_MAX];
} FrameSetBps;

typedef struct _BpsFrameProcessData
{

    // Maximal requested number of BPS cores to
    // use (default 1). Configured by SW.
    uint32_t            maxNumOfCoresToUse;

    // target time (ns) for the frame to complete in hardware
    // 0 value disables dynamic QoS health calculation
    uint32_t            targetCompletionTimeInNs;

    // UBWC stats buffer
    SMMU_ADDR           ubwcStatsBufferAddress;
    uint32_t            ubwcStatsBufferSize; // in bytes

    // BL memory buffer for generating CDM commands inside the FW.
    SMMU_ADDR           cdmBufferAddress;
    uint32_t            cdmBufferSize;

    // Image Quality setting. Configured by SW.
    // It points to a data structure in an SMMU-mapped region
    // of type BpsIQSettings
    SMMU_ADDR           iqSettingsAddr;

    // SMMU-mapped address for output struct of striping library
    SMMU_ADDR           stripingLibOutAddr;

    // Pass-through IQ settings that FW programs HW untouched.
    // It points to a data structure in an SMMU-mapped region
    // of type CDMProgramArray. See definition of CDMProgramArray
    // in fwdefs.h
    SMMU_ADDR           cdmProgramArrayAddr;

    // unique frame process request ID, used for identifying specific frames to abort
    uint32_t            requestId;

    // Number of buffer sets (1 for single frame, more for HFR use-case)
    uint32_t            numFrameSetsInBatch;

    // Pointers to the buffers (main and scaled). Their sizes and format should already be known. Configured by SW.
    FrameSetBps         frameSets[MAX_HFR_GROUP];

} BpsFrameProcessData;

typedef struct _BpsFrameProcessResult
{
    uint32_t            hangDumpBufferSize;
    SMMU_ADDR           hangDumpBufferAddr;
} BpsFrameProcessResult;

typedef struct _BpsAbortData
{
    uint32_t            numRequestIds;  // Number of request IDs to abort
    uint32_t*           requestIds;     // Array of request IDs to abort
} BpsAbortData;

#pragma pack(pop)

// BPS_CONFIG_IO command
typedef struct _BpsConfigIo
{
    BpsConfigIoData         cmdData;    // Input data
    FWUSERARG               userArg;    // FW-reserved argument
} BpsConfigIo;

// BPS_CONFIG_IO_DONE command response
typedef struct _BpsConfigIoDone
{
    FWSTATUS                rc;         // Async return code
    BpsConfigIoResult       cmdRes;     // Result data
    FWUSERARG               userArg;    // FW-reserved argument
} BpsConfigIoDone;

// BPS_FRAME_PROCESS command
typedef struct _BpsFrameProcess
{
    BpsFrameProcessData     cmdData;    // Input data
    FWUSERARG               userArg;    // FW-reserved argument
} BpsFrameProcess;

// BPS_FRAME_PROCESS_DONE command response
typedef struct _BpsFrameProcessDone
{
    FWSTATUS                rc;         // Async return code
    BpsFrameProcessResult   cmdRes;     // Result data
    FWUSERARG               userArg;    // FW-reserved argument
} BpsFrameProcessDone;

// BPS_ABORT command
typedef struct _BpsAbort
{
    BpsAbortData            cmdData;    // Input data
    FWUSERARG               userArg;    // FW-reserved argument
} BpsAbort;

// BPS_ABORT_DONE command response
typedef struct _BpsAbortDone
{
    FWSTATUS                rc;         // Async return code
    FWUSERARG               userArg;    // FW-reserved argument
} BpsAbortDone;

// BPS_DESTROY command
typedef struct _BpsDestroy
{
    FWUSERARG               userArg;    // FW-reserved argument
} BpsDestroy;

// BPS_DESTROY_DONE command response
typedef struct _BpsDestroyDone
{
    FWSTATUS                rc;         // Async return code
    FWUSERARG               userArg;    // FW-reserved argument
} BpsDestroyDone;

// A handle to BPS instance
typedef struct
{
    uint32_t                tagHandle;  // Opaque 32-bit value
} BpsHandle;

typedef enum _BpsOperationMode
{
    BPS_REAL_TIME_STREAM,
    BPS_NON_REAL_TIME_STREAM,
    BPS_SEMI_REAL_TIME_STREAM,
} BpsOperationMode;

#endif // _BPS_DATA_H_


