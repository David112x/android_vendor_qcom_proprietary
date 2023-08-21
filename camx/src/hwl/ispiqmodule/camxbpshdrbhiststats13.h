////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpshdrbhiststats13.h
/// @brief AWB BG Stats14 class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXBPSHDRBHISTSTATS13_H
#define CAMXBPSHDRBHISTSTATS13_H

#include "camxispstatsmodule.h"

CAMX_NAMESPACE_BEGIN

// Register range from the SWI/HPG
static const UINT32 HDRBHistStats13RegionWidth      = 2;    ///< HDR BHist stats region width
static const UINT32 HDRBHistStats13RegionHeight     = 2;    ///< HDR BHist stats region Height

static const UINT32 HDRBHist13MinHorizontalregions  = 2;    ///< Minimum HDR Bhist stats horizontal region
static const UINT32 HDRBHist13MaxHorizontalregions  = 3120; ///< Maximum HDR Bhist stats horizontal region

static const UINT32 HDRBHist13MinVerticalregions    = 1;    ///< Minimum HDR Bhist stats vertical region
static const UINT32 HDRBHist13MaxVerticalregions    = 8192; ///< Maximum HDR Bhist stats vertical region

static const UINT32 HDRBHist13MaxHorizontalOffset   = 444;
static const UINT32 HDRBHist13MaxVerticalOffset     = 16382;

// Green channel source options: Gr or Gb
static const UINT32 HDRBHistGreenChannelSelectGr    = 0x0;  ///< Channel selection for Gr channel
static const UINT32 HDRBHistGreenChannelSelectGb    = 0x1;  ///< Channel selection for Gb channel

// Input Field selection for HDR_BHIST: non-HDR(use All) / HDR mode (use T1 or T2) mode
static const UINT32 HDRBHistFieldSelectAll          = 0x0;  ///< Non-HDR: All lines
static const UINT32 HDRBHistFieldSelectT1           = 0x1;  ///< HDR: T1 long exposure
static const UINT32 HDRBHistFieldSelectT2           = 0x2;  ///< HDR: T2 short exposure

struct BPSHDRBHistConfig
{
    UINT32  offsetHorizNum; ///< HDRBHist stats num of horizontal regions
    UINT32  offsetVertNum;  ///< HDRBHist stats num of vertical regions
    UINT32  regionHorizNum; ///< HDRBHist stats num of horizontal regions
    UINT32  regionVertNum;  ///< HDRBHist stats num of vertical regions
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief AWB BG Stats14 Class Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class BPSHDRBHist13Stats final : public ISPIQModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create BPSHDRBHist13Stats Object
    ///
    /// @param  pCreateData Pointer to the BPSHDRBHist13Stats Creation
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        BPSModuleCreateData* pCreateData);

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
    /// ~BPSHDRBHist13Stats
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~BPSHDRBHist13Stats();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BPSHDRBHist13Stats
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BPSHDRBHist13Stats();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateDependenceParams
    ///
    /// @brief  Validate the stats region of interest configuration from stats module
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
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckDependenceChange(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateRegionConfiguration
    ///
    /// @brief  Adjust ROI from stats to requirement based on hardware
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculateRegionConfiguration();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateBPSInternalData
    ///
    /// @brief  Update IFE internal data
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateBPSInternalData(
        ISPInputData * pInputData);

    BPSHDRBHist13Stats(const BPSHDRBHist13Stats&)            = delete;    ///< Disallow the copy constructor
    BPSHDRBHist13Stats& operator=(const BPSHDRBHist13Stats&) = delete;    ///< Disallow assignment operator

    HDRBHistConfig      m_HDRBHistConfig;       ///< HDR BHist configuration data from stats
    BPSHDRBHistConfig   m_moduleSetting;        ///< HDR Bhist module setting
    BOOL                m_isAdjusted;           ///< Flag to indicate if ROI from stats was modified
    UINT32              m_greenChannelSelect;   ///< Green channel selection between Gr and Gb
    UINT32              m_inputFieldSelect;     ///< Input Field selection for stats in non-HDR(All) and HDR(T1/T2) mode
};

CAMX_NAMESPACE_END

#endif // CAMXBPSHDRBHISTSTATS13_H
