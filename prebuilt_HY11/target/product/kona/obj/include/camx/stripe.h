#ifndef _STRIPE_H_
#define _STRIPE_H_

//-------------------------------------------------------------------------
// Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

//------------------------------------------------------------------------
// @file  stripe.h
// @brief Algorithm IPE\BPS Lower-layer
//------------------------------------------------------------------------

/*------------------------------------------------------------------------
*       Include Files
* ----------------------------------------------------------------------- */
#ifndef NDEBUG
#include <stdio.h>
#endif /* !NDEBUG */

#include "list.h"
#include "stripe_common.h"
#include "scalers.h"
#include "modules.h"


#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------
*       Definitions and Constants
* ----------------------------------------------------------------------- */

#if defined(_MSC_VER)
#define WHILE_0        while(0,0)
#else
#define WHILE_0        while(0)
#endif /*  */

#ifndef AEE_SUCCESS
#define AEE_SUCCESS 0
#endif //AEE_SUCCESS

#ifndef AEE_EFAIL
#define AEE_EFAIL 1
#endif //AEE_EFAIL

#ifndef NDEBUG
#define CSADBG_ERROR(FMT, ...) printf( "\n Camera ICP : %s : " FMT, __FUNCTION__, __VA_ARGS__ )
#define CSADBG_HIGH(FMT, ...) printf( "\n Camera ICP : %s : " FMT, __FUNCTION__, __VA_ARGS__ )
#else
#define CSADBG_ERROR(FMT, ...)
#define CSADBG_HIGH(FMT, ...)
#endif /* !NDEBUG */

#ifndef DUMP_DIM_INFO
#define DUMP_DIM_INFO 0
#endif /* DUMP_DIM_INFO */

#ifndef CHECK_FORMAT_CONSTRAINT
#define CHECK_FORMAT_CONSTRAINT 0
#endif /* CHECK_FORMAT_CONSTRAINT */

#ifndef STRIPING_LIBRARY_STRIPE_TOP_OUTPUT
/**< Possible values:
 *    0 - print to file only
 *    1 - print to terminal only
 *    2 - print to both terminal and file
 */
#define STRIPING_LIBRARY_STRIPE_TOP_OUTPUT   0
#endif /* STRIPING_LIBRARY_STRIPE_TOP_OUTPUT */

#if !((STRIPING_LIBRARY_STRIPE_TOP_OUTPUT == 0) || (STRIPING_LIBRARY_STRIPE_TOP_OUTPUT == 1) || (STRIPING_LIBRARY_STRIPE_TOP_OUTPUT == 2))
#error Invalid value for STRIPING_LIBRARY_STRIPE_TOP_OUTPUT
#endif /* STRIPING_LIBRARY_STRIPE_TOP_OUTPUT value check */

#define PROVIDE_DEFAULT_FIRMWARE_INPUT() 0

#define DIM_FNAME_SIZE      255

// performance modeling flag
#define calculate_BW
#ifndef PERF
#define PERF
#endif



#ifndef ROUND_UP
//#define ROUND_UP(value, grid) ((((value) + (grid)-1)/(grid))*(grid))
#define ROUND_UP(value, grid) (((value) >= 0)? ((((value) + (grid)-1)/(grid))*(grid)): (((value)/(grid))*(grid)))
#endif

#ifndef ROUND_DOWN
//#define ROUND_DOWN(value, grid) ((value) - ((value)%(grid)))
#define ROUND_DOWN(value, grid) (((value) >= 0)? (((value)/(grid))*(grid)):  ((((value) - (grid) + 1)/(grid))*(grid)))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(value, grid) (((value) >= 0)? (((value) + (grid)-1)/(grid)): ((value)/(grid)))
#endif

#ifndef DIV_ROUND_DOWN
#define DIV_ROUND_DOWN(value, grid) (((value) >= 0)? ((value)/(grid)):  (((value) - (grid) + 1)/(grid)))
#endif

#ifndef KERNEL_FOLD
#define KERNEL_FOLD(unfolded_kernel_size) ((unfolded_kernel_size) / 2)
#endif

#ifndef KERNEL_UNFOLD
#define KERNEL_UNFOLD(folded_kernel_size) ((folded_kernel_size) * 2)
#endif

#ifndef EXTEND_START
#define EXTEND_START ROUND_DOWN
#endif

#ifndef SHRINK_START
#define SHRINK_START ROUND_UP
#endif


#ifndef EXTEND_END
#define EXTEND_END(value, grid) (ROUND_DOWN(value, grid) + (grid-1))
#endif

#ifndef SHRINK_END
#define SHRINK_END(value, grid) (ROUND_DOWN((value)+1, grid) - 1)
#endif

#ifndef MPIX_PADDING_END
#define MPIX_PADDING_END(start, end, grid) ((start) + ROUND_UP(LENGTH((start), (end)), grid) - 1)
#endif

#ifndef IDX2POS  // Index to Position. Example: n-th indexed pixel/ stripe is (n + 1)st position of pixel/ stripe.
#define IDX2POS(index) ((index) + 1)
#endif

#ifndef POS2IDX  // Position to Index. Example: n-th position of pixel/ stripe is at (n - 1)st index.
#define POS2IDX(position) ((position) - 1)
#endif

#ifndef NEXT // Get next value. Next value of n-th pixel/ stripe is (n + 1).
#define NEXT(value) ((value) + 1)
#endif

#ifndef PREV  // Get previous value. Previous value of n-th pixel is (n - 1).
#define PREV(value) ((value) - 1)
#endif

// scaling
#define CDS       2
#define CDS_SHIFT 1
#define QRT_SHIFT 2



// TODO : Remove below definitions and replace by enums from ImageFormat
#define     IMAGE_FORMAT_ICA_MASTER_OUT         (IMAGE_FORMAT_MAX + 1)
#define     IMAGE_FORMAT_YUV444                 (IMAGE_FORMAT_ICA_MASTER_OUT + 1)       // Same as LTMDC_DUMMY_FORMAT


#define YUV420_UBWC_NV12_MINWIDTH            130
#define YUV420_UBWC_NV12_4R_MINWIDTH         130
#define YUV420_UBWC_TP10_MINWIDTH            98

#define MNDSv23_OUT_LUMA_MINWIDTH            6

#define YUV420_UBWC_NV12_CONSTRAINT_H        2
#define YUV420_UBWC_NV12_4R_CONSTRAINT_H     2
#define YUV420_UBWC_TP10_CONSTRAINT_H        6
#define PDI_CONSTRAINT_H                     10
#define YUV420_PD10_CONSTRAINT_H_READ             4
#define YUV420_PD10_CONSTRAINT_H_WRITE             8
#define TFI_CONSTRAINT_H                     2
#define BAYER_PLAIN_CONSTRAINT_H             2
#define LTMDC_DUMMY_CONSTRAINT_H             1 // TODO : not sure what is the correct symbol: YUV444 or LTMDC_DUMMY
#define YUV420_LINEAR_NV12_CONSTRAINT_H      2
#define BAYER_MIPI10_CONSTRAINT_H            4
#define BAYER_MIPI12_CONSTRAINT_H            2
#define BAYER_MIPI14_CONSTRAINT_H            4
#define YUV420_LINEAR_P010_CONSTRAINT_H      2
#define YUV420_LINEAR_TP10_CONSTRAINT_H      6
#define YUV420_PLAIN_CONSTRAINT_H            2
#define YUV422_PLAIN16_CONSTRAINT_H          2
#define YUV420_UBWC_P010_CONSTRAINT_H        2
#define YUV420_ICA_MASTER_OUT_CONSTRAINT_H   2
#define YUV422_PLAIN8_CONSTRAINT_H           2

#define YUV420_UBWC_NV12_CONSTRAINT_V        2
#define YUV420_UBWC_NV12_4R_CONSTRAINT_V     2
#define YUV420_UBWC_TP10_CONSTRAINT_V        2
#define PDI_CONSTRAINT_V                     2
#define YUV420_PD10_CONSTRAINT_V             2
#define TFI_CONSTRAINT_V                     1
#define BAYER_PLAIN_CONSTRAINT_V             2
#define LTMDC_DUMMY_CONSTRAINT_V             1 // TODO : not sure what is the correct symbol: YUV444 or LTMDC_DUMMY
#define YUV420_LINEAR_NV12_CONSTRAINT_V      2
#define BAYER_MIPI10_CONSTRAINT_V            2
#define BAYER_MIPI12_CONSTRAINT_V            2
#define BAYER_MIPI14_CONSTRAINT_V            2
#define YUV420_LINEAR_P010_CONSTRAINT_V      2
#define YUV420_LINEAR_TP10_CONSTRAINT_V      2
#define YUV420_PLAIN_CONSTRAINT_V            2
#define YUV422_PLAIN16_CONSTRAINT_V          2
#define YUV420_UBWC_P010_CONSTRAINT_V        2
#define YUV420_ICA_MASTER_OUT_CONSTRAINT_V   2
#define YUV422_PLAIN8_CONSTRAINT_V           2


/*------------------------------------------------------------------------
*       Type Declarations
* ----------------------------------------------------------------------- */
#pragma pack(push)
#pragma pack(4)

typedef struct
{
    int32_t xStart;        // in pixels
    int32_t xEnd;
    int32_t yStart;        // in lines
    int32_t yEnd;
    int32_t frameStride;   // in #of bytes
    int16_t frameFormat;
    int16_t dummy_field_for_byte_padding;
}
StripeCoordinates;

#pragma pack(pop)

#define resetCrop1D(pCrop)          memset(pCrop, 0, sizeof(*pCrop))

/*------------------------------------------------------------------------
*       External Variables
* ----------------------------------------------------------------------- */
extern const int q_factor;
extern const int q_mask;

/*------------------------------------------------------------------------
*       Function Declarations
* ----------------------------------------------------------------------- */

void alignOutData(int16_t * pStart, int16_t * pEnd, const int16_t constraint, const int16_t cummulativeWidth);

int16_t
limitLeft2(int16_t var1, int16_t var2);

int16_t
limitLeft3(int16_t var1, int16_t var2, int16_t var3);

int16_t
limitLeft4(int16_t var1, int16_t var2, int16_t var3, int16_t var4);

int16_t
limitRight2(int16_t var1, int16_t var2);

int16_t
limitRight3(int16_t var1, int16_t var2, int16_t var3);

int16_t
limitRight4(int16_t var1, int16_t var2, int16_t var3, int16_t var4);

void
clampToBuffer(int16_t * pStart, int16_t * pEnd, int16_t bufLength);

void clampToFrame(int16_t *pStart, int16_t *pEnd, int16_t frameSize);           //: to make the backward process more precise

#if 0  // TODO : remove dead code
void
applyOffsetBack(int16_t * pStart, int16_t * pEnd, int32_t offsetSize);
#endif /* 0 */

void applyOffsetForward(int16_t * pStart, int16_t * pEnd, const int32_t offsetSize);

void
applyKernelBack(int16_t * pStart, int16_t * pEnd, int16_t kernelSize);

void
scaleBack(int16_t * pStart, int16_t * pEnd, int32_t R, int16_t kernelSize);

void scaleBack_V20(int16_t * pStart,
                   int16_t * pEnd,
                   int32_t R,
                   const MNScaleDownInStruct_V20_1D * pFramecfg);

void scaleForward_V20(int16_t * pStart,
                 int16_t * pEnd,
                 int32_t R,
    const MNScaleDownInStruct_V20_1D * pFramecfg,
                 MNScaleDownInStruct_V20_1D * pStripecfg);

int16_t getFormatConstraint_MALaligned(int16_t format, BOOL horizontal);

int16_t genFormatMinOutputSize(int16_t format, BOOL horizontal);

void chroma_upsample_config(int input_l, int output_l,
    const BOOL is_horizontal, const BOOL cosited, const BOOL cosited_even, const UpscaleInStruct_V11_1D* frameCfg, ChromaUpv201dStripingOutputParams * pStripecfg);
void chromaUpScaleBack( int16_t *start, int16_t* end, const BOOL cosited, const BOOL cosited_even );
void adjustChromaUpForPostCropKernel
    (int *cropL_444, int *cropR_444, const BOOL edgeStripeLT, const BOOL edgeStripeRB, UpscaleInStruct_V11_1D * pStripecfg);
void chroma_upsample_Y_config(int input_l, int output_l,
    UpscaleInStruct_V11_1D* frameCfg, UpscaleInStruct_V11_1D * pStripecfg);

int32_t
LTMDCStripeConfiguraion(
    const uint16_t xStartIdxDCframe,   // this is the dimension after down scaler
    const int16_t xEndIdxDCframe,
    const uint16_t redundant_dc_start,
    const BOOL firstStripe,
    const uint16_t ltmDCtableOption,
    LTMV13StripeDCParams* ltmDCStripeParams
    ) ;
int32_t
LTMDCStripeConfiguration_mpix(
    const uint16_t xStartIdxDCframe,
    const int16_t xEndIdxDCframe,       // after down scaler
    const uint16_t redundant_dc_start,
    const uint16_t drop_first_output,
    const BOOL firstStripe,
    const uint16_t ltmDCtableOption,    //  option:  0->288x216,  1-> 144x108, 2 -> 72*54
    LTMV13StripeDCParams* ltmDCStripeParams
);

int32_t
LTMIPStripeConfiguraion(
        uint16_t  ip_StartIdxframe,      // reference to full frame
        BOOL  is_horizontal,             //  0: vertical  1: horizontal
        uint16_t  fullFrameSize,         // size on the same scale as 1:1 full frame,  both non-zoom & zoom cases
        LTMV13StripeIPParams_1D* ltmIpStripeParams
    ) ;

int generateLTMDCmode(const int widthin, const int heightin );
int generateLTMDCoutW( const int ds_fac );
int generateLTMDCoutH( const int ds_fac );
int genOffsetFrom2Outputs(const int offset1, const BOOL en1, const int offset2, const BOOL en2);
int16_t IsRgbFormat(const int16_t format);
int16_t IsBayerFormat(const int16_t format);

#ifdef __cplusplus
}
#endif

#endif /* _STRIPE_H_ */
