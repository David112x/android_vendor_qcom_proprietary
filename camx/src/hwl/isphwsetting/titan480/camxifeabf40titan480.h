////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeabf40titan480.h
/// @brief IFE ABF40 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEABF40TITAN480_H
#define CAMXIFEABF40TITAN480_H

#include "titan480_ife.h"
#include "camxisphwsetting.h"
#include "abf40setting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IFE ABF40 Register Command Set 1
struct IFEABF40RegCmd1
{
    IFE_IFE_0_PP_CLC_ABF_DMI_LUT_BANK_CFG      DMILUTBankConfig;        ///< DMI LUT Bank Config
    IFE_IFE_0_PP_CLC_ABF_MODULE_LUT_BANK_CFG   moduleLUTBankConfig;     ///< Module LUT Bank Config
    IFE_IFE_0_PP_CLC_ABF_MODULE_CFG            configReg;               ///< Config Register
} CAMX_PACKED;

/// @brief IFE ABF40 Register Command Set 2
struct IFEABF40RegCmd2
{
    IFE_IFE_0_PP_CLC_ABF_ABF_0_CFG      config0;    ///< ABF Config 0
    IFE_IFE_0_PP_CLC_ABF_ABF_1_CFG      config1;    ///< ABF Config 1
    IFE_IFE_0_PP_CLC_ABF_ABF_2_CFG      config2;    ///< ABF Config 2
    IFE_IFE_0_PP_CLC_ABF_ABF_3_CFG      config3;    ///< ABF Config 3
    IFE_IFE_0_PP_CLC_ABF_ABF_4_CFG      config4;    ///< ABF Config 4
    IFE_IFE_0_PP_CLC_ABF_ABF_5_CFG      config5;    ///< ABF Config 5
    IFE_IFE_0_PP_CLC_ABF_ABF_6_CFG      config6;    ///< ABF Config 6
    IFE_IFE_0_PP_CLC_ABF_ABF_7_CFG      config7;    ///< ABF Config 7
    IFE_IFE_0_PP_CLC_ABF_ABF_8_CFG      config8;    ///< ABF Config 8
    IFE_IFE_0_PP_CLC_ABF_ABF_9_CFG      config9;    ///< ABF Config 9
    IFE_IFE_0_PP_CLC_ABF_ABF_10_CFG     config10;   ///< ABF Config 10
    IFE_IFE_0_PP_CLC_ABF_ABF_11_CFG     config11;   ///< ABF Config 11
    IFE_IFE_0_PP_CLC_ABF_ABF_12_CFG     config12;   ///< ABF Config 12
    IFE_IFE_0_PP_CLC_ABF_ABF_13_CFG     config13;   ///< ABF Config 13
    IFE_IFE_0_PP_CLC_ABF_ABF_14_CFG     config14;   ///< ABF Config 14
    IFE_IFE_0_PP_CLC_ABF_ABF_15_CFG     config15;   ///< ABF Config 15
    IFE_IFE_0_PP_CLC_ABF_ABF_16_CFG     config16;   ///< ABF Config 16
    IFE_IFE_0_PP_CLC_ABF_ABF_17_CFG     config17;   ///< ABF Config 17
    IFE_IFE_0_PP_CLC_ABF_ABF_18_CFG     config18;   ///< ABF Config 18
    IFE_IFE_0_PP_CLC_ABF_ABF_19_CFG     config19;   ///< ABF Config 19
    IFE_IFE_0_PP_CLC_ABF_ABF_20_CFG     config20;   ///< ABF Config 20
    IFE_IFE_0_PP_CLC_ABF_ABF_21_CFG     config21;   ///< ABF Config 21
    IFE_IFE_0_PP_CLC_ABF_ABF_22_CFG     config22;   ///< ABF Config 22
    IFE_IFE_0_PP_CLC_ABF_ABF_23_CFG     config23;   ///< ABF Config 23
    IFE_IFE_0_PP_CLC_ABF_ABF_24_CFG     config24;   ///< ABF Config 24
    IFE_IFE_0_PP_CLC_ABF_ABF_25_CFG     config25;   ///< ABF Config 25
    IFE_IFE_0_PP_CLC_ABF_ABF_26_CFG     config26;   ///< ABF Config 26
    IFE_IFE_0_PP_CLC_ABF_ABF_27_CFG     config27;   ///< ABF Config 27
    IFE_IFE_0_PP_CLC_ABF_ABF_28_CFG     config28;   ///< ABF Config 28
    IFE_IFE_0_PP_CLC_ABF_ABF_29_CFG     config29;   ///< ABF Config 29
    IFE_IFE_0_PP_CLC_ABF_ABF_30_CFG     config30;   ///< ABF Config 30
    IFE_IFE_0_PP_CLC_ABF_ABF_31_CFG     config31;   ///< ABF Config 31
    IFE_IFE_0_PP_CLC_ABF_ABF_32_CFG     config32;   ///< ABF Config 32
    IFE_IFE_0_PP_CLC_ABF_ABF_33_CFG     config33;   ///< ABF Config 33
    IFE_IFE_0_PP_CLC_ABF_ABF_34_CFG     config34;   ///< ABF Config 34
    IFE_IFE_0_PP_CLC_ABF_ABF_35_CFG     config35;   ///< ABF Config 35
    IFE_IFE_0_PP_CLC_ABF_ABF_36_CFG     config36;   ///< ABF Config 36
    IFE_IFE_0_PP_CLC_ABF_ABF_37_CFG     config37;   ///< ABF Config 37
    IFE_IFE_0_PP_CLC_ABF_ABF_38_CFG     config38;   ///< ABF Config 38
    IFE_IFE_0_PP_CLC_ABF_ABF_39_CFG     config39;   ///< ABF Config 39
    IFE_IFE_0_PP_CLC_ABF_ABF_40_CFG     config40;   ///< ABF Config 40
    IFE_IFE_0_PP_CLC_ABF_ABF_41_CFG     config41;   ///< ABF Config 41
    IFE_IFE_0_PP_CLC_ABF_ABF_42_CFG     config42;   ///< ABF Config 42
    IFE_IFE_0_PP_CLC_ABF_ABF_43_CFG     config43;   ///< ABF Config 43
    IFE_IFE_0_PP_CLC_ABF_ABF_44_CFG     config44;   ///< ABF Config 44
    IFE_IFE_0_PP_CLC_ABF_ABF_45_CFG     config45;   ///< ABF Config 45
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IFEABF40RegLength1DWord         = sizeof(IFEABF40RegCmd1) / sizeof(UINT32);
static const UINT32 IFEABF40RegLength2DWord         = sizeof(IFEABF40RegCmd2) / sizeof(UINT32);
static const UINT32 IFEABF40LUTLengthDWord          = DMIRAM_ABF40_NOISESTD_LENGTH;
static const UINT32 IFENoiseLUT0                    = IFE_IFE_0_PP_CLC_ABF_DMI_LUT_CFG_LUT_SEL_NOISE_LUT;
static const UINT32 IFEActivityLUT                  = IFE_IFE_0_PP_CLC_ABF_DMI_LUT_CFG_LUT_SEL_ACT_LUT;
static const UINT32 IFEDarkLUT                      = IFE_IFE_0_PP_CLC_ABF_DMI_LUT_CFG_LUT_SEL_DARK_LUT;
static const UINT8  IFEABFLUTBank0                  = IFE_IFE_0_PP_CLC_ABF_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT8  IFEABFLUTBank1                  = IFE_IFE_0_PP_CLC_ABF_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK1;
static const UINT32 IFEMaxABFLUT                    = 4;
static const UINT32 IFEABF40NoiseLUTSizeDword       = 64;  // 64 entries in the DMI table
static const UINT32 IFEABF40NoiseLUTSize            = (IFEABF40NoiseLUTSizeDword * sizeof(UINT32));
static const UINT32 IFEABF40ActivityLUTSizeDword    = 32;  // 32 entries in the DMI table
static const UINT32 IFEABF40ActivityLUTSize         = (IFEABF40ActivityLUTSizeDword * sizeof(UINT32));
static const UINT32 IFEABF40DarkLUTSizeDword        = 42;  // 48 entries in the DMI table
static const UINT32 IFEABF40DarkLUTSize             = (IFEABF40DarkLUTSizeDword * sizeof(UINT32));
static const UINT32 IFEABFTotalDMILengthDword       = (IFEABF40NoiseLUTSizeDword    +
                                                       IFEABF40ActivityLUTSizeDword +
                                                       IFEABF40DarkLUTSizeDword);
static const UINT32 IFEABFTotalLUTBufferSize        = (IFEABFTotalDMILengthDword * sizeof(UINT32));



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE ABF40 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEABF40Titan480 final : public ISPHWSetting
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
    /// ~IFEABF40Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEABF40Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEABF40Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEABF40Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFEABF40RegCmd1 m_regCmd1;          ///< Register Set 1 of this Module
    IFEABF40RegCmd2 m_regCmd2;          ///< Register Set 2 of this Module
    UINT32*         m_pNoiseLUT;        ///< ABF DMI Noise LUT pointer
    UINT32*         m_pActivityLUT;     ///< ABF DMI Activity LUT pointer
    UINT32*         m_pDarkLUT;         ///< ABF DMI Dark LUT pointer

    IFEABF40Titan480(const IFEABF40Titan480&)            = delete; ///< Disallow the copy constructor
    IFEABF40Titan480& operator=(const IFEABF40Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEABF40TITAN480_H
