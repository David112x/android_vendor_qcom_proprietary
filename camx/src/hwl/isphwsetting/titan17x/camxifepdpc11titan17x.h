////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepdpc11titan17x.h
/// @brief IFE PDPC11 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEPDPC11TITAN17X_H
#define CAMXIFEPDPC11TITAN17X_H

#include "titan170_ife.h"
#include "camxisphwsetting.h"
#include "ifepdpc11setting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IFE PDPC Module Register Set
struct IFEPDPC11RegCmd
{
    IFE_IFE_0_VFE_PDAF_CFG            configRegister;           ///< Configuration Register. Offset: 0xcb4
    IFE_IFE_0_VFE_PDAF_HDR_EXP_RATIO  ratioHDRRegister;         ///< HDR Ratio Register. Offset: 0xcb8
    IFE_IFE_0_VFE_PDAF_BP_TH          setBPTHregister;          ///< Bad Pixtel Threshold Register. Offset: 0xcbc
    IFE_IFE_0_VFE_PDAF_T2_BP_OFFSET   offsetT2BPRegister;       ///< Bad Pixtel Threshold Offset Register. Offset: 0xcc0
    IFE_IFE_0_VFE_PDAF_BP_OFFSET      offsetBPRegister;         ///< Bad Pixel Threshold Offset for T2 Green Pixel. 0xcc4
    IFE_IFE_0_VFE_PDAF_RG_WB_GAIN     gainRGWBRegister;         ///< AWG Gain Register for Red/Green Channel. offset: 0xcc8
    IFE_IFE_0_VFE_PDAF_BG_WB_GAIN     gainBGWBRegister;         ///< AWG Gain Register for Blue/Green Channel. Offset: 0xccc
    IFE_IFE_0_VFE_PDAF_GR_WB_GAIN     gainGRWBRegister;         ///< AWG Gain Register for Green/Red Channel. Offset: 0xCD0
    IFE_IFE_0_VFE_PDAF_GB_WB_GAIN     gainGBWBRegister;         ///< AWG Gain Register for Green/Blue Channel. Offset: 0xCD4
    IFE_IFE_0_VFE_PDAF_LOC_OFFSET_CFG offsetLOCConfigRegister;  ///< PDAF Line Offset Register. Offset: 0xCD8
    IFE_IFE_0_VFE_PDAF_LOC_END_CFG    setLOCENDConfigRegister;  ///< PDAF Pixel end Location register. Offset: 0xCDC
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 PDAFLUT                 = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_PDAF_LUT;
static const UINT32 IFEPDPC11RegLengthDword = sizeof(IFEPDPC11RegCmd) / sizeof(UINT32);
static const UINT32 IFEPDPC11DMILengthDword = DMIRAM_PDAF_LUT_LENGTH;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE PDPC11 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEPDPC11Titan17x final : public ISPHWSetting
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
    /// ~IFEPDPC11Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEPDPC11Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEPDPC11Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEPDPC11Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFEPDPC11RegCmd    m_regCmd;        ///< Register List of this Module
    BOOL               m_isLUTLoaded;   ///< Determine if the Lut has been loaded

    IFEPDPC11Titan17x(const IFEPDPC11Titan17x&)            = delete; ///< Disallow the copy constructor
    IFEPDPC11Titan17x& operator=(const IFEPDPC11Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEPDPC11TITAN17X_H
