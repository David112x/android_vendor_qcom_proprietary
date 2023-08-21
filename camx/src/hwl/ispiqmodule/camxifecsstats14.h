////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecsstats14.h
/// @brief Colum Sum stats v1.4 class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFECSSTATS14_H
#define CAMXIFECSSTATS14_H

#include "camxispstatsmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

/// @brif HW capability data
struct CS14StatsHWCapability
{
    UINT32 inputDepth;      ///< Input depth to CS stats
    UINT32 outputDepth;     ///< Output bit depths of CS stats
    UINT32 maxRegionWidth;  ///< Maximum CS stats region width
    UINT32 minRegionWidth;  ///< Minimum CS stats region width
    UINT32 minRegionHeight; ///< Minimum CS stats region height
    UINT32 minHorizRegions; ///< Minimum CS stats horizontal region
    UINT32 maxHorizRegions; ///< Maximum CS stats horizontal region
    UINT32 minVertRegions;  ///< Minimum CS stats vertical region
    UINT32 maxVertRegions;  ///< Maximum CS stats vertical region
};

/// @brief CS14 config data
struct CS14ConfigData
{
    const ISPInputData* pISPInputData;  ///< Pointer to general ISP input data
    ISPCSStatsConfig*   pCSConfig;      ///< CS configuration information
    UINT16              shiftBits;      ///< Number of bit shift required to get 16 bits row sum output
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief CS Stats14 Class Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSStats14 final : public ISPStatsModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create CSStats14 Object
    ///
    /// @param  pCreateData Pointer to the CSStats14 Creation
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        IFEStatsModuleCreateData* pCreateData);

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
    /// ~CSStats14
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~CSStats14();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CSStats14
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CSStats14();

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
        IFEStatsModuleCreateData* pCreateData);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AdjustROI
    ///
    /// @brief  Adjust the 3A ROI to fit into hardware registers
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AdjustROI();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckDependency
    ///
    /// @brief  Check to see if the Dependencies have been met
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckDependency(
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
        ISPInputData * pInputData);

    CSStats14(const CSStats14&)            = delete;    ///< Disallow the copy constructor
    CSStats14& operator=(const CSStats14&) = delete;    ///< Disallow assignment operator

    ISPCSStatsConfig        m_CSConfig;         ///< RS configuration information
    UINT32                  m_CAMIFWidth;       ///< CAMIF width
    UINT32                  m_CAMIFHeight;      ///< CAMIF height
    CS14StatsHWCapability   m_hwCapability;     ///< HW capabilities
    CS14ConfigData          m_inputConfigData;  ///< Input configuration data for HW registers

};

CAMX_NAMESPACE_END

#endif // CAMXIFECSSTATS14_H
