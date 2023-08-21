////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedualpd10titan17x.h
/// @brief IFE DUALPD register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEDUALPD10TITAN17X_H
#define CAMXIFEDUALPD10TITAN17X_H

#include "titan170_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

struct IFEDUALPD10ModuleEnable
{
    IFE_IFE_0_VFE_MODULE_DUAL_PD_EN pdModuleEnable;     ///< PD Module enable
} CAMX_PACKED;

/// @brief IFE DUALPD10 Module Dependence Data
struct IFEDUALPD10RegCmd
{
    IFE_IFE_0_VFE_DUAL_PD_CFG               pdCfg;              ///< pdCfg
    IFE_IFE_0_VFE_DUAL_PD_CROP_WIDTH_CFG    pdCropWidthCfg;     ///< pdCropWidthCfg
    IFE_IFE_0_VFE_DUAL_PD_CROP_HEIGHT_CFG   pdCropHeightCfg;    ///< pdCropHeightCfg
    IFE_IFE_0_VFE_DUAL_PD_BLS_CFG           pdBLSCfg;           ///< pdBLSCfg
    IFE_IFE_0_VFE_DUAL_PD_RGN_OFFSET_CFG    pdRegionOffsetCfg;  ///< pdRegionOffsetCfg
    IFE_IFE_0_VFE_DUAL_PD_RGN_NUM_CFG       pdRegionNumberCfg;  ///< pdRegionNumberCfg
    IFE_IFE_0_VFE_DUAL_PD_RGN_SIZE_CFG      pdRegionSizeCfg;    ///< pdRegionSizeCfg
    IFE_IFE_0_VFE_DUAL_PD_HDR_CFG           pdHDRCfg;           ///< pdHDRCfg
    IFE_IFE_0_VFE_DUAL_PD_BIN_SKIP_CFG      pdBinSkipCfg;       ///< pdBinSkipCfg
    IFE_IFE_0_VFE_DUAL_PD_GM_CFG_0          pdGMCfg0;           ///< pdGMCfg0
    IFE_IFE_0_VFE_DUAL_PD_GM_CFG_1          pdGMCfg1;           ///< pdGMCfg1
    IFE_IFE_0_VFE_DUAL_PD_GM_CFG_2          pdGMCfg2;           ///< pdGMCfg2
    IFE_IFE_0_VFE_DUAL_PD_GM_CFG_3          pdGMCfg3;           ///< pdGMCfg3
    IFE_IFE_0_VFE_DUAL_PD_IIR_CFG_0         pdIIRCfg0;          ///< pdIIRCfg0
    IFE_IFE_0_VFE_DUAL_PD_IIR_CFG_1         pdIIRCfg1;          ///< pdIIRCfg1
    IFE_IFE_0_VFE_DUAL_PD_IIR_CFG_2         pdIIRCfg2;          ///< pdIIRCfg2
    IFE_IFE_0_VFE_DUAL_PD_IIR_CFG_3         pdIIRCfg3;          ///< pdIIRCfg3
    IFE_IFE_0_VFE_DUAL_PD_SAD_PHASE_CFG_0   pdSADPhaseCfg0;     ///< pdSADPhaseCfg0
    IFE_IFE_0_VFE_DUAL_PD_SAD_PHASE_CFG_1   pdSADPhaseCfg1;     ///< pdSADPhaseCfg1
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT DualPDLUTBank0 = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_DUAL_PD_GM_LUT_BANK0;
static const UINT DualPDLUTBank1 = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_DUAL_PD_GM_LUT_BANK1;

static const UINT32 IFEDUALPD10RegLengthDWord = sizeof(IFEDUALPD10RegCmd) / sizeof(UINT32);

static const UINT32 IFEDualPD10LutTableRow      = 17;
static const UINT32 IFEDualPD10LutTableColumn   = 13;
static const UINT32 IFEDualPD10LutTableSize     = (IFEDualPD10LutTableRow * IFEDualPD10LutTableColumn);
static const UINT32 IFEDualPD10DMILengthDWord   = IFEDualPD10LutTableSize * sizeof(UINT32);

#define FLOAT_TO_Q(exp, f) \
    (static_cast<INT32>(((f)*(1<<(exp))) + (((f)<0) ? -0.5 : 0.5)))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE DUALPD register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEDualPD10Titan17x final : public ISPHWSetting
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
    /// ~IFEDualPD10Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEDualPD10Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEDualPD10Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEDualPD10Titan17x();

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
    IFEDUALPD10ModuleEnable m_pdModuleEnable;            ///< Register for PD module Enable
    IFEDUALPD10RegCmd       m_regCmd;                    ///< Register List of this Module

    IFEDualPD10Titan17x(const IFEDualPD10Titan17x&)            = delete; ///< Disallow the copy constructor
    IFEDualPD10Titan17x& operator=(const IFEDualPD10Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEDUALPD10TITAN17X_H
