////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxjpegencconfig.cpp
/// @brief JPEGEncConfig class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-1988) Code clean up apply NC001 coding standard on entire project
/// @todo (CAMX-1989) Code clean up apply GR030 coding standard on entire project

#include "camxjpegencconfig.h"
#include "camximagebuffer.h"
#include "camximageformatutils.h"

CAMX_NAMESPACE_BEGIN

#define CEILING16(X) (((X) + 0x000F) & 0xFFF0)
#define CEILING8(X)  (((X) + 0x0007) & 0xFFF8)
#define CEILING2(X)  (((X) + 0x0001) & 0xFFFE)
#define FLOOR16(X) ((X) & 0xFFF0)
#define FLOOR8(X)  ((X) & 0xFFF8)
#define FLOAT_TO_Q(exp, f) \
    (static_cast<INT32>(((f)*(1<<(exp))) + (((f)<0) ? -0.5 : 0.5)))

static const UINT JPEG_REG_SCALE_DEFAULT_HSTEP_VSTEP  = 0x200000;
static const UINT JPEGEncBlockHeight16                = 16;
static const UINT JPEGEncBlockWidth16                 = 16;
static const UINT JPEGEncBlockHeight8                 = 8;
static const UINT JPEGEncBlockWidth8                  = 8;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult JPEGEncConfig::Create(
        JPEGEncConfig** ppJPEGEncoder)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != ppJPEGEncoder)
    {
        *ppJPEGEncoder = CAMX_NEW JPEGEncConfig();
        if (NULL == ppJPEGEncoder)
        {
            result = CamxResultENoMemory;
            CAMX_ASSERT_ALWAYS_MESSAGE("Memory allocation failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Input Null pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::GetRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* JPEGEncConfig::GetRegCmd()
{
    return &m_regCmd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEncConfig::DumpRegConfig() const
{
    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "jpegEncCoreCfg [0x%x]", m_regCmd.jpegEncCoreCfg);
    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "jpegEncIRQMask [0x%x]", m_regCmd.jpegEncIRQMask);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::JPEGEncConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGEncConfig::JPEGEncConfig()
{
    m_moduleEnable = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncCoreCFG
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncCoreCFG(
    JPEGInputData* pInputData)
{
    m_regCmd.jpegEncCoreCfg.u32All =
        ((JPEG_ENCODE_JPEG_0_CORE_CFG_MODE_OFFLINE_JPEG_ENCODE << JPEG_ENCODE_JPEG_0_CORE_CFG_MODE_SHIFT) |
        (JPEG_ENCODE_JPEG_0_CORE_CFG_TESTBUS_ENABLE_TESTBUS_ENABLED << JPEG_ENCODE_JPEG_0_CORE_CFG_TESTBUS_ENABLE_SHIFT) |
        (static_cast<UINT32>(pInputData->scaleConfig.bScaleEnabled) << JPEG_ENCODE_JPEG_0_CORE_CFG_SCALE_ENABLE_SHIFT) |
        (JPEG_ENCODE_JPEG_0_CORE_CFG_JPEG_ENCODE_ENABLE_JPEG_ENCODE_ENABLED <<
            JPEG_ENCODE_JPEG_0_CORE_CFG_JPEG_ENCODE_ENABLE_SHIFT) |
        (JPEG_ENCODE_JPEG_0_CORE_CFG_WE_ENABLE_WRITE_ENGINE_ENABLED << JPEG_ENCODE_JPEG_0_CORE_CFG_WE_ENABLE_SHIFT) |
        (JPEG_ENCODE_JPEG_0_CORE_CFG_FE_ENABLE_FETCH_ENGINE_ENABLED << JPEG_ENCODE_JPEG_0_CORE_CFG_FE_ENABLE_SHIFT));

    m_regCmd.jpegEncIRQMask.u32All = 0x6455ca9c;


    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncFEConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncFEConfig(
    JPEGInputData* pInputData)
{
    switch (pInputData->inFormat.format)
    {
        case Format::YUV420NV12:
            m_regCmd.jpegEncFECfg.bitfields.CBCR_ORDER = 0;
            break;

        case Format::YUV420NV21:
            m_regCmd.jpegEncFECfg.bitfields.CBCR_ORDER = 1;
            break;

        default:
            m_regCmd.jpegEncFECfg.bitfields.CBCR_ORDER = 0;
    }
    m_regCmd.jpegEncFECfg.bitfields.BYTE_ORDERING = 0x0;
    m_regCmd.jpegEncFECfg.bitfields.BURST_LENGTH_MAX = 0x7;
    switch (pInputData->inFormat.format)
    {
        case Format::Y8:
            m_regCmd.jpegEncFECfg.bitfields.MEMORY_FORMAT = JPEG_ENCODE_JPEG_0_FE_CFG_MEMORY_FORMAT_MONOCHROME;
            m_regCmd.jpegEncFECfg.bitfields.PLN0_EN = 0x1;
            pInputData->numInputPlanes = 1;
            pInputData->color_format = JPEG_ENCODE_JPEG_0_ENCODE_CFG_IMAGE_FORMAT_MONOCHROME;
            break;

        case Format::YUV420NV12:
        case Format::YUV420NV21:
            m_regCmd.jpegEncFECfg.bitfields.MEMORY_FORMAT = JPEG_ENCODE_JPEG_0_FE_CFG_MEMORY_FORMAT_PSEUDO_PLANAR;
            m_regCmd.jpegEncFECfg.bitfields.PLN0_EN = 0x1;
            m_regCmd.jpegEncFECfg.bitfields.PLN1_EN = 0x1;
            pInputData->numInputPlanes = 2;
            pInputData->color_format = JPEG_ENCODE_JPEG_0_ENCODE_CFG_IMAGE_FORMAT_H2V2;
            break;

        case Format::YUV422NV16:
            m_regCmd.jpegEncFECfg.bitfields.MEMORY_FORMAT = JPEG_ENCODE_JPEG_0_FE_CFG_MEMORY_FORMAT_PLANAR;
            m_regCmd.jpegEncFECfg.bitfields.PLN0_EN = 0x1;
            m_regCmd.jpegEncFECfg.bitfields.PLN1_EN = 0x1;
            m_regCmd.jpegEncFECfg.bitfields.PLN2_EN = 0x1;
            pInputData->numInputPlanes = 3;
            pInputData->color_format = JPEG_ENCODE_JPEG_0_ENCODE_CFG_IMAGE_FORMAT_H2V1;
            break;

        default:
            m_regCmd.jpegEncFECfg.bitfields.MEMORY_FORMAT = 0;
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid Format : %d\n", pInputData->inFormat.format);
    }
    m_regCmd.jpegEncFECfg.bitfields.BOTTOM_VPAD_EN = 0x1;
    m_regCmd.jpegEncFECfg.bitfields.SIXTEEN_MCU_EN = 0x0;
    // m_regCmd.jpegEncFECfg.bitfields.SIXTEEN_MCU_EN = 0x1;// suraj try this fix color shift
    m_regCmd.jpegEncFECfg.bitfields.MCUS_PER_BLOCK = 0x0;
    m_regCmd.jpegEncFECfg.bitfields.MAL_BOUNDARY = 0x2;

    m_regCmd.jpegEncFECfg.bitfields.MAL_EN = 0x1;
    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "FE_CFG : 0x%0x\n", m_regCmd.jpegEncFECfg.u32All);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncWEConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncWEConfig(
    JPEGInputData* pInputData)
{
    m_regCmd.jpegEnc_WE_CFG.bitfields.BYTE_ORDERING = 0;
    m_regCmd.jpegEnc_WE_CFG.bitfields.BURST_LENGTH_MAX = 3;
    m_regCmd.jpegEnc_WE_CFG.bitfields.MEMORY_FORMAT = JPEG_ENCODE_JPEG_0_WE_CFG_MEMORY_FORMAT_PLANAR;
    switch (pInputData->inFormat.format)
    {
        case Format::YUV420NV12:
            m_regCmd.jpegEnc_WE_CFG.bitfields.CBCR_ORDER = 0;
            break;

        case Format::YUV420NV21:
            m_regCmd.jpegEnc_WE_CFG.bitfields.CBCR_ORDER = 1;
            break;

        case Format::YUV422NV16:
            m_regCmd.jpegEnc_WE_CFG.bitfields.CBCR_ORDER = 1;
            break;

        case Format::Y8:
            m_regCmd.jpegEnc_WE_CFG.bitfields.CBCR_ORDER = 0;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid cbcr format : %d\n", pInputData->inFormat.format);
    }

    m_regCmd.jpegEnc_WE_CFG.bitfields.CBCR_ORDER = 1;
    m_regCmd.jpegEnc_WE_CFG.bitfields.PLN0_EN = 1;
    m_regCmd.jpegEnc_WE_CFG.bitfields.MAL_BOUNDARY = JPEG_ENCODE_JPEG_0_WE_CFG_MAL_BOUNDARY_MAL_1024_BYTES;
    m_regCmd.jpegEnc_WE_CFG.bitfields.MAL_EN = 0;
    m_regCmd.jpegEnc_WE_CFG.bitfields.POP_BUFF_ON_EOS = 1;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncDefaultScaleConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncDefaultScaleConfig(
    JPEGInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);

    m_regCmd.jpegEnc_SCALE_PLN0_HSTEP.u32All = JPEG_REG_SCALE_DEFAULT_HSTEP_VSTEP;
    m_regCmd.jpegEnc_SCALE_PLN1_HSTEP.u32All = JPEG_REG_SCALE_DEFAULT_HSTEP_VSTEP;
    m_regCmd.jpegEnc_SCALE_PLN2_HSTEP.u32All = JPEG_REG_SCALE_DEFAULT_HSTEP_VSTEP;
    m_regCmd.jpegEnc_SCALE_PLN0_VSTEP.u32All = JPEG_REG_SCALE_DEFAULT_HSTEP_VSTEP;
    m_regCmd.jpegEnc_SCALE_PLN1_VSTEP.u32All = JPEG_REG_SCALE_DEFAULT_HSTEP_VSTEP;
    m_regCmd.jpegEnc_SCALE_PLN2_VSTEP.u32All = JPEG_REG_SCALE_DEFAULT_HSTEP_VSTEP;

    return CamxResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncScaleConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncScaleConfig(
    JPEGInputData* pInputData)
{
    UINT8  horUpscale        = 0;
    UINT8  vertUpscale       = 0;
    UINT8  horDownscale      = 0;
    UINT8  vertDownscale     = 0;
    UINT32 blockHeight       = 0;
    UINT32 blockWidth        = 0;
    UINT32 horScaleRatio     = 0;
    UINT32 vertScaleRatio    = 0;

    /* Cropping is always done before scaling. If cropping is enables
    the input to the scaler is the cropped image. Use the cropped
    image size as input to the scalar */
    if (pInputData->scaleConfig.cropWidth < pInputData->scaleConfig.outputWidth)
    {
        horUpscale = 1;
    }
    else if (pInputData->scaleConfig.cropWidth > pInputData->scaleConfig.outputWidth)
    {
        horDownscale = 1;
    }

    if (pInputData->scaleConfig.cropHeight < pInputData->scaleConfig.outputHeight)
    {
        vertUpscale = 1;
    }
    else if (pInputData->scaleConfig.cropHeight > pInputData->scaleConfig.outputHeight)
    {
        vertDownscale = 1;
    }

    if (0 != vertUpscale)
    {
        m_regCmd.jpegEncSCALE_CFG.bitfields.V_SCALE_FIR_ALGORITHM =
            JPEG_ENCODE_JPEG_0_SCALE_CFG_V_SCALE_FIR_ALGORITHM_BI_CUBIC;
        m_regCmd.jpegEncSCALE_CFG.bitfields.VSCALE_ALGORITHM =
            JPEG_ENCODE_JPEG_0_SCALE_CFG_VSCALE_ALGORITHM_FIR_UPSCALE;

        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "jpege_lib_hw_scale_cfg: vert_upscale true reg_val = %x\n",
            m_regCmd.jpegEncSCALE_CFG.u32All);
    }

    if (0 != horUpscale)
    {
        m_regCmd.jpegEncSCALE_CFG.bitfields.H_SCALE_FIR_ALGORITHM =
            JPEG_ENCODE_JPEG_0_SCALE_CFG_H_SCALE_FIR_ALGORITHM_BI_CUBIC;
        m_regCmd.jpegEncSCALE_CFG.bitfields.HSCALE_ALGORITHM =
            JPEG_ENCODE_JPEG_0_SCALE_CFG_HSCALE_ALGORITHM_FIR_UPSCALE;;

        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "jpege_lib_hw_scale_cfg: hor_upscale true reg_val = %x\n",
            m_regCmd.jpegEncSCALE_CFG.u32All);
    }

    if ((1 == horUpscale) || (1 == horDownscale))
    {
        m_regCmd.jpegEncSCALE_CFG.bitfields.HSCALE_ENABLE = 1;
    }

    if ((1 == vertUpscale) || (1 == vertDownscale))
    {
        m_regCmd.jpegEncSCALE_CFG.bitfields.VSCALE_ENABLE = 1;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "jpege_lib_hw_scale_cfg : scale cfg value  %x\n", m_regCmd.jpegEncSCALE_CFG.u32All);

    // lib_hw_scale_reg_cfg
    switch (pInputData->inFormat.format)
    {
        case Format::YUV420NV12:
        case Format::YUV420NV21:
            //  case JPEGE_INPUT_H2V2:
            blockHeight = JPEGEncBlockHeight16;
            blockWidth  = JPEGEncBlockWidth16;
            break;
        case Format::YUV422NV16:
            // case JPEGE_INPUT_H2V1:
            blockHeight = JPEGEncBlockHeight8;
            blockWidth  = JPEGEncBlockWidth16;
            break;
        case Format::Y8:
            blockHeight = JPEGEncBlockHeight8;
            blockWidth  = JPEGEncBlockWidth8;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid Color Format : %d\n", pInputData->inFormat.format);
    }

    m_regCmd.jpegEncSCALE_PLN0_OUTPUT_CFG.bitfields.BLOCK_HEIGHT = (blockHeight - 1);
    m_regCmd.jpegEncSCALE_PLN0_OUTPUT_CFG.bitfields.BLOCK_WIDTH  = (blockWidth - 1);
    /* for PLN1 and PLN2  block width height always 8 */
    blockHeight = JPEGEncBlockHeight8;
    blockWidth  = JPEGEncBlockWidth8;
    m_regCmd.jpegEncSCALE_PLN1_OUTPUT_CFG.bitfields.BLOCK_HEIGHT = (blockHeight - 1);
    m_regCmd.jpegEncSCALE_PLN1_OUTPUT_CFG.bitfields.BLOCK_WIDTH  = (blockWidth - 1);
    m_regCmd.jpegEncSCALE_PLN2_OUTPUT_CFG.bitfields.BLOCK_HEIGHT = (blockHeight - 1);
    m_regCmd.jpegEncSCALE_PLN2_OUTPUT_CFG.bitfields.BLOCK_WIDTH  = (blockWidth - 1);

    if ((0 < pInputData->scaleConfig.outputWidth) && (0 < pInputData->scaleConfig.outputHeight))
    {
        horScaleRatio = static_cast<UINT32>(FLOAT_TO_Q(21,
                                                        static_cast<DOUBLE>(pInputData->scaleConfig.cropWidth) /
                                                        static_cast<DOUBLE>(pInputData->scaleConfig.outputWidth)));
        vertScaleRatio = static_cast<UINT32>(FLOAT_TO_Q(21,
                                                        static_cast<DOUBLE>(pInputData->scaleConfig.cropHeight) /
                                                        static_cast<DOUBLE>(pInputData->scaleConfig.outputHeight)));
    }

    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Scale horScaleRatio = %x, vertScaleRatio =%x\n", horScaleRatio,
        vertScaleRatio);

    m_regCmd.jpegEnc_SCALE_PLN0_HSTEP.u32All = horScaleRatio;
    m_regCmd.jpegEnc_SCALE_PLN1_HSTEP.u32All = horScaleRatio;
    m_regCmd.jpegEnc_SCALE_PLN2_HSTEP.u32All = horScaleRatio;
    m_regCmd.jpegEnc_SCALE_PLN0_VSTEP.u32All = vertScaleRatio;
    m_regCmd.jpegEnc_SCALE_PLN1_VSTEP.u32All = vertScaleRatio;
    m_regCmd.jpegEnc_SCALE_PLN2_VSTEP.u32All = vertScaleRatio;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncCropConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncCropConfig(
    JPEGInputData* pInputData)
{
    UINT32 pln1HOffset = 0;
    UINT32 pln1VOffset = 0;

    switch (pInputData->inFormat.format)
    {
        case Format::YUV420NV12:
        case Format::YUV420NV21:
            // case JPEGE_INPUT_H2V2:
            pln1HOffset = static_cast<UINT32>(floor(pInputData->scaleConfig.hOffset / 2));
            pln1VOffset = static_cast<UINT32>(floor(pInputData->scaleConfig.vOffset / 2));
            break;
        case Format::YUV422NV16:
            // case JPEGE_INPUT_H2V1:
            pln1HOffset = static_cast<UINT32>(floor(pInputData->scaleConfig.hOffset / 2));
            pln1VOffset = pInputData->scaleConfig.vOffset;
            break;
        case Format::Y8:
            pln1HOffset = static_cast<UINT32>(floor(pInputData->scaleConfig.hOffset / 2));
            pln1VOffset = pInputData->scaleConfig.vOffset;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid Color Format : %d\n", pInputData->inFormat.format);

    }

    m_regCmd.jpegEnc_PLN0_RD_HINIT_INT.u32All = pInputData->scaleConfig.hOffset;
    m_regCmd.jpegEnc_PLN1_RD_HINIT_INT.u32All = pln1HOffset;
    m_regCmd.jpegEnc_PLN2_RD_HINIT_INT.u32All = pln1HOffset;
    m_regCmd.jpegEnc_PLN0_RD_VINIT_INT.u32All = pInputData->scaleConfig.vOffset;
    m_regCmd.jpegEnc_PLN1_RD_VINIT_INT.u32All = pln1VOffset;
    m_regCmd.jpegEnc_PLN2_RD_VINIT_INT.u32All = pln1VOffset;
    return CamxResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncFEBufConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncFEBufConfig(
    JPEGInputData* pInputData)
{
    //     CamxResult result = CamxResultSuccess;
    UINT32 imageWidth = 0;
    UINT32 imageHeight = 0;
    UINT32 chromaWidth = 0;
    UINT32 chromaHeight = 0;
    //     INT32 i = 0;
    UINT32 yStride;
    UINT32 cStride;

    /* Set only valid area when scaler is enabled */
    if (TRUE == pInputData->scaleConfig.bScaleEnabled)
    {
        imageWidth = pInputData->scaleConfig.inputWidth + pInputData->scaleConfig.hOffset;
        imageHeight = pInputData->scaleConfig.inputHeight + pInputData->scaleConfig.vOffset;
    }
    else
    {
        imageWidth = pInputData->inFormat.width;
        imageHeight = pInputData->inFormat.height;
    }

    if (pInputData->inFormat.formatParams.yuvFormat[0].planeStride > pInputData->inFormat.width)
    {
        yStride = pInputData->inFormat.formatParams.yuvFormat[0].planeStride;
        cStride = pInputData->inFormat.formatParams.yuvFormat[0].planeStride;
    }
    else
    {
        yStride = pInputData->inFormat.width;
        cStride = pInputData->inFormat.width;
    }
    //  p_input_cfg->image_width = p_input_cfg->stride;

    m_regCmd.jpegEncPLN0RDBUFFERSIZE.bitfields.WIDTH = (imageWidth - 1);
    m_regCmd.jpegEncPLN0RDBUFFERSIZE.bitfields.HEIGHT = (imageHeight - 1);

    m_regCmd.jpegEncPLN0RDSTRIDE.bitfields.STRIDE = yStride;

    switch (pInputData->inFormat.format)
    {
            // case JPEGE_INPUT_H2V1:
        case Format::YUV422NV16:
            chromaWidth = imageWidth;
            chromaHeight = imageHeight;
            break;
        // case JPEGE_INPUT_H2V2:
        case Format::YUV420NV12:
        case Format::YUV420NV21:
            chromaWidth = imageWidth;
            chromaHeight = static_cast<UINT32>(floor(imageHeight / 2));
            break;
        case Format::Y8:
            chromaWidth = 0;
            chromaHeight = 0;
            cStride = 0;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid Color format = %d\n", pInputData->inFormat.format);
            return CamxResultSuccess;
    }
    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "chromaWidth %d chromaHeight %d", chromaWidth, chromaHeight);

    if (pInputData->numInputPlanes == 3)
    {
        chromaHeight = static_cast<UINT32>(floor(chromaHeight));
        chromaWidth = static_cast<UINT32>(floor(chromaWidth / 2));
        cStride /= 2;
    }

    m_regCmd.jpegEncPLN1RDBUFFERSIZE.bitfields.WIDTH = (chromaWidth - 1);
    m_regCmd.jpegEncPLN1RDBUFFERSIZE.bitfields.HEIGHT = (chromaHeight - 1);

    m_regCmd.jpegEncPLN1RDSTRIDE.bitfields.STRIDE = cStride;

    m_regCmd.jpegEncPLNRDBUFFERSIZE.bitfields.WIDTH = (chromaWidth - 1);
    m_regCmd.jpegEncPLNRDBUFFERSIZE.bitfields.HEIGHT = (chromaHeight - 1);

    m_regCmd.jpegEncPLN2RDSTRIDE.bitfields.STRIDE = cStride;

    m_regCmd.jpegEncPLN0RDHINIT.u32All = 0x0;
    m_regCmd.jpegEncPLN1RDHINIT.u32All = 0x0;
    m_regCmd.jpegEncPLN2RDHINIT.u32All = 0x0;
    m_regCmd.jpegEncPLN0RDVINIT.u32All = 0x0;
    m_regCmd.jpegEncPLN1RDVINIT.u32All = 0x0;
    m_regCmd.jpegEncPLN2RDVINIT.u32All = 0x0;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncQuantizationTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncQuantizationTable(
    JPEGInputData* const pInputData,
    QuantTableType tableType)
{
    CamxResult  result              = CamxResultSuccess;
    UINT16      tempValue           = 0;
    UINT32      inverseValue        = 0;
    UINT16*     pQuantizationTable  = NULL;

    if ((NULL == pInputData) || (NULL == pInputData->pQuantTables))
    {
        if (NULL == pInputData)
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "invalid input pointer %p", pInputData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "invalid pointer to tables %p", pInputData->pQuantTables);
        }
        result = CamxResultEInvalidPointer;
    }

    if (QuantTableType::QuantTableMax < tableType)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid table type %d", static_cast<UINT32>(tableType));
        result = CamxResultEOutOfBounds;
    }

    if (CamxResultSuccess == result)
    {
        pQuantizationTable = pInputData->pQuantTables[static_cast<UINT32>(tableType)].GetTable();
        if (NULL == pQuantizationTable)
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "invalid pointer %p type %d", pQuantizationTable, static_cast<UINT32>(tableType));
            result = CamxResultEInvalidPointer;
        }
    }

    if (CamxResultSuccess == result)
    {
        for (UINT32 i = 0; i < QuantTableSize; i++)
        {
            tempValue = pQuantizationTable[i];
            CAMX_LOG_VERBOSE(CamxLogGroupJPEG,
                             "Quantization table type %d index %d val %d",
                             static_cast<UINT32>(tableType),
                             i,
                             tempValue);

            // since 1/Q value is an unsigned Q16 value in hardware
            // set 1/Q to max for the case of zero (ref SWI section 2.2.7)
            if (0 == tempValue || 1 == tempValue)
            {
                inverseValue = MaxJPEGDMIData;
            }
            else
            {
                // notes: no floating point can be used notes: this equation is from hw team for 1/Q
                inverseValue = static_cast<UINT32>(0x10000 / tempValue);
            }

            switch (tableType)
            {
                case QuantTableType::QuantTableLuma:
                    m_regCmd.jpegEnc_DMI_QUANTIZATION_LUMA_DATA[i].u32All = inverseValue;
                    break;
                case QuantTableType::QuantTableChroma:
                    m_regCmd.jpegEnc_DMI_QUANTIZATION_CHROMA_DATA[i].u32All = inverseValue;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "invalid type %d", static_cast<UINT32>(tableType));
                    result = CamxResultEInvalidArg;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LookupWeBufHeight
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 LookupWeBufHeight(
    UINT32 bufSize)
{
    UINT32 hwBufHeight = 0;
    UINT32 i;
    UINT32 lBufferRangeTbl[] =
    {
        1,
        131072,
        262144,
        524288,
        1048576,
        2097152,
        4194304,
        8388608,
        1677216,
        33554432,
        67108864,
        134217728,
        268435456,
        536870912,
        1073741824,
    };

    for (i = 0; i < ((sizeof(lBufferRangeTbl) / sizeof(UINT32)) - 1); i++)
    {
        if (bufSize > lBufferRangeTbl[i] &&
            bufSize < (lBufferRangeTbl[i + 1] - 1))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "selected index = %d ", i);

            hwBufHeight = static_cast<UINT32>((pow(2, i + 2)));
            break;
        }
    }

    if (hwBufHeight == 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Error, work buf not within valid range");
    }
    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Returning = %d", hwBufHeight);

    return hwBufHeight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncWEBufConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncWEBufConfig(
    JPEGInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    UINT32 hwBufHeight = 0;
    UINT32 inHwBufSize = ((pInputData->scaleConfig.outputHeight * pInputData->scaleConfig.outputWidth * 3) / 2);

    hwBufHeight = LookupWeBufHeight(inHwBufSize);
    m_regCmd.jpegEnc_PLN0_WR_BUFFER_SIZE.u32All = inHwBufSize;
    if (0 != hwBufHeight)
    {
        m_regCmd.jpegEnc_PLN0_WR_STRIDE.bitfields.STRIDE = inHwBufSize / hwBufHeight;
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Error, hwBufHeight is zero\n");
    }
    //  0x00007800;   bufsize/height index 2^(6+2) 256
    m_regCmd.jpegEnc_PLN0_WR_HINIT.u32All = 0x0;
    m_regCmd.jpegEnc_PLN0_WR_VINIT.u32All = 0x0;

    m_regCmd.jpegEnc_PLN0_WR_HSTEP.u32All = inHwBufSize / hwBufHeight;
    // 0x00007800;// based on 8998 test // 0x00007080; //  0x0;
    m_regCmd.jpegEnc_PLN0_WR_VSTEP.u32All = hwBufHeight;
    // 256 //  0x00000010; // from test log for 640 480  try 020 also
    m_regCmd.jpegEnc_PLN0_WR_BLOCK_CFG.bitfields.BLOCKS_PER_ROW = CEILING16(pInputData->scaleConfig.outputWidth / 128);
    m_regCmd.jpegEnc_PLN0_WR_BLOCK_CFG.bitfields.BLOCKS_PER_COL = CEILING16(pInputData->scaleConfig.outputHeight / 16);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncEncodeConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncEncodeConfig(
    JPEGInputData* pInputData)
{
    CamxResult result      = CamxResultSuccess;

    UINT32 encodedWidth    = 0;
    UINT32 encodedHeight   = 0;
    UINT32 paddedHeight    = 0;
    UINT32 paddedWidth     = 0;
    UINT32 horSubsampling  = 0;
    UINT32 vertSubsampling = 0;

    m_regCmd.jpegEnc_ENCODE_CFG.bitfields.IMAGE_FORMAT = pInputData->color_format;
    CAMX_LOG_INFO(CamxLogGroupJPEG, "width = %d, height = %d\n", pInputData->scaleConfig.outputWidth,
        pInputData->scaleConfig.outputHeight);

    m_regCmd.jpegEnc_ENCODE_CFG.bitfields.APPLY_EOI = 1; // ENABLED

    CAMX_LOG_INFO(CamxLogGroupJPEG, "InputFormat =%d, Encode cfg = %x\n",
        pInputData->color_format, m_regCmd.jpegEnc_ENCODE_CFG.u32All);

    /// Encoded image size is the image size in MCUs after scaling if scaling is
    /// enabled or image size in MCUs after crop if cropping is enabled without
    /// scaling or input size in MCUs otherwise

    switch (pInputData->inFormat.format)
    {
        case Format::YUV420NV12:
        case Format::YUV420NV21:
            horSubsampling = 2;
            vertSubsampling = 2;
            paddedHeight = CEILING16(pInputData->scaleConfig.outputHeight);
            paddedWidth = CEILING16(pInputData->scaleConfig.outputWidth);
            break;
        case Format::YUV422NV16:
            horSubsampling = 2;
            vertSubsampling = 1;
            paddedHeight = CEILING8(pInputData->scaleConfig.outputHeight);
            paddedWidth = CEILING16(pInputData->scaleConfig.outputWidth);
            break;
        case Format::Y8:
            horSubsampling = 1;
            vertSubsampling = 1;
            paddedHeight = CEILING8(pInputData->scaleConfig.outputHeight);
            paddedWidth = CEILING8(pInputData->scaleConfig.outputWidth);
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid Color Format : %d\n", pInputData->inFormat.format);

    }
    if ((0 != vertSubsampling) && (0 != horSubsampling))
    {
        encodedHeight = static_cast<UINT32>((paddedHeight / (8 * vertSubsampling)) - 1);
        encodedWidth = static_cast<UINT32>((paddedWidth / (8 * horSubsampling)) - 1);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Error, vertSubsampling : %d, horSubsampling : %d\n",
            vertSubsampling, horSubsampling);
        result = CamxResultEFailed;
    }
    CAMX_LOG_INFO(CamxLogGroupJPEG, "padded_width = %d, padded_height = %d\n", paddedWidth,
        paddedHeight);
    CAMX_LOG_INFO(CamxLogGroupJPEG, "encoded_width = %d, encoded_height = %d\n",
        encodedWidth, encodedHeight);

    m_regCmd.jpegEnc_ENCODE_IMAGE_SIZE.bitfields.ENCODE_IMAGE_HEIGHT = encodedHeight;
    m_regCmd.jpegEnc_ENCODE_IMAGE_SIZE.bitfields.ENCODE_IMAGE_WIDTH = encodedWidth;

    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "nRegVal = %x\n", m_regCmd.jpegEnc_ENCODE_IMAGE_SIZE.u32All);

    // VBPad - same as encoded height
    m_regCmd.jpegEnc_FE_VBPAD_CFG.bitfields.BLOCK_ROW = encodedHeight;

    m_regCmd.jpegEnc_ENCODE_FSC_REGION_SIZE.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_0.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_1.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_2.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_3.u32All = 0x0;
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncEncoderState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncEncoderState(
    JPEGInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);
    m_regCmd.jpegEnc_ENCODE_PREDICTION_Y_STATE.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_PREDICTION_C_STATE.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_RSM_STATE.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_PACKER_STATE.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD0_STATE.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD1_STATE.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD2_STATE.u32All = 0x0;
    m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD3_STATE.u32All = 0x0;

    return CamxResultSuccess;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncFEPingUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncFEPingUpdate(
    JPEGInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);

    m_regCmd.jpegEnc_IRQ_MASK.u32All = 0xFFFFFFFF;

    m_regCmd.jpegEnc_CMD.bitfields.CLEAR_RD_PLN0_QUEUE = 1;
    m_regCmd.jpegEnc_CMD.bitfields.CLEAR_RD_PLN1_QUEUE = 1;
    m_regCmd.jpegEnc_CMD.bitfields.CLEAR_RD_PLN2_QUEUE = 1;
    m_regCmd.jpegEnc_CMD.bitfields.CLEAR_WR_PLN0_QUEUE = 1;
    m_regCmd.jpegEnc_CMD.bitfields.CLEAR_WR_PLN1_QUEUE = 1;
    m_regCmd.jpegEnc_CMD.bitfields.CLEAR_WR_PLN2_QUEUE = 1;

    m_regCmd.jpegEnc_PLN0_RD_OFFSET.u32All = 0x0;
    // add patching here . y_buffer_addr
    m_regCmd.jpegEnc_PLN0_RD_PNTR.u32All = 0x0;

    m_regCmd.jpegEnc_PLN1_RD_OFFSET.u32All = 0x0;
    // add patching here .  cbcr_buffer_addr
    m_regCmd.jpegEnc_PLN1_RD_PNTR.u32All = 0x0;

    m_regCmd.jpegEnc_PLN2_RD_OFFSET.u32All = 0x0;
    // add patching here .  plane 2 (actually 3rd plane) _buffer_addr
    m_regCmd.jpegEnc_PLN2_RD_PNTR.u32All = 0x0;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::FillEncWEPingUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::FillEncWEPingUpdate(
    JPEGInputData* pInputData)
{
    CAMX_UNREFERENCED_PARAM(pInputData);

    m_regCmd.jpegEnc_PLN0_WR_PNTR.u32All = 0;
    m_regCmd.jpegEnc_PLN1_WR_PNTR.u32All = 0;
    m_regCmd.jpegEnc_PLN2_WR_PNTR.u32All = 0;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::UpdateCmdConfigsFromInputData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::UpdateCmdConfigsFromInputData(
    JPEGInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pInputData)
    {
        result = CamxResultEInvalidPointer;
        return result;
    }

    if (pInputData->scaleCropReq.bScaleEnabled)
    {
        // Scaling and cropping are treated as separate operations
        // Check if scaling is really enabled
        /* when crop not enabled crop dim = input dim*/
        if (pInputData->scaleCropReq.cropWidth < pInputData->scaleCropReq.inputWidth ||
            pInputData->scaleCropReq.cropHeight < pInputData->scaleCropReq.inputHeight )
        {
            pInputData->scaleConfig.bCropEnabled = TRUE;
        }

        if (pInputData->scaleCropReq.cropWidth < pInputData->scaleCropReq.outputWidth ||
            pInputData->scaleCropReq.cropWidth > pInputData->scaleCropReq.outputWidth ||
            pInputData->scaleCropReq.cropHeight < pInputData->scaleCropReq.outputHeight ||
            pInputData->scaleCropReq.cropHeight > pInputData->scaleCropReq.outputHeight)
        {
            pInputData->scaleConfig.bScaleEnabled = TRUE;
        }

        pInputData->scaleConfig.outputWidth     = pInputData->scaleCropReq.outputWidth;
        pInputData->scaleConfig.outputHeight    = pInputData->scaleCropReq.outputHeight;
        pInputData->scaleConfig.inputWidth      = pInputData->scaleCropReq.inputWidth;
        pInputData->scaleConfig.inputHeight     = pInputData->scaleCropReq.inputHeight;
        pInputData->scaleConfig.hOffset         = pInputData->scaleCropReq.hOffset;
        pInputData->scaleConfig.vOffset         = pInputData->scaleCropReq.vOffset;
        pInputData->scaleConfig.cropWidth       = pInputData->scaleCropReq.cropWidth;
        pInputData->scaleConfig.cropHeight      = pInputData->scaleCropReq.cropHeight;
    }
    else
    {
        pInputData->scaleConfig.bScaleEnabled = FALSE;
        pInputData->scaleConfig.bCropEnabled  = FALSE;
        pInputData->scaleConfig.inputWidth    = pInputData->inFormat.width;
        pInputData->scaleConfig.inputHeight   = pInputData->inFormat.height;
        pInputData->scaleConfig.hOffset       = 0;
        pInputData->scaleConfig.vOffset       = 0;
        pInputData->scaleConfig.outputWidth   = pInputData->outFormat.width;
        pInputData->scaleConfig.outputHeight  = pInputData->outFormat.height;
    }

    Utils::Memset(&m_regCmd, 0, sizeof(JPEGEncoderRegCmd));

    FillEncCoreCFG(pInputData);

    FillEncFEConfig(pInputData);

    FillEncFEBufConfig(pInputData);

    FillEncWEConfig(pInputData);

    FillEncWEBufConfig(pInputData);

    FillEncEncodeConfig(pInputData);

    if (TRUE == pInputData->scaleConfig.bCropEnabled)
    {
        FillEncCropConfig(pInputData);
    }

    if (TRUE == pInputData->scaleConfig.bScaleEnabled)
    {
        FillEncScaleConfig(pInputData);
    }
    else
    {
        FillEncDefaultScaleConfig(pInputData);
    }

    FillEncEncoderState(pInputData);

    FillEncFEPingUpdate(pInputData);

    FillEncWEPingUpdate(pInputData);

    FillEncQuantizationTable(pInputData, QuantTableType::QuantTableLuma);

    FillEncQuantizationTable(pInputData, QuantTableType::QuantTableChroma);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::CreateCmdList(
    CmdBuffer*      pCmdBuffer,
    JPEGInputData*  pInputData)
{
    JPEGEncoderRegNumValSet regNumVal[MaxJPEGEncoderRegisters];
    UINT32                  i           = 0;
    UINT32                  cmdCount    = 0;
    CamxResult              result      = CamxResultSuccess;

    if (NULL != pCmdBuffer)
    {
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_CORE_CFG;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncCoreCfg) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncCoreCfg);
        i++;

        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_IRQ_MASK;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncIRQMask) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncIRQMask);
        i++;

        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_FE_CFG;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncFECfg) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncFECfg);
        i++;

        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_RD_BUFFER_SIZE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN0RDBUFFERSIZE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN0RDBUFFERSIZE);
        i++;

        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_RD_STRIDE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN0RDSTRIDE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN0RDSTRIDE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN1_RD_BUFFER_SIZE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN1RDBUFFERSIZE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN1RDBUFFERSIZE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN1_RD_STRIDE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN1RDSTRIDE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN1RDSTRIDE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN2_RD_BUFFER_SIZE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLNRDBUFFERSIZE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLNRDBUFFERSIZE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN2_RD_STRIDE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN2RDSTRIDE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN2RDSTRIDE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_RD_HINIT;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN0RDHINIT) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN0RDHINIT);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN1_RD_HINIT;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN1RDHINIT) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN1RDHINIT);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN2_RD_HINIT;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN2RDHINIT) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN2RDHINIT);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_RD_VINIT;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN0RDVINIT) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN0RDVINIT);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN1_RD_VINIT;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN1RDVINIT) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN1RDVINIT);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN2_RD_VINIT;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncPLN2RDVINIT) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncPLN2RDVINIT);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_WE_CFG;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_WE_CFG) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_WE_CFG);
        i++;

        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_WR_BUFFER_SIZE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_WR_BUFFER_SIZE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_WR_BUFFER_SIZE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_WR_STRIDE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_WR_STRIDE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_WR_STRIDE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_WR_HINIT;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_WR_HINIT) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_WR_HINIT);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_WR_VINIT;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_WR_VINIT) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_WR_VINIT);
        i++;

        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_WR_HSTEP;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_WR_HSTEP) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_WR_HSTEP);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_WR_VSTEP;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_WR_VSTEP) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_WR_VSTEP);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_WR_BLOCK_CFG;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_WR_BLOCK_CFG) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_WR_BLOCK_CFG);
        i++;

        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_CFG;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_CFG) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_CFG);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_IMAGE_SIZE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_IMAGE_SIZE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_IMAGE_SIZE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_FE_VBPAD_CFG;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_FE_VBPAD_CFG) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_FE_VBPAD_CFG);
        i++;

        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_FSC_REGION_SIZE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_FSC_REGION_SIZE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_FSC_REGION_SIZE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_FSC_BUDGET_0;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_0) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_0);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_FSC_BUDGET_1;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_1) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_1);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_FSC_BUDGET_2;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_2) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_2);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_FSC_BUDGET_3;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_3) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_FSC_BUDGET_3);
        i++;

        if (TRUE == pInputData->scaleConfig.bCropEnabled)
        {
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_RD_HINIT_INT;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_RD_HINIT_INT) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_RD_HINIT_INT);
            i++;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN1_RD_HINIT_INT;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN1_RD_HINIT_INT) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN1_RD_HINIT_INT);
            i++;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN2_RD_HINIT_INT;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN2_RD_HINIT_INT) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN2_RD_HINIT_INT);
            i++;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_RD_VINIT_INT;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_RD_VINIT_INT) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_RD_VINIT_INT);
            i++;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN1_RD_VINIT_INT;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN1_RD_VINIT_INT) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN1_RD_VINIT_INT);
            i++;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN2_RD_VINIT_INT;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN2_RD_VINIT_INT) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN2_RD_VINIT_INT);
            i++;
        }

        if (TRUE == pInputData->scaleConfig.bScaleEnabled)
        {
            // Only if scaling enabled
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_SCALE_CFG;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncSCALE_CFG) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncSCALE_CFG);
            i++;

            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_SCALE_PLN0_OUTPUT_CFG;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncSCALE_PLN0_OUTPUT_CFG) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncSCALE_PLN0_OUTPUT_CFG);
            i++;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_SCALE_PLN1_OUTPUT_CFG;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncSCALE_PLN1_OUTPUT_CFG) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncSCALE_PLN1_OUTPUT_CFG);
            i++;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_SCALE_PLN2_OUTPUT_CFG;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEncSCALE_PLN2_OUTPUT_CFG) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEncSCALE_PLN2_OUTPUT_CFG);
            i++;
        }

        // for both default and scaled case.
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_SCALE_PLN0_HSTEP;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_SCALE_PLN0_HSTEP) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_SCALE_PLN0_HSTEP);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_SCALE_PLN1_HSTEP;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_SCALE_PLN1_HSTEP) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_SCALE_PLN1_HSTEP);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_SCALE_PLN2_HSTEP;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_SCALE_PLN2_HSTEP) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_SCALE_PLN2_HSTEP);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_SCALE_PLN0_VSTEP;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_SCALE_PLN0_VSTEP) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_SCALE_PLN0_VSTEP);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_SCALE_PLN1_VSTEP;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_SCALE_PLN1_VSTEP) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_SCALE_PLN1_VSTEP);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_SCALE_PLN2_VSTEP;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_SCALE_PLN2_VSTEP) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_SCALE_PLN2_VSTEP);
        i++;
        // encoder state.
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_PREDICTION_Y_STATE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_PREDICTION_Y_STATE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_PREDICTION_Y_STATE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_PREDICTION_C_STATE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_PREDICTION_C_STATE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_PREDICTION_C_STATE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_RSM_STATE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_RSM_STATE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_RSM_STATE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_PACKER_STATE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_PACKER_STATE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_PACKER_STATE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_BYTE_PACKER_WORD0_STATE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD0_STATE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD0_STATE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_BYTE_PACKER_WORD1_STATE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD1_STATE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD1_STATE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_BYTE_PACKER_WORD2_STATE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD2_STATE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD2_STATE);
        i++;
        regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_ENCODE_BYTE_PACKER_WORD3_STATE;
        regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD3_STATE) / RegisterWidthInBytes);
        regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_ENCODE_BYTE_PACKER_WORD3_STATE);
        i++;

        if (NULL != pInputData->pQuantTables)
        {
            UINT32 minTableIdx  = static_cast<UINT32>(QuantTableType::QuantTableMin);
            UINT32 maxTableIdx  = static_cast<UINT32>(QuantTableType::QuantTableMax);

            // standard sequence for configuring q tables
            m_regCmd.jpegEnc_DMI_ADDR.u32All = 0x00000000;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_DMI_ADDR;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_DMI_ADDR) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_DMI_ADDR);
            i++;
            m_regCmd.jpegEnc_DMI_CFG.u32All = 0x00000011;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_DMI_CFG;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_DMI_CFG) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_DMI_CFG);
            i++;

            for (UINT32 j = minTableIdx; j < maxTableIdx; j++)
            {
                for (UINT32 k = 0; k < QuantTableSize; k++)
                {
                    switch (static_cast<QuantTableType>(j))
                    {
                        case QuantTableType::QuantTableLuma:
                            regNumVal[i].nRegister      = regJPEG_ENCODE_JPEG_0_DMI_DATA;
                            regNumVal[i].numberOfValues = sizeof(m_regCmd.jpegEnc_DMI_QUANTIZATION_LUMA_DATA[k]) /
                                                            RegisterWidthInBytes;
                            regNumVal[i].pValues        = reinterpret_cast<UINT32*>(
                                                            &m_regCmd.jpegEnc_DMI_QUANTIZATION_LUMA_DATA[k]);
                            break;
                        case QuantTableType::QuantTableChroma:
                            regNumVal[i].nRegister      = regJPEG_ENCODE_JPEG_0_DMI_DATA;
                            regNumVal[i].numberOfValues = sizeof(m_regCmd.jpegEnc_DMI_QUANTIZATION_CHROMA_DATA[k]) /
                                                            RegisterWidthInBytes;
                            regNumVal[i].pValues        = reinterpret_cast<UINT32*>(
                                                            &m_regCmd.jpegEnc_DMI_QUANTIZATION_CHROMA_DATA[k]);
                            break;
                        default:
                            regNumVal[i].nRegister      = regJPEG_ENCODE_JPEG_0_DMI_DATA;
                            regNumVal[i].numberOfValues = sizeof(m_regCmd.jpegEnc_DMI_QUANTIZATION_LUMA_DATA[k]) /
                                                            RegisterWidthInBytes;
                            regNumVal[i].pValues        = reinterpret_cast<UINT32*>(
                                                            &m_regCmd.jpegEnc_DMI_QUANTIZATION_LUMA_DATA[k]);
                    }
                    i++;
                }
            }

            m_regCmd.jpegEnc_DMI_CFG2.u32All = 0x00000000;
            // note: same register written 2nd time with different value.
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_DMI_CFG;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_DMI_CFG2) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_DMI_CFG2);
            i++;
        }

        {
            // for fe_ping_update .. input buffer update
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_IRQ_MASK;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_IRQ_MASK) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_IRQ_MASK);
            i++;

            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_CMD;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_CMD) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_CMD);
            i++;

            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_RD_OFFSET;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_RD_OFFSET) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_RD_OFFSET);
            i++;

            // add patching here ,offset of SMMU_ADDR for y plane
            pInputData->inYBufOffset = ((i * 3) + 2)*RegisterWidthInBytes;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_RD_PNTR;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_RD_PNTR) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_RD_PNTR);
            i++;

            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN1_RD_OFFSET;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN1_RD_OFFSET) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN1_RD_OFFSET);
            i++;

            // add patching here ,offset of SMMU_ADDR for cbcr plane
            pInputData->inCBCRBufOffset = ((i * 3) + 2)*RegisterWidthInBytes;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN1_RD_PNTR;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN1_RD_PNTR) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN1_RD_PNTR);
            i++;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN2_RD_OFFSET;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN2_RD_OFFSET) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN2_RD_OFFSET);
            i++;

            // add patching here ,plane 2 (actually 3rd plane) _buffer_addr
            pInputData->inPLN2BufOffset = ((i * 3) + 2)*RegisterWidthInBytes;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN2_RD_PNTR;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN2_RD_PNTR) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN2_RD_PNTR);
            i++;
        }

        {
            // for we_ping_update
            // add patching here offset of SMMU_ADDR for y plane
            pInputData->outYBufOffset = ((i * 3) + 2)*RegisterWidthInBytes;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN0_WR_PNTR;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN0_WR_PNTR) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN0_WR_PNTR);
            i++;

            // add patching here offset of SMMU_ADDR for y plane
            pInputData->outCBCRBufOffset = ((i * 3) + 2)*RegisterWidthInBytes;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN1_WR_PNTR;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN1_WR_PNTR) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN1_WR_PNTR);
            i++;

            // add patching here offset of SMMU_ADDR for y plane
            pInputData->outPLN2BufOffset = ((i * 3) + 2)*RegisterWidthInBytes;
            regNumVal[i].nRegister = regJPEG_ENCODE_JPEG_0_PLN2_WR_PNTR;
            regNumVal[i].numberOfValues = (sizeof(m_regCmd.jpegEnc_PLN2_WR_PNTR) / RegisterWidthInBytes);
            regNumVal[i].pValues = reinterpret_cast<UINT32*>(&m_regCmd.jpegEnc_PLN2_WR_PNTR);
            i++;
        }

        /// write all sets in cmdbuffer.
        cmdCount = i;
        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Cmd Count %d", cmdCount);

        for (i = 0; i < cmdCount; i++)
        {
            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer, regNumVal[i].nRegister,
                    regNumVal[i].numberOfValues, regNumVal[i].pValues);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncConfig::~JPEGEncConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGEncConfig::~JPEGEncConfig()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncConfig::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncConfig::Execute(
    CmdBuffer*      pCmdBuffer,
    JPEGInputData*  pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        // TBD: Add function to validate input data
        if (CamxResultSuccess == result)
        {
            result = UpdateCmdConfigsFromInputData(pInputData);
            if (CamxResultSuccess == result)
            {
                result = CreateCmdList(pCmdBuffer, pInputData);
            }
        }
    }

    return result;
}

CAMX_NAMESPACE_END
