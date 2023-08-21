////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipegrainadder10.h
/// @brief IPEGrainAdder class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIPEGRAINADDER10_H
#define CAMXIPEGRAINADDER10_H

#include "camxispiqmodule.h"
#include "gra10setting.h"
#include "gra_1_0_0.h"
#include "ipe_data.h"

CAMX_NAMESPACE_BEGIN


static UINT32 DMI_Y_LUT[32] =
{
    0x000000ff, 0x000000f7, 0x000000ef, 0x000000e7, 0x000000df, 0x000000d7, 0x000000cf, 0x000000c7,
    0x000000bf, 0x000000b7, 0x000000af, 0x000000a7, 0x0000009f, 0x00000097, 0x0000008f, 0x00000087,
    0x0000007f, 0x00000077, 0x0000006f, 0x00000067, 0x0000005f, 0x00000057, 0x0000004f, 0x00000047,
    0x0000003f, 0x00000037, 0x0000002f, 0x00000027, 0x0000001f, 0x00000017, 0x0000000f, 0x00000007,
};

/// @todo (CAMX-735) Remove hard coding and hook up with Chromatix
static UINT32 DMI_Cb_LUT[32] =
{
    0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
    0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
    0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
    0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff
};

/// @todo (CAMX-735) Remove hard coding and hook up with Chromatix
static UINT32 DMI_Cr_LUT[32] =
{
    0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
    0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
    0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
    0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff
};

static const UINT32 GrainAdder10LUTBufferSize  = MaxGRA10LUTNumEntries * GRA10LUTEntrySize;
static const UINT32 GrainAdder10LUTBufferDSize = GrainAdder10LUTBufferSize / sizeof(UINT32);

struct GRAHWSettingParams
{
    CmdBuffer* pLUTCmdBuffer;       ///< Command buffer pointer for holding all 3 LUTs
    UINT*      pOffsetLUTCmdBuffer; ///< Pointer to array of Offset of all tables within LUT CmdBuffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Class for IPE GrainAdder10 Module
///
///         This IQ block adds fine grain (dither, LUT random generator) to remove banding artifacts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPEGrainAdder10 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create Grain Adder Object
    ///
    /// @param  pCreateData  Pointer to resource and intialization data for GrainAdder10 Creation
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        IPEModuleCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Execute
    ///
    /// @brief  Execute process capture request to configure module
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Execute(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCmdBufferManagerParams
    ///
    /// @brief  Fills the command buffer manager params needed by IQ Module
    ///
    /// @param  pInputData Pointer to the IQ Module Input data structure
    /// @param  pParam     Pointer to the structure containing the command buffer manager param to be filled by IQ Module
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult FillCmdBufferManagerParams(
       const ISPInputData*     pInputData,
       IQModuleCmdBufferParam* pParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRegCmd
    ///
    /// @brief  Retrieve the buffer of the register value
    ///
    /// @return Pointer of the register buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID* GetRegCmd();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IPEGrainAdder10
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPEGrainAdder10();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEGrainAdder10
    ///
    /// @brief  Constructor
    ///
    /// @param  pNodeIdentifier     String identifier for the Node that creating this IQ Module object
    /// @param  pCreateData         Initialization data for IPEGRA
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit IPEGrainAdder10(
        const CHAR*          pNodeIdentifier,
        IPEModuleCreateData* pCreateData);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateCommonLibraryData
    ///
    /// @brief  Allocate memory required for common library
    ///
    /// @return CamxResult success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocateCommonLibraryData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeallocateCommonLibraryData
    ///
    /// @brief  Deallocate memory required for common library
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DeallocateCommonLibraryData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize parameters
    ///
    /// @param  pCreateData Pointer to GRA10 Creation data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Initialize(
        IPEModuleCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateDependenceParams
    ///
    /// @brief  Validate the input info from client
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResult Indicates if the input is valid or invalid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ValidateDependenceParams(
        const ISPInputData* pInputData
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckDependenceChange
    ///
    /// @brief  Check to see if the Dependence Trigger Data Changed
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return BOOL Indicates if the settings have changed compared to last setting
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckDependenceChange(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateLUTFromChromatix
    ///
    /// @brief  Check to see if the Dependence Trigger Data Changed
    ///
    /// @param  pLUT Pointer to start of command buffer where Look up tables are present
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateLUTFromChromatix(
        UINT32* pLUT);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateIPEInternalData
    ///
    /// @brief  Update Pipeline input data, such as metadata, IQSettings.
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResult success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateIPEInternalData(
        ISPInputData* pInputData
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunCalculation
    ///
    /// @brief  Calculate the Register Value
    ///
    /// @param  pInputData Pointer to the IQ Module Input data
    ///
    /// @return CamxResult Indicates if configuration calculation was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RunCalculation(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchDMIBuffer
    ///
    /// @brief  Fetch GrainAdder10 DMI LUT
    ///
    /// @return CamxResult Indicates if fetch DMI was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FetchDMIBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Grain Adder module for debug
    ///
    /// @param  pDMIDataPtr Pointer to the DMI data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig(
        UINT32* pDMIDataPtr
        ) const;

    IPEGrainAdder10(const IPEGrainAdder10&)            = delete;      ///< Disallow the copy constructor
    IPEGrainAdder10& operator=(const IPEGrainAdder10&) = delete;      ///< Disallow assignment operator

    const CHAR*                     m_pNodeIdentifier;                ///< String identifier for the Node that created this
    GRA10IQInput                    m_dependenceData;                 ///< Dependence Data for this Module
    UINT                            m_enableDitheringC;               ///< dithering C enable
    UINT                            m_enableDitheringY;               ///< dithering Y enable
    CmdBufferManager*               m_pLUTCmdBufferManager;           ///< Command buffer manager for all LUTs of GRA
    CmdBuffer*                      m_pLUTCmdBuffer;                  ///< Command buffer for holding all 3 LUTs
    UINT                            m_offsetLUTCmdBuffer[GRALUTMax];  ///< Offset of all tables within LUT CmdBuffer
    UINT                            m_grainStrength;                  ///< Grain Strength
    UINT                            m_grainSeed;                      ///< Grain seed value
    UINT                            m_mcgA;                           ///< Multiplier parameter for MCG calculation
    UINT                            m_skipAheadJump;                  ///< It will be equal to mcg_an mod m.
    BOOL                            m_enableCommonIQ;                 ///< EnableCommon IQ module
    UINT32*                         m_pGrainAdderLUTs;                ///< Tuning, hold LUTs
    gra_1_0_0::chromatix_gra10Type* m_pChromatix;                     ///< Pointer to tuning mode data
};

CAMX_NAMESPACE_END

#endif // CAMXIPEGRAINADDER10_H
