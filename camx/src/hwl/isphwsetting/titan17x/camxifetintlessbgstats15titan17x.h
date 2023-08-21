////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifetintlessbgstats15titan17x.h
/// @brief IFE TINTLESSBGSTATS15 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFETINTLESSBGSTATS15TITAN17X_H
#define CAMXIFETINTLESSBGSTATS15TITAN17X_H

#include "titan170_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispstatsmodule.h"
#include "camxifetintlessbgstats15.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
/// @brief TintlessBG Stats ROI Configuration
struct IFETintlessBG15RegionConfig
{
    IFE_IFE_0_VFE_STATS_AEC_BG_RGN_OFFSET_CFG     regionOffset;   ///< TintlessBG stats region offset config
    IFE_IFE_0_VFE_STATS_AEC_BG_RGN_NUM_CFG        regionNumber;   ///< TintlessBG stats region number config
    IFE_IFE_0_VFE_STATS_AEC_BG_RGN_SIZE_CFG       regionSize;     ///< TintlessBG stats region size config
    IFE_IFE_0_VFE_STATS_AEC_BG_HI_THRESHOLD_CFG_0 highThreshold0; ///< TintlessBG stats pixel upper threshold config
    IFE_IFE_0_VFE_STATS_AEC_BG_HI_THRESHOLD_CFG_1 highThreshold1; ///< TintlessBG stats pixel upper threshold config
    IFE_IFE_0_VFE_STATS_AEC_BG_LO_THRESHOLD_CFG_0 lowThreshold0;  ///< TintlessBG stats pixel lower threshold config
    IFE_IFE_0_VFE_STATS_AEC_BG_LO_THRESHOLD_CFG_1 lowThreshold1;  ///< TintlessBG stats pixel lower threshold config
} CAMX_PACKED;

/// @brief TintlessBG Stats Configuration
struct IFETintlessBG15RegCmd
{
    IFETintlessBG15RegionConfig         regionConfig;          ///< TintlessBG stats region and threshold config
    IFE_IFE_0_VFE_STATS_AEC_BG_CH_Y_CFG lumaConfig;            ///< TintlessBG stats luma channel config
    IFE_IFE_0_VFE_STATS_AEC_BG_CFG      tintlessBGStatsconfig; ///< TintlessBG stats config
}CAMX_PACKED;

CAMX_END_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE TINTLESSBGSTATS15 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFETintlessBGStats15Titan17x final : public ISPHWSetting
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
    /// ~IFETintlessBGStats15Titan17x
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFETintlessBGStats15Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFETintlessBGStats15Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFETintlessBGStats15Titan17x();

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
    IFETintlessBG15RegCmd    m_regCmd; ///< Register List of this Module

    IFETintlessBGStats15Titan17x(const IFETintlessBGStats15Titan17x&)            = delete; ///< Disallow the copy constructor
    IFETintlessBGStats15Titan17x& operator=(const IFETintlessBGStats15Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFETINTLESSBGSTATS15TITAN17X_H
