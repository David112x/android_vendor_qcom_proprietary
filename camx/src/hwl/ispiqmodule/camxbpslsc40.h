////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpslsc40.h
/// @brief bpslsc34 class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXBPSLSC40_H
#define CAMXBPSLSC40_H

// Camx Includes
#include "bps_data.h"
#include "camxispiqmodule.h"
#include "tintless_2_0_0.h"
#include "lsc40setting.h"

// Common library includes
#include "iqcommondefs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPS LSC 34 Class Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class BPSLSC40 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create BPSLSC34 Object
    ///
    /// @param  pCreateData Pointer to the BPSLSC34 Creation
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
    /// @param  pCreateData Pointer to the BPSLSC34 Creation
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
    /// ~BPSLSC40
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~BPSLSC40();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BPSLSC40
    ///
    /// @brief  Constructor
    ///
    /// @param  pNodeIdentifier     String identifier for the Node that creating this IQ Module object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit BPSLSC40(
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
    /// TranslateCalibrationTableToCommonLibrary
    ///
    /// @brief  Translate calibration table to common library
    ///
    /// @param  pInputData Pointer to the Input Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID TranslateCalibrationTableToCommonLibrary(
        const ISPInputData* pInputData);

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
    /// @param  pInputData       Pointer to the ISP input data
    ///
    /// @return CamxResult Indicates if configure DMI and Registers was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RunCalculation(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchDMIBuffer
    ///
    /// @brief  Fetch DMI buffer
    ///
    /// @param  pInputData Pointer to the LSC Module Input data
    ///
    /// @return CamxResult Indicates if configure DMI was success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FetchDMIBuffer(
        const ISPInputData* pInputData);

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
        ISPInputData* pInputData);


    BPSLSC40(const BPSLSC40&)            = delete;                     ///< Disallow the copy constructor
    BPSLSC40& operator=(const BPSLSC40&) = delete;                     ///< Disallow assignment operator

    const CHAR*                               m_pNodeIdentifier;       ///< String identifier of this Node
    CmdBufferManager*                         m_pLUTCmdBufferManager;  ///< Command buffer manager for all LUTs of LSC
    CmdBuffer*                                m_pLUTDMICmdBuffer;      ///< Command buffer for holding all LUTs
    LSC40InputData                            m_dependenceData;        ///< Dependence Data for this Module
    LSCCalibrationData*                       m_pLSCCalibrationData;   ///< LSC Calibration table
    OSLIBRARYHANDLE                           m_hHandleTintless;       ///< Tintless Algo Library handle
    CHITintlessAlgorithm*                     m_pTintlessAlgo;         ///< Tintless Algorithm Instance
    OSLIBRARYHANDLE                           m_hHandleAlsc;           ///< Alsc Algo Library handle
    CHIALSCAlgorithm*                         m_pAlscAlgo;             ///< ALSC Algorithm Pointer
    TintlessConfig                            m_tintlessConfig;        ///< Tintless Config
    ParsedTintlessBGStatsOutput*              m_pTintlessBGStats;      ///< Pointer to TintlessBG stats
    ParsedAWBBGStatsOutput*                   m_pAWBBGStats;           ///< Pointer to the AWBBG stats
    UINT32*                                   m_pGRRLUTDMIBuffer;      ///< Tuning pointer to GRR LSC mesh table
    UINT32*                                   m_pGBBLUTDMIBuffer;      ///< Tuning pointer to GBB SC mesh table
    UINT32*                                   m_pGridLUTDMIBuffer;     ///< Tuning pointer to Grid mesh table
    LSC40UnpackedData                         m_unpackedData;          ///< unpacked data
    UINT8                                     m_shadingMode;           ///< Shading mode
    UINT8                                     m_lensShadingMapMode;    ///< Lens shading map mode
    lsc_4_0_0::chromatix_lsc40Type*           m_pChromatix;            ///< Pointer to LSC chromatix
    tintless_2_0_0::chromatix_tintless20Type* m_pTintlessChromatix;    ///< Pointer to Tintless Chromatix
    BOOL                                      m_AWBLock;               ///< Flag to track AWB state
    BOOL                                      m_AELock;                ///< Flag to track AEC state
    LSC40TintlessRatio                        m_tintlessRatio;         ///< tintless ratio

    UINT32 m_alscHelperBuffer[ALSC_SCRATCH_BUFFER_SIZE_IN_DWORD];      ///< Scratch buffer for ALSC algorithm
};

CAMX_NAMESPACE_END

#endif // CAMXBPSLSC40_H
