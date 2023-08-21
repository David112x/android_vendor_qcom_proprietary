////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeabf34titan17x.h
/// @brief IFE ABF34 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEABF34TITAN17X_H
#define CAMXIFEABF34TITAN17X_H

#include "titan170_ife.h"
#include "camxisphwsetting.h"
#include "ifeabf34setting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

struct IFEABF34RegCmd1
{
    IFE_IFE_0_VFE_ABF_CFG                  configReg;            ///< Config Register. Offset: 0x5e8
} CAMX_PACKED;

/// @brief IFE ABF34 Register Command Set 2
struct IFEABF34RegCmd2
{
    IFE_IFE_0_VFE_ABF_GR_CFG               configGRChannel;      ///< Config Register for GR Channel. Offset: 0x5f4
    IFE_IFE_0_VFE_ABF_GB_CFG               configGBChannel;      ///< Config Register for GB Channel. Offset: 0x5f8
    IFE_IFE_0_VFE_ABF_R_CFG                configRChannel;       ///< Config Register for R Channel. Offset: 0x5fc
    IFE_IFE_0_VFE_ABF_B_CFG                configBChannel;       ///< Config Register for B Channel. Offset: 0x600
    IFE_IFE_0_VFE_ABF_RNR_CFG_0            configRNR0;           ///< RNR Config Register 0 Channel. Offset: 0x604
    IFE_IFE_0_VFE_ABF_RNR_CFG_1            configRNR1;           ///< RNR Config Register 1 Channel. Offset: 0x608
    IFE_IFE_0_VFE_ABF_RNR_CFG_2            configRNR2;           ///< RNR Config Register 2 Channel. Offset: 0x60C
    IFE_IFE_0_VFE_ABF_RNR_CFG_3            configRNR3;           ///< RNR Config Register 3 Channel. Offset: 0x610
    IFE_IFE_0_VFE_ABF_RNR_CFG_4            configRNR4;           ///< RNR Config Register 4 Channel. Offset: 0x614
    IFE_IFE_0_VFE_ABF_RNR_CFG_5            configRNR5;           ///< RNR Config Register 5 Channel. Offset: 0x618
    IFE_IFE_0_VFE_ABF_RNR_CFG_6            configRNR6;           ///< RNR Config Register 6 Channel. Offset: 0x61C
    IFE_IFE_0_VFE_ABF_RNR_CFG_7            configRNR7;           ///< RNR Config Register 7 Channel. Offset: 0x620
    IFE_IFE_0_VFE_ABF_RNR_CFG_8            configRNR8;           ///< RNR Config Register 8 Channel. Offset: 0x624
    IFE_IFE_0_VFE_ABF_RNR_CFG_9            configRNR9;           ///< RNR Config Register 9 Channel. Offset: 0x628
    IFE_IFE_0_VFE_ABF_RNR_CFG_10           configRNR10;          ///< RNR Config Register 10 Channel. Offset: 0x62C
    IFE_IFE_0_VFE_ABF_RNR_CFG_11           configRNR11;          ///< RNR Config Register 11 Channel. Offset: 0x630
    IFE_IFE_0_VFE_ABF_RNR_CFG_12           configRNR12;          ///< RNR Config Register 12 Channel. Offset: 0x634
    IFE_IFE_0_VFE_ABF_BPC_CFG_0            configBPC0;           ///< BPC Config Register 0 Channel. Offset: 0x638
    IFE_IFE_0_VFE_ABF_BPC_CFG_1            configBPC1;           ///< BPC Config Register 1 Channel. Offset: 0x63C
    IFE_IFE_0_VFE_ABF_NOISE_PRESERVE_CFG_0 noisePreserveConfig0; ///< Noise Preserve Config Register 0. Offset: 0x640
    IFE_IFE_0_VFE_ABF_NOISE_PRESERVE_CFG_1 noisePreserveConfig1; ///< Noise Preserve Config Register 1. Offset: 0x644
    IFE_IFE_0_VFE_ABF_NOISE_PRESERVE_CFG_2 noisePreserveConfig2; ///< Noise Preserve Config Register 2. Offset: 0x648
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT8  ABFBank0                = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_ABF_STD2_L0_BANK0;
static const UINT8  ABFBank1                = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_ABF_STD2_L0_BANK1;
static const UINT32 IFEABF34RegLength1DWord = sizeof(IFEABF34RegCmd1) / sizeof(UINT32);
static const UINT32 IFEABF34RegLength2DWord = sizeof(IFEABF34RegCmd2) / sizeof(UINT32);
static const UINT32 IFEABF34LUTLengthDWord  = DMIRAM_ABF34_NOISESTD_LENGTH;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE ABF34 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEABF34Titan17x final : public ISPHWSetting
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
    /// ~IFEABF34Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEABF34Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEABF34Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEABF34Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFEABF34RegCmd1 m_regCmd1;  ///< Register Set 1 of this Module
    IFEABF34RegCmd2 m_regCmd2;  ///< Register Set 2 of this Module
    UINT32*         m_pDMIData; ///< DMI buffer pinter

    IFEABF34Titan17x(const IFEABF34Titan17x&)            = delete; ///< Disallow the copy constructor
    IFEABF34Titan17x& operator=(const IFEABF34Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEABF34TITAN17X_H
