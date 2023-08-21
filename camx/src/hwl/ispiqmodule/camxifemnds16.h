////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 - 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifemnds16.h
/// @brief camxIFEMNDS16 class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIFEMNDS16_H
#define CAMXIFEMNDS16_H

#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class for IFE MNDS16 Module
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFEMNDS16 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create MNDS16 Object
    ///
    /// @param  pCreateData  Pointer to resource and intialization data for MNDS Creation
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        IFEModuleCreateData* pCreateData);

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
    /// PrepareStripingParameters
    ///
    /// @brief  Prepare striping parameters for striping lib
    ///
    /// @param  pInputData Pointer to the Inputdata
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PrepareStripingParameters(
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
    /// GetDualIFEData
    ///
    /// @brief  Provides information on how dual IFE mode affects the IQ module
    ///
    /// @param  pDualIFEData Pointer to dual IFE data the module will fill in
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID GetDualIFEData(
        IQModuleDualIFEData* pDualIFEData)
    {
        CAMX_ASSERT(NULL != pDualIFEData);

        pDualIFEData->dualIFESensitive = TRUE;
    }

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFEMNDS16
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFEMNDS16();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFEMNDS16
    ///
    /// @brief  Constructor
    ///
    /// @param  pCreateData  Pointer to resource and intialization data for MNDS Creation
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit IFEMNDS16(
        IFEModuleCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize parameters
    ///
    /// @param  pCreateData Input data to initialize the module
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Initialize(
        IFEModuleCreateData* pCreateData);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateDependenceParams
    ///
    /// @brief  Validate the input crop info from client
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return CamxResult Indicates if the input is valid or invalid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ValidateDependenceParams(
        ISPInputData* pInputData);

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
    /// CalculateScalerOutput
    ///
    /// @brief  Calculate scaler output dimensions
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculateScalerOutput(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateIFEInternalData
    ///
    /// @brief  Update IFE internal data
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateIFEInternalData(
        ISPInputData* pInputData);

    IFEMNDS16(const IFEMNDS16&)            = delete;    ///< Disallow the copy constructor
    IFEMNDS16& operator=(const IFEMNDS16&) = delete;    ///< Disallow assignment operator

    MNDSState*         m_pState;                        ///< State
    Format             m_pixelFormat;                   ///< Output pixel format
    BIT                m_isBayer;                       ///< Flag to indicate Bayer sensor
    IFEPipelinePath    m_modulePath;                    ///< IFE pipeline path for module
    UINT32             m_output;                        ///< IPE piepline output
    ISPInputData*      m_pInputData;                    ///< Pointer to current input data
};

CAMX_NAMESPACE_END

#endif // CAMXIFEMNDS16_H
