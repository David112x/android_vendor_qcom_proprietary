////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebhiststats14titan480.h
/// @brief IFE BHISTStat14 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEBHISTSTATS14TITAN480_H
#define CAMXIFEBHISTSTATS14TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispstatsmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief BHist HW specific const values
static const UINT8  IFEBHist14Titan480DMITables                 = 1;
static const UINT32 IFEBHist14Titan480NumberOfEntriesPerLUT     = 1024;
static const UINT32 IFEBHist14Titan480LutTableSize              = IFEBHist14Titan480NumberOfEntriesPerLUT * sizeof(UINT32);
static const UINT32 IFEBHist14Titan480DMILengthDword            =
                        IFEBHist14Titan480NumberOfEntriesPerLUT * IFEBHist14Titan480DMITables;
static const UINT   IFEBHist14Titan480DMILUTBankSelect          =
                        IFE_IFE_0_PP_CLC_STATS_BHIST_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT   IFEBHist14Titan480ModuleLUTBankSelect       =
                        IFE_IFE_0_PP_CLC_STATS_BHIST_MODULE_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT8  IFEBHist14Titan480LUT                       =
                        IFE_IFE_0_PP_CLC_STATS_BHIST_DMI_LUT_CFG_LUT_SEL_STATS_BHIST_PLUT;

/// @brief BHist Region configuration
struct IFEBHist14Titan480RegionConfig
{
    IFE_IFE_0_PP_CLC_STATS_BHIST_RGN_OFFSET_CFG regionOffset;   ///< BHist stats region offset config
    IFE_IFE_0_PP_CLC_STATS_BHIST_RGN_NUM_CFG    regionNumber;   ///< BHist stats region number config
} CAMX_PACKED;

/// @brief BHist DMI and LUT configuration
struct IFEBHist14Titan480DMILUTConfig
{
    IFE_IFE_0_PP_CLC_STATS_BHIST_DMI_LUT_BANK_CFG       DMILUTBankconfig;      ///< DMI LUT bank config
    IFE_IFE_0_PP_CLC_STATS_BHIST_MODULE_LUT_BANK_CFG    moduleLUTBankConfig;   ///< LUT bank config
} CAMX_PACKED;

/// @brief BHist Stats Configuration
struct IFEBHist14Titan480Config
{
    IFE_IFE_0_PP_CLC_STATS_BHIST_MODULE_CFG moduleConfig;   ///< BHist stats module config
    IFEBHist14Titan480RegionConfig          regionConfig;   ///< BHist stats region config
    IFEBHist14Titan480DMILUTConfig          DMILUTConfig;   ///< BHist stats DMI LUT config
} CAMX_PACKED;

CAMX_END_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE Bayer Histogram 1.4 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEBHistStats14Titan480 final : public ISPHWSetting
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
    /// ~IFEBHistStats14Titan480
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEBHistStats14Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEBHistStats14Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEBHistStats14Titan480();

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
    IFEBHist14Titan480Config    m_regCmd;   ///< Register List of this Module

    IFEBHistStats14Titan480(const IFEBHistStats14Titan480&)            = delete; ///< Disallow the copy constructor
    IFEBHistStats14Titan480& operator=(const IFEBHistStats14Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEBHISTSTATS14TITAN480_H
