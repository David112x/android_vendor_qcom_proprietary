////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelinearization33titan17x.h
/// @brief IFE Linearization33 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFELINEARIZATION33TITAN17X_H
#define CAMXIFELINEARIZATION33TITAN17X_H

#include "titan170_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IFE Linearization Module Register Set
struct IFELinearization33RegCmd
{
    IFE_IFE_0_VFE_BLACK_CFG               configRegister;                     ///< BLS Configuration Register. 0x4dc
    IFE_IFE_0_VFE_BLACK_INTERP_R_0        interpolationR0Register;            ///< R Channel Register 0. 0x4e0
    IFE_IFE_0_VFE_BLACK_INTERP_R_1        interpolationR1Register;            ///< R Channel Register 1. 0x4e4
    IFE_IFE_0_VFE_BLACK_INTERP_R_2        interpolationR2Register;            ///< R Channel Register 2. 0x4e8
    IFE_IFE_0_VFE_BLACK_INTERP_R_3        interpolationR3Register;            ///< R Channel Register 3. 0x4ec
    IFE_IFE_0_VFE_BLACK_INTERP_GB_0       interpolationGB0Register;           ///< GB Channel Register 0. 0x4f0
    IFE_IFE_0_VFE_BLACK_INTERP_GB_1       interpolationGB1Register;           ///< GB Channel Register 1. 0x4f4
    IFE_IFE_0_VFE_BLACK_INTERP_GB_2       interpolationGB2Register;           ///< GB Channel Register 2. 0x4f8
    IFE_IFE_0_VFE_BLACK_INTERP_GB_3       interpolationGB3Register;           ///< GB Channel Register 3. 0x4fc
    IFE_IFE_0_VFE_BLACK_INTERP_B_0        interpolationB0Register;            ///< B Channel Register 0. 0x500
    IFE_IFE_0_VFE_BLACK_INTERP_B_1        interpolationB1Register;            ///< B Channel Register 1. 0x504
    IFE_IFE_0_VFE_BLACK_INTERP_B_2        interpolationB2Register;            ///< B Channel Register 2. 0x508
    IFE_IFE_0_VFE_BLACK_INTERP_B_3        interpolationB3Register;            ///< B Channel Register 3. 0x50c
    IFE_IFE_0_VFE_BLACK_INTERP_GR_0       interpolationGR0Register;           ///< GR Channel Register 0. 0x510
    IFE_IFE_0_VFE_BLACK_INTERP_GR_1       interpolationGR1Register;           ///< GR Channel Register 1. 0x514
    IFE_IFE_0_VFE_BLACK_INTERP_GR_2       interpolationGR2Register;           ///< GR Channel Register 2. 0x518
    IFE_IFE_0_VFE_BLACK_INTERP_GR_3       interpolationGR3Register;           ///< GR Channel Register 3. 0x51c
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_R_0  rightPlaneInterpolationR0Register;  ///< Right Plane R Channel Register 0. 0x520
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_R_1  rightPlaneInterpolationR1Register;  ///< Right Plane R Channel Register 1. 0x524
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_R_2  rightPlaneInterpolationR2Register;  ///< Right Plane R Channel Register 2. 0x528
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_R_3  rightPlaneInterpolationR3Register;  ///< Right Plane R Channel Register 3. 0x52c
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_GB_0 rightPlaneInterpolationGB0Register; ///< Right Plane GB Channel Register 0. 0x530
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_GB_1 rightPlaneInterpolationGB1Register; ///< Right Plane GB Channel Register 1. 0x534
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_GB_2 rightPlaneInterpolationGB2Register; ///< Right Plane GB Channel Register 2. 0x538
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_GB_3 rightPlaneInterpolationGB3Register; ///< Right Plane GB Channel Register 3. 0x53c
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_B_0  rightPlaneInterpolationB0Register;  ///< Right Plane B Channel Register 0. 0x540
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_B_1  rightPlaneInterpolationB1Register;  ///< Right Plane B Channel Register 1. 0x544
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_B_2  rightPlaneInterpolationB2Register;  ///< Right Plane B Channel Register 2. 0x548
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_B_3  rightPlaneInterpolationB3Register;  ///< Right Plane B Channel Register 3. 0x54c
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_GR_0 rightPlaneInterpolationGR0Register; ///< Right Plane GR Channel Register 0. 0x550
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_GR_1 rightPlaneInterpolationGR1Register; ///< Right Plane GR Channel Register 1. 0x554
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_GR_2 rightPlaneInterpolationGR2Register; ///< Right Plane GR Channel Register 2. 0x548
    IFE_IFE_0_VFE_BLACK_RIGHT_INTERP_GR_3 rightPlaneInterpolationGR3Register; ///< Right Plane GR Channel Register 3. 0x54c
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT BlackLUTBank0 = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_BLACK_LUT_RAM_BANK0;
static const UINT BlackLUTBank1 = IFE_IFE_0_VFE_DMI_CFG_DMI_RAM_SEL_BLACK_LUT_RAM_BANK1;

static const UINT32 IFELinearization33RegLengthDWord = sizeof(IFELinearization33RegCmd) / sizeof(UINT32);
static const UINT32 IFELinearizationLutTableSize     = 36;
static const UINT32 IFELinearization33DMILengthDWord = IFELinearizationLutTableSize * sizeof(UINT64) / sizeof(UINT32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE Linearization33 register setting for Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFELinearization33Titan17x final : public ISPHWSetting
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
    /// ~IFELinearization33Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFELinearization33Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFELinearization33Titan17x
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFELinearization33Titan17x();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFELinearization33RegCmd    m_regCmd;        ///< Register List of this Module
    UINT64*                     m_pLUTDMIBuffer; ///< DMI LUT buffer address

    IFELinearization33Titan17x(const IFELinearization33Titan17x&)            = delete; ///< Disallow the copy constructor
    IFELinearization33Titan17x& operator=(const IFELinearization33Titan17x&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFELINEARIZATION33TITAN17X_H
