////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebfstats25.h
/// @brief IFEBFStats25 class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIFEBFSTATS25_H
#define CAMXIFEBFSTATS25_H

#include "chistatsproperty.h"
#include "camxiqinterface.h"
#include "camxispstatsmodule.h"

CAMX_NAMESPACE_BEGIN

/// @todo (CAMX-1404): BF Stats Pass GammaSupported and Scaler supported from OEM
// BF stats modular support selection
static const UINT32 GammaSupported            = TRUE;
static const UINT32 DownScalerSupported       = TRUE;

// BF minimum margins
static const UINT32 HorizontalFIRMargin       = 12;
static const INT32 VerticalFIRMargin          = 4;
static const UINT32 HorizontalIIRMargin       = 64;
static const UINT32 VerticalIIRMargin         = 32;
static const UINT32 MinXOffset                = HorizontalFIRMargin + HorizontalIIRMargin;
static const UINT32 MinYOffset                = VerticalFIRMargin + VerticalIIRMargin;
static const INT32 MinWidth                   = 6;
static const INT32 MinHeight                  = 8;

// BF stats ROI overlap constrains
static const UINT32 MinStartPixelOverlap      = 6;
static const UINT32 MinEndPixelOverlap        = 6;
static const UINT32 MaxHorizontalRegions      = 21;
static const UINT32 MaxVerticalSpacing        = 8;
static const UINT32 MaxOverlappingRegions     = 0;

// BF stats Gamma Constats offset
static const UINT32 BFGammaEntries                = 32;
static const UINT32 BFGammaDMIDeltaShift          = 14;
static const UINT32 BFGammaDownscaleFactor        = 1;
static const UINT32 BFGammaDownscaleFactorTwo     = 2;
static const UINT32 BFGammaDownscaleFactorFour    = 4;
static const UINT32 BFGammaDownscaleFactorEight   = 8;
static const UINT32 BFGammaDownscaleFactorSixteen = 16;
static const UINT32 BFGammaUseYChannel            = FALSE;
static const UINT32 BFGammaMaxValue               = (1 << IFEPipelineBitWidth) - 1;    ///< 14 bit value (1 << 14) - 1
static const INT32  BFGammaDeltaMinValue          = -8192;
static const INT32  BFGammaDeltaMaxValue          = 8191;
static const UINT32 LumaConversionConfigShift     = 11;
static const UINT32 NumLumaCoefficients           = 3;
static const UINT32 ConfigGSelectGR               = 0x0;
static const UINT32 ConfigGSelectGB               = 0x1;
static const UINT32 ConfigChannelSelectG          = 0x0;
static const UINT32 ConfigChannelSelectY          = 0x1;
static const FLOAT  BFGammaScaleRatioMinValue     = 0.125f;
static const FLOAT  BFGammaScaleRatioMaxValue     = 8.0f;

// BF stats Dowm Scaler constants
static const UINT32 ScaleMaxHorizontalConfig  = 16383;
static const UINT32 ScaleMaxVerticalConfig    = 16383;
static const UINT32 PhaseAdder                = 14;

// BF stats FIR, IIR coring config bits
static const UINT8  FIRBits                   = 6;
static const UINT8  IIRBits                   = 16;
static const UINT8  ShifterBits               = 4;
static const UINT32 ThresholdMask             = 0x3FFFF;
static const UINT32 CoringBits                = 0x3FF;
static const UINT32 BF_GAMMA_ENTRIES          = 32;

// BF stats YA config limit
static const FLOAT  YAConfigMin                = 0.0f;
static const FLOAT  YAConfigMax                = 1.0f;
static const FLOAT  YAConfigCoEffSum           = 1.0f;
static const FLOAT  YAConfigOverrideRatioMax   = 1.999f;
static const FLOAT  YAConfigNoOverrideRatioMax = 1.0f;

// BF stats interpolation const
static const UINT32 InterpolationResolution0 = 0;
static const UINT32 InterpolationResolution1 = 1;
static const UINT32 InterpolationResolution2 = 2;
static const UINT32 InterpolationResolution3 = 3;
static const UINT32 InterpolationResolution4 = 4;

// BF stats interpolation const
static const UINT32 ScaleRatio2  = 2;
static const UINT32 ScaleRatio4  = 4;
static const UINT32 ScaleRatio8  = 8;
static const UINT32 ScaleRatio16 = 16;

// BF stats buffer output size
static const UINT32 BFStatsMaximumRegion   = 180;
static const UINT32 BFStatsNumberOfEntries = 4;
static const UINT32 BFStatsSizePerEntries  = 4;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CAMX_BEGIN_PACKED

/// @brief ROI Index LUT type. Need to align in 32-bit for DMI programming
struct BFStats25ROIIndexLUTType
{
    UINT32  height      : 13;   ///< [12:0]
    UINT32  unused1     : 1;    ///< [13:13]
    UINT32  width       : 12;   ///< [25:14]
    UINT32  unused2     : 1;    ///< [26:26]
    UINT32  top         : 14;   ///< [40:27]
    UINT32  left        : 13;   ///< [53:41]
    UINT32  unused3     : 1;    ///< [54:54]
    UINT32  regionID    : 8;    ///< [62:55]
    UINT32  outputID    : 8;    ///< [70:63]
    UINT32  needMerged  : 1;    ///< [71:71]
    UINT32  endOfBuffer : 1;    ///< [72:72]
    UINT32  regionType  : 1;    ///< [73:73]: 0: secondary region (12x12), 1: primary region (6x6)
    UINT32  unused4     : 22;   ///< [95:74]: Most significant bit side paddding to align in 32-bits
} CAMX_PACKED;

CAMX_END_PACKED

// BF stats DMI length in DWord
static const UINT32 IFEBF25ROIDMILengthDWord      = BFStatsMaximumRegion * sizeof(BFStats25ROIIndexLUTType) / sizeof(UINT32);
static const UINT32 IFEBF25GammaLUTDMILengthDWord = MaxBFGammaEntries;

// Bit shift (i.e., bit position) of whole 96-bit ROI LUT entry
static const UINT32 BF25ROIRegionTypeShift    = 73;
static const UINT32 BF25ROIEndOfBufferShift   = 72;
static const UINT32 BF25ROINeedMergedShift    = 71;
static const UINT32 BF25ROIOutputIDShift      = 63;
static const UINT32 BF25ROIRegionIDShift      = 55;
static const UINT32 BF25ROILeftShift          = 41;
static const UINT32 BF25ROITopShift           = 27;
static const UINT32 BF25ROIWidthShift         = 14;
static const UINT32 BF25ROIHeightShift        = 0;

// ROI Index LUT DMI description
static const UINT32 BF25ROILeftBits               = 0x1FFF; // 13-bit mask
static const UINT32 BF25ROITopBits                = 0x3FFF; // 14-bit mask
static const UINT32 BF25ROIWidthBits              = 0xFFF;  // 12-bit mask
static const UINT32 BF25ROIHeightBits             = 0x1FFF; // 13-bit mask

/// @brief enum identify filter type
enum class IFEBFFilterType
{
    Horizontal1 = 1,    ///< Horizontal1 filter type
    Horizontal2,        ///< Horizontal2 filter type
    Vertical            ///< Vertical filter type
};

/// @brief Input for IFE BAF stats luma conversion configuration
struct BFStats25LumaConversionConfigInput
{
    BOOL    forceYChannel;          ///< Flag to indicate use Y channel
    BOOL    overrideYCoefficient;   ///< Flag to override Y coeffcient
    FLOAT   ratio;                  ///< Ratio calculated by gamma downscaler algorithm
};

/// @brief Input for IFE BAF stats down scaler configuration
struct BFStats25DownscalerConfigInput
{
    BOOL    enableScaler;                       ///< Flag to indicate to enable BAF scaler or not

    // Horizontal scaler configuration
    UINT32  scalerImageSizeIn;                  ///< 13-bit. Programming a value of n means n+1
    UINT32  scalerImageSizeOut;                 ///< 13-bit. Programming a value of n means n+1
    UINT32  scalerPhaseInterpolationResolution; ///< 4-bit.
    UINT32  scalerPhaseMultiplicationFactor;    ///< 22-bit.
    UINT32  scalerPhaseInitialValue;            ///< 22-bit.
    UINT32  scalerMNInitialValue;               ///< 13-bit.
    UINT32  scalerSkipCount;                    ///< 13-bit.
    UINT32  scalerRoundingPattern;              ///< Rounding pattern
    UINT32  scalerInputWidth;                   ///< 13-bit. Programming a value of n means n+1
};

/// @brief Input for ISPInputData and the input for IFE BAF stats input
struct BFStats25ConfigData
{
    const ISPInputData*                 pISPInputData;          ///< Pointer to ISP input data
    AFConfigParams*                     pStatsConfig;           ///< Pointer to m_BFStatsConfig
    BFStats25LumaConversionConfigInput* pLumaConversionConfig;  ///< Pointer to m_lumaConversionConfigInput
    BFStats25DownscalerConfigInput*     pDownscalerConfig;      ///< Pointer to m_downscalerConfigInput
    BFStatsROIConfigType*               pHwROIConfig;           ///< Valid list of HW ROI config
    BOOL                                inputConfigUpdate;      ///< Value from m_inputConfigUpdate
    BOOL                                downScalerSupported;    ///< Value from m_downScalerSupported
    BOOL                                enableGammaLUT;         ///< Flag to indicate gamma LUT
    BFStats25ROIIndexLUTType*           pROIDMIConfig;          ///< DMI ROI configuration
    UINT32*                             pGammaLUT;              ///< Gamma LUT
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BFStats25 Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class BFStats25 final : public ISPStatsModule
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create BFStats25 Object
    ///
    /// @param  pCreateData Pointer to the BFStats25 Creation
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
    /// ~BFStats25
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~BFStats25();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BFStats25
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BFStats25();

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
    /// @return CamxResult Indicates if the input is valid or invalid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckDependenceChange(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RunCalculation
    ///
    /// @brief  Calculate the Register Value
    ///
    /// @param  pInputData Pointer to the ISP input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RunCalculation(
        BFStats25ConfigData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateAndAdjustROIBoundary
    ///
    /// @brief  Validate and Adjust if the number of ROIs in in any line does not exceed the max limitation of hardware
    ///
    /// @param  pInputData      Pointer to ISP input data
    /// @param  pBFStatsConfig  BF stats configuration
    ///
    /// @return True if ROI is adjusted
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL ValidateAndAdjustROIBoundary(
        ISPInputData*   pInputData,
        AFConfigParams* pBFStatsConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AdjustScaledROIBoundary
    ///
    /// @brief  Validate and Adjust ROI based on BAF Scaling
    ///
    /// @param  pInputData      Pointer to ISP input data
    /// @param  pRoi            Pointer to the ROI to be adjusted
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AdjustScaledROIBoundary(
        ISPInputData*   pInputData,
        RectangleCoordinate* pRoi);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SortRegionsStartPixelOrderBased
    ///
    /// @brief  Bubble sort ROI in starting pixel order
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SortRegionsStartPixelOrderBased();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndAdjustStartPixelROILimitation
    ///
    /// @brief  Check whether both ROI starts on same pixel or starting line apart by min, and then adjust
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CheckAndAdjustStartPixelROILimitation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegionsOverlapping
    ///
    /// @brief  Check if the input ROI's overlap
    ///
    /// @param  currentROI Current ROI parameters
    /// @param  nextROI    Next ROI parameters
    ///
    /// @return BOOL       Return TRUE if ROI's overlap FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL RegionsOverlapping(
        BFStatsROIDimensionParams currentROI,
        BFStatsROIDimensionParams nextROI);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckMaxHorizontalLimitationPerRow
    ///
    /// @brief  Validate if the number of ROIs in in any line does not exceed the max limitation of hardware
    ///
    /// @return CamxResult  Returns success if the ROI config to be comsumed is valid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckMaxHorizontalLimitationPerRow();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckMaxVerticalSpacingAndDisjoint
    ///
    /// @brief  Validate max vertical spacing between i'th and i + 21st region and validate disjoint ROI's
    ///
    /// @return CamxResult  Return success if the vertical spacing between disjoint ROI is valid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckMaxVerticalSpacingAndDisjoint();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckMaxRegionOverlap
    ///
    /// @brief  Validate max overlapping region specified by hardware limitation
    ///
    /// @return CamxResult Returns success if the max overlapping region is valid
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckMaxRegionOverlap();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LumaConversionConfig
    ///
    /// @brief  Configure BF stats Luma conversion registers
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID LumaConversionConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// YCoefficientSumCheck
    ///
    /// @brief  Check the sum of Y conversion co-efficients
    ///
    /// @return BOOL Return true if the sum of co-efficients are less than equal to 1.0f
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL YCoefficientSumCheck() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateGammaDownscaleFactor
    ///
    /// @brief  Calculate gamma downscaler factor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculateGammaDownscaleFactor();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DownscalerConfig
    ///
    /// @brief  Configure BF stats Down scaler registers
    ///
    /// @param  pInputData       Pointer to the InputData
    /// @param  pBFStatsConfig   BF stats configuration detail
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DownscalerConfig(
        const ISPInputData* pInputData,
        AFConfigParams*     pBFStatsConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DownscalerCalculateInterpolationResolution
    ///
    /// @brief  Create the Command List
    ///
    /// @param  pBFStatsConfig  BF stats configuration detail
    ///
    /// @return UINT32 Interpolation resolution value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 DownscalerCalculateInterpolationResolution(
        const AFConfigParams* pBFStatsConfig
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateROIDMITable
    ///
    /// @brief  Update the DMI LUT for ROI
    ///
    /// @param  pHWROIConfig    BF ROI Config
    /// @param  pROIDMIConfig   DMI configuration
    /// @param  sizeOfDMIConfig size of DMI configuraiton array
    /// @param  stripeId        zero-based stripe ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateROIDMITable(
        BFStatsROIConfigType*       pHWROIConfig,
        BFStats25ROIIndexLUTType*   pROIDMIConfig,
        const UINT32                sizeOfDMIConfig,
        const UINT32                stripeId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateROIDMITableForStripingInput
    ///
    /// @brief  Update the DMI LUT for ROI (due to the difference of data structure represnetation)
    ///
    /// @param  pHWROIConfig                BF ROI Config
    /// @param  ppStripingLibROIDMIConfig   DMI configuration from striping library
    /// @param  sizeOfDMIConfig             Size of DMI configuraiton array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateROIDMITableForStripingInput(
        BFStatsROIConfigType*       pHWROIConfig,
        UINT64                      ppStripingLibROIDMIConfig[][2],
        UINT32                      sizeOfDMIConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateHWROI
    ///
    /// @brief  Update the valid list of HW ROI config
    ///
    /// @param  pInputROIConfig     Input ROI configuration
    /// @param  pHWROIConfig        HW configuration to be updated
    /// @param  stripeId            zero-based stripe ID
    /// @param  overwriteStripes    Flag to indicate if striping information is provided by striping library
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateHWROI(
        BFStatsROIConfigType*   pInputROIConfig,
        BFStatsROIConfigType*   pHWROIConfig,
        const UINT32            stripeId,
        BOOL                    overwriteStripes);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SortRegionsEndPixelOrderBased
    ///
    /// @brief  Bubble sort ROI in ending pixel order
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SortRegionsEndPixelOrderBased();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndAdjustEndPixelROILimitation
    ///
    /// @brief  Check whether both ROI ends on same pixel or ending line apart by min, adn then adjust
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CheckAndAdjustEndPixelROILimitation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureGamma
    ///
    /// @brief  Configure BF stats Gamma registers
    ///
    /// @param  pInputData Pointer to the IFE input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureGamma(
        const ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GammaGetHighLowBits
    ///
    /// @brief  Pack the higher 12 bits in the configuration contains the delta between the current GammaTable value and
    ///         the next value, while the lower 12 bits contains the current GammaTable value
    ///
    /// @param  pGamma Pointer to the gamma table
    /// @param  index  Current index in the gamma table
    ///
    /// @return INT32  Packed HW LUT entry
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32 GammaGetHighLowBits(
        UINT32* pGamma,
        UINT32  index
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GammaGetLastValue
    ///
    /// @brief  Fetch the last HW LUT gamma value packed higher 12 bits with delta between the last GammaTable value and
    ///         the max value, while the lower 12 bits contains the current GammaTable value
    ///
    /// @param  pGamma Pointer to the gamma table
    ///
    /// @return UINT32  Packed HW LUT entry
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GammaGetLastValue(
        UINT32* pGamma
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleEmptyROI
    ///
    /// @brief  Handle empty ROI dimensions
    ///
    /// @param  pInputData  Pointer to ISP input data
    /// @param  pROIOut     ROI configuration to be used
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleEmptyROI(
        ISPInputData*           pInputData,
        BFStatsROIConfigType*   pROIOut);

    /// @todo (CAMX-1301): Enable BF stats meta reporting

    BFStats25(const BFStats25&)            = delete;          ///< Disallow the copy constructor
    BFStats25& operator=(const BFStats25&) = delete;          ///< Disallow assignment operator

    BFStats25ROIIndexLUTType    m_ROIDMIConfig[BFMaxROIRegions + 1];    ///< DMI ROI configuration

    UINT32                  m_gammaLUT[MaxBFGammaEntries];          ///< Gamma LUT
    UINT32                  m_gammaSupported;                       ///< BF stats gamma supported flag
    UINT32                  m_downScalerSupported;                  ///< BF stats Down scaler supported flag
    UINT32*                 m_pHorizontalRegionCount;               ///< Counter to keep to track of no of ROI per line
    AFConfigParams          m_BFStatsConfig;                        ///< BF configuration data from stats
    FLOAT                   m_refSensitivity;                       ///< Exposure Index, Same as Lux index for non ADRC cases
    BOOL                    m_isInputConfigAdjusted;                ///< Flag to check if input configuration has been adjusted
    BOOL                    m_inputConfigUpdate;                    ///< Track if the input config has been updated
    BOOL                    m_ROIConfigUpdate;                      ///< Track if we need to update the ROI
    UINT32                  m_CAMIFWidth;                           ///< CAMIF width to compare against ROI width
    UINT32                  m_CAMIFHeight;                          ///< CAMIF height to compare against ROI height
    UINT32                  m_gammaDownscaleFactor;                 ///< Down scale factor for BF stats gamma
    BFStatsROIConfigType    m_endOrderROI;                          ///< End pixel reorder of the ROI
    BFStatsROIConfigType    m_hwROIConfig;                          ///< Valid list of HW ROI config
    AFConfigParams          m_previousAFConfig;                     ///< Previous working AF config

    BFStats25LumaConversionConfigInput  m_lumaConversionConfigInput;    ///< Luma conversion config
    BFStats25DownscalerConfigInput      m_downscalerConfigInput;        ///< Downscaler config
    BOOL                                m_enableGammaLUT;               ///< Decision flag to indicate to enable gamma LUT
};

CAMX_NAMESPACE_END

#endif // CAMXIFEBFSTATS25_H
