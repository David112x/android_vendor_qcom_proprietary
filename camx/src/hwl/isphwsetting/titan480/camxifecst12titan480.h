////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecst12titan480.h
/// @brief IFE CST12 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFECST12TITAN480_H
#define CAMXIFECST12TITAN480_H

#include "titan480_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

///@ brief IFE CST12 Channel Config
struct IFECST12Titan480ChannelConfig
{
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH0_COEFF_CFG_0    ch0Coefficient0;        ///< CH1 COEFF CFG 0
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH0_COEFF_CFG_1    ch0Coefficient1;        ///< CH1 COEFF CFG 1
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH0_OFFSET_CFG     ch0OffsetConfig;        ///< CH0 Offset CFG
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH0_CLAMP_CFG      ch0ClampConfig;         ///< CH0 Clamp CFG
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH1_COEFF_CFG_0    ch1Coefficient0;        ///< CH1 COEFF CFG 0
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH1_COEFF_CFG_1    ch1Coefficient1;        ///< CH1 COEFF CFG 1
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH1_OFFSET_CFG     ch1OffsetConfig;        ///< CH1 Offset CFG
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH1_CLAMP_CFG      ch1ClampConfig;         ///< CH1 Clamp CFG
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH2_COEFF_CFG_0    ch2Coefficient0;        ///< CH2 COEFF CFG 0
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH2_COEFF_CFG_1    ch2Coefficient1;        ///< CH2 COEFF CFG 1
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH2_OFFSET_CFG     ch2OffsetConfig;        ///< CH2 Offset CFG
    IFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH2_CLAMP_CFG      ch2ClampConfig;         ///< CH2 Clamp CFG
} CAMX_PACKED;

/// @brief IFE CST12 Module Dependence Data
struct IFECST12Titan480RegCmd
{
    IFE_IFE_0_PP_CLC_COLOR_XFORM_MODULE_CFG  moduleConfig;           ///< Module config
    IFECST12Titan480ChannelConfig            channelConfig;          ///< Channel Config
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IFECST12Titan480RegLengthDWord = sizeof(IFECST12Titan480RegCmd) / sizeof(UINT32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE CST12 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFECST12Titan480 final : public ISPHWSetting
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
    /// ~IFECST12Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFECST12Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFECST12Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFECST12Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFECST12Titan480RegCmd    m_regCmd; ///< Register List of this Module

    IFECST12Titan480(const IFECST12Titan480&)            = delete; ///< Disallow the copy constructor
    IFECST12Titan480& operator=(const IFECST12Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFECST12TITAN480_H
