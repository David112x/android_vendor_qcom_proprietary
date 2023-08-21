////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelsc40titan480.h
/// @brief IFE LSC40 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFELSC40TITAN480_H
#define CAMXIFELSC40TITAN480_H

#include "titan480_ife.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief IFE LSC Module Register Set
struct IFELSC40RegCmd2
{
    IFE_IFE_0_PP_CLC_LSC_LSC_0_CFG  config0;    ///< LSC 0 config
    IFE_IFE_0_PP_CLC_LSC_LSC_1_CFG  config1;    ///< LSC 1 config
    IFE_IFE_0_PP_CLC_LSC_LSC_2_CFG  config2;    ///< LSC 2 config
    IFE_IFE_0_PP_CLC_LSC_LSC_3_CFG  config3;    ///< LSC 3 config
    IFE_IFE_0_PP_CLC_LSC_LSC_4_CFG  config4;    ///< LSC 4 config
    IFE_IFE_0_PP_CLC_LSC_LSC_5_CFG  config5;    ///< LSC 5 config
    IFE_IFE_0_PP_CLC_LSC_LSC_6_CFG  config6;    ///< LSC 6 config
    IFE_IFE_0_PP_CLC_LSC_LSC_7_CFG  config7;    ///< LSC 7 config
    IFE_IFE_0_PP_CLC_LSC_LSC_8_CFG  config8;    ///< LSC 8 config
    IFE_IFE_0_PP_CLC_LSC_LSC_9_CFG  config9;    ///< LSC 9 config
    IFE_IFE_0_PP_CLC_LSC_LSC_10_CFG config10;   ///< LSC 10 config
} CAMX_PACKED;

struct IFELSC40RegCmd1
{
    IFE_IFE_0_PP_CLC_LSC_DMI_LUT_BANK_CFG       dmiLUTConfig;    ///< DMI LUT configuration
    IFE_IFE_0_PP_CLC_LSC_MODULE_LUT_BANK_CFG    moduleLUTConfig; ///< Module LUT configuration
    IFE_IFE_0_PP_CLC_LSC_MODULE_CFG             moduleConfig;    ///< Module configuration
} CAMX_PACKED;
CAMX_END_PACKED

static const UINT32 IFERolloffMeshPtHV40 = 17;  // MESH_PT_H = MESH_H + 1
static const UINT32 IFERolloffMeshPtVV40 = 13;  // MESH_PT_V = MESH_V + 1
static const UINT32 IFERolloffMeshSize   = IFERolloffMeshPtHV40 * IFERolloffMeshPtVV40;

static const UINT32 IFELSC40RegLengthDword1  = sizeof(IFELSC40RegCmd1) / sizeof(UINT32);
static const UINT32 IFELSC40RegLengthDword2  = sizeof(IFELSC40RegCmd2) / sizeof(UINT32);
static const UINT8  IFELSC40NumDMITables     = 3; // RED LUT, BLUE LUT, GRID LUT
static const UINT32 IFELSC40DMISetSizeDword  = IFERolloffMeshSize;
static const UINT32 IFELSC40LUTTableSize     = IFELSC40DMISetSizeDword * sizeof(UINT32);
static const UINT32 IFELSC40DMILengthDword   = IFELSC40DMISetSizeDword * IFELSC40NumDMITables;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE LSC40 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFELSC40Titan480 final : public ISPHWSetting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteLUTtoDMI
    ///
    /// @brief  Write Look up table data pointer into DMI header
    ///
    /// @param  pInputData Pointer to the ISP input data
    /// @param  offset     DMI Buffer offset
    ///
    /// @return CamxResult Indicates if write to DMI header was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WriteLUTtoDMI(
        VOID*  pInputData,
        UINT32 offset);

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
    /// ~IFELSC40Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFELSC40Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFELSC40Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFELSC40Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

private:
    IFELSC40RegCmd1    m_regCmd1;           ///< Register List of this Module
    IFELSC40RegCmd2    m_regCmd2;           ///< Register List of this Module
    UINT32*            m_pGRRLUTDMIBuffer;  ///< Pointer to GRR LSC mesh table
    UINT32*            m_pGBBLUTDMIBuffer;  ///< Pointer to GBB LSC mesh table
    UINT32*            m_pGridLUTDMIBuffer; ///< Pointer to Grid LSC mesh table

    IFELSC40Titan480(const IFELSC40Titan480&)            = delete; ///< Disallow the copy constructor
    IFELSC40Titan480& operator=(const IFELSC40Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFELSC40TITAN480_H
