////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepedestal13titan17x.h
/// @brief IFE Pedestal13 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEPEDESTAL13TITAN17X_H
#define CAMXIFEPEDESTAL13TITAN17X_H

#include "titan170_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IFE Pedestal Module Dependence Data
struct IFEPedestal13RegCmd
{
    IFE_IFE_0_VFE_PEDESTAL_CFG                config;             ///< Configuration Register. Offset: 0x4b0
    IFE_IFE_0_VFE_PEDESTAL_GRID_CFG_0         config0;            ///< Grid Config0 Register. Offset: 0x4b4
    IFE_IFE_0_VFE_PEDESTAL_GRID_CFG_1         config1;            ///< Grid Config1 Register. Offset: 0x4b8
    IFE_IFE_0_VFE_PEDESTAL_GRID_CFG_2         config2;            ///< Grid Config2 Register. Offset: 0x4bc
    IFE_IFE_0_VFE_PEDESTAL_RIGHT_GRID_CFG_0   rightGridConfig0;   ///< Right Grid Config0 Register. Offset: 0x4c0
    IFE_IFE_0_VFE_PEDESTAL_RIGHT_GRID_CFG_1   rightGridConfig1;   ///< Right Grid Config1 Register. Offset: 0x4c4
    IFE_IFE_0_VFE_PEDESTAL_RIGHT_GRID_CFG_2   rightGridConfig2;   ///< Right Grid Config2 Register. Offset: 0x4c8
    IFE_IFE_0_VFE_PEDESTAL_STRIPE_CFG_0       stripeConfig0;      ///< Stripe Config0 Register. Offset: 0x4cc
    IFE_IFE_0_VFE_PEDESTAL_STRIPE_CFG_1       stripeConfig1;      ///< Stripe Config1 Register. Offset: 0x4d0
    IFE_IFE_0_VFE_PEDESTAL_RIGHT_STRIPE_CFG_0 rightStripeConfig0; ///< Right Stripe Config0 Register. Offset: 0x4d4
    IFE_IFE_0_VFE_PEDESTAL_RIGHT_STRIPE_CFG_1 rightStripeConfig1; ///< Right Stripe Config1 Register. Offset: 0x4d8
} CAMX_PACKED;

CAMX_END_PACKED

/// @brief Value for select DMI BANK
static const UINT PedestalLGRRBank0              = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_PEDESTAL_RAM_L_GR_R_BANK0;
static const UINT PedestalLGRRBank1              = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_PEDESTAL_RAM_L_GR_R_BANK1;
static const UINT PedestalLGBBBank0              = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_PEDESTAL_RAM_L_GB_B_BANK0;
static const UINT PedestalLGBBBank1              = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_PEDESTAL_RAM_L_GB_B_BANK1;
static const UINT32 IFEPedestal13RegLengthDword  = (sizeof(IFEPedestal13RegCmd) / sizeof(UINT32));
static const UINT8  IFEPedestal13NumDMITables    = 2;
static const UINT32 IFEPedestal13DMISetSizeDword = 130;   // DMI LUT table has 130 entries
static const UINT32 IFEPedestal13LUTTableSize    = IFEPedestal13DMISetSizeDword * sizeof(UINT32);
static const UINT32 IFEPedestal13DMILengthDword  = (IFEPedestal13DMISetSizeDword * IFEPedestal13NumDMITables);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE Pedestal13 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEPedestal13Titan17x final : public ISPHWSetting
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
    /// ~IFEPedestal13Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEPedestal13Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEPedestal13Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEPedestal13Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFEPedestal13RegCmd    m_regCmd;            ///< Register List of this Module
    UINT8                  m_leftGRRBankSelect; ///< Left Plane, GR/R Bank Selection
    UINT8                  m_leftGBBBankSelect; ///< Left Plane, GB/B Bank Selection
    UINT32*                m_pGRRLUTDMIBuffer;  ///< Pointer to GRR table
    UINT32*                m_pGBBLUTDMIBuffer;  ///< Pointer to GBB table

    IFEPedestal13Titan17x(const IFEPedestal13Titan17x&)             = delete; ///< Disallow the copy constructor
    IFEPedestal13Titan17x& operator=(const IFEPedestal13Titan17x&)  = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEPEDESTAL13TITAN17X_H
