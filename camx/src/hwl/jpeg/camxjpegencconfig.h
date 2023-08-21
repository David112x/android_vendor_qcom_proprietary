////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxjpegencconfig.h
/// @brief camxjpegencconfig class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-1988) Code clean up apply NC001 coding standard on entire project
/// @todo (CAMX-1989) Code clean up apply GR030 coding standard on entire project

#ifndef CAMXJPEGENCCONFIG_H
#define CAMXJPEGENCCONFIG_H

#include "camxcmdbuffermanager.h"
#include "camxjpegutil.h"
#include "camxdefs.h"
#include "camxformats.h"
#include "camxhal3types.h"
#include "camxhwcontext.h"
#include "camxmem.h"
#include "camxpacketbuilder.h"
#include "camxtitan17xdefs.h"
#include "titan170_jpeg_encode.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 MaxJPEGEncoderRegisters = 300;          ///< Max encoder registers
static const UINT32 MaxJPEGDMIData          = 0x0000FFFF;   ///< Max DMI data

/// @brief This structure encapsulates all possible register values for jpeg encoder.
struct JPEGEncoderRegCmd
{
    JPEG_ENCODE_JPEG_0_RESET_CMD             jpegEncResetCmd; ///< jpeg register
    JPEG_ENCODE_JPEG_0_CORE_CFG              jpegEncCoreCfg;  ///< jpeg register
    JPEG_ENCODE_JPEG_0_IRQ_MASK              jpegEncIRQMask;  ///< jpeg register
    JPEG_ENCODE_JPEG_0_IRQ_CLEAR             jpegEncIRQClear; ///< jpeg register
    JPEG_ENCODE_JPEG_0_FE_CFG                jpegEncFECfg;    ///< jpeg register

    /// FE_BUF_CFG
    JPEG_ENCODE_JPEG_0_PLN0_RD_BUFFER_SIZE   jpegEncPLN0RDBUFFERSIZE; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_RD_STRIDE        jpegEncPLN0RDSTRIDE     ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN1_RD_BUFFER_SIZE   jpegEncPLN1RDBUFFERSIZE; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN1_RD_STRIDE        jpegEncPLN1RDSTRIDE     ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN2_RD_BUFFER_SIZE   jpegEncPLNRDBUFFERSIZE; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN2_RD_STRIDE        jpegEncPLN2RDSTRIDE     ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_RD_HINIT         jpegEncPLN0RDHINIT      ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN1_RD_HINIT         jpegEncPLN1RDHINIT      ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN2_RD_HINIT         jpegEncPLN2RDHINIT      ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_RD_VINIT         jpegEncPLN0RDVINIT      ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN1_RD_VINIT         jpegEncPLN1RDVINIT      ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN2_RD_VINIT         jpegEncPLN2RDVINIT      ; ///< jpeg register

    /// jpege_lib_hw_we_cfg
    JPEG_ENCODE_JPEG_0_WE_CFG                jpegEnc_WE_CFG;    ///< jpeg register

    /// we buffer cfg
    JPEG_ENCODE_JPEG_0_PLN0_WR_BUFFER_SIZE   jpegEnc_PLN0_WR_BUFFER_SIZE; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_WR_STRIDE        jpegEnc_PLN0_WR_STRIDE     ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_WR_HINIT         jpegEnc_PLN0_WR_HINIT      ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_WR_VINIT         jpegEnc_PLN0_WR_VINIT      ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_WR_HSTEP         jpegEnc_PLN0_WR_HSTEP      ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_WR_VSTEP         jpegEnc_PLN0_WR_VSTEP      ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_WR_BLOCK_CFG     jpegEnc_PLN0_WR_BLOCK_CFG;   ///< jpeg register

    /// encode cfg
    JPEG_ENCODE_JPEG_0_ENCODE_CFG                 jpegEnc_ENCODE_CFG                ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_IMAGE_SIZE          jpegEnc_ENCODE_IMAGE_SIZE         ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_FE_VBPAD_CFG               jpegEnc_FE_VBPAD_CFG              ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_FSC_REGION_SIZE     jpegEnc_ENCODE_FSC_REGION_SIZE    ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_FSC_BUDGET_0        jpegEnc_ENCODE_FSC_BUDGET_0       ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_FSC_BUDGET_1        jpegEnc_ENCODE_FSC_BUDGET_1       ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_FSC_BUDGET_2        jpegEnc_ENCODE_FSC_BUDGET_2       ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_FSC_BUDGET_3        jpegEnc_ENCODE_FSC_BUDGET_3       ; ///< jpeg register

    /// crop cfg
    JPEG_ENCODE_JPEG_0_PLN0_RD_HINIT_INT        jpegEnc_PLN0_RD_HINIT_INT; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN1_RD_HINIT_INT        jpegEnc_PLN1_RD_HINIT_INT; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN2_RD_HINIT_INT        jpegEnc_PLN2_RD_HINIT_INT; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_RD_VINIT_INT        jpegEnc_PLN0_RD_VINIT_INT; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN1_RD_VINIT_INT        jpegEnc_PLN1_RD_VINIT_INT; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN2_RD_VINIT_INT        jpegEnc_PLN2_RD_VINIT_INT; ///< jpeg register

    /// for both scaling disabled and enabled.
    JPEG_ENCODE_JPEG_0_SCALE_PLN0_HSTEP        jpegEnc_SCALE_PLN0_HSTEP; ///< jpeg register
    JPEG_ENCODE_JPEG_0_SCALE_PLN1_HSTEP        jpegEnc_SCALE_PLN1_HSTEP; ///< jpeg register
    JPEG_ENCODE_JPEG_0_SCALE_PLN2_HSTEP        jpegEnc_SCALE_PLN2_HSTEP; ///< jpeg register
    JPEG_ENCODE_JPEG_0_SCALE_PLN0_VSTEP        jpegEnc_SCALE_PLN0_VSTEP; ///< jpeg register
    JPEG_ENCODE_JPEG_0_SCALE_PLN1_VSTEP        jpegEnc_SCALE_PLN1_VSTEP; ///< jpeg register
    JPEG_ENCODE_JPEG_0_SCALE_PLN2_VSTEP        jpegEnc_SCALE_PLN2_VSTEP; ///< jpeg register

    /// if scaling enabled.
    JPEG_ENCODE_JPEG_0_SCALE_CFG              jpegEncSCALE_CFG;             ///< jpeg register
    JPEG_ENCODE_JPEG_0_SCALE_PLN0_OUTPUT_CFG  jpegEncSCALE_PLN0_OUTPUT_CFG; ///< jpeg register
    JPEG_ENCODE_JPEG_0_SCALE_PLN1_OUTPUT_CFG  jpegEncSCALE_PLN1_OUTPUT_CFG; ///< jpeg register
    JPEG_ENCODE_JPEG_0_SCALE_PLN2_OUTPUT_CFG  jpegEncSCALE_PLN2_OUTPUT_CFG; ///< jpeg register

    /// encode state
    JPEG_ENCODE_JPEG_0_ENCODE_PREDICTION_Y_STATE        jpegEnc_ENCODE_PREDICTION_Y_STATE;      ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_PREDICTION_C_STATE        jpegEnc_ENCODE_PREDICTION_C_STATE;      ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_RSM_STATE                 jpegEnc_ENCODE_RSM_STATE;               ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_PACKER_STATE              jpegEnc_ENCODE_PACKER_STATE;            ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_BYTE_PACKER_WORD0_STATE   jpegEnc_ENCODE_BYTE_PACKER_WORD0_STATE; ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_BYTE_PACKER_WORD1_STATE   jpegEnc_ENCODE_BYTE_PACKER_WORD1_STATE; ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_BYTE_PACKER_WORD2_STATE   jpegEnc_ENCODE_BYTE_PACKER_WORD2_STATE; ///< jpeg register
    JPEG_ENCODE_JPEG_0_ENCODE_BYTE_PACKER_WORD3_STATE   jpegEnc_ENCODE_BYTE_PACKER_WORD3_STATE; ///< jpeg register

    /// qtables set
    JPEG_ENCODE_JPEG_0_DMI_ADDR     jpegEnc_DMI_ADDR;                           ///< jpeg register
    JPEG_ENCODE_JPEG_0_DMI_CFG      jpegEnc_DMI_CFG;                            ///< jpeg register
    JPEG_ENCODE_JPEG_0_DMI_CFG      jpegEnc_DMI_CFG2;                           ///< jpeg register
    JPEG_ENCODE_JPEG_0_DMI_DATA     jpegEnc_DMI_QUANTIZATION_LUMA_DATA[64];     ///< jpeg register
    JPEG_ENCODE_JPEG_0_DMI_DATA     jpegEnc_DMI_QUANTIZATION_CHROMA_DATA[64];   ///< jpeg register

    /// for enque input buffer. fe_buffer_update. fe_ping_update
    JPEG_ENCODE_JPEG_0_IRQ_MASK         jpegEnc_IRQ_MASK        ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_CMD              jpegEnc_CMD             ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_RD_OFFSET   jpegEnc_PLN0_RD_OFFSET  ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN0_RD_PNTR     jpegEnc_PLN0_RD_PNTR    ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN1_RD_OFFSET   jpegEnc_PLN1_RD_OFFSET  ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN1_RD_PNTR     jpegEnc_PLN1_RD_PNTR    ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN2_RD_OFFSET   jpegEnc_PLN2_RD_OFFSET  ; ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN2_RD_PNTR     jpegEnc_PLN2_RD_PNTR    ; ///< jpeg register

    /// enque output buffer
    JPEG_ENCODE_JPEG_0_PLN0_WR_PNTR     jpegEnc_PLN0_WR_PNTR;    ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN1_WR_PNTR     jpegEnc_PLN1_WR_PNTR;    ///< jpeg register
    JPEG_ENCODE_JPEG_0_PLN2_WR_PNTR     jpegEnc_PLN2_WR_PNTR;    ///< jpeg register

    /// only if MMU_Prefetch is enabled.
    JPEG_ENCODE_JPEG_0_S0_MMU_PF_ADDR_MIN   jpegEnc_S0_MMU_PF_ADDR_MIN; ///< jpeg register
    JPEG_ENCODE_JPEG_0_S0_MMU_PF_ADDR_MAX   jpegEnc_S0_MMU_PF_ADDR_MAX; ///< jpeg register
    JPEG_ENCODE_JPEG_0_S1_MMU_PF_ADDR_MIN   jpegEnc_S1_MMU_PF_ADDR_MIN; ///< jpeg register
    JPEG_ENCODE_JPEG_0_S1_MMU_PF_ADDR_MAX   jpegEnc_S1_MMU_PF_ADDR_MAX; ///< jpeg register
    JPEG_ENCODE_JPEG_0_S2_MMU_PF_ADDR_MIN   jpegEnc_S2_MMU_PF_ADDR_MIN; ///< jpeg register
    JPEG_ENCODE_JPEG_0_S2_MMU_PF_ADDR_MAX   jpegEnc_S2_MMU_PF_ADDR_MAX; ///< jpeg register
    JPEG_ENCODE_JPEG_0_S3_MMU_PF_ADDR_MIN   jpegEnc_S3_MMU_PF_ADDR_MIN; ///< jpeg register
    JPEG_ENCODE_JPEG_0_S3_MMU_PF_ADDR_MAX   jpegEnc_S3_MMU_PF_ADDR_MAX; ///< jpeg register

    /// starting offline jpeg encode
    JPEG_ENCODE_JPEG_0_CMD                  jpegEnc_CMD2; ///< jpeg register
};

/// @brief This structure encapsulates each regsiter value pair
struct JPEGEncoderRegNumValSet
{
    UINT32     nRegister;    ///< register address
    UINT32     numberOfValues;    ///< number of integers in value
    UINT32*    pValues;    ///< value or pointer to values
};

/// @brief This structure encapsulates data for a crop region from the client
struct JpegCropRegion
{
    UINT32    left;      ///< starting pixel of crop window
    UINT32    top;       ///< starting line of crop window
    UINT32    width;     ///< width of the crop region in pixels
    UINT32    height;    ///< height of the crop region in pixels
};

/// @brief This structure encapsulates data for a CAMIF crop request from the sensor
struct JpegCropInfo
{
    UINT32    firstPixel;    ///< starting pixel for CAMIF crop
    UINT32    firstLine;     ///< starting line for CAMIF crop
    UINT32    lastPixel;     ///< last pixel for CAMIF crop
    UINT32    lastLine;      ///< last line for CAMIF crop
};

/// @ brief Stream dimension
struct JpegStreamDimension
{
    UINT32    width;     ///< Stream width in pixels
    UINT32    height;    ///< Stream Height in pixels
};

/// @ brief Structure to encapsulate sensor dependent data
struct JPEGHALConfigureData
{
    JpegStreamDimension    stream[10];       ///< HAL streams, corresponding to JPEG output paths DS4, DS16, Full, FD
    JpegCropRegion         cropWindow;       ///< Crop region from the client
    HALPixelFormat         format;           ///< output pixel format
};


/// @ brief Structure to share MNDS output details with Crop module
struct JPEGEScalerOutput
{
    JpegStreamDimension    dimension;        ///< MNDS out put width and height
    FLOAT                  scalingFactor;    ///< MNDS input to output scaling factor
};

/// @ JPEGInternalData
struct JPEGInternalData
{
    JPEGEScalerOutput          scalerOutput[2];    ///< output configuration.
};

/// @brief JPEGEScaleConfig
struct JPEGEScaleConfig
{
    BOOL   bScaleEnabled; ///< Flag for scaling enable
    BOOL   bCropEnabled;  ///< Flag for crop enable
    UINT32 inputWidth;    ///< Input width
    UINT32 inputHeight;   ///< Input height
    UINT32 outputWidth;   ///< Output width
    UINT32 outputHeight;  ///< Output height
    UINT32 hOffset;       ///< Horizontal offset
    UINT32 vOffset;       ///< Vertical offset
    UINT32 cropWidth;     ///< Cropped Width
    UINT32 cropHeight;    ///< Cropped Height
};

/// @brief JPEGInputData
struct JPEGInputData
{
    CmdBuffer*                  pCmdBuffer;        ///< Pointer to the Command Buffer Object
    JPEGHALConfigureData        HALData;           ///< Configuration Data from HAL/App
    JPEGInternalData            calculatedData;    ///< Data Calculated
    HwContext*                  pHwContext;        ///< Pointer to the Hardware Context

    BYTE*    pInYBufAddr;      ///< input plane 0 address
    BYTE*    pInCBCRBufAddr;   ///< input plane 1 address
    BYTE*    pInPLN2BufAddr;   ///< input plane 2 address
    BYTE*    pOutYBufAddr;     ///< output plane 0 address
    BYTE*    pOutCBCRBufAddr;  ///< output plane 1 address
    BYTE*    pOutPLN2BufAddr;  ///< output plane 1 address
    UINT32   inYBufOffset;     ///< input plane 0 offset
    UINT32   inCBCRBufOffset;  ///< input plane 1 offset
    UINT32   inPLN2BufOffset;  ///< input plane 2 offset
    UINT32   outYBufOffset;    ///< output plane 0 offset
    UINT32   outCBCRBufOffset; ///< output plane 1 offset
    UINT32   outPLN2BufOffset; ///< output plane 2 offset

    ImageFormat inFormat;                ///< input image format
    ImageFormat outFormat;                ///< output image format

    JPEGEScaleConfig scaleCropReq; ///< input scaling requested

    JPEG_ENCODE_JPEG_0_ENCODE_CFG_IMAGE_FORMAT_ENUM color_format; ///< color format needed by hw

    UINT32  numInputPlanes;  ///< number of plames
    UINT32  cbcr_order;      ///<  0 cbcr, 1 crcb
    UINT32  quality;         ///< jpeg quality- unused right now

    JPEGEScaleConfig   scaleConfig;      ///< final scale config

    JPEGQuantTable*    pQuantTables;     ///< Pointer to Quantization tables
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class for JPEGEncConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class JPEGEncConfig
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create JPEG encode config Object
    ///
    /// @param  ppJPEGEncoder  Pointer to the JPEGEncConfig instance to be created
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        JPEGEncConfig** ppJPEGEncoder);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Execute
    ///
    /// @brief  Execute process capture request to configure module
    ///
    /// @param  pCmdBuffer Pointer to command buffer
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Execute(
        CmdBuffer*      pCmdBuffer,
        JPEGInputData*  pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRegCmd
    ///
    /// @brief  Retrieve the buffer of the register value
    ///
    /// @return Pointer of the register buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID* GetRegCmd();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy the object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID Destroy()
    {
        CAMX_DELETE this;
    };

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~JPEGEncConfig
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~JPEGEncConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// JPEGEncConfig
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit JPEGEncConfig();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateCmdConfigsFromInputData
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateCmdConfigsFromInputData(
        JPEGInputData * pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdList
    ///
    /// @brief  Create the Command List
    ///
    /// @param  pCmdBuffer Pointer to command buffer
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateCmdList(
        CmdBuffer*      pCmdBuffer,
        JPEGInputData*  pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncCoreCFG
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncCoreCFG(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncFEConfig
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncFEConfig(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncFEBufConfig
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncFEBufConfig(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncWEConfig
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncWEConfig(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncWEBufConfig
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncWEBufConfig(
            JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncDefaultScaleConfig
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncDefaultScaleConfig(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncEncoderState
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncEncoderState(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncFEPingUpdate
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncFEPingUpdate(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncWEPingUpdate
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncWEPingUpdate(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncScaleConfig
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncScaleConfig(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncCropConfig
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncCropConfig(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncEncodeConfig
    ///
    /// @brief  Update command config from input data
    ///
    /// @param  pInputData Pointer to the JPEG input data
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncEncodeConfig(
        JPEGInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillEncQuantizationTable
    ///
    /// @brief  Update quantization table based on type
    ///
    /// @param  pInputData Pointer to the JPEG input data
    /// @param  tableType  Quantization table type
    ///
    /// @return CamxResult Success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillEncQuantizationTable(
        JPEGInputData* const pInputData,
        QuantTableType tableType);

    JPEGEncConfig(const JPEGEncConfig&)            = delete;        ///< Disallow the copy constructor
    JPEGEncConfig& operator=(const JPEGEncConfig&) = delete;        ///< Disallow assignment operator

    BOOL                 m_moduleEnable;                        ///< Flag to indicated if this module is enabled
    JPEGEncoderRegCmd    m_regCmd;                              ///< Register List of this Module
};

CAMX_NAMESPACE_END

#endif // CAMXJPEGENCCONFIG_H
