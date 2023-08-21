////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebfstats23titan17x.h
/// @brief IFE BF23 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEBFSTATS23TITAN17X_H
#define CAMXIFEBFSTATS23TITAN17X_H

#include "titan170_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispstatsmodule.h"
#include "camxifebfstats23.h"

CAMX_NAMESPACE_BEGIN

// BF stats region index LUT bank select
static const UINT IFEBF23RegionIndexLUTBank0 = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_STATS_BAF_RGN_IND_LUT_BANK0;
static const UINT IFEBF23RegionIndexLUTBank1 = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_STATS_BAF_RGN_IND_LUT_BANK1;

// BF stats gamma LUT bank select
static const UINT IFEBF23GammaLUTBank0 = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_STATS_BAF_GAMMA_LUT_BANK0;
static const UINT IFEBF23GammaLUTBank1 = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_STATS_BAF_GAMMA_LUT_BANK1;

// BF stats region index LUT bank select values
static const UINT IFEBF23RegionIndexLUTBank0SelectValue = 0;
static const UINT IFEBF23RegionIndexLUTBank1SelectValue = 1;

// BF stats gamma LUT bank select values
static const UINT IFEBF23GammaLUTBank0SelectValue = 0;
static const UINT IFEBF23GammaLUTBank1SelectValue = 1;


CAMX_BEGIN_PACKED

/// @brief IFE BAF Stats Configuration
struct IFEBF23Config
{
    IFE_IFE_0_VFE_STATS_BAF_CFG                       config;          ///< Config
    IFE_IFE_0_VFE_STATS_BAF_Y_CONV_CFG_0              yConvConfig0;    ///< Y color conversion config 0
    IFE_IFE_0_VFE_STATS_BAF_Y_CONV_CFG_1              yConvConfig1;    ///< Y color conversion config 1
} CAMX_PACKED;

/// @todo (CAMX-1468): Discuss if we want to change the variable names in BF stats
/// @brief IFE Register set for H FIR and IIR config
struct IFEBF23HFIRIIRConfig
{
    IFE_IFE_0_VFE_STATS_BAF_H_1_FIR_CFG_0             h1FIRConfig0;    ///< Horizontal1 FIR config 0
    IFE_IFE_0_VFE_STATS_BAF_H_1_FIR_CFG_1             h1FIRConfig1;    ///< Horizontal1 FIR config 1
    IFE_IFE_0_VFE_STATS_BAF_H_1_FIR_CFG_2             h1FIRConfig2;    ///< Horizontal1 FIR config 2
    IFE_IFE_0_VFE_STATS_BAF_H_1_FIR_CFG_3             h1FIRConfig3;    ///< Horizontal1 FIR config 3
    IFE_IFE_0_VFE_STATS_BAF_H_1_IIR_CFG_0             h1IIRConfig0;    ///< Horizontal1 IIR config 0
    IFE_IFE_0_VFE_STATS_BAF_H_1_IIR_CFG_1             h1IIRConfig1;    ///< Horizontal1 IIR config 1
    IFE_IFE_0_VFE_STATS_BAF_H_1_IIR_CFG_2             h1IIRConfig2;    ///< Horizontal1 IIR config 2
    IFE_IFE_0_VFE_STATS_BAF_H_1_IIR_CFG_3             h1IIRConfig3;    ///< Horizontal1 IIR config 3
    IFE_IFE_0_VFE_STATS_BAF_H_1_IIR_CFG_4             h1IIRConfig4;    ///< Horizontal1 IIR config 4
} CAMX_PACKED;

/// @brief IFE Register set for V IIR config 1
struct IFEBF23VIIRConfig1
{
    IFE_IFE_0_VFE_STATS_BAF_V_IIR_CFG_0               vIIRConfig0;    ///< Vertical IIR config 0
    IFE_IFE_0_VFE_STATS_BAF_V_IIR_CFG_1               vIIRConfig1;    ///< Vertical IIR config 1
    IFE_IFE_0_VFE_STATS_BAF_V_IIR_CFG_2               vIIRConfig2;    ///< Vertical IIR config 2
} CAMX_PACKED;

/// @brief IFE Register set for V IIR config 2
struct IFEBF23VIIRConfig2
{
    IFE_IFE_0_VFE_STATS_BAF_V_IIR_CFG_3               vIIRConfig3;    ///< Vertical IIR config 3
    IFE_IFE_0_VFE_STATS_BAF_V_IIR_CFG_4               vIIRConfig4;    ///< Vertical IIR config 4
} CAMX_PACKED;

/// @brief IFE Register set for H Threshold config
struct IFEBF23HThresholdCoringConfig
{
    IFE_IFE_0_VFE_STATS_BAF_H_1_TH_CFG                h1ThresholdConfig;    ///< Horizontal1 Threshold Config
    IFE_IFE_0_VFE_STATS_BAF_H_1_CORING_CFG_0          h1CoringConfig0;      ///< Horizontal1 Coring Config 0
    IFE_IFE_0_VFE_STATS_BAF_H_1_CORING_CFG_1          h1CoringConfig1;      ///< Horizontal1 Coring Config 1
    IFE_IFE_0_VFE_STATS_BAF_H_1_CORING_CFG_2          h1CoringConfig2;      ///< Horizontal1 Coring Config 2
    IFE_IFE_0_VFE_STATS_BAF_H_1_CORING_CFG_3          h1CoringConfig3;      ///< Horizontal1 Coring Config 3
} CAMX_PACKED;

/// @brief IFE Register set for V Threshold config
struct IFEBF23VThresholdCoringConfig
{
    IFE_IFE_0_VFE_STATS_BAF_V_TH_CFG                  vThresholdConfig;      ///< Vertical Threshold Config
    IFE_IFE_0_VFE_STATS_BAF_V_CORING_CFG_0            vCoringConfig0;        ///< Vertical Coring Config 0
    IFE_IFE_0_VFE_STATS_BAF_V_CORING_CFG_1            vCoringConfig1;        ///< Vertical Coring Config 1
    IFE_IFE_0_VFE_STATS_BAF_V_CORING_CFG_2            vCoringConfig2;        ///< Vertical Coring Config 2
    IFE_IFE_0_VFE_STATS_BAF_V_CORING_CFG_3            vCoringConfig3;        ///< Vertical Coring Config 3
} CAMX_PACKED;

/// @brief IFE Register set for Coring Config
struct IFEBF23CoringConfig
{
    IFE_IFE_0_VFE_STATS_BAF_CORING_GAIN_CFG_0         coringGainConfig0;         ///< Vertical Coring Gain Config 0
    IFE_IFE_0_VFE_STATS_BAF_CORING_GAIN_CFG_1         coringGainConfig1;         ///< Vertical Coring Gain Config 1
} CAMX_PACKED;

/// @brief IFE Register set for Interrupt Config
struct IFEBF23PhaseStripeInterruptConfig
{
    IFE_IFE_0_VFE_STATS_BAF_SCALE_H_IMAGE_SIZE_CFG    hScaleImageSizeConfig;     ///< Horizontal image size config
    IFE_IFE_0_VFE_STATS_BAF_SCALE_H_PHASE_CFG         hScalePhaseConfig;         ///< Horizontal Phase config
    IFE_IFE_0_VFE_STATS_BAF_SCALE_H_STRIPE_CFG_0      hScaleStripeConfig0;       ///< Horizontal Scale stripe config0
    IFE_IFE_0_VFE_STATS_BAF_SCALE_H_STRIPE_CFG_1      hScaleStripeConfig1;       ///< Horizontal Scale stripe config1
    IFE_IFE_0_VFE_STATS_BAF_SCALE_H_PAD_CFG           hScalePadConfig;           ///< Horizontal Scale pad config
    IFE_IFE_0_VFE_STATS_BAF_EARLY_INTR_CFG            earlyInterruptConfig;      ///< early interrupt config
} CAMX_PACKED;

/// @brief IFE Register set for active window config
struct IFEBF23ActiveWindow
{
    IFE_IFE_0_VFE_STATS_BAF_ACTIVE_WINDOW_CFG_0       bafActiveWindow1;          ///< Active window config0
    IFE_IFE_0_VFE_STATS_BAF_ACTIVE_WINDOW_CFG_1       bafActiveWindow2;          ///< Active window config1
} CAMX_PACKED;

CAMX_END_PACKED

/// @brief IFE RegCmd for BF stats
struct IFEBF23RegCmd
{
    IFEBF23Config                                     BFConfig;                  ///< baf config
    IFEBF23HFIRIIRConfig                              hFIRIIRConfig;             ///< Horizontal FIR IIR config
    IFEBF23VIIRConfig1                                vIIRConfig1;               ///< Vertical IIR config 1
    IFEBF23VIIRConfig2                                vIIRConfig2;               ///< Vertical IIR config 1
    IFEBF23ActiveWindow                               activeWindow;              ///< Active Window
    IFE_IFE_0_VFE_STATS_BAF_SHIFT_BITS_CFG            shiftBitsConfig;           ///< shift bits config
    IFEBF23HThresholdCoringConfig                     hThresholdCoringConfig;    ///< Horizontal Coring config
    IFEBF23VThresholdCoringConfig                     vThresholdCoringConfig;    ///< Vertical Coring config
    IFEBF23CoringConfig                               coringConfig;              ///< Coring Config
    IFEBF23PhaseStripeInterruptConfig                 phaseStripeConfig;         ///< Phase Stripe Config
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE BFSTATS23 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEBFStats23Titan17x final : public ISPHWSetting
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
    /// ResetPerFrameData
    ///
    /// @brief  Reset per frame data before processing current stats configuration
    ///
    /// @param  pInputData  Pointer to ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ResetPerFrameData(
        const ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LumaConversionConfig
    ///
    /// @brief  Configure BF stats Luma conversion registers
    ///
    /// @param  pInputBFStatsConfig     Pointer to the input BF stats config data
    /// @param  pLumaConversionConfig   Pointer to the intpu data for luma converion config
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LumaConversionConfig(
        const AFConfigParams*               pInputBFStatsConfig,
        BFStats23LumaConversionConfigInput* pLumaConversionConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DownscalerCalculateInterpolationResolution
    ///
    /// @brief  Calculate the BF module downscaler interpolation resolution
    ///
    /// @param  pBFStatsConfig  BF stats configuration detail
    ///
    /// @return UINT32 Interpolation resolution value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 DownscalerCalculateInterpolationResolution(
        const AFConfigParams*   pBFStatsConfig
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillDownscalerConfig
    ///
    /// @brief  Fill the register values for BF module downscaler
    ///
    /// @param  pConfig Pointer to the downscaler configuration input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VOID FillDownscalerConfig(
        const BFStats23DownscalerConfigInput*     pConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFilters
    ///
    /// @brief  Configure the set of filter related register values
    ///
    /// @param  pBFStatsConfig  Pointer to the FIR, IIR, and coring register configuration input data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureFilters(
        const AFConfigParams*   pBFStatsConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetActiveWindow
    ///
    /// @brief  Generate the Command List
    ///
    /// @param  pInputData  Pointer to ISP intput data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetActiveWindow(
        const ISPInputData*   pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFIRFilters
    ///
    /// @brief  Configure FIR filter register values
    ///
    /// @param  pFIRConfig    BF stats FIR configuration data
    /// @param  filter        type of filter
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureFIRFilters(
        const BFFIRFilterConfigType*    pFIRConfig,
        IFEBFFilterType                 filter);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureIIRFilters
    ///
    /// @brief  Configure IIR filter register values
    ///
    /// @param  pIIRConfig BF stats IIR configuration data
    /// @param  filter     type of filter
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureIIRFilters(
        const BFIIRFilterConfigType*    pIIRConfig,
        IFEBFFilterType                 filter);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureCoring
    ///
    /// @brief  Configure coring filter register values
    ///
    /// @param  pCoringConfig BF stats coring configuration data
    /// @param  filter        type of filter
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureCoring(
        const BFFilterCoringConfigParams*   pCoringConfig,
        IFEBFFilterType                     filter);

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
    /// ~IFEBFStats23Titan17x
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEBFStats23Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEBFStats23Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEBFStats23Titan17x();

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
    IFEBF23RegCmd           m_regCmd;                               ///< Register list for IFE BF stats

    IFEBFStats23Titan17x(const IFEBFStats23Titan17x&)            = delete; ///< Disallow the copy constructor
    IFEBFStats23Titan17x& operator=(const IFEBFStats23Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEBFSTATS23TITAN17X_H
