////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifetintlessbgstats15titan480.h
/// @brief IFE TINTLESSBGSTATS15 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFETINTLESSBGSTATS15TITAN480_H
#define CAMXIFETINTLESSBGSTATS15TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispstatsmodule.h"
#include "camxifetintlessbgstats15.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
/// @brief TintlessBG Stats ROI Configuration
struct IFETintlessBG15Titan480RegionConfig
{
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_HORZ_RGN_CFG_0   horizontalRegionConfig0;    ///< Tintless BG stats horizontal
                                                                                    ///  region config for offset and number
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_HORZ_RGN_CFG_1   horizontalRegionConfig1;    ///< Tintless BG stats horizontal
                                                                                    ///  region config for width
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_VERT_RGN_CFG_0   verticalRegionConfig0;      ///< Tintless BG stats vertical
                                                                                    ///  region config for offset and number
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_VERT_RGN_CFG_1   verticalRegionConfig1;      ///< Tintless BG stats vertical
                                                                                    ///  region config for width
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_RGN_WIDTH_CFG    unused;
} CAMX_PACKED;

struct IFETintlessBG15Titan480PixelThresholdConfig
{
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_HI_THRESHOLD_CFG_0    highThreshold0; ///< TintlessBG stats pixel upper threshold;
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_HI_THRESHOLD_CFG_1    highThreshold1; ///< TintlessBG stats pixel upper threshold;
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_LO_THRESHOLD_CFG_0    lowThreshold0;  ///< TintlessBG stats pixel lower threshold;
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_LO_THRESHOLD_CFG_1    lowThreshold1;  ///< TintlessBG stats pixel lower threshold;

} CAMX_PACKED;

/// @brief TintlessBG Stats Configuration
struct IFETintlessBG15Titan480RegCmd
{
    IFETintlessBG15Titan480RegionConfig             regionConfig;           ///< TintlessBG stats region config
    IFETintlessBG15Titan480PixelThresholdConfig     pixelThresholdConfig;   ///< TintlessBG stats pixel threshold config
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_CH_Y_CFG     lumaConfig;             ///< TintlessBG stats luma channel config
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_MODULE_CFG   tintlessBGStatsconfig;  ///< TintlessBG stats config
} CAMX_PACKED;

CAMX_END_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE TINTLESSBGSTATS15 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFETintlessBGStats15Titan480 final : public ISPHWSetting
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
    /// ~IFETintlessBGStats15Titan480
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFETintlessBGStats15Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFETintlessBGStats15Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFETintlessBGStats15Titan480();

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
    IFETintlessBG15Titan480RegCmd    m_regCmd; ///< Register List of this Module

    IFETintlessBGStats15Titan480(const IFETintlessBGStats15Titan480&)            = delete; ///< Disallow the copy constructor
    IFETintlessBGStats15Titan480& operator=(const IFETintlessBGStats15Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFETINTLESSBGSTATS15TITAN480_H
