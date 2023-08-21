////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebhiststats14.h
/// @brief Bayer Histogram (BHist) stats v1.4
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEBHISTSTATS14_H
#define CAMXIFEBHISTSTATS14_H

#include "camxispstatsmodule.h"

CAMX_NAMESPACE_BEGIN

// Register range from the SWI/HPG
static const UINT32 BHistStats14RegionWidth     = 2;    ///< BHist stats region width
static const UINT32 BHistStats14RegionHeight    = 2;    ///< BHist stats region width

static const UINT32 BHistStats14MinHorizRegions = 1;   ///< Minimum BHist stats horizontal region
static const UINT32 BHistStats14MinVertRegions  = 0;   ///< Minimum BHist stats vertical region

static const UINT32 ChannelSelectY              = 0x0;  ///< Channel selecton for Y channel
static const UINT32 ChannelSelectR              = 0x1;  ///< Channel selecton for R channel
static const UINT32 ChannelSelectGr             = 0x2;  ///< Channel selecton for Gr channel
static const UINT32 ChannelSelectGb             = 0x3;  ///< Channel selecton for Gb channel
static const UINT32 ChannelSelectB              = 0x4;  ///< Channel selecton for B channel

static const UINT32 BHistStats14NonUniformBin   = 0;    ///< Bit to select Uniform bins
static const UINT32 BHistStats14UniformBin      = 1;    ///< Bit to select Non -uniform bins

static const UINT32 BHistBinsPerChannel         = 1024; ///< Number of bins per channel  26
static const UINT32 BHistStatsSingleBinSize     = 4;    ///< Bayer Histogram stats output bits (per bin)

/// @brief BHist region configuration
struct BHistRegionConfig
{
    UINT32             horizontalOffset;       ///< BHist stats horizontal offset
    UINT32             verticalOffset;         ///< BHist stats vertical offset regions
    UINT32             horizontalRegionNum;    ///< BHist stats num of horizontal regions
    UINT32             verticalRegionNum;      ///< BHist stats num of vertical regionns
};

/// @brief BHist Config Data
struct BHist14ConfigData
{
    const ISPInputData* pISPInputData;          ///< Pointer to general ISP input data
    UINT32              channel;                ///< Select channel to collect stats
    UINT32              uniform;                ///< Uniform stats
    UINT32              regionMultipleFactor;   ///< BHist region offset and dimension multiple factor
    BHistRegionConfig   regionConfig;           ///< BHist region configuration
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BHist Stats14 Class Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class BHistStats14 final : public ISPStatsModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create BHistStats14 Object
    ///
    /// @param  pCreateData Pointer to the BHistStats14 Creation
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        IFEStatsModuleCreateData* pCreateData);

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
    /// GetDMITable
    ///
    /// @brief  Retrieve the DMI LUT
    ///
    /// @param  ppDMITable Pointer to which the module should update different DMI tables
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID GetDMITable(
        UINT32** ppDMITable);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~BHistStats14
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~BHistStats14();

protected:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BHistStats14
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BHistStats14();

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
    /// UpdateIFEInternalData
    ///
    /// @brief  Update BHist configuration to the Internal Data
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateIFEInternalData(
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
    /// RunCalculation
    ///
    /// @brief  Calculate the Register Value
    ///
    /// @param  pInputData   Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RunCalculation(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateRegionConfiguration
    ///
    /// @brief  Adjust ROI from stats to requirement based on hardware
    ///
    /// @param  pConfigData Get region configuration for BHist
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculateRegionConfiguration(
        BHist14ConfigData* pConfigData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapColorChannelsToReg
    ///
    /// @brief  Map input color channels to HW spec
    ///
    /// @param  channel Input color channel to be map to HW spec
    ///
    /// @return The map register value according to HW spec
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32 MapColorChannelsToReg(
        ColorChannel channel);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapUniformityBinToReg
    ///
    /// @brief  Map bin uniformity to HW spec
    ///
    /// @param  binUniformity   Bin uniformity value to be map to HW spec
    ///
    /// @return The map register value according to HW spec
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 MapUniformityBinToReg(
        const BOOL binUniformity)
    {
        // In Non uniform bin mode the pixel data will be mapped in the range of 0 - 1023
        // In unform bin the pixel data will be shifted from 14 bits to 10 bits without mapping
        UINT32 mappedUniformityRegister = BHistStats14UniformBin;

        if (FALSE == binUniformity)
        {
            mappedUniformityRegister = BHistStats14NonUniformBin;
        }
        else
        {
            mappedUniformityRegister = BHistStats14UniformBin;
        }

        return mappedUniformityRegister;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// isColorChannelValid
    ///
    /// @brief  Helper to know if the provided channel is valid for BHist
    ///
    /// @param  colorChannel    Color channel to verify
    ///
    /// @return TRUE if the channel is valid for BHist
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL isColorChannelValid(
        const ColorChannel colorChannel)
    {
        BOOL isValid = FALSE;
        switch (colorChannel)
        {
            case ColorChannel::ColorChannelR:
            case ColorChannel::ColorChannelGR:
            case ColorChannel::ColorChannelGB:
            case ColorChannel::ColorChannelB:
            case ColorChannel::ColorChannelY:
                isValid = TRUE;
                break;
            default:
                isValid = FALSE;
                break;
        }

        return isValid;
    }

    BHistStats14(const BHistStats14&)            = delete;    ///< Disallow the copy constructor
    BHistStats14& operator=(const BHistStats14&) = delete;    ///< Disallow assignment operator

    ISPBHistStatsConfig    m_BHistConfig;       ///< BHist configuration data from stats
    BHist14ConfigData      m_inputConfigData;   ///< Input configuration data for HW registers
};

CAMX_NAMESPACE_END

#endif // CAMXIFEBHISTSTATS14_H
