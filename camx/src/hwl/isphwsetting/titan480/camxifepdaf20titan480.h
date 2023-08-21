////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepdaf20titan480.h
/// @brief IFE PDAF20 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEPDAF20TITAN480_H
#define CAMXIFEPDAF20TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IFE PDAF20 Line Extractor Config
struct IFEPDAF20LineExtractorConfig
{
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG1  cfg1;  ///< Line Extractor Config 1
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG2  cfg2;  ///< Line Extractor Config 2
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG3  cfg3;  ///< Line Extractor Config 3
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG4  cfg4;  ///< Line Extractor Config 4
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG5  cfg5;  ///< Line Extractor Config 5
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG6  cfg6;  ///< Line Extractor Config 6
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG7  cfg7;  ///< Line Extractor Config 7
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG8  cfg8;  ///< Line Extractor Config 8
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG9  cfg9;  ///< Line Extractor Config 9
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG10 cfg10; ///< Line Extractor Config 10
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG11 cfg11; ///< Line Extractor Config 11
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG12 cfg12; ///< Line Extractor Config 12
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG13 cfg13; ///< Line Extractor Config 13
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG14 cfg14; ///< Line Extractor Config 14
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG15 cfg15; ///< Line Extractor Config 15
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG16 cfg16; ///< Line Extractor Config 16
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG17 cfg17; ///< Line Extractor Config 17
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG18 cfg18; ///< Line Extractor Config 18
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG19 cfg19; ///< Line Extractor Config 19
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_LE_CFG20 cfg20; ///< Line Extractor Config 20
} CAMX_PACKED;

/// @brief IFE PDAF20 FIR Config
struct IFEPDAF20SparsePDFIRConfig
{
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_FIR_CFG0 cfg0; ///< FIR Config 0
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_FIR_CFG1 cfg1; ///< FIR Config 1
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_FIR_CFG2 cfg2; ///< FIR Config 2
} CAMX_PACKED;

/// @brief IFE PDAF20 Sparse PD Config
struct IFEPDAF20SparsePDConfig
{
    IFE_IFE_0_CLC_CSID_PPP_SPARSE_PD_PIX_EXTRACT_CFG0 pixelExtractor;       ///< CSID pixel Extractor Config
    IFE_IFE_0_CLC_CSID_PPP_HCROP                      horizontalCrop;       ///< CSID Horizontal Crop
    IFE_IFE_0_CLC_CSID_PPP_VCROP                      verticalCrop;         ///< CSID Vertical Crop
    IFEPDAF20LineExtractorConfig                      lineExtractorConfig;  ///< Line Extractor Config
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_PS_CFG0             pixelSeparatorConfig; ///< Pixel Separator Config
    IFE_IFE_0_CLC_PDLIB_SPARSE_PD_RESAMPLER           resamplerConfig;      ///< Resampler Config
    IFEPDAF20SparsePDFIRConfig                        firConfig;            ///< FIR Config
} CAMX_PACKED;

/// @brief PDAF20 DUAL PD Configuration
struct IFEPDAF20DualPDConfigCmd1
{
    IFE_IFE_0_CLC_PDLIB_CROP_WIDTH_CFG    pdCropWidthCfg;     ///< pdCropWidthCfg
    IFE_IFE_0_CLC_PDLIB_CROP_HEIGHT_CFG   pdCropHeightCfg;    ///< pdCropHeightCfg
    IFE_IFE_0_CLC_PDLIB_BLS_CFG           pdBLSCfg;           ///< pdBLSCfg
    IFE_IFE_0_CLC_PDLIB_RGN_OFFSET_CFG    pdRegionOffsetCfg;  ///< pdRegionOffsetCfg
    IFE_IFE_0_CLC_PDLIB_RGN_NUM_CFG       pdRegionNumberCfg;  ///< pdRegionNumberCfg
    IFE_IFE_0_CLC_PDLIB_RGN_SIZE_CFG      pdRegionSizeCfg;    ///< pdRegionSizeCfg
    IFE_IFE_0_CLC_PDLIB_HDR_CFG           pdHDRCfg;           ///< pdHDRCfg
    IFE_IFE_0_CLC_PDLIB_BIN_SKIP_CFG      pdBinSkipCfg;       ///< pdBinSkipCfg
    IFE_IFE_0_CLC_PDLIB_GM_CFG_0          pdGMCfg0;           ///< pdGMCfg0
    IFE_IFE_0_CLC_PDLIB_GM_CFG_1          pdGMCfg1;           ///< pdGMCfg1
} CAMX_PACKED;

/// @brief PDAF20 DUAL PD Configuration
struct IFEPDAF20DualPDConfigCmd2
{
    IFE_IFE_0_CLC_PDLIB_GM_CFG_2          pdGMCfg2;           ///< pdGMCfg2
    IFE_IFE_0_CLC_PDLIB_GM_CFG_3          pdGMCfg3;           ///< pdGMCfg3
    IFE_IFE_0_CLC_PDLIB_IIR_CFG_0         pdIIRCfg0;          ///< pdIIRCfg0
    IFE_IFE_0_CLC_PDLIB_IIR_CFG_1         pdIIRCfg1;          ///< pdIIRCfg1
    IFE_IFE_0_CLC_PDLIB_IIR_CFG_2         pdIIRCfg2;          ///< pdIIRCfg2
    IFE_IFE_0_CLC_PDLIB_IIR_CFG_3         pdIIRCfg3;          ///< pdIIRCfg3
    IFE_IFE_0_CLC_PDLIB_SAD_PHASE_CFG_0   pdSADPhaseCfg0;     ///< pdSADPhaseCfg0
    IFE_IFE_0_CLC_PDLIB_SAD_PHASE_CFG_1   pdSADPhaseCfg1;     ///< pdSADPhaseCfg1
} CAMX_PACKED;

/// @brief PDAF20 DUAL PD Configuration
struct IFEPDAF20DualPDConfig
{
    IFEPDAF20DualPDConfigCmd1 cfg1;  ///< Dual PD Config1
    IFEPDAF20DualPDConfigCmd2 cfg2;  ///< Dual PD Config2
} CAMX_PACKED;

/// @brief IFE PDAF20 Module Dependence Data
struct IFEPDAF20RegCmd
{
    IFE_IFE_0_CLC_PDLIB_DMI_LUT_BANK_CFG    DMILUTBankConfig;       ///< DMI LUT bank config
    IFE_IFE_0_CLC_PDLIB_MODULE_LUT_BANK_CFG moduleLUTBankConfig;    ///< Module LUT bank config
    IFE_IFE_0_CLC_PDLIB_MODULE_CFG          moduleConfig;           ///< PDAF20 module config
    IFEPDAF20SparsePDConfig                 sparsePDHWConfig;       ///< Sparse PD HW Config
    IFEPDAF20DualPDConfig                   dualPDConfig;           ///< Dual PD Config
} CAMX_PACKED;

/// @brief Pixel Separator LUT
struct IFEPDAF20PixelSeparatorMapLUT
{
    UINT32 rightPixelIndex  : 8;  ///< Right Pixel Index
    UINT32 leftPixelIndex   : 8;  ///< Left Pixel  Index
    UINT32 UNUSED           : 16; ///< UNUSED bits
} CAMX_PACKED;

/// @brief Resampler Pixel LUT
struct IFEPDAF20SparsePDResamplerPixelLUT
{
    UINT32 pixel0X : 7;  ///< Pixel0 X Coordinate
    UINT32 pixel0Y : 4;  ///< Pixel0 Y Coordinate
    UINT32 pixel1X : 7;  ///< Pixel1 X Coordinate
    UINT32 pixel1Y : 4;  ///< Pixel1 Y coordinate
    UINT32 pixel2X : 7;  ///< Pixel2 X Coordinate
    UINT32 pixel2Y : 4;  ///< Pixel2 Y Coordinate
    UINT32 pixel3X : 7;  ///< Pixel3 X Coordinate
    UINT32 pixel3Y : 4;  ///< Pixel3 Y Coordinate
    UINT32 UNUSED  : 20; ///< UNUSED BITS
} CAMX_PACKED;

///@ brief Resampler Coefficients LUT
struct IFEPDAF20SparsePDResamplerCoefficientLUT
{
    UINT32 OP0K0 : 6; ///< K0 in OP0
    UINT32 OP0K1 : 6; ///< K1 in OP0
    UINT32 OP0K2 : 6; ///< K2 in OP0
    UINT32 OP0K3 : 6; ///< K3 in OP0
    UINT32 OP1K0 : 6; ///< K0 in OP1
    UINT32 OP1K1 : 6; ///< K1 in OP1
    UINT32 OP1K2 : 6; ///< K2 in OP1
    UINT32 OP1K3 : 6; ///< K3 in OP1
    UINT32 OP2K0 : 6; ///< K0 in OP2
    UINT32 OP2K1 : 6; ///< K1 in OP2
    UINT32 OP2K2 : 6; ///< K2 in OP2
    UINT32 OP2K3 : 6; ///< K3 in OP2
    UINT32 OP3K0 : 6; ///< K0 in OP3
    UINT32 OP3K1 : 6; ///< K1 in OP3
    UINT32 OP3K2 : 6; ///< K2 in OP3
    UINT32 OP3K3 : 6; ///< K3 in OP3
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT PDAF20LUTBank0 = 0;
static const UINT PDAF20LUTBank1 = 1;

static const UINT32 IFEPDAF20RegLengthDWord = sizeof(IFEPDAF20RegCmd) / sizeof(UINT32);

// Gain Map has 221 entries ( 17 rows x 13 colomns)
static const UINT32 IFEPDAF20GainMapLutTableRow    = 17;
static const UINT32 IFEPDAF20GainMapLutTableColumn = 13;
static const UINT32 IFEPDAF20GainMapLutTableSize   = (IFEPDAF20GainMapLutTableRow * IFEPDAF20GainMapLutTableColumn);
static const UINT32 IFEPDAF20GainMapDMILengthWord  = IFEPDAF20GainMapLutTableSize * sizeof(UINT32);

// Pixel Separator has 128 entries
static const UINT32 IFEPDAF20PixelSeparatorMapTableEntries = 128;
static const UINT32 IFEPDAF20PixelSepartorMapDMILengthWord = sizeof(IFEPDAF20PixelSeparatorMapLUT)  *
                                                             IFEPDAF20PixelSeparatorMapTableEntries;

// Resampler pixel map has 32 entries
static const UINT32 IFEPDAF20ResamplerPixelLutEntries       = 32;
static const UINT32 IFEPDAF20ResamplerPixelMapDMILengthWord = sizeof(IFEPDAF20SparsePDResamplerPixelLUT) *
                                                              IFEPDAF20ResamplerPixelLutEntries;

// Resampler Coefficients LUT has 32 entries
static const UINT32 IFEPDAF20ResamplerCoefficientLutEntries    = 32;
static const UINT32 IFEPDAF20ResamplerCoefficientDMILengthWord = sizeof(IFEPDAF20SparsePDResamplerCoefficientLUT) *
                                                                 IFEPDAF20ResamplerCoefficientLutEntries;

// CSID Pixel Map Entries has 64 entries
static const UINT32 IFEPDAF20CSIDPixelExtractorRows             = 64;
static const UINT32 IFEPDAF20CSIDPixelExtractorColomns          = 64;
static const UINT32 IFEPDAF20CSIDPixelExtractorEntries          = 64;
static const UINT32 IFEPDAF20CSIDPixelExtracionMapDMILengthWord = IFEPDAF20CSIDPixelExtractorEntries * sizeof(UINT64) * 2;
static const UINT32 IFEPDAF20CSIDSkipDMIValueOffset             = 4;

// PDAF20 DMI LUT SIZE
static const UINT32 IFEPDAF20DMILutSize      = IFEPDAF20GainMapDMILengthWord               +
                                               IFEPDAF20PixelSepartorMapDMILengthWord      +
                                               IFEPDAF20ResamplerPixelMapDMILengthWord     +
                                               IFEPDAF20ResamplerCoefficientDMILengthWord  +
                                               IFEPDAF20CSIDPixelExtracionMapDMILengthWord;
static const UINT32 IFEPDAF20DMILutSizeDword = IFEPDAF20DMILutSize / sizeof(UINT32);

static const UINT32 IFEPDAF20GainMapDMIOffset               = 0;
static const UINT32 IFEPDAF20PixelSeparatorMapDMIOffset     = IFEPDAF20GainMapDMIOffset + IFEPDAF20GainMapDMILengthWord;
static const UINT32 IFEPDAF20ResamplerPixelMapDMIOffset     = IFEPDAF20PixelSeparatorMapDMIOffset +
                                                              IFEPDAF20PixelSepartorMapDMILengthWord;
static const UINT32 IFEPDAF20ResamplerCoefficientDMIOffset  = IFEPDAF20ResamplerPixelMapDMIOffset +
                                                              IFEPDAF20ResamplerPixelMapDMILengthWord;
static const UINT32 IFEPDAF20CSIDPixelExtracionMapDMIOffset = IFEPDAF20ResamplerCoefficientDMIOffset +
                                                              IFEPDAF20ResamplerCoefficientDMILengthWord;
static const UINT32 IFEPDAF20PixelSeparatorLUTSelect = 1;
static const UINT32 IFEPDAF20ResamplerPixelLUTSelect = 2;
static const UINT32 IFEPDAF20ResamplerCoeffLUTSelect = 3;
static const UINT32 IFEPDAF20GainMapLUTSelect        = 4;

static const UINT32 IFEPDAF20CSIDPixelExtractorDMISelect = 1;

#define FLOAT_TO_Q(exp, f) \
    (static_cast<INT32>(((f)*(1<<(exp))) + (((f)<0) ? -0.5 : 0.5)))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE PDAF20 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEPDAF20Titan480 final : public ISPHWSetting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdList
    ///
    /// @brief  Generate the Command List
    ///
    /// @param  pInputData       Pointer to the Inputdata
    /// @param  pDMIBufferOffset Pointer for DMI Buffer
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult CreateCmdList(
        VOID*   pInputData,
        UINT32* pDMIBufferOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PackIQRegisterSetting
    ///
    /// @brief  Calculate register settings based on common library result
    ///
    /// @param  pInput       Pointer to the Input data to the module for calculation
    /// @param  pOutput      Pointer to the Output data to the module for DMI buffer
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PackIQRegisterSetting(
        VOID* pInput,
        VOID* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRegisterSetting
    ///
    /// @brief  Setup register value based on CamX Input
    ///
    /// @param  pInput       Pointer to the Input data to the module for calculation
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupRegisterSetting(
        VOID*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFEPDAF20Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEPDAF20Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEPDAF20Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEPDAF20Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyRegCmd
    ///
    /// @brief  Copy register settings to the input buffer
    ///
    /// @param  pData  Pointer to the Input data buffer
    ///
    /// @return Number of bytes copied
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 CopyRegCmd(
        VOID* pData);

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureSparsePD
    ///
    /// @brief  Calculate register settings based on common library result
    ///
    /// @param  pHWConfig    Pointer to the PD HW Config
    /// @param  pOutput      Pointer to the Output data to the module for DMI buffer
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureSparsePD(
        PDHwConfig* pHWConfig,
        VOID*       pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDualPD
    ///
    /// @brief  Calculate register settings based on common library result
    ///
    /// @param  pHWConfig    Pointer to the PD HW Config
    /// @param  pOutput      Pointer to the Output data to the module for DMI buffer
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureDualPD(
        PDHwConfig* pHWConfig,
        VOID*       pOutput);

    IFEPDAF20RegCmd m_regCmd;       ///< Register List of this Module
    UINT32          m_bankSelect;   ///< The DMI bank select

    IFEPDAF20Titan480(const IFEPDAF20Titan480&)            = delete; ///< Disallow the copy constructor
    IFEPDAF20Titan480& operator=(const IFEPDAF20Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEPDAF20TITAN480_H
