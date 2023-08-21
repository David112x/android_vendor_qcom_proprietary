////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeltm13.h
/// @brief IPELTM class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIPELTM13_H
#define CAMXIPELTM13_H

#include "camxispiqmodule.h"
#include "ltm_1_3_0.h"
#include "iqcommondefs.h"
#include "ltm13setting.h"

CAMX_NAMESPACE_BEGIN

/// @brief: This enumerator maps Look Up Tables indices with DMI LUT_SEL in LTM module SWI.
enum IPELTMLUTIndex
{
    LTMIndexWeight = 0,                                                           ///< Weight Curve
    LTMIndexLA0,                                                                  ///< LA0 Curve
    LTMIndexLA1,                                                                  ///< LA1 Curve
    LTMIndexCurve,                                                                ///< Curve
    LTMIndexScale,                                                                ///< Scale Curve
    LTMIndexMask,                                                                 ///< Mask Curve
    LTMIndexLCEPositive,                                                          ///< LCE positive Curve
    LTMIndexLCENegative,                                                          ///< LCE Negative Curve
    LTMIndexRGamma0,                                                              ///< Gamma0 Curve
    LTMIndexRGamma1,                                                              ///< Gamma1 Curve
    LTMIndexRGamma2,                                                              ///< Gamma2 Curve
    LTMIndexRGamma3,                                                              ///< Gamma3 Curve
    LTMIndexRGamma4,                                                              ///< Gamma4 Curve
    LTMIndexMax                                                                   ///< Max LUTs
};

/// @brief: This structure has information of number of entries of each LUT.
static const UINT IPELTMLUTNumEntries[LTMIndexMax] =
{
    12,     ///< Weight Curve
    64,     ///< LA0 Curve
    64,     ///< LA1 Curve
    64,     ///< Curve
    64,     ///< Scale Curve
    64,     ///< Mask Curve
    16,     ///< LCE positive Curve
    16,     ///< LCE Negative Curve
    64,     ///< Gamma0 Curve
    64,     ///< Gamma1 Curve
    64,     ///< Gamma2 Curve
    64,     ///< Gamma3 Curve
    64,     ///< Gamma4 Curve
};

static const UINT32 IPELTM13LUTBufferSizeInDwords = 824; // Total Entries 824 (Sum of all entries from all LUT curves)
static const UINT32 IPELTM13LUTBufferSize         = IPELTM13LUTBufferSizeInDwords * sizeof(UINT32);

struct LTMHWSettingParams
{
    CmdBuffer* pLUTCmdBuffer;       ///< Command buffer pointer for holding all 3 LUTs
    UINT*      pOffsetLUTCmdBuffer; ///< Offset of all tables within LUT CmdBuffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Class for IPE LTM13 Module
///
///         This IQ block does mapping of wide dynamic range captured image to a more visible data range. Supports two separate
///         functions: Data Collection (statistic collection) during 1:4/1:16 passes and tri-linear image interpolation during
///         1:1 pass.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPELTM13 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create Local Tone Map (LTM) Object
    ///
    /// @param  pCreateData  Pointer to resource and intialization data for LTM13 Creation
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
    /// Initialize
    ///
    /// @brief  Initialize parameters
    ///
    /// @param  pCreateData Pointer to the Module Creation
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Initialize(
        IPEModuleCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IPELTM13
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPELTM13();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPELTM13
    ///
    /// @brief  Constructor
    ///
    /// @param  pNodeIdentifier     String identifier for the Node that creating this IQ Module object
    /// @param  pCreateData         Initialization data for IPEICA
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit IPELTM13(
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
    /// WriteLUTInDMI
    ///
    /// @brief  writes all 14 LUTs from LTM into cmd buffer
    ///
    /// @param  pInputData Pointer to IPE module input data
    ///
    /// @return CamxResult Indicates if LUTs are successfully written into DMI cdm header
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WriteLUTInDMI(
        const ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchDMIBuffer
    ///
    /// @brief  Fetch DMI buffer
    ///
    /// @return CamxResult Indicates if fetch DMI was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FetchDMIBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunCalculation
    ///
    /// @brief  Calculate the Register Value
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResult Indicates if configuration calculation was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RunCalculation(
        ISPInputData* pInputData);

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
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of LTM module for debug
    ///
    /// @param  pDMIDataPtr Pointer to the DMI data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig(
        UINT32* pDMIDataPtr
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLTMEnableStatus
    ///
    /// @brief  Helper method to check condition to enable/disable LTM module and return the status
    ///
    /// @param  pInputData       pointer to ISP Input data
    /// @param  pIPEIQSettings   pointer to IPE IQ settings
    ///
    /// @return TRUE if LTM IQ module is enabled, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL GetLTMEnableStatus(
        const ISPInputData*  pInputData,
        IpeIQSettings* pIPEIQSettings) const
    {
        BOOL bEnableLTM = FALSE;

        if ((TRUE == pIPEIQSettings->anrParameters.parameters[PASS_NAME_DC_4].moduleCfg.EN) ||
            (IpeTopologyType::TOPOLOGY_TYPE_NO_NPS_LTM == pInputData->pipelineIPEData.configIOTopology))
        {
            bEnableLTM = m_moduleEnable;
        }
        else
        {
            bEnableLTM = 0;
        }

        return bEnableLTM;
    }

    IPELTM13(const IPELTM13&)            = delete;          ///< Disallow the copy constructor
    IPELTM13& operator=(const IPELTM13&) = delete;          ///< Disallow assignment operator

    const CHAR*          m_pNodeIdentifier;                 ///< String identifier for the Node that created this object
    ADRCData*            m_pADRCData;                       ///< ADRC Data
    TMC10InputData       m_pTMCInput;                       ///< TMC Input data for ADRC
    LTM13InputData       m_dependenceData;                  ///< Dependence Data for this Module
    CmdBufferManager*    m_pLUTCmdBufferManager;            ///< Command buffer manager for all LUTs of LTM
    CmdBuffer*           m_pLUTCmdBuffer;                   ///< Command buffer for holding all 3 LUTs
    UINT                 m_offsetLUTCmdBuffer[LTMIndexMax]; ///< Offset of all tables within LUT CmdBuffer
    BOOL                 m_ignoreChromatixRGammaFlag;       ///< TRUE to hardcode RGAMMA_EN to 0; FALSE to use Chromatix data
    BOOL                 m_useHardcodedGamma;               ///< TRUE to use hardcode Gamma; FALSE to use published Gamma
    INT32                m_gammaPrev[LTM_GAMMA_LUT_SIZE];   ///< Cached version of the InverseGamma() input array gamma
    INT32                m_igammaPrev[LTM_GAMMA_LUT_SIZE];  ///< Cached version of the InverseGamma() output array igamma

    UINT32*              m_pLTMLUTs;                        ///< Tuning LTM LUTs place holder
    ltm_1_3_0::chromatix_ltm13Type* m_pChromatix;           ///< Pointers to tuning mode data
    tmc_1_0_0::chromatix_tmc10Type* m_ptmcChromatix;        ///
};

CAMX_NAMESPACE_END

#endif // CAMXIPELTM13_H
