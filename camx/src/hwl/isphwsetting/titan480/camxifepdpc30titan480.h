////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepdpc30titan480.h
/// @brief IFE PDPC30 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEPDPC30TITAN480_H
#define CAMXIFEPDPC30TITAN480_H

#include "titan480_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC register Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct IFEPDPC30RegConfig0
{
    IFE_IFE_0_PP_CLC_BPC_PDPC_PDPC_BLACK_LEVEL      pdpcBlackLevel;            ///< PDPC Black level
    IFE_IFE_0_PP_CLC_BPC_PDPC_PDAF_HDR_EXP_RATIO    hdrExposureRatio;          ///< HDR exposure ratio
    IFE_IFE_0_PP_CLC_BPC_PDPC_BAD_PIXEL_THRESHOLDS  bpcPixelThreshold;         ///< BPC Pixel threshold
    IFE_IFE_0_PP_CLC_BPC_PDPC_BAD_PIXEL_DET_OFFSET  badPixelDetectionOffset;   ///< BPC/BCC detection offset
    IFE_IFE_0_PP_CLC_BPC_PDPC_PDAF_RG_WB_GAIN       pdafRGWhiteBalanceGain;    ///< PDAF R/G white balance gain
    IFE_IFE_0_PP_CLC_BPC_PDPC_PDAF_BG_WB_GAIN       pdafBGWhiteBalanceGain;    ///< PDAF B/G white balance gain
    IFE_IFE_0_PP_CLC_BPC_PDPC_PDAF_GR_WB_GAIN       pdafGRWhiteBalanceGain;    ///< PDAF G/R white balance gain
    IFE_IFE_0_PP_CLC_BPC_PDPC_PDAF_GB_WB_GAIN       pdafGBWhiteBalanceGain;    ///< PDAF G/B white balance gain
    IFE_IFE_0_PP_CLC_BPC_PDPC_PDAF_LOC_OFFSET_CFG   pdafLocationOffsetConfig;  ///< PDAF location offset config
    IFE_IFE_0_PP_CLC_BPC_PDPC_PDAF_LOC_END_CFG      pdafLocationEndConfig;     ///< PDAF location end config
} CAMX_PACKED;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC register Configuration 1
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct IFEPDPC30RegConfig1
{
    IFE_IFE_0_PP_CLC_BPC_PDPC_BAD_PIXEL_THRESHOLDS_FLAT     bpcPixelThresholdFlat;      ///< BPC Pixel threshold flat
    IFE_IFE_0_PP_CLC_BPC_PDPC_SATURATION_THRESHOLD          pdpcSaturationThreshold;    ///< PDPC saturation threshold
    IFE_IFE_0_PP_CLC_BPC_PDPC_PDAF_TAB_OFFSET_CFG           pdafTabOffsetConfig;        ///< PDAF tab offset config
    IFE_IFE_0_PP_CLC_BPC_PDPC_DD_THRESHOLD_RATIO            ddThresholdRatio;           ///< Threshhold Ratio
    IFE_IFE_0_PP_CLC_BPC_PDPC_FLAT_TH_RECIP                 flatThresholdReciprocal;    ///< Flat Threshold Reciprocal
    IFE_IFE_0_PP_CLC_BPC_PDPC_BAD_PIXEL_DET_OFFSET_FLAT     bpcDetectOffsetFlat;        ///< BPC Detect Offset
    IFE_IFE_0_PP_CLC_BPC_PDPC_THIN_LINE_NOISE_OFFSET        gicThinLineNoiseOffset;     ///< GIC Thin line noise offset
    IFE_IFE_0_PP_CLC_BPC_PDPC_GIC_FILTER_CFG                gicFilterStrength;          ///< GIC filter strength
    IFE_IFE_0_PP_CLC_BPC_PDPC_FMAX_GIC                      fmaxGIC;                    ///< GIC Factor threshold
    IFE_IFE_0_PP_CLC_BPC_PDPC_BPC_OFFSET_GIC                bpcOffsetGIC;               ///< GIC bad pixel offset
} CAMX_PACKED;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC module Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct IFEPDPC30ModuleConfig
{
    IFE_IFE_0_PP_CLC_BPC_PDPC_DMI_LUT_BANK_CFG      DMILUTBankConfig;       ///< PDPC DMI LUT bank config
    IFE_IFE_0_PP_CLC_BPC_PDPC_MODULE_LUT_BANK_CFG   moduleLUTBankConfig;    ///< PDPC LUT bank config
    IFE_IFE_0_PP_CLC_BPC_PDPC_MODULE_CFG            moduleConfig;           ///< BPCPDPC30 Module config
} CAMX_PACKED;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC register Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct IFEPDPC30RegCmd
{
    IFEPDPC30ModuleConfig   config;         ///< BPC/PDPC Config
    IFEPDPC30RegConfig0     config0;        ///< BPC/PDPC Config 0
    IFEPDPC30RegConfig1     config1;        ///< BPC/PDPC Config 1
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 DMIRAM_PDPC30_PDAF_LUT_LENGTH   = 64;
static const UINT32 DMIRAM_PDPC30_NOISE_LUT_LENGTH  = 64;
static const UINT32 IFEPDPC30PDAFDMISize            = DMIRAM_PDPC30_PDAF_LUT_LENGTH * sizeof(UINT64);
static const UINT32 IFEPDPC30NoiseDMISize           = DMIRAM_PDPC30_NOISE_LUT_LENGTH * sizeof(UINT32);
static const UINT32 IFEPDPC30PDAFDMILengthDWord     = IFEPDPC30PDAFDMISize / sizeof(UINT32);
static const UINT32 IFEPDPC30NoiseDMILengthDWord    = IFEPDPC30NoiseDMISize / sizeof(UINT32);
static const UINT32 PDAFLUT                         = IFE_IFE_0_PP_CLC_BPC_PDPC_DMI_LUT_CFG_LUT_SEL_PDAF_LUT;
static const UINT32 NoiseLUT                        = IFE_IFE_0_PP_CLC_BPC_PDPC_DMI_LUT_CFG_LUT_SEL_NOISE_STD2_LUT;
static const UINT32 IFEPDPC30RegLengthDword         = sizeof(IFEPDPC30RegCmd) / sizeof(UINT32);
static const UINT32 IFEPDPC30DMILengthDword         = IFEPDPC30PDAFDMILengthDWord + IFEPDPC30NoiseDMILengthDWord;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE PDPC30 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEPDPC30Titan480 final : public ISPHWSetting
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
    /// CreateSubCmdList
    ///
    /// @brief  Generate the Sub Command List
    ///
    /// @param  pInputData       Pointer to the Inputdata
    /// @param  pDMIBufferOffset Pointer for DMI Buffer
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult CreateSubCmdList(
        VOID*   pInputData,
        UINT32* pDMIBufferOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateTuningMetadata
    ///
    /// @brief  Update Tuning Metadata
    ///
    /// @param  pTuningMetadata      Pointer to the Tuning Metadata
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult UpdateTuningMetadata(
        VOID*  pTuningMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PackIQRegisterSetting
    ///
    /// @brief  Packing register setting based on calculation data
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
    /// ~IFEPDPC30Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEPDPC30Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEPDPC30Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEPDPC30Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFEPDPC30RegCmd    m_regCmd;                    ///< Register List of this Module
    BOOL               m_isLUTLoaded;               ///< Determine if the Lut has been loaded
    UINT32*            m_pDMINoiseLUTDataPtr;       ///< DMI Noise LUT Data
    UINT32*            m_pDMIPDAFMaskLUTDataPtr;    ///< DMI PDAF Mask LUT Data

    IFEPDPC30Titan480(const IFEPDPC30Titan480&)            = delete; ///< Disallow the copy constructor
    IFEPDPC30Titan480& operator=(const IFEPDPC30Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEPDPC30TITAN480_H
