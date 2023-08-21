////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeihiststats12titan480.h
/// @brief IFE IHISTStat12 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEIHISTSTATS12TITAN480_H
#define CAMXIFEIHISTSTATS12TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispstatsmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IHist HW specific const values
static const UINT8  IFEIHist12Titan480DMITables              = 4;
static const UINT32 IFEIHist12Titan480NumberOfEntriesPerLUT  = 256;
static const UINT32 IFEIHist12Titan480LutTableSize           = IFEIHist12Titan480NumberOfEntriesPerLUT * sizeof(UINT32);
static const UINT32 IFEIHist12Titan480DMILengthDword         =
                        IFEIHist12Titan480NumberOfEntriesPerLUT * IFEIHist12Titan480DMITables;
static const UINT   IFEIHist12Titan480DMILUTBankSelect       =
                        IFE_IFE_0_PP_CLC_STATS_IHIST_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT   IFEIHist12Titan480ModuleLUTBankSelect    =
                        IFE_IFE_0_PP_CLC_STATS_IHIST_MODULE_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;

static const UINT8  IFEIHist12Titan480YLUT = IFE_IFE_0_PP_CLC_STATS_IHIST_DMI_LUT_CFG_LUT_SEL_IHIST_STORE_YCC_PLUT;
static const UINT8  IFEIHist12Titan480GLUT = IFE_IFE_0_PP_CLC_STATS_IHIST_DMI_LUT_CFG_LUT_SEL_IHIST_STORE_G_PLUT;
static const UINT8  IFEIHist12Titan480BLUT = IFE_IFE_0_PP_CLC_STATS_IHIST_DMI_LUT_CFG_LUT_SEL_IHIST_STORE_B_PLUT;
static const UINT8  IFEIHist12Titan480RLUT = IFE_IFE_0_PP_CLC_STATS_IHIST_DMI_LUT_CFG_LUT_SEL_IHIST_STORE_R_PLUT;

/// @brief Image Histogram Region configuration
struct IFEIHist12Titan480RegionConfig
{
    IFE_IFE_0_PP_CLC_STATS_IHIST_RGN_OFFSET_CFG regionOffset;   ///< Image Histogram region offset
    IFE_IFE_0_PP_CLC_STATS_IHIST_RGN_NUM_CFG    regionNumber;   ///< Image Histogram region number
} CAMX_PACKED;

/// @brief IHist DMI and LUT configuration
struct IFEIHist12Titan480DMILUTConfig
{
    IFE_IFE_0_PP_CLC_STATS_IHIST_DMI_LUT_BANK_CFG       DMILUTBankconfig;       ///< DMI LUT bank config
    IFE_IFE_0_PP_CLC_STATS_IHIST_MODULE_LUT_BANK_CFG    moduleLUTBankConfig;    ///< LUT bank config
} CAMX_PACKED;

/// @brief Image Histogram region configuration
struct IFEIHist12Titan480Config
{
    IFE_IFE_0_PP_CLC_STATS_IHIST_MODULE_CFG moduleConfig;   ///< IHist stats module config
    IFEIHist12Titan480RegionConfig          regionConfig;   ///< IHist stats region config
    IFEIHist12Titan480DMILUTConfig          DMILUTConfig;   ///< IHist stats DMI LUT config
} CAMX_PACKED;

CAMX_END_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE Image Histogram 1.2 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEIHistStats12Titan480 final : public ISPHWSetting
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
    /// ~IFEIHistStats12Titan480
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEIHistStats12Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEIHistStats12Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEIHistStats12Titan480();

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
    IFEIHist12Titan480Config    m_regCmd;   ///< Register List of this Module

    IFEIHistStats12Titan480(const IFEIHistStats12Titan480&)            = delete; ///< Disallow the copy constructor
    IFEIHistStats12Titan480& operator=(const IFEIHistStats12Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEIHISTSTATS12TITAN480_H
