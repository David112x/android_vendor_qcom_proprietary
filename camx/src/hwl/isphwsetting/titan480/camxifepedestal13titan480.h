////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepedestal13titan480.h
/// @brief IFE Pedestal13 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEPEDESTAL13TITAN480_H
#define CAMXIFEPEDESTAL13TITAN480_H

#include "titan480_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IFE Pedestal Module Dependence Data
struct IFEPedestal13Titan480RegCmd1
{
    IFE_IFE_0_PP_CLC_PEDESTAL_MODULE_LUT_BANK_CFG   lutBankConfig0;     ///< LUT Bank Config Register.
    IFE_IFE_0_PP_CLC_PEDESTAL_MODULE_CFG            config;             ///< Configuration Register.
} CAMX_PACKED;

struct IFEPedestal13Titan480RegCmd2
{
    IFE_IFE_0_PP_CLC_PEDESTAL_MODULE_1_CFG          config1;            ///< Config1 Register.
    IFE_IFE_0_PP_CLC_PEDESTAL_MODULE_2_CFG          config2;            ///< Config2 Register.
    IFE_IFE_0_PP_CLC_PEDESTAL_MODULE_3_CFG          config3;            ///< Config3 Register.
    IFE_IFE_0_PP_CLC_PEDESTAL_MODULE_4_CFG          config4;            ///< Config4 Register.
    IFE_IFE_0_PP_CLC_PEDESTAL_MODULE_5_CFG          config5;            ///< Config5 Register.
} CAMX_PACKED;

CAMX_END_PACKED

/// @brief Value for select DMI BANK
static const UINT32 IFEPedestal13Titan480RegLengthDword1 = (sizeof(IFEPedestal13Titan480RegCmd1) / sizeof(UINT32));
static const UINT32 IFEPedestal13Titan480RegLengthDword2 = (sizeof(IFEPedestal13Titan480RegCmd2) / sizeof(UINT32));
static const UINT8  IFEPedestal13Titan480NumDMITables    = 2;
static const UINT32 IFEPedestal13Titan480DMISetSizeDword = 130;   // DMI LUT table has 130 entries
static const UINT32 IFEPedestal13Titan480LUTTableSize    = IFEPedestal13Titan480DMISetSizeDword * sizeof(UINT32);
static const UINT32 IFEPedestal13Titan480DMILengthDword  = (IFEPedestal13Titan480DMISetSizeDword *
                                                            IFEPedestal13Titan480NumDMITables);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE Pedestal13 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEPedestal13Titan480 final : public ISPHWSetting
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
    /// ~IFEPedestal13Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEPedestal13Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEPedestal13Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEPedestal13Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    UINT32*                        m_pGRRLUTDMIBuffer;  ///< Pointer to GRR table
    UINT32*                        m_pGBBLUTDMIBuffer;  ///< Pointer to GBB table
    IFEPedestal13Titan480RegCmd1   m_regCmd1;            ///< Register List of this Module
    IFEPedestal13Titan480RegCmd2   m_regCmd2;            ///< Register List of this Module

    IFEPedestal13Titan480(const IFEPedestal13Titan480&)             = delete; ///< Disallow the copy constructor
    IFEPedestal13Titan480& operator=(const IFEPedestal13Titan480&)  = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEPEDESTAL13TITAN480_H
