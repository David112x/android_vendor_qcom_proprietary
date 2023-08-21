////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdrbhiststats13titan480.h
/// @brief IFE HDRBHISTStats13 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEHDRBHISTSTATS13TITAN480_H
#define CAMXIFEHDRBHISTSTATS13TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispstatsmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief HDRBHist HW specific const values
static const UINT8  IFEHDRBHist13Titan480DMITables              = 3;
static const UINT32 IFEHDRBHist13Titan480NumberOfEntriesPerLUT  = 256;
static const UINT32 IFEHDRBHist13Titan480LutTableSize           = IFEHDRBHist13Titan480NumberOfEntriesPerLUT * sizeof(UINT32);
static const UINT32 IFEHDRBHist13Titan480DMILengthDword         =
                                                    IFEHDRBHist13Titan480NumberOfEntriesPerLUT * IFEHDRBHist13Titan480DMITables;
static const UINT   IFEHDRBHist13Titan480DMILUTBankSelect       =
                        IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT   IFEHDRBHist13Titan480ModuleLUTBankSelect    =
                        IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_MODULE_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;

static const UINT8  IFEHDRBHist13Titan480GLUT = IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_DMI_LUT_CFG_LUT_SEL_HBHIST_STORE_G_PLUT;
static const UINT8  IFEHDRBHist13Titan480BLUT = IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_DMI_LUT_CFG_LUT_SEL_HBHIST_STORE_B_PLUT;
static const UINT8  IFEHDRBHist13Titan480RLUT = IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_DMI_LUT_CFG_LUT_SEL_HBHIST_STORE_R_PLUT;

/// @brief HDRBHist Region configuration
struct IFEHDRBHist13Titan480RegionConfig
{
    IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_RGN_OFFSET_CFG regionOffset;   ///< HDR BHist stats region offset config
    IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_RGN_NUM_CFG    regionNumber;   ///< HDR BHist stats region number config
} CAMX_PACKED;

/// @brief HDRBHist DMI and LUT configuration
struct IFEHDRBHist13Titan480DMILUTConfig
{
    IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_DMI_LUT_BANK_CFG    DMILUTBankconfig;      ///< DMI LUT bank config
    IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_MODULE_LUT_BANK_CFG moduleLUTBankConfig;   ///< LUT bank config
} CAMX_PACKED;

/// @brief HDRBHist Configuration
struct IFEHDRBHist13Titan480Config
{
    IFE_IFE_0_PP_CLC_STATS_HDR_BHIST_MODULE_CFG moduleConfig;   ///< HDR BHist stats module config
    IFEHDRBHist13Titan480RegionConfig           regionConfig;   ///< HDR BHist stats region config
    IFEHDRBHist13Titan480DMILUTConfig           DMILUTConfig;   ///< HDR BHist stats DMI LUT config
} CAMX_PACKED;

CAMX_END_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE HDR Bayer Histogram 1.3 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEHDRBHistStats13Titan480 final : public ISPHWSetting
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
    /// ~IFEHDRBHistStats13Titan480
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEHDRBHistStats13Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEHDRBHistStats13Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEHDRBHistStats13Titan480();

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
    IFEHDRBHist13Titan480Config m_regCmd;   ///< Register List of this Module

    IFEHDRBHistStats13Titan480(const IFEHDRBHistStats13Titan480&)            = delete; ///< Disallow the copy constructor
    IFEHDRBHistStats13Titan480& operator=(const IFEHDRBHistStats13Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEHDRBHISTSTATS13TITAN480_H
