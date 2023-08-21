////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifegamma16titan480.h
/// @brief IFE Gamma16 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEGAMMA16TITAN480_H
#define CAMXIFEGAMMA16TITAN480_H

#include "titan480_ife.h"
#include "camxisphwsetting.h"
#include "gamma16setting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

// @brief: This structure contains the register list of the Gamma16 module
struct IFEGamma16Titan480RegCmd
{
    IFE_IFE_0_PP_CLC_GLUT_DMI_LUT_BANK_CFG      DMILUTBankConfig;       ///< DMI LUT bank config
    IFE_IFE_0_PP_CLC_GLUT_MODULE_LUT_BANK_CFG   moduleLUTBankConfig;    ///< Module LUT bank config
    IFE_IFE_0_PP_CLC_GLUT_MODULE_CFG            config;                 ///< module config
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IFEGamma16Titan480RegLengthDword      = sizeof(IFEGamma16Titan480RegCmd) / sizeof(UINT32);
static const UINT8  IFEGamma16Titan480NumDMITables        = 3;
static const UINT32 Gamma16Titan480NumberOfEntriesPerLUT  = 64;
static const UINT32 IFEGamma16Titan480LutTableSize        = Gamma16Titan480NumberOfEntriesPerLUT * sizeof(UINT32);
static const UINT32 IFEGamma16Titan480DMILengthDword      = Gamma16Titan480NumberOfEntriesPerLUT *
                                                            IFEGamma16Titan480NumDMITables;
static const UINT   Gamma16Titan480LUTBank0               = IFE_IFE_0_PP_CLC_GLUT_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT   Gamma16Titan480LUTBank1               = IFE_IFE_0_PP_CLC_GLUT_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK1;

static const UINT32 Gamma16Titan480Channel0LUT = IFE_IFE_0_PP_CLC_GLUT_DMI_LUT_CFG_LUT_SEL_GLUT_CH0_LUT;
static const UINT32 Gamma16Titan480Channel1LUT = IFE_IFE_0_PP_CLC_GLUT_DMI_LUT_CFG_LUT_SEL_GLUT_CH1_LUT;
static const UINT32 Gamma16Titan480Channel2LUT = IFE_IFE_0_PP_CLC_GLUT_DMI_LUT_CFG_LUT_SEL_GLUT_CH2_LUT;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE Gamma16 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEGamma16Titan480 final : public ISPHWSetting
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
    /// CreateSubCmdList
    ///
    /// @brief  Generate the Sub Command List
    ///
    /// @param  pInputData       Pointer to the Inputdata
    /// @param  pDMIBufferOffset Pointer for DMI Buffer
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult CreateSubCmdList(
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
    /// ~IFEGamma16Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEGamma16Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEGamma16Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEGamma16Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFEGamma16Titan480RegCmd    m_regCmd;                   ///< Register List of this Module
    UINT8                       m_channelLUTBankSelect;     ///< Bank Selection

    UINT32*                     m_pGammaRLUT;               ///< DMI R LUT Pointer
    UINT32*                     m_pGammaGLUT;               ///< DMI G LUT Pointer
    UINT32*                     m_pGammaBLUT;               ///< DMI B LUT Pointer

    IFEGamma16Titan480(const IFEGamma16Titan480&)            = delete; ///< Disallow the copy constructor
    IFEGamma16Titan480& operator=(const IFEGamma16Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEGAMMA16TITAN480_H
