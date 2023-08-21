////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecsstats14titan17x.h
/// @brief IFE CSStats14 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFECSSTATS14TITAN17X_H
#define CAMXIFECSSTATS14TITAN17X_H

#include "titan170_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispstatsmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief CS Stats Configuration
struct IFECS14RegionConfig
{
    IFE_IFE_0_VFE_STATS_CS_RGN_OFFSET_CFG   regionOffset;   ///< CS stats region offset config
    IFE_IFE_0_VFE_STATS_CS_RGN_NUM_CFG      regionNum;      ///< CS stats region number config
    IFE_IFE_0_VFE_STATS_CS_RGN_SIZE_CFG     regionSize;     ///< CS stats region size config
} CAMX_PACKED;

CAMX_END_PACKED

// HW capabilities
static const UINT32 CSTitan17xInputDepth             = 12;      ///< Input depth to CS stats
static const UINT32 CSTitan17xOutputDepth            = 16;      ///< Output bit depths of CS stats
static const UINT32 CSStats14Titan17xMaxRegionWidth  = 4;       ///< Maximum CS stats region width
static const UINT32 CSStats14Titan17xMinRegionWidth  = 2;       ///< Minimum CS stats region width
static const UINT32 CSStats14Titan17xMinRegionHeight = 1;       ///< Minimum CS stats region height
static const UINT32 CSStats14Titan17xMinHorizRegions = 5;       ///< Minimum CS stats horizontal region
static const UINT32 CSStats14Titan17xMaxHorizRegions = 1560;    ///< Maximum CS stats horizontal region
static const UINT32 CSStats14Titan17xMinVertRegions  = 1;       ///< Minimum CS stats vertical region
static const UINT32 CSStats14Titan17xMaxVertRegions  = 4;       ///< Maximum CS stats vertical region

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE CS Stats 1.4 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFECSStats14Titan17x final : public ISPHWSetting
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
    /// ~IFECSStats14Titan17x
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFECSStats14Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFECSStats14Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFECSStats14Titan17x();

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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AdjustHorizontalRegionNumber
    ///
    /// @brief  Adjust horizontal region number for hardware limitations which states h_num % 8 must be within [4,7]
    ///
    /// @param  regionHNum      horizontal region number
    /// @param  divider         divider for horizontal number
    /// @param  minRemainder    minimum remainder value to limit to
    ///
    /// @return adjusted horizontal number
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 AdjustHorizontalRegionNumber(
        UINT32  regionHNum,
        UINT32  divider,
        UINT32  minRemainder);

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

private:
    IFECS14RegionConfig m_regCmd;   ///< Register List of this Module

    IFECSStats14Titan17x(const IFECSStats14Titan17x&)            = delete; ///< Disallow the copy constructor
    IFECSStats14Titan17x& operator=(const IFECSStats14Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFECSSTATS14TITAN17X_H
