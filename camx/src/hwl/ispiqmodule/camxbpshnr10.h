////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpshnr10.h
/// @brief bpshnr10 class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXBPSHNR10_H
#define CAMXBPSHNR10_H

#include "bps_data.h"
#include "camxispiqmodule.h"
#include "iqcommondefs.h"
#include "hnr10setting.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 BPSLNRLutBufferSize         = HNR_V10_LNR_ARR_NUM * sizeof(UINT32);
static const UINT32 BPSFNRAndGAinLutBufferSize  = HNR_V10_FNR_ARR_NUM * sizeof(UINT32);
static const UINT32 BPSFNRAcLutBufferSize       = HNR_V10_FNR_ARR_NUM * sizeof(UINT32);
static const UINT32 BPSSNRLutBufferSize         = HNR_V10_SNR_ARR_NUM * sizeof(UINT32);
static const UINT32 BPSBlendLNRLutBufferSize    = HNR_V10_BLEND_LNR_ARR_NUM * sizeof(UINT32);
static const UINT32 BPSBlendSNRLutBufferSize    = HNR_V10_BLEND_SNR_ARR_NUM * sizeof(UINT32);

static const UINT32 BPSHNRLutBufferSize = BPSLNRLutBufferSize +
                                          BPSFNRAndGAinLutBufferSize +
                                          BPSFNRAcLutBufferSize +
                                          BPSSNRLutBufferSize +
                                          BPSBlendLNRLutBufferSize +
                                          BPSBlendSNRLutBufferSize;

static const UINT32 BPSHNRLutBufferSizeDWORD = BPSHNRLutBufferSize / sizeof(UINT32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPSHNR10 Class Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class BPSHNR10 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create BPSHNR10 Object
    ///
    /// @param  pCreateData Pointer to the BPSHNR10 Creation
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        BPSModuleCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize parameters
    ///
    /// @param  pCreateData Pointer to the BPSHNR10 Creation
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Initialize(
        BPSModuleCreateData* pCreateData);

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
    /// ~BPSHNR10
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~BPSHNR10();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BPSHNR10
    ///
    /// @brief  Constructor
    ///
    /// @param  pNodeIdentifier     String identifier for the Node that creating this IQ Module object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit BPSHNR10(
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
    /// CheckDependenceChange
    ///
    /// @brief  Check to see if the Dependence Trigger Data Changed
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return TRUE if dependencies met
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckDependenceChange(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunCalculation
    ///
    /// @brief  Calculate the Register Value
    ///
    /// @param  pInputData Pointer to the HNR Module input data
    ///
    /// @return CamxResult Indicates if interpolation and configure registers was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RunCalculation(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchDMIBuffer
    ///
    /// @brief  Fetch DMI buffer
    ///
    /// @return CamxResult Indicates if fetch DMI was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FetchDMIBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateBPSInternalData
    ///
    /// @brief  Update BPS internal data
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateBPSInternalData(
        ISPInputData* pInputData
        ) const;

    BPSHNR10(const BPSHNR10&)            = delete;            ///< Disallow the copy constructor
    BPSHNR10& operator=(const BPSHNR10&) = delete;            ///< Disallow assignment operator

    const CHAR*                     m_pNodeIdentifier;        ///< String identifier of this Node
    HNR10InputData                  m_dependenceData;         ///< InputData
    CmdBufferManager*               m_pLUTCmdBufferManager;   ///< Command buffer manager for all LUTs of HNR
    CmdBuffer*                      m_pLUTDMICmdBuffer;       ///< Command buffer for holding all LUTs

    UINT32*                         m_pLNRDMIBuffer;          ///< Tuning pointer to the LNR Dmi data
    UINT32*                         m_pFNRAndClampDMIBuffer;  ///< Tuning pointer to the Merged FNR and Gain Clamp data
    UINT32*                         m_pFNRAcDMIBuffer;        ///< Tuning pointer to the FNR ac data
    UINT32*                         m_pSNRDMIBuffer;          ///< Tuning pointer to the SNR data
    UINT32*                         m_pBlendLNRDMIBuffer;     ///< Tuning pointer to the Blend LNR data
    UINT32*                         m_pBlendSNRDMIBuffer;     ///< Tuning pointer to the Blend SNR data
    UINT8                           m_noiseReductionMode;     ///< noise reduction mode
    hnr_1_0_0::chromatix_hnr10Type* m_pChromatix;             ///< Pointer to tuning mode data
};

CAMX_NAMESPACE_END

#endif // CAMXBPSHNR10_H
