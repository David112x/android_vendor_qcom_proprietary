////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdr30titan480.h
/// @brief IFE HDR30 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEHDR30TITAN480_H
#define CAMXIFEHDR30TITAN480_H

#include "titan480_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
struct IFEHDR30RegCmd1
{
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_MODULE_CFG      configModule;           ///< HDR Module Config
}  CAMX_PACKED;

struct IFEHDR30RegCmd2
{
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_0_CFG       configRegister0;        ///< HDR Config Register0
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_1_CFG       configRegister1;        ///< HDR Config Register1
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_2_CFG       configRegister2;        ///< HDR Config Register2
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_3_CFG       configRegister3;        ///< HDR Config Register3
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_4_CFG       configRegister4;        ///< HDR Config Register4
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_5_CFG       configRegister5;        ///< HDR Config Register5
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_REC_0_CFG   reconstructionConfig0;  ///< HDR reconstruction Config0
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_REC_1_CFG   reconstructionConfig1;  ///< HDR reconstruction Config1
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_REC_2_CFG   reconstructionConfig2;  ///< HDR reconstruction Config2
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_REC_3_CFG   reconstructionConfig3;  ///< HDR reconstruction Config3
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_REC_4_CFG   reconstructionConfig4;  ///< HDR reconstruction Config4
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_MAC_0_CFG   macConfigRegister0;     ///< HDR MAC Config0
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_MAC_1_CFG   macConfigRegister1;     ///< HDR MAC Config1
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_MAC_2_CFG   macConfigRegister2;     ///< HDR MAC Config2
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_MAC_3_CFG   macConfigRegister3;     ///< HDR MAC Config3
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_MAC_4_CFG   macConfigRegister4;     ///< HDR MAC Config4
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_MAC_5_CFG   macConfigRegister5;     ///< HDR MAC Config5
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_BIN_CORR_0_CFG  bcConfigRegister0;      ///< binning correction Config 0
    IFE_IFE_0_PP_CLC_HDR_BINCORRECT_BIN_CORR_1_CFG  bcConfigRegister1;      ///< binning correction Config 1
} CAMX_PACKED;
CAMX_END_PACKED

static const UINT32 IFEHDR30RegLengthDWord1 = sizeof(IFEHDR30RegCmd1) / sizeof(UINT32);
static const UINT32 IFEHDR30RegLengthDWord2 = sizeof(IFEHDR30RegCmd2) / sizeof(UINT32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE HDR30 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEHDR30Titan480 final : public ISPHWSetting
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
    /// @param  pInput    Pointer to the Input data to the module for calculation
    /// @param  pOutput   Pointer to the Output data to the module for DMI buffer
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
    /// @param  pInput Pointer to the Input data to the module for calculation
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupRegisterSetting(
        VOID*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFEHDR30Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEHDR30Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEHDR30Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEHDR30Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFEHDR30RegCmd1    m_regCmd1; ///< Register List of this Module
    IFEHDR30RegCmd2    m_regCmd2; ///< Register List of this Module

    IFEHDR30Titan480(const IFEHDR30Titan480&)            = delete; ///< Disallow the copy constructor
    IFEHDR30Titan480& operator=(const IFEHDR30Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEHDR30TITAN480_H
