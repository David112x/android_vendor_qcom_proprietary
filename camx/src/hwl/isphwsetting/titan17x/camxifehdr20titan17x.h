////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdr20titan17x.h
/// @brief IFE HDR20 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEHDR20TITAN17X_H
#define CAMXIFEHDR20TITAN17X_H

#include "titan170_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IFE HDR Module Register Set 1
struct IFEHDR20RegCmd1
{
    IFE_IFE_0_VFE_HDR_CFG_0       configRegister0;        ///< Config Register 0. Offset: 0x57c
    IFE_IFE_0_VFE_HDR_CFG_1       configRegister1;        ///< Config Register 1. Offset: 0x580
    IFE_IFE_0_VFE_HDR_CFG_2       configRegister2;        ///< Config Register 2. Offset: 0x584
    IFE_IFE_0_VFE_HDR_CFG_3       configRegister3;        ///< Config Register 3. Offset: 0x588
    IFE_IFE_0_VFE_HDR_CFG_4       configRegister4;        ///< Config Register 4. Offset: 0x58c
    IFE_IFE_0_VFE_HDR_RECON_CFG_0 reconstructionConfig0;  ///< Reconstruction Config Register 0. Offset: 0x590
    IFE_IFE_0_VFE_HDR_RECON_CFG_1 reconstructionConfig1;  ///< Reconstruction Config Register 1. Offset: 0x594
    IFE_IFE_0_VFE_HDR_RECON_CFG_2 reconstructionConfig2;  ///< Reconstruction Config Register 2. Offset: 0x598
    IFE_IFE_0_VFE_HDR_RECON_CFG_3 reconstructionConfig3;  ///< Reconstruction Config Register 3. Offset: 0x59c
    IFE_IFE_0_VFE_HDR_RECON_CFG_4 reconstructionConfig4;  ///< Reconstruction Config Register 4. Offset: 0x5a0
    IFE_IFE_0_VFE_HDR_MAC_CFG_0   macConfigRegister0;     ///< Mac COnfig Register 0. Offset: 0x5a4
    IFE_IFE_0_VFE_HDR_MAC_CFG_1   macConfigRegister1;     ///< Mac COnfig Register 1. Offset: 0x5a8
    IFE_IFE_0_VFE_HDR_MAC_CFG_2   macConfigRegister2;     ///< Mac COnfig Register 2. Offset: 0x5ac
    IFE_IFE_0_VFE_HDR_MAC_CFG_3   macConfigRegister3;     ///< Mac COnfig Register 3. Offset: 0x5b0
    IFE_IFE_0_VFE_HDR_MAC_CFG_4   macConfigRegister4;     ///< Mac COnfig Register 4. Offset: 0x5b4
    IFE_IFE_0_VFE_HDR_MAC_CFG_5   macConfigRegister5;     ///< Mac COnfig Register 5. Offset: 0x5b8
    IFE_IFE_0_VFE_HDR_MAC_CFG_6   macConfigRegister6;     ///< Mac COnfig Register 6. Offset: 0x5bc
    IFE_IFE_0_VFE_HDR_MAC_CFG_7   macConfigRegister7;     ///< Mac COnfig Register 7. Offset: 0x5c0
} CAMX_PACKED;

/// @brief IFE BPCBCC Module register set 2
struct IFEHDR20RegCmd2
{
    IFE_IFE_0_VFE_HDR_CFG_5       configRegister5;        ///< Config Register 5. Offset: 0xc84
} CAMX_PACKED;

/// @brief IFE BPCBCC Module register set 3
struct IFEHDR20RegCmd3
{
    IFE_IFE_0_VFE_HDR_RECON_CFG_5 reconstructionConfig5; ///< Reconstruction Config Register 5. Offset: 0xC8C
    IFE_IFE_0_VFE_HDR_RECON_CFG_6 reconstructionConfig6; ///< Reconstruction Config Register 6. Offset: 0xC90
} CAMX_PACKED;
CAMX_END_PACKED

struct IFEHDR20RegCmd
{
    IFEHDR20RegCmd1 m_regCmd1;             ///< Register Set 1 of this Module
    IFEHDR20RegCmd2 m_regCmd2;             ///< Register Set 2 of this Module
    IFEHDR20RegCmd3 m_regCmd3;             ///< Register Set 3 of this Module
};

static const UINT32 IFEHDR20RegLengthDWord1 = sizeof(IFEHDR20RegCmd1) / sizeof(UINT32);
static const UINT32 IFEHDR20RegLengthDWord2 = sizeof(IFEHDR20RegCmd2) / sizeof(UINT32);
static const UINT32 IFEHDR20RegLengthDWord3 = sizeof(IFEHDR20RegCmd3) / sizeof(UINT32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE HDR20 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEHDR20Titan17x final : public ISPHWSetting
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
    /// ~IFEHDR20Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEHDR20Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEHDR20Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEHDR20Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFEHDR20RegCmd    m_regCmd; ///< Register List of this Module

    IFEHDR20Titan17x(const IFEHDR20Titan17x&)            = delete; ///< Disallow the copy constructor
    IFEHDR20Titan17x& operator=(const IFEHDR20Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEHDR20TITAN17X_H
