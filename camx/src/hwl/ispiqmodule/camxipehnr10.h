////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipehnr10.h
/// @brief ipehnr10 class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIPEHNR10_H
#define CAMXIPEHNR10_H

#include "ipe_data.h"
#include "camxispiqmodule.h"
#include "iqcommondefs.h"
#include "hnr10setting.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 IPELNRLutBufferSize         = HNR_V10_LNR_ARR_NUM * sizeof(UINT32);
static const UINT32 IPEFNRAndGAinLutBufferSize  = HNR_V10_FNR_ARR_NUM * sizeof(UINT32);
static const UINT32 IPEFNRAcLutBufferSize       = HNR_V10_FNR_ARR_NUM * sizeof(UINT32);
static const UINT32 IPESNRLutBufferSize         = HNR_V10_SNR_ARR_NUM * sizeof(UINT32);
static const UINT32 IPEBlendLNRLutBufferSize    = HNR_V10_BLEND_LNR_ARR_NUM * sizeof(UINT32);
static const UINT32 IPEBlendSNRLutBufferSize    = HNR_V10_BLEND_SNR_ARR_NUM * sizeof(UINT32);

static const UINT32 IPEHNRLutBufferSize = IPELNRLutBufferSize +
                                          IPEFNRAndGAinLutBufferSize +
                                          IPEFNRAcLutBufferSize +
                                          IPESNRLutBufferSize +
                                          IPEBlendLNRLutBufferSize +
                                          IPEBlendSNRLutBufferSize;

static const UINT32 IPEHNRLutBufferSizeDWORD = IPEHNRLutBufferSize / sizeof(UINT32);

struct HNRHwSettingParams
{
    CmdBuffer* pLUTDMICmdBuffer;   ///< Pointer to module LUT DMI cmd buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IPEHNR10 Class Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPEHNR10 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create IPEHNR10 Object
    ///
    /// @param  pCreateData Pointer to the IPEHNR10 Creation
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        IPEModuleCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize parameters
    ///
    /// @param  pCreateData Pointer to the IPEHNR10 Creation
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Initialize(
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
    /// ~IPEHNR10
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPEHNR10();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEHNR10
    ///
    /// @brief  Constructor
    ///
    /// @param  pNodeIdentifier     String identifier for the Node that creating this IQ Module object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit IPEHNR10(
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
    /// UpdateIPEInternalData
    ///
    /// @brief  Update IPE internal data
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateIPEInternalData(
        ISPInputData* pInputData
        ) const;

    IPEHNR10(const IPEHNR10&)            = delete;            ///< Disallow the copy constructor
    IPEHNR10& operator=(const IPEHNR10&) = delete;            ///< Disallow assignment operator

    const CHAR*                     m_pNodeIdentifier;        ///< String identifier of this Node
    HNR10InputData                  m_dependenceData;         ///< InputData
    CmdBufferManager*               m_pLUTCmdBufferManager;   ///< Command buffer manager for all LUTs of HNR
    CmdBuffer*                      m_pLUTDMICmdBuffer;       ///< Command buffer for holding all LUTs
    HnrParameters                   m_HNRParameters;          ///< HNR parameters needed for the Firmware

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

#endif // CAMXIPEHNR10_H
