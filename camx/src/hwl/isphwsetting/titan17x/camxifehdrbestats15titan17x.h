////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdrbestats15titan17x.h
/// @brief IFE HDRBEStats15 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEHDRBESTATS15TITAN17X_H
#define CAMXIFEHDRBESTATS15TITAN17X_H

#include "titan170_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispstatsmodule.h"
#include "camxifehdrbestats15.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

// Register range from the SWI/HPG
// Register range from the SWI/HPG
static const UINT32 HDRBEStats15Titan17xRegionMinWidth  = 6;            ///< Minimum HDR BE stats region min width
static const UINT32 HDRBEStats15Titan17xRegionMaxWidth  = 390;          ///< Maximum HDR BE stats region max width
static const UINT32 HDRBEStats15Titan17xRegionMinHeight = 2;            ///< Minimum HDR BE stats region min height
static const UINT32 HDRBEStats15Titan17xRegionMaxHeight = 512;          ///< Maximum HDR BE stats region max height

static const INT32 HDRBEStats15Titan17xMaxHorizontalregions = 64;       ///< Maximum HDR BE stats horizontal region
static const INT32 HDRBEStats15Titan17xMaxVerticalregions   = 48;       ///< Maximum HDR BE stats vertical region

static const UINT32 HDRBEStats15Titan17xMaxChannelThreshold =
                                  (1 << IFEPipelineBitWidth) - 1;       ///< Max HDR BE stats Channel threshold



/// @brief HDR BE Stats Configuration
struct IFEHDRBE15RegionConfig
{
    IFE_IFE_0_VFE_STATS_HDR_BE_RGN_OFFSET_CFG        regionOffset;      ///< HDR BE stats region offset config
    IFE_IFE_0_VFE_STATS_HDR_BE_RGN_NUM_CFG           regionNumber;      ///< HDR BE stats region number config
    IFE_IFE_0_VFE_STATS_HDR_BE_RGN_SIZE_CFG          regionSize;        ///< HDR BE stats region size config
    IFE_IFE_0_VFE_STATS_HDR_BE_HI_THRESHOLD_CFG_0    highThreshold0;    ///< HDR BE stats pixel upper threshold config
    IFE_IFE_0_VFE_STATS_HDR_BE_HI_THRESHOLD_CFG_1    highThreshold1;    ///< HDR BE stats pixel upper threshold config
    IFE_IFE_0_VFE_STATS_HDR_BE_LO_THRESHOLD_CFG_0    lowThreshold0;     ///< HDR BE stats pixel lower threshold config
    IFE_IFE_0_VFE_STATS_HDR_BE_LO_THRESHOLD_CFG_1    lowThreshold1;     ///< HDR BE stats pixel lower threshold config
} CAMX_PACKED;
CAMX_END_PACKED

struct IFEHDRBE15RegCmd
{
    IFEHDRBE15RegionConfig                 regionConfig;                ///< HDR BE stats region and threshold config
    IFE_IFE_0_VFE_STATS_HDR_BE_CH_Y_CFG    lumaConfig;                  ///< HDR BE stats luma channel config
    IFE_IFE_0_VFE_STATS_HDR_BE_CFG         HDRBEStatsConfig;            ///< HDR BE stats config
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE HDRBESTATS15 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEHDRBEStats15Titan17x final : public ISPHWSetting
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
    /// ~IFEHDRBEStats15Titan17x
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEHDRBEStats15Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEHDRBEStats15Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEHDRBEStats15Titan17x();

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
    IFEHDRBE15RegCmd    m_regCmd; ///< Register List of this Module

    IFEHDRBEStats15Titan17x(const IFEHDRBEStats15Titan17x&)            = delete; ///< Disallow the copy constructor
    IFEHDRBEStats15Titan17x& operator=(const IFEHDRBEStats15Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEHDRBESTATS15TITAN17X_H
