////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpslsc34.h
/// @brief bpslsc34 class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXBPSLSC34_H
#define CAMXBPSLSC34_H

// Camx Includes
#include "bps_data.h"
#include "camxispiqmodule.h"
#include "lsc_3_4_0.h"
#include "tintless_2_0_0.h"

// Common library includes
#include "iqcommondefs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPS LSC 34 Class Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class BPSLSC34 final : public ISPIQModule
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
    /// ~BPSLSC34
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~BPSLSC34();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BPSLSC34
    ///
    /// @brief  Constructor
    ///
    /// @param  pNodeIdentifier     String identifier for the Node that creating this IQ Module object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit BPSLSC34(
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
        ISPInputData*    pInputData);

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


    BPSLSC34(const BPSLSC34&)            = delete;                     ///< Disallow the copy constructor
    BPSLSC34& operator=(const BPSLSC34&) = delete;                     ///< Disallow assignment operator

    const CHAR*                               m_pNodeIdentifier;       ///< String identifier of this Node
    CmdBufferManager*                         m_pLUTCmdBufferManager;  ///< Command buffer manager for all LUTs of LSC
    CmdBuffer*                                m_pLUTDMICmdBuffer;      ///< Command buffer for holding all LUTs
    LSC34InputData                            m_dependenceData;        ///< Dependence Data for this Module
    LSCCalibrationData*                       m_pLSCCalibrationData;   ///< LSC Calibration table
    OSLIBRARYHANDLE                           m_hHandle;               ///< Tintless Algo Library handle
    CHITintlessAlgorithm*                     m_pTintlessAlgo;         ///< Tintless Algorithm Instance
    TintlessConfig                            m_tintlessConfig;        ///< Tintless Config
    ParsedTintlessBGStatsOutput*              m_pTintlessBGStats;      ///< Pointer to TintlessBG stats
    UINT32*                                   m_pGRRLUTDMIBuffer;      ///< Tuning pointer to GRR LSC mesh table
    UINT32*                                   m_pGBBLUTDMIBuffer;      ///< Tuning pointer to GBB SC mesh table
    LSC34UnpackedData                         m_unpackedData;          ///< unpacked data
    UINT8                                     m_shadingMode;           ///< Shading mode
    UINT8                                     m_lensShadingMapMode;    ///< Lens shading map mode
    lsc_3_4_0::chromatix_lsc34Type*           m_pChromatix;            ///< Pointer to tuning mode data
    tintless_2_0_0::chromatix_tintless20Type* m_pTinlessChromatix;     ///
    BOOL                                      m_AWBLock;               ///< Flag to track AWB state
    LSC34TintlessRatio                        m_tintlessRatio;         ///< Tintless ratio
};

CAMX_NAMESPACE_END

#endif // CAMXBPSLSC34_H
