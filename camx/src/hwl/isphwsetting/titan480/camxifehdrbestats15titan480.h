////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdrbestats15titan480.h
/// @brief IFE HDRBEStats15 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEHDRBESTATS15TITAN480_H
#define CAMXIFEHDRBESTATS15TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispstatsmodule.h"

CAMX_NAMESPACE_BEGIN

// @note: Need to check again for the Kona if max horizontal region width/height are changed or not
// static const UINT32 HDRBEStats15MaxHorizontalregions    = 76;              ///< Maximum HDR BE stats horizontal region
// static const UINT32 HDRBEStats15MaxVerticalregions      = 90;              ///< Maximum HDR BE stats vertical region

// Register range from the SWI/HPG
static const UINT32 HDRBEStats15Titan480RegionMinWidth  = 6;            ///< Minimum HDR BE stats region min width
static const UINT32 HDRBEStats15Titan480RegionMaxWidth  = 390;          ///< Maximum HDR BE stats region max width
static const UINT32 HDRBEStats15Titan480RegionMinHeight = 8;            ///< Minimum HDR BE stats region min height
static const UINT32 HDRBEStats15Titan480RegionMaxHeight = 512;          ///< Maximum HDR BE stats region max height

static const INT32 HDRBEStats15Titan480MaxHorizontalregions = 64;       ///< Maximum HDR BE stats horizontal region
static const INT32 HDRBEStats15Titan480MaxVerticalregions   = 48;       ///< Maximum HDR BE stats vertical region

static const UINT32 HDRBEStats15Titan480MaxChannelThreshold =
                   (1 << IFEPipelineBitWidth) - 1;                      ///< Max HDR BE stats Channel threshold

CAMX_BEGIN_PACKED

/// @brief HDR BE Stats Configuration
struct IFEHDRBE15Titan480RegionConfig
{
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_HORZ_RGN_CFG_0        horizontalRegionConfig0;    ///< HDR BE stats horizontal region config
                                                                                    ///  for offset and number
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_HORZ_RGN_CFG_1        horizontalRegionConfig1;    ///< HDR BE stats horizontal region config
                                                                                    ///  for width
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_VERT_RGN_CFG_0        verticalRegionConfig0;      ///< HDR BE stats vertical region config
                                                                                    ///  for offset and number
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_VERT_RGN_CFG_1        verticalRegionConfig1;      ///< HDR BE stats vertical region config
                                                                                    ///  for height
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_HI_THRESHOLD_CFG_0    highThreshold0;             ///< HDR BE stats pixel upper threshold
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_HI_THRESHOLD_CFG_1    highThreshold1;             ///< HDR BE stats pixel upper threshold
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_LO_THRESHOLD_CFG_0    lowThreshold0;              ///< HDR BE stats pixel lower threshold
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_LO_THRESHOLD_CFG_1    lowThreshold1;              ///< HDR BE stats pixel lower threshold
} CAMX_PACKED;
CAMX_END_PACKED

struct IFEHDRBE15Titan480RegCmd
{
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_MODULE_CFG    HDRBEStatsConfig;   ///< HDR BE stats config
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_MODULE_CFG1   HDRBEStatsConfig1;  ///< HDR BE stats config1
                                                                    ///  HDR pattern selection and
                                                                    ///  ZHDR first RB exposure (T2/T1)

    IFE_IFE_0_PP_CLC_STATS_HDR_BE_CH_Y_CFG      lumaConfig;         ///< HDR BE stats luma channel config;
    IFEHDRBE15Titan480RegionConfig              regionConfig;       ///< HDR BE stats region and threshold config
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE HDRBESTATS15 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEHDRBEStats15Titan480 final : public ISPHWSetting
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
    /// GetHWCapability
    ///
    /// @brief  Get HW capability data, initialize common values used for initialization and adjustments
    ///
    /// @param  pHWCapability    Data to be fill with specific HW capability
    ///
    /// @return Return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetHWCapability(
        VOID*   pHWCapability);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFEHDRBEStats15Titan480
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEHDRBEStats15Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEHDRBEStats15Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEHDRBEStats15Titan480();

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
    IFEHDRBE15Titan480RegCmd    m_regCmd; ///< Register List of this Module

    IFEHDRBEStats15Titan480(const IFEHDRBEStats15Titan480&)            = delete; ///< Disallow the copy constructor
    IFEHDRBEStats15Titan480& operator=(const IFEHDRBEStats15Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEHDRBESTATS15TITAN480_H
