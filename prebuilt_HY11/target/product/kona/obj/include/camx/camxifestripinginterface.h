////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifestripinginterface.h
/// @brief camxifestripinginterface class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef CAMXIFESTRIPINGINTERFACE_H
#define CAMXIFESTRIPINGINTERFACE_H

#include "camxdefs.h"

CAMX_NAMESPACE_BEGIN


static const UINT32 NUM_OF_BAF_REGIONS                    = 10;
static const UINT32 PDAF_IIR_OVERLAP                      = 50;
static const UINT32 STRIPE_BF_STATS_RGNCOUNT_V23          = 182;
static const UINT32 STRIPE_BF_STATS_RGNCOUNT_V25          = 181;
static const UINT32 STRIPE_CSIDPE_V10_MAX_BLOCK_HEIGHT    = 64;
static const UINT32 STRIPE_CSIDPE_V10_MAX_BLOCK_WIDTH     = 64;
static const UINT32 STRIPE_MAX_PIXEL_ENTRIES_RS           = 32 * 4;
static const UINT32 STRIPE_PDAF_V20_ROLLOFF_MESH_POINTS_H = 17;
static const UINT32 STRIPE_PDAF_V20_ROLLOFF_MESH_POINTS_V = 13;


enum IFEStripeTitanVersion
{
    TITAN_180 = 0x00010800,      ///< TITAN_180
    TITAN_175 = 0x00010705,      ///< TITAN_175
    TITAN_170 = 0x00010700,      ///< TITAN_170
    TITAN_160 = 0x00010600,      ///< TITAN_160
    TITAN_150 = 0x00010500,      ///< TITAN_150
    TITAN_140 = 0x00010400,      ///< TITAN_140
    TITAN_120 = 0x00010200,      ///< TITAN_120
    TITAN_480 = 0x00040800,      ///< TITAN_480
};

/* Derived information combining Titan Tiering version and HW version */
enum IFEStripeChipVersion
{
    TITAN_VERSION_FIRST = 0,   ///< Titan version

    TITAN_170_V1 = 1,          ///< SDM845 (Napali)
    TITAN_160_V1 = 2,          ///< SDM840 (NapaliQ)
    TITAN_175_V1 = 3,          ///< SDM855 (Hana)
    TITAN_170_V2 = 4,          ///< SDM845 (Napali) v2
    TITAN_150_V1 = 5,          ///< SDM640 (Talos)
    TITAN_170_V2_ONEIPE = 6,   ///< SDM670 (Warlock 1 core)
    TITAN_480_V1 = 7,          ///< SDM865 (Kona)
    SPECTRA_580  = 8,          ///< Lahaina
    TITAN_VERSION_MAX,         ///< Max val

    SANDBOX = TITAN_175_V1,    ///< SANDBOX is initially the same as T175, need to have its own value after it diverges
    SPECTRA_480 = TITAN_480_V1 ///< ALIAS names
};


struct IFEStripeDSXInputV1D
{
    INT lumaStartLocationX;        ///< luma_start_location_x;
    INT chromaStratLocationX;      ///< chroma_start_location_x;

    INT lumaScaleRatioX;           ///< luma_scale_ratio_x;
    INT chromaScaleRatioX;         ///< chroma_scale_ratio_x;

    INT lumaOutWidth;              ///< luma_out_width;
    INT chromaOutWidth;            ///< chroma_out_width;

    INT lumaInputWidth;            ///< luma_input_width;
    INT chromaInputWidth;          ///< chroma_input_width;
};  // HW parameters

struct IFEStripeDSXStripingOutputParams
{
    INT                   lumaEnable;    ///< luma_en;
    INT                   chromaEnable;  ///< chroma_en;
    IFEStripeDSXInputV1D* pDSx_x;        ///< dsx_x;
    IFEStripeDSXInputV1D* pDSx_y;        ///< dsx_y;
};

struct IFEStripeRnrStripingOutputParams
{
    INT16 bx;                ///< bx;
    INT16 by;                ///< by;
    INT32 rSquareInit;       ///< r_square_init;
    INT16 rSquareShift;      ///< r_square_shift;
    INT16 rSquareScale;      ///< r_square_scale;
};


struct IFEStripeMFHDRMACv10StripingInputParams
{
    INT16 enable;                               ///< enable;
    INT16 hdr_mode;                             ///< hdr_mode; // 2u
    INT16 hdr_mac1_motion_en;                   ///< hdr_mac1_motion_en;
    INT16 hdr_mac2_motion_en;                   ///< hdr_mac2_motion_en;
    INT16 hdr_mac1_highlight_motion_en;         ///< hdr_mac1_highlight_motion_en;
    INT16 hdr_mac2_highlight_motion_en;         ///< hdr_mac2_highlight_motion_en;
    INT16 hdr_mac1_background_rec_en;           ///< hdr_mac1_background_rec_en;
    INT16 hdr_mac2_background_rec_en;           ///< hdr_mac2_background_rec_en;
};

struct IFEStripeMFHDRMACv10StripingOutputParams
{
    INT16 hdr_width_4x_unaligned;                ///< hdr_width_4x_unaligned
};

struct IFEStripeLCRv10InputParam
{
    UINT16 firstPixel;       ///< firstPixel
    UINT16 lastPixel;        ///< lastPixel
    UINT16 firstPDCol;       ///< firstPDCol
    UINT16 blockWidth;       ///< blockWidth
};

struct IFEStripeLCRv10OutputParam
{
    UINT16 enable;          ///< enable
    UINT16 firstPixel;      ///< firstPixel
    UINT16 lastPixel;       ///< lastPixel
    UINT16 stripeCut;       ///< Stripe Cut
};

struct IFEStripeStripeROI
{
    UINT16 startX;        ///< start X
    UINT16 endX;          ///< end X
};

struct IFEStripeMNDSInputv16
{
    UINT16 input;                     ///< input_l;
    UINT16 output;                    ///< output_l;
    INT32  roundingOptionHor;         ///< rounding_option_h;
    INT32  roundingOptionVer;         ///< rounding_option_v;
};

struct IFEStripePDPCv11stripingInputParams
{
    INT32  PDAFGlobalOffsetX;           ///< pdaf_global_offset_x;
    INT32  PDAFEndX;                    ///< pdaf_x_end;
    INT32  PDAFzzHDRFirstRBExp;         ///< pdaf_zzHDR_first_rb_exp;
    UINT32 PDAFPDMask[64];              ///< PDAF_PD_Mask[64];
};

struct IFEStripePDPCv20v30stripingInputParams
{
    BOOL  enable;                        ///< enable;
    INT32 PDAFGlobalOffsetX;             ///< pdaf_global_offset_x;
    INT32 PDAFTableoffsetX;              ///< pdaf_table_x_offset;
    INT32 PDAFEndX;                      ///< pdaf_x_end;
    INT32 PDAFzzHDRFirstRBExp;           ///< pdaf_zzHDR_first_rb_exp;
    INT16 PDAFPDPCEnable;                ///< pdaf_pdpc_en;
    INT16 PDAFdsbpcEnable;               ///< pdaf_dsbpc_en;
};

struct IFEStripePedestalv13InputParams
{
    BOOL   enable;                          ///< enable;
    UINT16 blockWidth;                      ///< Bwidth_l;
    UINT16 meshGridBwidth;                  ///< MeshGridBwidth_l;
    UINT16 lxStart;                         ///< Lx_start_l;
    UINT16 bxStart;                         ///< Bx_start_l;
    UINT16 bx_d1_l;                         ///< Bx_d1_l;
    UINT16 dummyFieldForBytePadding;        ///< dummy_field_for_byte_padding;
};

struct IFEStripeRollOffv34InputParams
{
    BOOL   enable;                       ///< enable;
    UINT16 blockWidth;                   ///< Bwidth_l;
    UINT16 meshGridBwidth;               ///< MeshGridBwidth_l;
    UINT16 lxStart;                      ///< Lx_start_l;
    UINT16 bxStart;                      ///< Bx_start_l;
    UINT16 bx_d1_l;                      ///< Bx_d1_l;
    UINT16 numMeshgainHoriz;             ///< num_meshgain_h;
};


struct IFEStripeHDRBayerHistV13InputParams
{
    INT32 bihistEnabled;                ///< bihist_enabled;
    INT32 bihistROIHorOffset;           ///< bihist_roi_h_offset;
    INT32 bihistROIVerOffset;           ///< bihist_roi_v_offset
    INT32 bihistRgnHorNum;              ///< bihist_rgn_h_num;
};

struct IFEStripeiHistv12InputParam
{
    INT32 enable;                       ///< enable;
    INT32 histRgnHorOffset;             ///< hist_rgn_h_offset;
    INT32 histRgnHorNum;                ///< hist_rgn_h_num;
};

struct IFEStripeBayerGridv15InputParam
{
    INT32 BGEnabled;                    ///< bg_enabled;
    INT32 BGRgnVertNum;                 ///< bg_rgn_v_num;
    INT32 BGRgnHorizNum;                ///< bg_rgn_h_num;
    INT32 BGRegionSampling;             ///< bg_region_sampling;
    INT32 BGRgnWidth;                   ///< bg_rgn_width;
    INT32 BGROIHorizOffset;             ///< bg_roi_h_offset;
    INT32 BGSatOutputEnable;            ///< bg_sat_output_enable;
    INT32 BGYOutputEnable;              ///< bg_Y_output_enable;
};

struct IFEStripeBayerExpv15InputParam
{
    INT32 BEEnable;                    ///< be_enable;
    INT32 BEzzHDRFirstRBExp;           ///< be_zHDR_first_rb_exp;
    INT32 BEROIHorizOffset;            ///< be_roi_h_offset;
    INT32 BERgnWidth;                  ///< be_rgn_width;
    INT32 BERgnHorizNum;               ///< be_rgn_h_num;
    INT32 BERgnVertNum;                ///< be_rgn_v_num;
    INT32 BESatOutputEnable;           ///< be_sat_output_enable;
    INT32 BEYOutputEnable;             ///< be_Y_output_enable;
};

struct IFEStripeMNScaleDownInputV16
{
    UINT16 enable;                    ///< en;
    UINT16 input;                     ///< input_l;
    UINT16 output;                    ///< output_l;
    UINT16 pixelOffset;               ///< pixel_offset;
    UINT16 cntInit;                   ///< cnt_init;
    UINT16 interpReso;                ///< interp_reso;
    UINT16 roundingOptionVer;         ///< rounding_option_v;
    UINT16 roundingOptionHor;         ///< rounding_option_h;
    UINT16 rightPadEnable;            ///< right_pad_en;
    UINT16 inputProcessedLength;      ///< input_processed_length;
    UINT32 phaseInit;                 ///< phase_init;
    UINT32 phaseStep;                 ///< phase_step;
};  // HW parameters

struct IFEStripeBayerFocusv23InputParam
{
    UINT64 BAFROIIndexLUT[STRIPE_BF_STATS_RGNCOUNT_V23];    ///< baf_roi_indexLUT
    UINT16 BAFHorizScalerEn;                                ///< baf_h_scaler_en;
    UINT16 BAF_fir_h1_en;                                   ///< baf_fir_h1_en;
    UINT16 BAF_iir_h1_en;                                   ///< baf_iir_h1_en;
    UINT16 dummyFieldForBytePadding;                        ///< dummy_field_for_byte_padding;
    IFEStripeMNScaleDownInputV16 mndsParam;                 ///< mndsParam;
};


struct IFEStripeBayerFocusv24InputParam
{
    UINT64 BAFROIIndexLUT[STRIPE_BF_STATS_RGNCOUNT_V23];    ///< baf_roi_indexLUT
    UINT16 BAFHorizScalerEn;                                ///< baf_h_scaler_en;
    UINT16 BAF_fir_h1_en;                                   ///< baf_fir_h1_en;
    UINT16 BAF_iir_h1_en;                                   ///< baf_iir_h1_en;
    UINT16 dummyFieldForBytePadding;                        ///< dummy_field_for_byte_padding
    IFEStripeMNScaleDownInputV16 mndsParam;                 ///< mndsParam;
};


struct IFEStripeBayerFocusv25InputParam
{
    UINT64 BAFROIIndexLUT[STRIPE_BF_STATS_RGNCOUNT_V25][2];   ///< baf_roi_indexLUT
    UINT16 BAFHorizScalerEn;                                  ///< baf_h_scaler_en;
    UINT16 BAF_fir_h1_en;                                     ///< baf_fir_h1_en;
    UINT16 BAF_iir_h1_en;                                     ///< baf_iir_h1_en;
    UINT16 dummyFieldForBytePadding;                          ///< dummy_field_for_byte_padding;
    IFEStripeMNScaleDownInputV16 mndsParam;                   ///< mndsParam;
    UINT16 bankSel;                                           ///< bankSel;
};


struct IFEStriperowSumColSumv14InputParam
{
    INT32 RSEnable;              ///< rs_enable;
    INT32 CSEnable;              ///< cs_enable;
    INT32 RSRgnHorOffset;        ///< rs_rgn_h_offset
    INT32 RSRgnWidth;            ///< rs_rgn_width;
    INT32 RSRgnHorNum;           ///< rs_rgn_h_num;
    INT32 RSRgnVerNum;           ///< rs_rgn_v_num;
    INT32 CSRgnHorOffset;        ///< cs_rgn_h_offset;
    INT32 CSRgnWidth;            ///< cs_rgn_width;
    INT32 CSRgnHorNum;           ///< cs_rgn_h_num;
    INT32 CSRgnVerNum;           ///< cs_rgn_v_num;
};



struct IFEStripeCrop
{
    INT16 enable;     ///< enable;
    INT16 inDim;      ///< inDim;
    INT16 firstOut;   ///< firstOut
    INT16 lastOut;    ///< lastOut;
};

struct IFEStripeRollOffOutputParameters
{
    UINT32 gridIndex;                    ///< grid_index
    UINT32 subgridIndex;                 ///< subgrid_index
    UINT32 pixelIndexWithinSubgrid;      ///< pixel_index_within_subgrid
};

struct IFEStripePedestalOutputParameters
{
    UINT32 gridIndex;                    ///< grid_index
    UINT32 subgridIndex;                 ///< subgrid_index
    UINT32 pixelIndexWithinSubgrid;      ///< pixel_index_within_subgrid
};

struct IFEStripeHDROutputV20
{
    INT16 HDRZrecFirstRBExp;               ///< hdr_zrec_first_rb_exp;
    INT16 dummyFieldForBytePadding;        ///< dummy_field_for_byte_padding;
};


struct IFEStripePDPCv11stripingOutputParams
{
    INT32  PDAFGlobalOffset;               ///< pdaf_global_offset;
    INT32  PDAFEndX;                       ///< pdaf_x_end;
    INT32  PDAFzzHDRFirstRBExp;            ///< pdaf_zzHDR_first_rb_exp
    UINT32 PDAFPDMask[64];                 ///< PDAF_PD_Mask[64];
};


struct  IFEStripePDPCv20v30stripingOutputParams
{
    INT32 PDAFGlobalOffset;             ///< pdaf_global_offset;
    INT32 PDAFTableOffset;              ///< pdaf_table_offset;
    INT32 PDAFEndX;                     ///< pdaf_x_end;
    INT32 PDAFzzHDRFirstRBExp;          ///< pdaf_zzHDR_first_rb_exp;
    INT16 PDAFPDPCEnable;               ///< pdaf_pdpc_en;
};


struct IFEStripeHDRBayerHistV13OutputParams
{
    INT32 bihistEnabled;                ///< bihist_enabled;
    INT32 bihistROIHorOffset;           ///< bihist_roi_h_offset;
    INT32 bihistRgnHorNum;              ///< bihist_rgn_h_num;
    INT32 bihistROIVerOffset;           ///< bihist_roi_v_offset
    INT32 writeEngineEnable;            ///< writeEngineEnable;
};

struct IFEStripeiHistv12OutputParam
{
    INT32 enable;                     ///< enable;
    INT32 histRgnHorOffset;           ///< hist_rgn_h_offset
    INT32 histRgnHorNum;              ///< hist_rgn_h_num;
};


struct IFEStripeBayerGridInputParam
{
    INT32 BGEnabled;                  ///< bg_enabled;
    INT32 BGRgnVerNum;                ///< bg_rgn_v_num;
    INT32 BGRgnHorNum;                ///< bg_rgn_h_num;
    INT32 BGRgnSampling;              ///< bg_region_sampling;
    INT32 BGRgnWidth;                 ///< bg_rgn_width;
    INT32 BGROIHorOffset;             ///< bg_roi_h_offset;
    INT32 BGROIVerOffset;             ///< bg_roi_v_offset;
    INT32 BGSatOutputEnable;          ///< bg_sat_output_enable;
    INT32 BGYOutputEnable;            ///< bg_Y_output_enable;
};


struct IFEStripeBayerGridOutputParam
{
    INT32 BGEnabled;                    ///< bg_enabled;
    INT32 gridOutStartX;                ///< gridOutStart_x;
    INT32 gridOutEndX;                  ///< gridOutEnd_x;
    INT32 BGRgnVerNum;                  ///< bg_rgn_v_num;
    INT32 BGRgnHorNumFrame;             ///< bg_rgn_h_num_frame;
    INT32 BGRgnSamplingFrame;           ///< bg_region_sampling_frame;
    INT32 BGFirstHorRgnWidth;           ///< bg_first_hor_rgn_width;
    INT32 BGLastHorRgnWidth;            ///< bg_last_hor_rgn_width;
    INT32 BGRegionSamplingStripe;       ///< bg_region_sampling_stripe;
    INT32 BGROIHorOffset;               ///< bg_roi_h_offset;
    INT32 BGROIVerOffset;               ///< bg_roi_v_offset;
    INT32 BGRgnHorNumStripe;            ///< bg_rgn_h_num_stripe;
};

struct IFEStripeBayerGridv15OutputParam
{
    INT32 BGEnabled;                 ///< bg_enabled;
    INT32 gridOutStartX;             ///< gridOutStart_x;
    INT32 gridOutEndX;               ///< gridOutEnd_x;
    INT32 BGRgnNumFrameHor;          ///< bg_rgn_h_num_frame;
    INT32 BGROIHorizOffset;          ///< bg_roi_h_offset;
    INT32 BGRgnNumStripeHor;         ///< bg_rgn_h_num_stripe
};

struct IFEStripeBayerExpv15OutputParam
{
    INT32 BEEnable;              ///< be_enable;
    INT32 BEROIHorOffset;        ///< be_roi_h_offset;
    INT32 BERgnHorNum;           ///< be_rgn_h_num;
    INT32 BEzzHDRFirstRBExp;     ///< be_zHDR_first_rb_exp;
    INT32 gridOutStartX;         ///< gridOutStart_x;
};

struct IFEStriperowSumColSumv14OutputParam
{
    INT32 RSEnable;              ///< rs_enable;
    INT32 CSEnable;              ///< cs_enable;
    INT32 RSRgnHorOffset;        ///< rs_rgn_h_offset;
    INT32 RSRgnHorNum;           ///< rs_rgn_h_num;
    INT32 CSRgnHorOffset;        ///< cs_rgn_h_offset;
    INT32 CSRgnHorNum;           ///< cs_rgn_h_num;
};

struct IFEStripeBayerFocusv23OutputParam
{
    UINT64                       BAFROIIndexLUT[STRIPE_BF_STATS_RGNCOUNT_V23];  ///< baf_roi_indexLUT
    IFEStripeMNScaleDownInputV16 mndsParam;                                     ///< mndsParam;
    UINT16                       enable;                                        ///< enable;
    UINT16                       dummyFieldForBytePadding;                      ///< dummy_field_for_byte_padding
};

struct IFEStripeBayerFocusv24OutputParam
{
    UINT64                       BAFROIIndexLUT[STRIPE_BF_STATS_RGNCOUNT_V23];  ///< baf_roi_indexLUT
    IFEStripeMNScaleDownInputV16 mndsParam;                                     ///< mndsParam;
    UINT16                       enable;                                        ///< enable;
    UINT16                       dummyFieldForBytePadding;                      ///< dummy_field_for_byte_padding
};

struct IFEStripeBayerFocusv25OutputParam
{
    UINT64                       BAFROIIndexLUT[STRIPE_BF_STATS_RGNCOUNT_V23][2];  ///< baf_roi_indexLUT
    IFEStripeMNScaleDownInputV16 mndsParam;                                        ///< mndsParam;
    UINT16                       enable;                                           ///< enable;
    UINT16                       dummyFieldForBytePadding;                         ///< dummy_field_for_byte_padding
    UINT16                       bankSel;                                          ///< bankSel
};

struct IFEStripeWriteEngineStripeParam
{
    INT32 enable1;                   ///< enable1;
    INT32 hInit1;                    ///< hInit1;
    INT32 stripeWidth1;              ///< stripeWidth1
    INT32 enable2;                   ///< enable2;
    INT32 hInit2;                    ///< hInit2;
    INT32 stripeWidth2;              ///< stripeWidth2
};

struct IFEStripeWriteEnginePassParam
{
    INT32 bufStride1;               ///< bufStride1;
    INT32 stripeHeight1;            ///< stripeHeight1
    INT32 bufStride2;               ///< bufStride2;
    INT32 stripeHeight2;            ///< stripeHeight2
};

struct IFEStripeHDRInputV20
{
    INT16 HDRZrecFirstRBExp;     ///< hdr_zrec_first_rb_exp
};

struct IFEStripePDAFv20OutputParam
{
    UINT16 lineExtractorLength;                                                                        ///< lineExtractorLength;
    UINT16 firstPixelPre;                                                                              ///< firstPixelPre;
    UINT16 lastPixelPre;                                                                               ///< lastPixelPre;
    UINT16 firstPixelPost;                                                                             ///< firstPixelPost;
    UINT16 lastPixelPost;                                                                              ///< lastPixelPost;
    UINT16 rgnHOffset;                                                                                 ///< rgnHOffset;
    UINT16 rgnWidth;                                                                                   ///< rgnWidth;
    UINT16 rgnHNum;                                                                                    ///< rgnHNum;
    UINT32 hPhaseInit;                                                                                 ///< hPhaseInit;
    UINT32 hPhaseStep;                                                                                 ///< hPhaseStep;
    UINT16 meshPointsHNum;                                                                             ///< meshPointsHNum;
    UINT16 enable;                                                                                     ///< enable;
    UINT16 gainMapLUTL[STRIPE_PDAF_V20_ROLLOFF_MESH_POINTS_H * STRIPE_PDAF_V20_ROLLOFF_MESH_POINTS_V]; ///< GainMapLUTL
    UINT16 gainMapLUTR[STRIPE_PDAF_V20_ROLLOFF_MESH_POINTS_H * STRIPE_PDAF_V20_ROLLOFF_MESH_POINTS_V]; ///< GainMapLUTR
};


struct IFEStripePDAFv20InputParam
{
    UINT16 lutBankSel;                                                                                 ///< lut_bank_sel;
    UINT16 firstPixelPre;                                                                              ///< firstPixelPre;
    UINT16 lastPixelPre;                                                                               ///< lastPixelPre;
    UINT16 firstPixelPost;                                                                             ///< firstPixelPost;
    UINT16 lastPixelPost;                                                                              ///< lastPixelPost;
    UINT16 csidHorizScale;                                                                             ///< csidHorizScale;
    UINT16 rgnHOffset;                                                                                 ///< rgnHOffset;
    UINT16 rgnWidth;                                                                                   ///< rgnWidth;
    UINT16 rgnHNum;                                                                                    ///< rgnHNum;
    UINT16 nativeBlockWidth;                                                                           ///< nativeBlockWidth;
    UINT16 nativeBlockHeight;                                                                          ///< nativeBlockHeight;
    UINT16 CSID_pe_map[STRIPE_CSIDPE_V10_MAX_BLOCK_HEIGHT][STRIPE_CSIDPE_V10_MAX_BLOCK_WIDTH];         ///< CSID_pe_map
    UINT16 RSpixelLUT[STRIPE_MAX_PIXEL_ENTRIES_RS];                                                    ///< RSpixelLUT
    UINT16 RSinWidth;                                                                                  ///< RSinWidth;
    UINT16 RSoutWidth;                                                                                 ///< RSoutWidth;
    UINT16 RSinHeight;                                                                                 ///< RSinHeight;
    UINT16 RSoutHeight;                                                                                ///< RSoutHeight;
    UINT16 psOutWidth;                                                                                 ///< PSoutWidth;
    UINT16 lineExtractorLength;                                                                        ///< lineExtractorLength
    UINT32 hPhaseInit;                                                                                 ///< hPhaseInit;
    UINT16 pdafMode;                                                                                   ///< pdafMode;
    UINT32 sadPhaseMask0;                                                                              ///< SADPhaseMask0;
    UINT32 sadPhaseMask1;                                                                              ///< SADPhaseMask1;
    UINT16 hBinPixelNum;                                                                               ///< HBinPixelNum;
    UINT16 IIRPadding;                                                                                 ///< IIRPadding;
    UINT16 IIRFilterEnable;                                                                            ///< IIRFilterEnable;
    UINT16 gainMapEnable;                                                                              ///< gainMapEnable;
    UINT16 meshPointsHNum;                                                                             ///< meshPointsHNum;
    UINT16 meshPointsVNum;                                                                             ///< meshPointsVNum;
    UINT16 HDREnable;                                                                                  ///< HDREnable;
    UINT32 hPhaseStep;                                                                                 ///< hPhaseStep;
    UINT16 gainMapLUTL[STRIPE_PDAF_V20_ROLLOFF_MESH_POINTS_H * STRIPE_PDAF_V20_ROLLOFF_MESH_POINTS_V]; ///< GainMapLUTL
    UINT16 gainMapLUTR[STRIPE_PDAF_V20_ROLLOFF_MESH_POINTS_H * STRIPE_PDAF_V20_ROLLOFF_MESH_POINTS_V]; ///< GainMapLUTR
};

struct IFEStripeMNScaleDownInputV20
{
    INT32  phaseInit;                ///< phase_init;
    UINT32 phaseStep;                ///< phase_step;
    UINT16 enable;                   ///< en;
    UINT16 input;                    ///< input_l;
    UINT16 output;                   ///< output_l;
    UINT16 pixelOffset;              ///< pixel_offset;
    UINT16 interpReso;               ///< interp_reso;
    UINT16 roundingOptionVer;        ///< rounding_option_v;
    UINT16 roundingOptionHor;        ///< rounding_option_h;
    BOOL   dropFirstOutput;          ///< drop_first_output;
    UINT16 dummyFieldForBytePadding; ///< dummy_field_for_byte_padding
    UINT16 earlyTerminationHor;      ///< early_termination_h
};  // HW params

struct IFEZoomWindow
{
    UINT16 zoomEnable;              ///< zoom_enable;
    UINT16 startX;                  ///< start_x;
    UINT16 startY;                  ///< start_y;
    UINT16 width;                   ///< width;
    UINT16 height;                  ///< height;
};

struct IFEStripeInterfaceOutput
{
    INT32                                    edgeStripeLT;          ///< edgeStripeLT
    INT32                                    edgeStripeRB;          ///< edgeStripeRB

    // fetch coordinates
    INT16                                    fetchFirstPixel;       ///< relative to frame origin (0,0)
    INT16                                    fetchLastPixel;        ///< fetchLastPixel
    INT16                                    fetchFirstLine;        ///< relative to frame origin (0,0)
    INT16                                    fetchLastLine;         ///< fetchLastLine

    // HVX TO pixel coordinates
    INT16                                    hvxTapoffFirstPixel;        ///< hvxTapoffFirstPixel;
    INT16                                    hvxTapoffLastPixel;         ///< hvxTapoffLastPixel;

    // write coordinates
    INT16                                    outRange_fd[2];             ///< outRange_fd[2];
    INT16                                    outRange_full[2];           ///< outRange_full[2];
    INT16                                    outRange_1to4[2];           ///< outRange_1to4[2];
    INT16                                    outRange_1to16[2];          ///< outRange_1to16[2];
    INT16                                    outRange_disp[2];           ///< outRange_disp[2];
    INT16                                    outRange_1to4_disp[2];      ///< outRange_1to4_disp[2];
    INT16                                    outRange_1to16_disp[2];     ///< outRange_1to16_disp[2];
    INT16                                    outRange_raw[2];            ///< outRange_raw[2];
    INT16                                    outRange_pdaf[2];           ///< outRange_pdaf[2];
    INT16                                    outRange_lcr[2];            ///< outRange_lcr[2];

    INT16                                    outHeight_full;             ///< outHeight_full;
    INT16                                    outHeight_disp;             ///< outHeight_disp;
    INT16                                    outImg1to4Height;           ///< outImg1to4Height;
    INT16                                    outImg4to16Height;          ///< outImg4to16Height;
    INT16                                    outImg1to4Height_disp;      ///< outImg1to4Height_disp;
    INT16                                    outImg4to16Height_disp;     ///< outImg4to16Height_disp;
    INT16                                    outHeight_fd;               ///< outHeight_fd;

    // crop amounts
    IFEStripeCrop                            outCropFDLuma;               ///< outCrop_fd_Y;
    IFEStripeCrop                            outCropFDChroma;             ///< outCrop_fd_C;
    IFEStripeCrop                            outCropVideoFullLuma;        ///< outCrop_full_Y;
    IFEStripeCrop                            outCropVideoFullChroma;      ///< outCrop_full_C;
    IFEStripeCrop                            outCropVideoDS4Luma;         ///< outCrop_1to4_Y;
    IFEStripeCrop                            outCropVideoDS4Chroma;       ///< outCrop_1to4_C;
    IFEStripeCrop                            outCropVideoDS16Luma;        ///< outCrop_1to16_Y;
    IFEStripeCrop                            outCropVideoDS16Chroma;      ///< outCrop_1to16_C;

    IFEStripeCrop                            outCropDispFullLuma;         ///< outCrop_Y_disp;
    IFEStripeCrop                            outCropDispFullChroma;       ///< outCrop_C_disp;
    IFEStripeCrop                            outCropDispDS4Luma;          ///< outCrop_1to4_Y_disp;
    IFEStripeCrop                            outCropDispDS4Chroma;        ///< outCrop_1to4_C_disp;
    IFEStripeCrop                            outCropDispDS16Luma;         ///< outCrop_1to16_Y_disp;
    IFEStripeCrop                            outCropDispDS16Chroma;       ///< outCrop_1to16_C_disp;
    IFEStripeCrop                            outCropRaw;                  ///< outCrop_raw;

    IFEStripeCrop                            preMndsCropVideoFullLuma;    ///< preMndsCrop_full_Y;
    IFEStripeCrop                            preMndsCropVideoFullChroma;  ///< preMndsCrop_full_C;
    IFEStripeCrop                            preMndsCropFDLuma;           ///< preMndsCrop_fd_Y;
    IFEStripeCrop                            preMndsCropFDChroma;         ///< preMndsCrop_fd_C;
    IFEStripeCrop                            preDS4CropVideoDS4Luma;      ///< preDS4Crop_1to4_Y;
    IFEStripeCrop                            preDS4CropVideoDS4Chroma;    ///< preDS4Crop_1to4_C;
    IFEStripeCrop                            preDS4CropVideoDS16Luma;     ///< preDS4Crop_1to16_Y;
    IFEStripeCrop                            preDS4CropVideoDS16Chroma;   ///< preDS4Crop_1to16_C;

    UINT64                                   preDSXphaseInitLuma;         ///< preDSX_phase_init_Y;
    UINT64                                   preDSXphaseInitChroma;       ///< preDSX_phase_init_C;

    IFEStripeCrop                            preMndsCropDispFullLuma;     ///< preMndsCrop_Y_disp;
    IFEStripeCrop                            preMndsCropDispFullChroma;   ///< preMndsCrop_C_disp;
    IFEStripeCrop                            preDS4CropDispDS4Luma;       ///< preDS4Crop_1to4_Y_disp;
    IFEStripeCrop                            preDS4CropDispDS4Chroma;     ///< preDS4Crop_1to4_C_disp;
    IFEStripeCrop                            preDS4CropDispDS16Luma;      ///< preDS4Crop_1to16_Y_disp;
    IFEStripeCrop                            preDS4CropDispDS16Chroma;    ///< preDS4Crop_1to16_C_disp;

    //// scalers
    IFEStripeMNScaleDownInputV16             mndsConfigFDLuma;             ///< mndsConfig_y_fd;
    IFEStripeMNScaleDownInputV16             mndsConfigFDChroma;           ///< mndsConfig_c_fd;
    IFEStripeMNScaleDownInputV20             mndsConfigFDLumav21;          ///< mndsConfig_y_fd_v21;
    IFEStripeMNScaleDownInputV20             mndsConfigFDChromav21;        ///< mndsConfig_c_fd_v21;

    IFEStripeMNScaleDownInputV16             mndsConfigVideoFullLuma;       ///< mndsConfig_y_full;
    IFEStripeMNScaleDownInputV16             mndsConfigVideoFullChroma;     ///< mndsConfig_c_full;
    IFEStripeMNScaleDownInputV20             mndsConfigVideoFullLumav21;    ///< mndsConfig_y_full_v21;
    IFEStripeMNScaleDownInputV20             mndsConfigVideoFullChromav21;  ///< mndsConfig_c_full_v21;

    IFEStripeMNScaleDownInputV16             mndsConfigDispFullLuma;        ///<  mndsConfig_y_disp;
    IFEStripeMNScaleDownInputV16             mndsConfigDispFullChroma;      ///<  mndsConfig_c_disp;
    IFEStripeMNScaleDownInputV20             mndsConfigDispFullLumav21;     ///<  mndsConfig_y_disp_v21;
    IFEStripeMNScaleDownInputV20             mndsConfigDispFullChromav21;   ///<  mndsConfig_c_disp_v21;

    IFEStripeMNScaleDownInputV16             bafDownscaler;                     ///< bafDownscaler;
    IFEStripeMNScaleDownInputV20             bafDownscalerV25;                  ///< bafDownscaler_v25;

    IFEStripeRollOffOutputParameters         rolloffOutStripe;                  ///< rolloffOutStripe;
    IFEStripePedestalOutputParameters        pedestalOutStripe;                 ///< pedestalOutStripe;

    IFEStripeHDROutputV20                    HDRout;                            ///< hdr_out;
    IFEStripeRnrStripingOutputParams         ABFOut;                            ///< abf_out;
    IFEStripePDPCv11stripingOutputParams     PDPCOut;                           ///< pdpc_out;
    IFEStripePDPCv20v30stripingOutputParams  PDPCOutv30;                        ///< pdpc_out_v30;

    IFEStripeHDRBayerHistV13OutputParams     hdrBhistOut;                       ///< hdrBhist_out;
    IFEStripeHDRBayerHistV13OutputParams     bHistOut;                          ///< bHist_out;
    IFEStripeiHistv12OutputParam             iHistOut;                          ///< iHist_out;
    IFEStripeBayerGridv15OutputParam         BGTintlessOut;                     ///< bg_tintless_out;
    IFEStripeBayerGridv15OutputParam         BGAWBOut;                          ///< bg_awb_out;
    IFEStripeBayerExpv15OutputParam          beOut;                             ///< be_out;
    IFEStriperowSumColSumv14OutputParam      rscsOut;                           ///< rscs_out;
    IFEStripePDAFv20OutputParam              pdafStatsOutput;                   ///< pdafStatsOutput;
    IFEStripeLCRv10OutputParam               lcrOutput;                         ///< lcrOutput;

    UINT16                                   numberOfBafRegions;                ///< numberOfBafRegions;
    INT16                                    dummyFieldForBytePadding;          ///< dummy_field_for_byte_padding;
    IFEStripeBayerFocusv23OutputParam        BAFOut;                            ///< baf_out;
    IFEStripeBayerFocusv25OutputParam        BAFOutv25;                         ///< baf_outv25;
    IFEStripeBayerFocusv24OutputParam        BAFOutv24;                         ///< baf_outv24;

    IFEStripeWriteEngineStripeParam          BGTintlessWriteEngineStripeParam;  ///< bg_tintlessWriteEngineStripeParam;
    IFEStripeWriteEngineStripeParam          BGAWBWriteEngineStripeParam;       ///< bg_awbWriteEngineStripeParam;
    IFEStripeWriteEngineStripeParam          BEWriteEngineStripeParam;          ///< beWriteEngineStripeParam;
    IFEStripeWriteEngineStripeParam          BAFWriteEngineStripeParam;         ///< bafWriteEngineStripeParam;
    IFEStripeWriteEngineStripeParam          rowSumWriteEngineStripeParam;      ///< rowSumWriteEngineStripeParam;
    IFEStripeWriteEngineStripeParam          colSumWriteEngineStripeParam;      ///< colSumWriteEngineStripeParam;
    IFEStripeWriteEnginePassParam            BAFWriteEnginePassParam;           ///< bafWriteEnginePassParam;
};

struct IFEStripeInterfaceInput
{
    // top level
    INT16                                   tiering;                       ///< tiering
    INT16                                   striping;                      ///< striping
    INT16                                   extendStripesForbaf;           ///< extend_for_baf

    INT16                                   inputFormat;                   ///< inputFormat;
    INT16                                   inputWidth;                    ///< inputWidth;
    INT16                                   inputHeight;                   ///< inputHeight;
    BOOL                                    useZoomSettingFromExternal;    ///< use_zoom_setting_from_external;
    IFEZoomWindow                           zoomWindow;                    ///< zoom_window;
    IFEStripeStripeROI                      roiMNDSoutfull;                ///< roi_mnds_out_full;
    IFEStripeStripeROI                      roiMNDSoutfd;                  ///< roi_mnds_out_fd;
    IFEStripeStripeROI                      roiMNDSoutdisp;                ///< roi_mnds_out_disp;

    INT16                                   videofulloutFormat;            ///< outFormat_full;
    INT16                                   videofulloutWidth;             ///< outWidth_full ;
    INT16                                   videofulloutHeight;            ///< outHeight_full;
    INT16                                   videofullDSXoutWidth;          ///< outWidth_DSX;
    INT16                                   videofullDSXoutHeight;         ///< outHeight_DSX;
    INT16                                   videofullDS4outFormat;         ///< outFormat_1to4;
    INT16                                   videofullDS16outFormat;        ///< outFormat_1to16;

    INT16                                   dispFulloutFormat;             ///< outFormat_disp;
    INT16                                   dispFulloutWidth;              ///< outWidth_disp ;
    INT16                                   dispFulloutHeight;             ///< outHeight_disp;
    INT16                                   dispfullDS4outFormat;          ///< outFormat_1to4_disp;
    INT16                                   dispfullDS16outFormat;         ///< outFormat_1to16_disp;

    INT16                                   fdOutFormat;                   ///< outFormat_fd;
    INT16                                   fdOutWidth;                    ///< outWidth_fd;
    INT16                                   fdOutHeight;                   ///< outHeight_fd;

    INT16                                   rawOutFormat;                  ///< outFormat_raw;
    INT16                                   rawOutWidth;                   ///< outWidth_raw;
    INT16                                   rawOutHeight;                  ///< outHeight_raw;

    INT16                                   PDAFOutFormat;                 ///< outFormat_pdaf;
    INT16                                   PDAFOutWidth;                  ///< outWidth_pdaf;
    INT16                                   PDAFOutHeight;                 ///< outHeight_pdaf;

    INT16                                   LCROutFormat;                  ///< outFormat_lcr
    INT16                                   LCROutWidth;                   ///< outWidth_lcr;
    INT16                                   LCROutHeight;                  ///< outHeight_lcr

    IFEStripeHDRInputV20                    HDRInput;                      ///< hdr_in;

    IFEStripeMNDSInputv16                   MNDS16InputLumaFD;       ///< mnds_in_y_fd;
    IFEStripeMNDSInputv16                   MNDS16InputChromaFD;     ///< mnds_in_c_fd;
    IFEStripeMNDSInputv16                   MNDS16InputLumaFull;     ///< mnds_in_y_full;
    IFEStripeMNDSInputv16                   MNDS16InputChromaFull;   ///< mnds_in_c_full;
    IFEStripeMNDSInputv16                   MNDS16InputLumaDisp;     ///< mnds_in_y_disp;
    IFEStripeMNDSInputv16                   MNDS16InputChromaDisp;   ///< mnds_in_c_disp;

    IFEStripeMNDSInputv16                   MNDS21InputLumaFD;       ///< mnds_in_y_fd_v21;
    IFEStripeMNDSInputv16                   MNDS21InputChromaFD;     ///< mnds_in_c_fd_v21;
    IFEStripeMNDSInputv16                   MNDS21InputLumaFull;     ///< mnds_in_y_full_v21;
    IFEStripeMNDSInputv16                   MNDS21InputChromaFull;   ///< mnds_in_c_full_v21;
    IFEStripeMNDSInputv16                   MNDS21InputLumaDisp;     ///< mnds_in_y_disp_v21;
    IFEStripeMNDSInputv16                   MNDS21InputChromaDisp;   ///< mnds_in_c_disp_v21;

    IFEStripeDSXInputV1D                    DSXInputVideoFullv10;    ///< dsx_in_full_v10;

    IFEStripePDPCv11stripingInputParams     PDPCInputV11;           ///< pdpc_in;
    IFEStripePDPCv20v30stripingInputParams  PDPCInputV30;           ///< pdpc_in_v30;

    INT16                                   pedestalEnable;         ///< pedestal_enable;
    INT16                                   rolloffEnable;          ///< rolloff_enable ;
    INT16                                   BAFEnable;              ///< baf_enable;
    INT16                                   BGTintlessEnable;       ///< bg_tintless_enable ;
    INT16                                   BGAWBEnable;            ///< bg_awb_enable ;
    INT16                                   BEEnable;               ///< be_enable ;
    INT16                                   PDAFEnable;             ///< pdaf_enable;
    INT16                                   HDREnable;              ///< hdr_enable ;
    INT16                                   BPCEnable;              ///< bpc_enable ;
    INT16                                   ABFEnable;              ///< abf_enable ;

    INT16                                   csid_pe_en;             ///< csid_pe_en;
    INT16                                   PDAFStatsEnable;        ///< pdaf_stats_enable;
    INT16                                   LCREnable;              ///< lcr_en;
    INT16                                   tappingPointBE;         ///< tappingPoint_be;

    INT16                                   tapoffPointHVX;         ///< tapoffPoint_hvx;
    INT16                                   kernelSizeHVX;          ///< kernelSize_hvx;

    INT16                                   fetchLeftStripeEnd;     ///< fetchLeftStripeEnd
    INT16                                   fetchRightStripeStart;  ///< fetchRightStripeStart

    INT16                                   dummyFieldForBytePadding;  ///< dummy_field_for_byte_padding

    IFEStripePedestalv13InputParams         pedestalParam;           ///< pedestalParam;
    IFEStripeRollOffv34InputParams          rollOffParam;            ///< rollOffParam;
    IFEStripeHDRBayerHistV13InputParams     HDRBhistInput;           ///< hdrBhist_input;
    IFEStripeHDRBayerHistV13InputParams     bHistInput;              ///< bHist_input;
    IFEStripeiHistv12InputParam             iHistInput;              ///< iHist_input;
    IFEStripeBayerGridv15InputParam         BGTintlessInput;         ///< bg_tintless_input;
    IFEStripeBayerGridv15InputParam         BGAWBInput;              ///< bg_awb_input;
    IFEStripeBayerExpv15InputParam          BEInput;                 ///< be_input;
    IFEStripeBayerFocusv23InputParam        BAFInput;                ///< baf_input;
    IFEStripeBayerFocusv24InputParam        BAFInputv24;             ///< baf_inputv24;
    IFEStripeBayerFocusv25InputParam        BAFInputv25;             ///< baf_inputv25;
    IFEStriperowSumColSumv14InputParam      RSCSInput;               ///< rscs_input;
    IFEStripePDAFv20InputParam              PDAFStatsInput;          ///< pdafStatsInput;
    IFEStripeLCRv10InputParam               LCRInput;                ///< lcrInput;
};

struct IFEStripingPassOutput
{
    INT16                           numStripes;                            ///< numStripes;
    INT16                           dummyFieldForBytePadding0;             ///< dummy_field_for_byte_padding_0;
    INT32                           dummyFieldForBytePadding1;             ///< dummy_field_for_byte_padding_1;
    IFEStripeInterfaceOutput*       pStripeOutput[2];                      ///< hStripeList - List to Array;

    IFEStripeWriteEnginePassParam*  pBGTintlessWriteEnginePassParam;       ///< bg_tintlessWriteEnginePassParam;
    IFEStripeWriteEnginePassParam*  pBGAWBWriteEnginePassParam;            ///< bg_awbWriteEnginePassParam;
    IFEStripeWriteEnginePassParam*  pBEWriteEnginePassParam;               ///< beWriteEnginePassParam;
    IFEStripeWriteEnginePassParam*  pRowSumWriteEnginePassParam;           ///< rowSumWriteEnginePassParam;
    IFEStripeWriteEnginePassParam*  pColSumWriteEnginePassParam;           ///< colSumWriteEnginePassParam;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the ifeStripingInterface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEStripingInterface
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeriveStriping
    ///
    /// @brief  DeriveStriping This API returns the Striping Output
    ///
    /// @param  pStripingInput   Striping Input
    /// @param  pPassOut         Striping Output
    ///
    /// @return INT32
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT32 DeriveStriping(
        IFEStripeInterfaceInput* pStripingInput,
        IFEStripingPassOutput*   pPassOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeriveFetchRange
    ///
    /// @brief  Derive Fetch Range. Extracts the Getch range fro the striping
    ///
    /// @param  pStripingInput   Striping Input
    ///
    /// @return INT32
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT32 DeriveFetchRange(
        IFEStripeInterfaceInput * pStripingInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEMNDSV16Config1d
    ///
    /// @brief  IFE MNDS config
    ///
    /// @param  inputFrameLength          Input Frame Length
    /// @param  MNDSOutFrameLength        MNDS Out Frame Length
    /// @param  roundingOptionHorChroma   Horizontal Rounding Option
    /// @param  roundingOptionVerChroma   Vertical rounding option
    /// @param  MNDSInputRange            MNDS input Range
    /// @param  MNDSOutputRange           MNDs output Range
    /// @param  pMNDSLumaConfig           MNDS Luma Config
    /// @param  pMNDSChromaConfig         MNDS Chroma Config
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID IFEMNDSV16Config1d(
        INT16                         inputFrameLength,
        INT16                         MNDSOutFrameLength,
        INT16                         roundingOptionHorChroma,
        INT16                         roundingOptionVerChroma,
        IFEStripeStripeROI            MNDSInputRange,
        IFEStripeStripeROI            MNDSOutputRange,
        IFEStripeMNScaleDownInputV16* pMNDSLumaConfig,
        IFEStripeMNScaleDownInputV16* pMNDSChromaConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseDualIFEPassOutput
    ///
    /// @brief  Release Moemory allocated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ReleaseDualIFEPassOutput();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFEStripingInterface
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEStripingInterface();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEStripingInterface
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEStripingInterface();

private:

    static VOID*     s_pIFEPassOut;                                   ///< Struct for IFE Passout

    IFEStripingInterface(const IFEStripingInterface&) = delete;                  ///< Disallow the copy constructor
    IFEStripingInterface& operator=(const IFEStripingInterface&) = delete;       ///< Disallow assignment operator
};

#ifdef __cplusplus
extern "C" {
#endif    // end of _cplusplus

struct IFEStripeInterface
{

    INT32 (*DeriveStriping) (
        IFEStripeInterfaceInput* pStripingInput,
        IFEStripingPassOutput*   pPassOut);

    INT32 (*DeriveFetchRange)(
        IFEStripeInterfaceInput * pStripingInput);

    VOID (*IFEMNDSV16Config1d)(
        INT16                         inputFrameLength,
        INT16                         MNDSOutFrameLength,
        INT16                         roundingOptionHorChroma,
        INT16                         roundingOptionVerChroma,
        IFEStripeStripeROI            MNDSInputRange,
        IFEStripeStripeROI            MNDSOutputRange,
        IFEStripeMNScaleDownInputV16* pMNDSLumaConfig,
        IFEStripeMNScaleDownInputV16* pMNDSChromaConfig);

    VOID (*ReleaseDualIFEPassOutput)();
};


typedef CamxResult(*PFStripeInterface) (IFEStripeInterface** pIFEStripeInterfaceCB);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CreateIFEStripeInterface
///
/// @brief  CreateIFEStripeInterface
///
/// @param  ppIFEStripeInterface   pointer to IFE stripe Interface
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult CreateIFEStripeInterface(
    IFEStripeInterface** ppIFEStripeInterface);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DestroyIFEStripeInterface
///
/// @brief  DestroyIFEStripeInterface
///
/// @param  ppIFEStripeInterface   pointer to IFE stripe Interface
///
/// @return CamxResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC CamxResult DestroyIFEStripeInterface(
    IFEStripeInterface** ppIFEStripeInterface);


#ifdef __cplusplus
}
#endif    // end of _cplusplus


CAMX_NAMESPACE_END

#endif // end of CAMX_IFE_STRIPE_INTERFACE_H
