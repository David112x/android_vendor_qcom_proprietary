////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifegamma16titan17x.h
/// @brief IFE Gamma16 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEGAMMA16TITAN17X_H
#define CAMXIFEGAMMA16TITAN17X_H

#include "titan170_ife.h"
#include "camxisphwsetting.h"
#include "gamma16setting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

// @brief: This structure contains the register list of the Gamma16 module
struct IFEGamma16RegCmd
{
    IFE_IFE_0_VFE_RGB_LUT_CFG rgb_lut_cfg; ///< RGB_LUT_CFG
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IFEGamma16RegLengthDword      = sizeof(IFEGamma16RegCmd) / sizeof(UINT32);
static const UINT8  IFEGamma16NumDMITables        = 3;
static const UINT32 Gamma16NumberOfEntriesPerLUT  = 64;
static const UINT32 IFEGamma16LutTableSize        = Gamma16NumberOfEntriesPerLUT * sizeof(UINT32);
static const UINT32 IFEGamma16DMILengthDword      = Gamma16NumberOfEntriesPerLUT * IFEGamma16NumDMITables;
static const UINT GammaGLUTBank0                  = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_RGBLUT_RAM_CH0_BANK0;
static const UINT GammaGLUTBank1                  = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_RGBLUT_RAM_CH0_BANK1;
static const UINT GammaBLUTBank0                  = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_RGBLUT_RAM_CH1_BANK0;
static const UINT GammaBLUTBank1                  = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_RGBLUT_RAM_CH1_BANK1;
static const UINT GammaRLUTBank0                  = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_RGBLUT_RAM_CH2_BANK0;
static const UINT GammaRLUTBank1                  = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_RGBLUT_RAM_CH2_BANK1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE Gamma16 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEGamma16Titan17x final : public ISPHWSetting
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
    /// ~IFEGamma16Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEGamma16Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEGamma16Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEGamma16Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFEGamma16RegCmd    m_regCmd;                   ///< Register List of this Module
    UINT8               m_channelRLUTBankSelect;    ///< Bank Selection for R Channel Lut
    UINT8               m_channelGLUTBankSelect;    ///< Bank Selection for G Channel Lut
    UINT8               m_channelBLUTBankSelect;    ///< Bank Selection for B Channel Lut
    UINT32*             m_pGammaRLUT;               ///< DMI R LUT Pointer
    UINT32*             m_pGammaGLUT;               ///< DMI G LUT Pointer
    UINT32*             m_pGammaBLUT;               ///< DMI B LUT Pointer

    IFEGamma16Titan17x(const IFEGamma16Titan17x&)            = delete; ///< Disallow the copy constructor
    IFEGamma16Titan17x& operator=(const IFEGamma16Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEGAMMA16TITAN17X_H
