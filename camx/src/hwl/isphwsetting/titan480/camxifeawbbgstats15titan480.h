////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeawbbgstats15titan480.h
/// @brief IFE AWBBGStat15 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEAWBBGSTATS15TITAN480_H
#define CAMXIFEAWBBGSTATS15TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispstatsmodule.h"
#include "camxifeawbbgstats14.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
/// @brief AWB BG Stats Configuration
struct IFEAWBBG15Titan480RegionConfig
{
    IFE_IFE_0_PP_CLC_STATS_AWB_BG_HORZ_RGN_CFG_0        horizontalRegionConfig0;    ///< AWB BG stats horizontal region config
                                                                                    ///  for offset and number
    IFE_IFE_0_PP_CLC_STATS_AWB_BG_HORZ_RGN_CFG_1        horizontalRegionConfig1;    ///< AWB BG stats horizontal region config
                                                                                    ///  for width
    IFE_IFE_0_PP_CLC_STATS_AWB_BG_VERT_RGN_CFG_0        verticalRegionConfig0;      ///< AWB BG stats vertical region config
                                                                                    ///  for offset and number
    IFE_IFE_0_PP_CLC_STATS_AWB_BG_VERT_RGN_CFG_1        verticalRegionConfig1;      ///< AWB BG stats vertical region config
                                                                                    ///  for height
} CAMX_PACKED;

struct IFEAWBBG15Titan480PixelThresholdConfig
{
    IFE_IFE_0_PP_CLC_STATS_AWB_BG_HI_THRESHOLD_CFG_0    highThreshold0;             ///< AWB BG stats pixel upper threshold
    IFE_IFE_0_PP_CLC_STATS_AWB_BG_HI_THRESHOLD_CFG_1    highThreshold1;             ///< AWB BG stats pixel upper threshold
    IFE_IFE_0_PP_CLC_STATS_AWB_BG_LO_THRESHOLD_CFG_0    lowThreshold0;              ///< AWB BG stats pixel lower threshold
    IFE_IFE_0_PP_CLC_STATS_AWB_BG_LO_THRESHOLD_CFG_1    lowThreshold1;              ///< AWB BG stats pixel lower threshold
} CAMX_PACKED;

CAMX_END_PACKED

struct IFEAWBBG15Titan480RegCmd
{
    IFEAWBBG15Titan480RegionConfig              regionConfig;           ///< AWB BG stats region and threshold config
    IFEAWBBG15Titan480PixelThresholdConfig      pixelThresholdConfig;   ///< AWB BG stats pixel threshold valueconfig
    IFE_IFE_0_PP_CLC_STATS_AWB_BG_MODULE_CFG    AWBBGStatsconfig;       ///< AWB BG stats module config
    IFE_IFE_0_PP_CLC_STATS_AWB_BG_CH_Y_CFG      lumaConfig;             ///< AWB BG stats luma channel config
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE AWBBGSTATS15 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEAWBBGStats15Titan480 final : public ISPHWSetting
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
    /// ~IFEAWBBGStats15Titan480
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEAWBBGStats15Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEAWBBGStats15Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEAWBBGStats15Titan480();

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
    IFEAWBBG15Titan480RegCmd    m_regCmd;   ///< Register List of this Module

    IFEAWBBGStats15Titan480(const IFEAWBBGStats15Titan480&)            = delete; ///< Disallow the copy constructor
    IFEAWBBGStats15Titan480& operator=(const IFEAWBBGStats15Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEAWBBGSTATS15TITAN480_H
