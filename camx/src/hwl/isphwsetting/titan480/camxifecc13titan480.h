////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecc13titan480.h
/// @brief IFE CC13 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFECC13TITAN480_H
#define CAMXIFECC13TITAN480_H

#include "titan480_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
/// @brief IFE Color Correction Module Register Set
struct IFECC13RegCmd1
{
    IFE_IFE_0_PP_CLC_COLOR_CORRECT_MODULE_CFG    moduleConfig;   ///< module config
} CAMX_PACKED;

/// @brief IFE Color Correction Module Register Set
struct IFECC13RegCmd2
{
    IFE_IFE_0_PP_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_A_CFG_0   coefficientAConfig0;   ///< Coefficient A Config0
    IFE_IFE_0_PP_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_A_CFG_1   coefficientAConfig1;   ///< Coefficient A Config1
    IFE_IFE_0_PP_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_B_CFG_0   coefficientBConfig0;   ///< Coefficient B Config0
    IFE_IFE_0_PP_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_B_CFG_1   coefficientBConfig1;   ///< Coefficient B Config1
    IFE_IFE_0_PP_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_C_CFG_0   coefficientCConfig0;   ///< Coefficient C Config0
    IFE_IFE_0_PP_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_C_CFG_1   coefficientCConfig1;   ///< Coefficient C Config1
    IFE_IFE_0_PP_CLC_COLOR_CORRECT_COLOR_CORRECT_OFFSET_K_CFG_0  offsetKConfig0;        ///< Offset K Config 0
    IFE_IFE_0_PP_CLC_COLOR_CORRECT_COLOR_CORRECT_OFFSET_K_CFG_1  offsetKConfig1;        ///< Offset K Config 1
    IFE_IFE_0_PP_CLC_COLOR_CORRECT_COLOR_CORRECT_SHIFT_M_CFG     shiftMConfig;          ///< sift M Config
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IFECC13RegLengthDWord1 = sizeof(IFECC13RegCmd1) / sizeof(UINT32);
static const UINT32 IFECC13RegLengthDWord2 = sizeof(IFECC13RegCmd2) / sizeof(UINT32);
static const UINT32 CC13QFactor           = 7;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE CC13 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFECC13Titan480 final : public ISPHWSetting
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
    /// ~IFECC13Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFECC13Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFECC13Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFECC13Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFECC13RegCmd1    m_regCmd1; ///< Register List of this Module
    IFECC13RegCmd2    m_regCmd2; ///< Register List of this Module

    IFECC13Titan480(const IFECC13Titan480&)            = delete; ///< Disallow the copy constructor
    IFECC13Titan480& operator=(const IFECC13Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFECC13TITAN480_H
