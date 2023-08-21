////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelinearization34titan480.h
/// @brief IFE Linearization34 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFELINEARIZATION34TITAN480_H
#define CAMXIFELINEARIZATION34TITAN480_H

#include "titan480_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IFE Linearization Module Register Set
struct IFELinearization34Titan480RegCmd1
{
    IFE_IFE_0_PP_CLC_LINEARIZATION_DMI_LUT_BANK_CFG     dmiBankConfigRegister;  ///< DMI Bank Configuration Register
    IFE_IFE_0_PP_CLC_LINEARIZATION_MODULE_LUT_BANK_CFG  lutBankConfigRegister;  ///< LUT Bank Configuration Register
    IFE_IFE_0_PP_CLC_LINEARIZATION_MODULE_CFG           configRegister;         ///< Module Config Register
} CAMX_PACKED;

/// @brief IFE Linearization Module Register Set
struct IFELinearization34Titan480RegCmd2
{
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_R_0_CFG    kneeR0Register;         ///< R Channel Register 0
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_R_1_CFG    kneeR1Register;         ///< R Channel Register 1
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_R_2_CFG    kneeR2Register;         ///< R Channel Register 2
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_R_3_CFG    kneeR3Register;         ///< R Channel Register 3
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_GR_0_CFG   kneeGR0Register;        ///< GR Channel Register 0
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_GR_1_CFG   kneeGR1Register;        ///< GR Channel Register 1
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_GR_2_CFG   kneeGR2Register;        ///< GR Channel Register 2
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_GR_3_CFG   kneeGR3Register;        ///< GR Channel Register 3
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_B_0_CFG    kneeB0Register;         ///< B Channel Register 0
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_B_1_CFG    kneeB1Register;         ///< B Channel Register 1
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_B_2_CFG    kneeB2Register;         ///< B Channel Register 2
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_B_3_CFG    kneeB3Register;         ///< B Channel Register 3
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_GB_0_CFG   kneeGB0Register;        ///< GB Channel Register 0
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_GB_1_CFG   kneeGB1Register;        ///< GB Channel Register 1
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_GB_2_CFG   kneeGB2Register;        ///< GB Channel Register 2
    IFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_GB_3_CFG   kneeGB3Register;        ///< GB Channel Register 3
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 LinearizationTitan480LUT    = IFE_IFE_0_PP_CLC_LINEARIZATION_DMI_LUT_CFG_LUT_SEL_LUT;
static const UINT32 BlackLUTBank0               = IFE_IFE_0_PP_CLC_LINEARIZATION_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT32 BlackLUTBank1               = IFE_IFE_0_PP_CLC_LINEARIZATION_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK1;

static const UINT32 IFELinearization34Titan480RegLengthDWord1 = sizeof(IFELinearization34Titan480RegCmd1) / sizeof(UINT32);
static const UINT32 IFELinearization34Titan480RegLengthDWord2 = sizeof(IFELinearization34Titan480RegCmd2) / sizeof(UINT32);
static const UINT32 IFELinearization34Titan480DMILengthDWord  = 36;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE Linearization34 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFELinearization34Titan480 final : public ISPHWSetting
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
    /// ~IFELinearization34Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFELinearization34Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFELinearization34Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFELinearization34Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFELinearization34Titan480RegCmd1    m_regCmd1;         ///< Register List of this Module
    IFELinearization34Titan480RegCmd2    m_regCmd2;         ///< Register List of this Module
    UINT32*                              m_pLUTDMIBuffer;   ///< DMI Buffer address

    IFELinearization34Titan480(const IFELinearization34Titan480&)            = delete; ///< Disallow the copy constructor
    IFELinearization34Titan480& operator=(const IFELinearization34Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFELINEARIZATION34TITAN480_H
