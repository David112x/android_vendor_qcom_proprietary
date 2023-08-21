////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelsc34titan17x.h
/// @brief IFE LSC34 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFELSC34TITAN17X_H
#define CAMXIFELSC34TITAN17X_H

#include "titan170_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IFE LSC Module Register Set
struct IFELSC34RegCmd
{
    IFE_IFE_0_VFE_ROLLOFF_CFG                configRegister;             ///< Config Register. 0x6bc
    IFE_IFE_0_VFE_ROLLOFF_GRID_CFG_0         gridConfigRegister0;        ///< Grid Config Register 0. 0x6c0
    IFE_IFE_0_VFE_ROLLOFF_GRID_CFG_1         gridConfigRegister1;        ///< Grid Config Register 1. 0x6c4
    IFE_IFE_0_VFE_ROLLOFF_GRID_CFG_2         gridConfigRegister2;        ///< Grid Config Register 2. 0x6c8
    IFE_IFE_0_VFE_ROLLOFF_RIGHT_GRID_CFG_0   rightGridConfigRegister0;   ///< Right Grid Config Register 0. 0x6cc
    IFE_IFE_0_VFE_ROLLOFF_RIGHT_GRID_CFG_1   rightGridConfigRegister1;   ///< Right Grid Config Register 1. 0x6d0
    IFE_IFE_0_VFE_ROLLOFF_RIGHT_GRID_CFG_2   rightGridConfigRegister2;   ///< Right Grid Config Register 2. 0x6d4
    IFE_IFE_0_VFE_ROLLOFF_STRIPE_CFG_0       stripeConfigRegister0;      ///< Stripe Config Register 0. 0x6d8
    IFE_IFE_0_VFE_ROLLOFF_STRIPE_CFG_1       stripeConfigRegister1;      ///< Stripe Config Register 1. 0x6DC
    IFE_IFE_0_VFE_ROLLOFF_RIGHT_STRIPE_CFG_0 rightStripeConfigRegister0; ///< Right Stripe Config Register 0. 0x6e0
    IFE_IFE_0_VFE_ROLLOFF_RIGHT_STRIPE_CFG_1 rightStripeConfigRegister1; ///< Right Stripe Config Register 1. 0x6e4
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT   RolloffLGRRBank0        = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_ROLLOFF_RAM_L_GR_R_BANK0;
static const UINT   RolloffLGBBBank0        = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_ROLLOFF_RAM_L_GB_B_BANK0;
static const UINT   RolloffLGRRBank1        = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_ROLLOFF_RAM_L_GR_R_BANK1;
static const UINT   RolloffLGBBBank1        = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_ROLLOFF_RAM_L_GB_B_BANK1;
static const UINT32 IFERolloffMeshPtHV34    = 17;  // MESH_PT_H = MESH_H + 1
static const UINT32 IFERolloffMeshPtVV34    = 13;  // MESH_PT_V = MESH_V + 1
static const UINT32 IFERolloffMeshSize      = IFERolloffMeshPtHV34 * IFERolloffMeshPtVV34;
static const UINT32 IFELSC34RegLengthDword  = sizeof(IFELSC34RegCmd) / sizeof(UINT32);
static const UINT8  IFELSC34NumDMITables    = 2;
static const UINT32 IFELSC34DMISetSizeDword = IFERolloffMeshSize;
static const UINT32 IFELSC34LUTTableSize    = IFELSC34DMISetSizeDword * sizeof(UINT32);
static const UINT32 IFELSC34DMILengthDword  = IFELSC34DMISetSizeDword * IFELSC34NumDMITables;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE LSC34 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFELSC34Titan17x final : public ISPHWSetting
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
    /// ~IFELSC34Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFELSC34Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFELSC34Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFELSC34Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFELSC34RegCmd    m_regCmd;             ///< Register List of this Module
    UINT32*           m_pGRRLUTDMIBuffer;   ///< Pointer to GRR LSC mesh table
    UINT32*           m_pGBBLUTDMIBuffer;   ///< Pointer to GBB LSC mesh table

    IFELSC34Titan17x(const IFELSC34Titan17x&)            = delete; ///< Disallow the copy constructor
    IFELSC34Titan17x& operator=(const IFELSC34Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFELSC34TITAN17X_H
