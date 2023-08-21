////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeupscaler12.h
/// @brief IPEUpscaler class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIPEUPSCALER12_H
#define CAMXIPEUPSCALER12_H

#include "camxispiqmodule.h"
#include "iqcommondefs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class for IPE Upscaler20 Module
///
/// This IQ block adds upscaler operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPEUpscaler12 final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create Upscaler Object
    ///
    /// @param  pCreateData  Pointer to resource and intialization data for Upscaler12 Creation
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        IPEModuleCreateData* pCreateData);

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
    /// ~IPEUpscaler12
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPEUpscaler12();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEUpscaler12
    ///
    /// @brief  Constructor
    ///
    /// @param  pNodeIdentifier     String identifier for the Node that creating this IQ Module object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit IPEUpscaler12(
        const CHAR* pNodeIdentifier);

private:
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
        const ISPInputData* pInputData);

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
        const ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Upscale module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateIPEInternalData
    ///
    /// @brief  Update Pipeline input data, such as metadata, IQSettings.
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateIPEInternalData(
        const ISPInputData* pInputData
        ) const;

    IPEUpscaler12(const IPEUpscaler12&)            = delete;         ///< Disallow the copy constructor
    IPEUpscaler12& operator=(const IPEUpscaler12&) = delete;         ///< Disallow assignment operator

    const CHAR*             m_pNodeIdentifier;      ///< String identifier for the Node that created this object
    Upscale12InputData      m_dependenceData;       ///< Dependence Data for this Module
};

CAMX_NAMESPACE_END

#endif // CAMXIPEUPSCALER12_H
