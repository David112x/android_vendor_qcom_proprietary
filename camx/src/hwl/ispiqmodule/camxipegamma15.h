////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipegamma15.h
/// @brief IPEGamma class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIPEGAMMA15_H
#define CAMXIPEGAMMA15_H

#include "camxispiqmodule.h"
#include "ipe_data.h"
#include "iqcommondefs.h"

CAMX_NAMESPACE_BEGIN


// @brief: Place holder for gamma curve
struct IPEManualGammaCurve
{
    FLOAT gammaR[Gamma15LUTNumEntriesPerChannel];   ///< Gamma curve for channel R
    FLOAT gammaG[Gamma15LUTNumEntriesPerChannel];   ///< Gamma curve for channel G
    FLOAT gammaB[Gamma15LUTNumEntriesPerChannel];   ///< Gamma curve for channel B
};

static const UINT32 Gamma15LUTNumEntriesPerChannelSize = Gamma15LUTNumEntriesPerChannel * sizeof(UINT32);
static const UINT32 MaxGamma15LUTNumEntries            = Gamma15LUTNumEntriesPerChannel * MaxGammaLUTNum;
static const UINT32 Gamma15LUTEntrySize                = sizeof(UINT32);
static const UINT32 Gamma15LUTBufferSize               = (MaxGamma15LUTNumEntries * Gamma15LUTEntrySize);
static const UINT32 Gamma15LUTBufferDWordSize          = Gamma15LUTBufferSize / sizeof(UINT32);
static const UINT32 GammaLUTChannel0                   = 0;
static const UINT32 GammaLUTChannel1                   = 1;
static const UINT32 GammaLUTChannel2                   = 2;

static const UINT32 IFEGamma15MaxGammaValue            = 0x3FF;
static const UINT16 IFEGamma15HwPackBit                = 10;

struct GammaHWSettingParams
{
    CmdBuffer* pLUTCmdBuffer;       ///< Command buffer pointer for holding all 3 LUTs
    UINT*      pOffsetLUTCmdBuffer; ///< Pointer to array of Offset of all tables within LUT CmdBuffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Class for IPE Gamma15 Module
///
///         This IQ block does mapping of wide dynamic range captured image to a more visible data range. Supports two separate
///         functions: Data Collection (statistic collection) during 1:4/1:16 passes and tri-linear image interpolation during
///         1:1 pass.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPEGamma15 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create Local Tone Map (Gamma) Object
    ///
    /// @param  pCreateData  Pointer to resource and intialization data for Gamma15 Creation
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        IPEModuleCreateData* pCreateData);

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
    /// @param  pCreateData Pointer to Gamma15 Creation data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Initialize(
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
    /// GetRegCmd
    ///
    /// @brief  Retrieve the buffer of the register value
    ///
    /// @return Pointer of the register buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID* GetRegCmd();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IPEGamma15
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPEGamma15();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEGamma15
    ///
    /// @brief  Constructor
    ///
    /// @param  pNodeIdentifier     String identifier for the Node that creating this IQ Module object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit IPEGamma15(
        const CHAR* pNodeIdentifier);

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
    /// ValidateDependenceParams
    ///
    /// @brief  Validate the input configuration from app
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResult Indicates if the input is valid or invalid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ValidateDependenceParams(
        const ISPInputData* pInputData);

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
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResult Indicates if configuration calculation was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RunCalculation(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchDMIBuffer
    ///
    /// @brief  Fetch Gamma15 DMI LUT
    ///
    /// @return CamxResult Indicates if fetch DMI was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FetchDMIBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Gamma module for debug
    ///
    /// @param  pLUT Pointer to the DMI data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig(
        UINT32* pLUT
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpretGammaCurve
    ///
    /// @brief  Generate the Command List
    ///
    /// @param  pInputData      Pointer to the Input Data
    /// @param  pManualCurve    Pointer to the manual tone map curve provided by the app
    /// @param  pDerivedCurve   Pointer to the interpreted gamma curve
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void InterpretGammaCurve(
        const ISPInputData*  pInputData,
        ISPTonemapPoint*     pManualCurve,
        FLOAT*               pDerivedCurve);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateManualToneMap
    ///
    /// @brief  Generate the Command List
    ///
    /// @param  pInputData  Pointer to the Input Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void UpdateManualToneMap(
       const ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PackDMIData
    ///
    /// @brief  Pack the Gamma LUT
    ///
    /// @param  pGammaTable Pointer to the input gamma table
    /// @param  pLUT        Pointer to the output gamma table
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PackDMIData(
        FLOAT*  pGammaTable,
        UINT32* pLUT);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// isGammaRunningByIPE
    ///
    /// @brief  Pack the Gamma LUT
    ///
    /// @param  pInputData  Pointer to the Input Data
    ///
    /// @return BOOL        Indicates if this Gamma15 module is executed by IPE or others(pre-calculation from TMC 1.1)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL isGammaRunningByIPE(
        ISPInputData* pInputData)
    {
        return (NULL != pInputData->pipelineIPEData.pIPEIQSettings)? TRUE: FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLUTCmdBuffer
    ///
    /// @brief  Get the command buffer with required size
    ///
    /// @param  numDwordsToReserve  The input required size of command buffer
    ///
    /// @return UINT32*     Return the UINT32 buffer pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32* GetLUTCmdBuffer(
        UINT32 numDwordsToReserve);

    IPEGamma15(const IPEGamma15&)            = delete;                        ///< Disallow the copy constructor
    IPEGamma15& operator=(const IPEGamma15&) = delete;                        ///< Disallow assignment operator

    BOOL                                m_isGamma15PreCalculated;             ///< Flag to indicate gamma15 is pre-calculated
    const CHAR*                         m_pNodeIdentifier;                    ///< String identifier for the Node that created
                                                                              ///  this object
    Gamma15InputData                    m_dependenceData;                     ///< Dependence Data for this Module
    CmdBufferManager*                   m_pLUTCmdBufferManager;               ///< Command buffer manager for all LUTs of GRA
    CmdBuffer*                          m_pLUTCmdBuffer;                      ///< Command buffer for holding all 3 LUTs
    UINT                                m_offsetLUTCmdBuffer[MaxGammaLUTNum]; ///< Offset of all tables within LUT CmdBuffer
    UINT32*                             m_pGammaG[MaxGammaLUTNum];            ///< Pointer to Gamma LUT
    FLOAT*                              m_pGamma[GammaMaxChannel];            ///< Pointer to Gamma LUT
    BOOL                                m_manualCCMOverride;                  ///< Manual Control
    UINT8                               m_tonemapMode;                        ///< Tone map mode
    IPEManualGammaCurve                 m_manualGammaCurve;                   ///< Manual gamma curve
    gamma_1_5_0::chromatix_gamma15Type* m_pChromatix;                         ///< Pointer to tuning mode data
    UINT32*                             m_pPreCalculationPacked;              ///< Pointer to the pre-calculation packed
};

CAMX_NAMESPACE_END

#endif // CAMXIPEGAMMA15_H
