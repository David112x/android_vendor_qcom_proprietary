////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeltm14.h
/// @brief IPELTM class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIPELTM14_H
#define CAMXIPELTM14_H

#include "camxispiqmodule.h"
#include "ltm_1_4_0.h"
#include "iqcommondefs.h"
#include "ltm14setting.h"

CAMX_NAMESPACE_BEGIN

/// @brief: This enumerator maps Look Up Tables indices with DMI LUT_SEL in LTM module SWI.
enum IPELTM14LUTIndex
{
    LTM14IndexWeight = 0,                                                           ///< Weight Curve
    LTM14IndexLA,                                                                   ///< LA Curve
    LTM14IndexCurve,                                                                ///< Curve
    LTM14IndexScale,                                                                ///< Scale Curve
    LTM14IndexMask,                                                                 ///< Mask Curve
    LTM14IndexLCEPositive,                                                          ///< LCE positive Curve
    LTM14IndexLCENegative,                                                          ///< LCE Negative Curve
    LTM14IndexRGamma,                                                               ///< Gamma Curve
    LTM14IndexMax                                                                   ///< Max LUTs
};

/// @brief: This structure has information of number of entries of each LUT.
static const UINT IPELTM14LUTNumEntries[LTM14IndexMax] =
{
    12,     ///< Weight Curve
    64,     ///< LA Curve
    64,     ///< Curve
    64,     ///< Scale Curve
    64,     ///< Mask Curve
    16,     ///< LCE positive Curve
    16,     ///< LCE Negative Curve
    64,     ///< Gamma Curve
};

static const UINT32 IPELTM14LUTBufferSizeInDwords = 364; // Total Entries 364 (Sum of all entries from all LUT curves)
static const UINT32 IPELTM14LUTBufferSize         = IPELTM14LUTBufferSizeInDwords * sizeof(UINT32);

struct LTM14HWSettingParams
{
    CmdBuffer* pLUTCmdBuffer;       ///< Command buffer pointer for holding all 3 LUTs
    UINT*      pOffsetLUTCmdBuffer; ///< Offset of all tables within LUT CmdBuffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Class for IPE LTM14 Module
///
///         This IQ block does mapping of wide dynamic range captured image to a more visible data range. Supports two separate
///         functions: Data Collection (statistic collection) during 1:4/1:16 passes and tri-linear image interpolation during
///         1:1 pass.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPELTM14 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create Local Tone Map (LTM) Object
    ///
    /// @param  pCreateData  Pointer to resource and intialization data for LTM14 Creation
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
    /// ~IPELTM14
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPELTM14();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPELTM14
    ///
    /// @brief  Constructor
    ///
    /// @param  pNodeIdentifier     String identifier for the Node that creating this IQ Module object
    /// @param  pCreateData         Initialization data for IPEICA
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit IPELTM14(
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
        const ISPInputData* pInputData
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
            bEnableLTM = FALSE;
        }

        return bEnableLTM;
    }

    IPELTM14(const IPELTM14&)            = delete;          ///< Disallow the copy constructor
    IPELTM14& operator=(const IPELTM14&) = delete;          ///< Disallow assignment operator

    const CHAR*          m_pNodeIdentifier;                   ///< String identifier for the Node that created this object
    ADRCData*            m_pADRCData;                         ///< ADRC Data
    TMC10InputData       m_pTMCInput;                         ///< TMC Input data for ADRC
    LTM14InputData       m_dependenceData;                    ///< Dependence Data for this Module
    CmdBufferManager*    m_pLUTCmdBufferManager;              ///< Command buffer manager for all LUTs of LTM
    CmdBuffer*           m_pLUTCmdBuffer;                     ///< Command buffer for holding all 3 LUTs
    UINT                 m_offsetLUTCmdBuffer[LTM14IndexMax]; ///< Offset of all tables within LUT CmdBuffer
    BOOL                 m_ignoreChromatixRGammaFlag;         ///< TRUE to hardcode RGAMMA_EN to 0; FALSE to use Chromatix data
    BOOL                 m_useHardcodedGamma;                 ///< TRUE to use hardcode Gamma; FALSE to use published Gamma
    INT32                m_gammaPrev[LTM14_GAMMA_LUT_SIZE];   ///< Cached version of the InverseGamma() input array gamma
    INT32                m_igammaPrev[LTM14_GAMMA_LUT_SIZE];  ///< Cached version of the InverseGamma() output array igamma

    UINT32*              m_pLTMLUTs;                          ///< Tuning LTM LUTs place holder
    ltm_1_4_0::chromatix_ltm14Type* m_pChromatix;             ///< Pointers to tuning mode data
    tmc_1_0_0::chromatix_tmc10Type* m_ptmcChromatix;          ///
};

CAMX_NAMESPACE_END

#endif // CAMXIPELTM14_H
