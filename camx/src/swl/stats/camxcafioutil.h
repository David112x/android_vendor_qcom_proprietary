////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcafioutil.h
/// @brief This class provides utility functions to handle input and output of AF algorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCAFIOUTIL_H
#define CAMXCAFIOUTIL_H

#include "chiafinterface.h"
#include "chipdlibinterface.h"
#include "chisoftwarestatsinterface.h"
#include "chistatsproperty.h"

#include "camxhwenvironment.h"
#include "camxlist.h"
#include "camxstatscommon.h"
#include "camxstatsparser.h"
#include "camxutils.h"
#include "camxncsservice.h"
#include "camxncssensordata.h"
#include "camxthreadmanager.h"
#include "camxtofsensorintf.h"

// Usecase pool has one slot number. Read/write from/to slot 0
static const UINT32 UseCasePoolSlotNumber   = 0;

CAMX_NAMESPACE_BEGIN

/// @brief List of AF Read Property tags
static UINT32 AFPropertyReadTags[] =
{
    PropertyIDUsecaseChiTuningModeParameter,      // 0
    InputScalerCropRegion,                        // 1
    PropertyIDParsedBFStatsOutput,                // 2
    PropertyIDISPHDRBEConfig,                     // 3
    PropertyIDParsedHDRBEStatsOutput,             // 4
    PropertyIDAECInternal,                        // 5
    PropertyIDAECFrameInfo,                       // 6
    SensorFrameDuration,                          // 7
    PropertyIDAFPDFrameInfo,                      // 8
    PropertyIDUsecaseSensorCurrentMode,           // 9
    PropertyIDUsecaseLensInfo,                    // 10
    PropertyIDUsecaseSensorModes,                 // 11
    PropertyIDUsecaseIFEInputResolution,          // 12
    InputControlAFMode,                           // 13
    InputControlSceneMode,                        // 14
    InputControlCaptureIntent,                    // 15
    InputControlAFRegions,                        // 16
    InputControlAFTrigger,                        // 17
    InputLensFocusDistance,                       // 18
    PropertyIDAFFrameControl,                     // 19
    PropertyIDAFFrameInfo,                        // 20
    PropertyIDAFStatsControl,                     // 21
    PropertyIDDebugDataAll,                       // 22
};

/// @brief List of AF Read Property types
enum AFPropertyReadType
{
    AFReadTypeInvalid = -1,
    AFReadTypePropertyIDUsecaseChiTuningModeParameter,      // 0
    AFReadTypeInputScalerCropRegion,                        // 1
    AFReadTypePropertyIDParsedBFStatsOutput,                // 2
    AFReadTypePropertyIDISPHDRBEConfig,                     // 3
    AFReadTypePropertyIDParsedHDRBEStatsOutput,             // 4
    AFReadTypePropertyIDAECInternal,                        // 5
    AFReadTypePropertyIDAECFrameInfo,                       // 6
    AFReadTypeSensorFrameDuration,                          // 7
    AFReadTypePropertyIDAFPDFrameInfo,                      // 8
    AFReadTypePropertyIDUsecaseSensorCurrentMode,           // 9
    AFReadTypePropertyIDUsecaseLensInfo,                    // 10
    AFReadTypePropertyIDUsecaseSensorModes,                 // 11
    AFReadTypePropertyIDUsecaseIFEInputResolution,          // 12
    AFReadTypeInputControlAFMode,                           // 13
    AFReadTypeInputControlSceneMode,                        // 14
    AFReadTypeInputControlCaptureIntent,                    // 15
    AFReadTypeInputControlAFRegions,                        // 16
    AFReadTypeInputControlAFTrigger,                        // 17
    AFReadTypeInputLensFocusDistance,                       // 18
    AFReadTypePropertyIDAFFrameControl,                     // 19
    AFReadTypePropertyIDAFFrameInfo,                        // 20
    AFReadTypePropertyIDAFStatsControl,                     // 21
    AFReadTypePropertyIDDebugDataAll,                       // 22
    AFReadTypePropertyIDCount,                              // 23
    AFReadTypeMax = 0x7FFFFFFF     ///< Anchor to indicate the last item in the defines
};

CAMX_STATIC_ASSERT(AFReadTypePropertyIDCount == CAMX_ARRAY_SIZE(AFPropertyReadTags));

/// @brief List of AWB Write Property tags
static UINT32 AFPropertyWriteTags[] =
{
    PropertyIDAFFrameControl,
    PropertyIDAFStatsControl,
    PropertyIDAFFrameInfo,
    ControlAFState,
    ControlAFMode,
    ControlAFRegions,
    LensFocusDistance,
    ControlAFTrigger,
};

static const UINT NumAFPropertyWriteTags = sizeof(AFPropertyWriteTags) / sizeof(UINT32); ///< Number of AF write Properties.

struct AFPropertyDependencyList;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BF Filter configuration parameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Enum to list number of Filter presents
enum OutputBAFFilterType
{
    BAFFilterTypeHorizontal1,   ///< First horizontal BAF filter
    BAFFilterTypeHorizontal2,   ///< Second horizontal BAF filter
    BAFFilterTypeVertical,      ///< Vertical BAF filter
    BAFFilterTypeMax            ///< Maximum number of filters present
};

/// @brief BF Filter parameters
struct OutputBFFilterConfigParams
{
    AFBAFFIRFilterConfig    FIRFilterConfig;                        ///< FIR filter configuration
    AFBAFIIRFilterConfig    IIRFilterConfig;                        ///< IIR filter configuration
    AFBAFFilterCoringConfig filterCoringConfig;                     ///< Coring required for filter
    INT32                   FIRFilterCoeff[MaxBAFFIRFilterSize];    ///< Holds BAF FIR filter coefficients
    UINT32                  coring[MaxAFBAFFilterCoringSize];       ///< Holds BAF Coring data
};

/// @brief BF filter configuration
struct OutputBFConfig
{
    AFBAFInputConfig            BAFInputConfig;                 ///< Filter channel configuration
    FLOAT                       yChannel[MaxAFYConfig];         ///< Y Channel configuration
    AFBAFGammaLUTConfig         BAFGammaLUTConfig;              ///< Gamma lookup table configuration
    UINT32                      gammaLUT[MaxAFBAFGammaEntries]; ///< Gamma lookup table
    AFBAFScaleConfig            BAFScaleConfig;                 ///< Scalar configuration
    AFBAFFilterConfig           BAFFilterConfigH1;              ///< Horizontal-1 filter configuration
    AFBAFFilterConfig           BAFFilterConfigH2;              ///< Horizontal-2 filter configuration
    AFBAFFilterConfig           BAFFilterConfigV;               ///< Vertical filter configuration
    OutputBFFilterConfigParams  filter[BAFFilterTypeMax];       ///< Filter configuration parameters
};

/// @brief Represents AF HAL output type
enum AFHALOutputType
{
    AFHALOutputTypeInvalid          = -1,           ///< Invalid type
    AFHALOutputTypeControlAFState,                  ///< AF status for HAL
                                                    ///  Payload : ControlAFStateValues
    AFHALOutputTypeControlFocusMode,                ///< Focus Mode
                                                    ///  Payload: ControlAFModeValues
    AFHALOutputTypeControlAFRegions,                ///< AF Regions
                                                    ///  Payload: ControlAFRegions
    AFHALOutputTypeLensFocusDistance,               ///< Lens Focus Distance
                                                    ///< Payload: FLOAT
    AFHALOutputTypeFocusValue,                      ///< Focus Value
                                                    ///< Payload: FLOAT
    AFHALOutputTypeLastIndex,                       ///< Last index of the enum
                                                    ///  Payload type: None
    AFHALOutputTypeMax              = 0x7FFFFFFF    ///< Max Type
};

/// @brief Represents AF individual HAL output
struct AFHALOutput
{
    VOID*               pAFOutput;              ///< Pointer to the payload.
    AFHALOutputType     outputType;             ///< Type of the payload
};

/// @brief Represents AF output for HAL
struct AFHALOutputList
{
    AFHALOutput*        pAFOutputs;     ///< Pointer to AFHALOutputs array
    UINT32              outputCount;    ///< Number of elements in AFHALOutputList
};

/// @brief This structure holds AF related HAL Data
struct AFHALData
{
    ControlAFStateValues   afState;                    ///< AF state value
    ControlAFModeValues    afMode;                     ///< AF Mode
    WeightedRegion         weightedRegion;             ///< region for focusing/metering area
    LensStateValues        lensState;                  ///< Lens state values
    FLOAT                  lensFocusDistance;          ///< Lens focus distance
    ControlAFTriggerValues afTriggerValues;            ///< AF state transition causes
};


/// @brief Enum to list number of Filter presents
enum AFBufferDependencyType
{
    AddDependencies         = 0,    ///< First call to AF Node EPR to ask for AF dependency
    AddBAFStatsDependency   = 1,    ///< Add BAF buffer dependency
    BAFStatsDependencyMet   = 1,    ///< BAF buffer dependency met
    ADDPDStatDependency     = 2,    ///< Add PD dependency
    PDStatDependencyMet     = 2,    ///< PD buffer dependency met
    FOVCCopyDependency      = 3     ///< Should copy FOVC data
};

/// @brief Enum to select output for AF logs
enum AFLogMask
{
    AFStatsDump                 = 0x1, ///< outputs AF Stats
    AFFrameInfoDump             = 0x2, ///< outputs AF Frame Info
    AFHWPDConfigDump            = 0x4, ///< outputs AF HWPD config info
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This class containes the required conditions for pdaf, DualPDHw, SparsePDHw to be enable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PDAFEnablementConditions
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PDAFEnablementConditions
    ///
    /// @brief  default constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PDAFEnablementConditions();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~PDAFEnablementConditions
    ///
    /// @brief  destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~PDAFEnablementConditions();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPdafEnableFromSetting
    ///
    /// @brief  Set if pdaf is enabled from camxsettings
    ///
    /// @param  disablePDAF  indicates if pdaf is disabled from setting
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPdafEnableFromSetting(
        BOOL disablePDAF);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCurrentSensorModeSupportedPDAF
    ///
    /// @brief  Set if current sensor mode supports pdaf
    ///
    /// @param  isCurrentSensorModeSupportedPDAF  indicates if pdaf is supported by current sensor mode
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetCurrentSensorModeSupportedPDAF(
        BOOL isCurrentSensorModeSupportedPDAF);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPDAFPortEnabled
    ///
    /// @brief  Set if pdaf port is avilable for this usecase
    ///
    /// @param  isPDAFPortEnabled  indicates if pdaf port is enabled for this usecase
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPDAFPortEnabled(
        BOOL isPDAFPortEnabled);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDualPDHWPortEnabled
    ///
    /// @brief  Set if dual PD port is available for this usecase
    ///
    /// @param  isDualPDHWPortEnabled  indicates if dual PD port is enable/available
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDualPDHWPortEnabled(
        BOOL isDualPDHWPortEnabled);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSparsePDHWPortEnabled
    ///
    /// @brief  Set if sparse PD port is available for this usecase
    ///
    /// @param  isSparsePDHWPortEnabled  indicates if dual PD port is enable/available
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetSparsePDHWPortEnabled(
        BOOL isSparsePDHWPortEnabled);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPDAFType
    ///
    /// @brief  Set sensor pdaf type
    ///
    /// @param  pdafType  sensor pdaf type
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPDAFType(
        PDLibSensorType pdafType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDualPDHWAvailable
    ///
    /// @brief  Set if dual PD HW is supported by IFE
    ///
    /// @param  isDualPDHWSupported  indicates if HW PD is supported
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDualPDHWAvailable(
        BOOL isDualPDHWSupported);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSparsePDHWAvailable
    ///
    /// @brief  Set if sparse PD HW HW is supported by IFE
    ///
    /// @param  isSparsePDHWSupported  indicates if sparse PD HW HW is supported
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetSparsePDHWAvailable(
        BOOL isSparsePDHWSupported);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLCRHWAvailable
    ///
    /// @brief  Set if LCR HW HW is supported by IFE
    ///
    /// @param  isLCRHWAvailable  indicates if sparse PD HW HW is supported
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetLCRHWAvailable(
        BOOL isLCRHWAvailable);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPDAFHWEnableMask
    ///
    /// @brief  Set pdafHWEnable from camxsettings
    ///
    /// @param  pdafHWEnable  pdafHWEnable from camxsettings
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPDAFHWEnableMask(
        BOOL pdafHWEnable);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPDCallBack
    ///
    /// @brief  Set PD Library entry pointer
    ///
    /// @param  pPDCallBack  PD Library entry pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPDCallBack(
        CHIPDLIBRARYCALLBACKS* pPDCallBack);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLCREnableFromSetting
    ///
    /// @brief  Set if LCR is enabled from camxsettings
    ///
    /// @param  disablePDLibLCR  indicates if LCR is disabled from setting
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetLCREnableFromSetting(
        BOOL disablePDLibLCR);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPDHWLCRHWEnableFromPDLib
    ///
    /// @brief  Set if LCR is enabled from PDLib
    ///
    /// @param  pPDHWConfig Pointer to Auto Focus frame information output data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPDHWLCRHWEnableFromPDLib(
        PDHwConfig* pPDHWConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLCREnableFromTuning
    ///
    /// @brief  Set LCR enable member flag which is read from PDLib Tuning
    ///
    /// @param  pLCREnable Pointer to LCR enable boolean
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetLCREnableFromTuning(
        BOOL* pLCREnable);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLCRHWPortEnabled
    ///
    /// @brief  Set if LCR port is available for this usecase
    ///
    /// @param  isLCRHWPortEnabled  indicates if dual PD port is enable/available
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetLCRHWPortEnabled(
        BOOL isLCRHWPortEnabled);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetIFESupportedPDHW
    ///
    /// @brief  Set if LCR port is available for this usecase
    ///
    /// @param  pPDHwAvailablity  pointer to a structure which is published by IFE containing PD HW availability in current
    ///                           target
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetIFESupportedPDHW(
        PDHwAvailablity* pPDHwAvailablity);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFType
    ///
    /// @brief  Retrieve pdaf type
    ///
    /// @return PDLibSensorType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PDLibSensorType GetPDAFType();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPDAFPortDefinedForSensorType
    ///
    /// @brief  Indicates if proper pdaf port is defined for sensor type.
    ///
    /// @param  sensorPDType    sensor type
    /// @param  portID          input port ID
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPDAFPortDefinedForSensorType(
        PDLibSensorType sensorPDType,
        UINT portID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPDAFEnabledFromSetting
    ///
    /// @brief  Indicates if pdaf is enabled from camxsettings.
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPDAFEnabledFromSetting();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSparsePDHWAvailableInTarget
    ///
    /// @brief  Indicates if sparse PD HW exists in current IFE HW.
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsSparsePDHWAvailableInTarget();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPDAFEnabled
    ///
    /// @brief  Based on required conditions, it evaluate if pdaf is enabled.
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPDAFEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDualPDHWEnabled
    ///
    /// @brief  Based on required conditions, it evaluate if dual pd/2pd HW is enabled.
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsDualPDHWEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSparsePDHWEnabled
    ///
    /// @brief  Based on required conditions, it evaluate if sparse PD HW is enabled.
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsSparsePDHWEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPDHWEnabled
    ///
    /// @brief  Check if PDHW (sparse PD or dual PD) is enabled.
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPDHWEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsLCRHWEnabled
    ///
    /// @brief  Based on required conditions, it evaluate if LCR HW is enabled.
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsLCRHWEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsLCRSWEnabled
    ///
    /// @brief  Based on required conditions, it evaluate if LCR SW is enabled.
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsLCRSWEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDHWEnabled
    ///
    /// @brief  Function to Get the PDHwEnable
    ///
    /// @param  pPDHWEnableConditions Pointer to the PDHwEnable information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetPDHWEnabled(
        PDHWEnableConditions* pPDHWEnableConditions);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintPDHWEnableConditions
    ///
    /// @brief  Function to print the PDHWEnableConditions
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintPDHWEnableConditions();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintLCREnableConditions
    ///
    /// @brief  Function to print the LCR EnableConditions
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintLCREnableConditions();

protected:

    /// @brief Represents if sparse/dual PD & LCR HW are enabled from PDLib. PDlib will consider Tuning enable flag along with
    ///         all required condition to have sparse/dual PD & LCR HW enabled. In AF Node, we don't have access to tuning.

    struct PDLCRHWEnabledFromPDLib
    {
        BOOL    m_isLCRHWEnabled; ///< Indicate if LCR is enabled from PDLib
        BOOL    m_isPDHWEnabled;  ///< Indicate if PD HW is enabled from PDLib
    };

    BOOL                    m_isPdafEnableFromSetting;          ///< Indicate if pdaf is enabled from setting
    BOOL                    m_isCurrentSensorModeSupportedPDAF; ///< Indicate if sensor current mode support pdaf
    BOOL                    m_isPDAFPortEnabled;                ///< Indicate if PDAF port is enabled in current usecase
    BOOL                    m_isDualPDHWPortEnabled;            ///< Indicate if dual PDHW port is enabled
    BOOL                    m_isSparsePDHWPortEnabled;          ///< Indicate if sparse PDHW port is enabled
    PDLibSensorType         m_pdafType;                         ///< specifies type of PDAF
    BOOL                    m_isDualPDHWAvailable;              ///< Does ISP support 2pd HW
    BOOL                    m_isSparsePDHWAvailable;            ///< Does ISP support sparse HW
    BOOL                    m_isLCRHWAvailable;                 ///< Does ISP support LCR HW
    BOOL                    m_pdafHWEnableFromSetting;          ///< Refer to pdafHWEnable from camxsettings to enable/diable
                                                                ///< PD HW feature
    CHIPDLIBRARYCALLBACKS*  m_pPDCallback;                      ///< PD Library entry pointer
    BOOL                    m_isLCREnabledFromSetting;          ///< Indicate if LCR is enabled from setting
    BOOL                    m_isLCRHWPortEnabled;               ///< Indicate if dual PDHW port is enabled
    BOOL                    m_isPDAFEnabled;                    ///< Indicate if PDAF is enabled based on required conditions
    PDLCRHWEnabledFromPDLib m_PDLCRHWEnabledFromPDLib;          ///< Indicate if sparse/dual pd or LCR HW are enabled from Algo
    BOOL                    m_isLCREnabledFromTuning;           ///< Indicate if LCR is enabled from PDLib tuning
    PDHWEnableConditions    m_PDHWEnableConditions;             ///< Indicate if dualPD/SparsePD/LCR HW Enablement conditions.
                                                                ///< It doesn't include Tuning enable/disable. Tuning will be
                                                                ///< considerd in PDLib
private:
    PDAFEnablementConditions(const PDAFEnablementConditions&) = delete;             ///< Do not implement copy constructor
    PDAFEnablementConditions& operator=(const PDAFEnablementConditions&) = delete;  ///< Do not implement assignment operator
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This class handles AF input and output parameters.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAFIOUtil
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Used to initialize the class.
    ///
    /// @param  pInitializeData Pointer to initial settings
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        const StatsInitializeData* pInitializeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadBFStat
    ///
    /// @brief  Read BF stats from the property pool
    ///
    /// @param  pBFStats   Pointer to BF statistics buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadBFStat(
        AFBAFStatistics* pBFStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadBGStat
    ///
    /// @brief  Read BG stats from the property pool
    ///
    /// @param  pBGStats   Pointer to BG statistics buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadBGStat(
        StatsBayerGrid* pBGStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadVendorTag
    ///
    /// @brief  Read VendorTAg from the property pool
    ///
    /// @param  pVendorTagList List of dependent vendor tags
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReadVendorTag(
        StatsVendorTagList* pVendorTagList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadVendorTagIsAFLock
    ///
    /// @brief  Read from VendorTag isAFLock
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL ReadVendorTagIsAFLock();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadAECInput
    ///
    /// @brief  Read AEC input from the property pool
    ///
    /// @param  pAECInfo   Pointer to AEC information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ReadAECInput(
        AFAECInfo* pAECInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadPDAFDataInput
    ///
    /// @brief  Pass pointer to the internal  PDAF input member
    ///
    ///
    /// @return Pointer to valid PDAF input data if successful, NULL if any failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AFPDAFData* ReadPDAFDataInput();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadSensorInput
    ///
    /// @brief  Read Sensor input from the property pool
    ///
    /// @param  pSensorInfo   Pointer to sensor information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReadSensorInput(
        AFSensorInfo* pSensorInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadHardwareCapability
    ///
    /// @brief  Read Hardware capabilities from the property pool
    ///
    /// @return Pointer to valid data if successful, NULL if any failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AFHardwareCapability* ReadHardwareCapability();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadFocusRegions
    ///
    /// @brief  Read Focus Regions from the property pool
    ///
    /// @param  pROIInfo   Pointer to ROI information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ReadFocusRegions(
        AFROIInfo* pROIInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadFaceROIInformation
    ///
    /// @brief  Read Focus Regions from the property pool
    ///
    /// @param  pFaceROIInfo              Pointer to Face ROI information
    /// @param  pUnstabilizedFaceROIInfo  Pointer to Face ROI information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadFaceROIInformation(
        AFFaceROIInfo* pFaceROIInfo,
        UnstabilizedROIInformation* pUnstabilizedFaceROIInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadTrackerROIInformation
    ///
    /// @brief  Read Focus Regions from the property pool
    ///
    /// @param  pTrackROIInfo             Pointer to  ROI information
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadTrackerROIInformation(
        UnstabilizedROIInformation* pTrackROIInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadFocusMode
    ///
    /// @brief  Read Focus Mode from the property pool
    ///
    /// @param  pAFFocusMode   Pointer to AF focus mode
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReadFocusMode(
        AFFocusMode* pAFFocusMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadCameraOperationalMode
    ///
    /// @brief  Read camera operational mode from the property pool
    ///
    /// @return Camera Operational mode
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AFRunMode ReadCameraOperationalMode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadControlAFTrigger
    ///
    /// @brief  Read control AF trigger from the property pool
    ///
    /// @return control AF trigger
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ControlAFTriggerValues ReadControlAFTrigger();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadFocusDistanceInfo
    ///
    /// @brief  Read focus distance from property pool for manual AF
    ///
    /// @param  pFocusDistance   Pointer to focus distance
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReadFocusDistanceInfo(
        AFFocusDistanceInfo* pFocusDistance);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAFOutputBuffers
    ///
    /// @brief  Get output buffers to get algorithm output into
    ///
    /// @param  ppOutputList    Pointer to pointer to algo output list
    /// @param  ppHALOutput     Pointer to pointer to HAL output list
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAFOutputBuffers(
        AFAlgoOutputList**  ppOutputList,
        AFHALOutputList**   ppHALOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishOutput
    ///
    /// @brief  Publishes the algorithm output to all metadata pool
    ///
    /// @param  pOutput                         Pointer to core algorithm's output
    /// @param  pHALOutput                      Pointer to HAL's output
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PublishOutput(
        const AFAlgoOutputList*         pOutput,
        const AFHALOutputList*          pHALOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishExternalCameraMetadata
    ///
    /// @brief  Publishes AF output to the metadata pool
    ///
    /// @param  pOutput     Pointer to core algorithm's output
    /// @param  pHALOutput  Pointer to the list of HAL output
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishExternalCameraMetadata(
        const AFAlgoOutputList* pOutput,
        const AFHALOutputList*  pHALOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPreRequestOutput
    ///
    /// @brief  Publishes the algorithm GetParam info to metadata pool
    ///
    /// @param  pGetParam                       Pointer to the list of algorithim output coming from GetParam call
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PublishPreRequestOutput(
        const AFAlgoGetParam* pGetParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishDefaultOutput
    ///
    /// @brief  Publishes the algorithm GetParam info to metadata pool
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PublishDefaultOutput();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFOVC
    ///
    /// @brief  Publishes the defaut FOVC info to metadata pool
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishFOVC();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishSkippedFrameOutput
    ///
    /// @brief  Publish AF frame and stats control from previous frame property pool
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishSkippedFrameOutput() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishHWPDConfig
    ///
    /// @brief  Publish PD Hw Configuration
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishHWPDConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPDHWEnableConditions
    ///
    /// @brief  Function to Publish PDHWEnableConditions to property pool
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  PublishPDHWEnableConditions() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseISPStats
    ///
    /// @brief  Function to Parse raw ISP stats and publish the output to MetadataPool
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseISPStats(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishCrossProperty
    ///
    /// @brief  Publish AF cross pipeline property for dual camera usecase.
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishCrossProperty(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishBAFStatDependencyMet
    ///
    /// @brief  Publish BAFDependencyMet.
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishBAFStatDependencyMet(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateChiStatsSession
    ///
    /// @brief  Function to update the stat process request data in chi stats session
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateChiStatsSession(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo)
    {
        m_statsSession.SetStatsProcessorRequestData(pStatsProcessRequestDataInfo);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParsePDStats
    ///
    /// @brief  Function to Parse raw PD stats and publish the output to MetadataPool
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParsePDStats(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CAFIOUtil
    ///
    /// @brief  default constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAFIOUtil();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CAFIOUtil
    ///
    /// @brief  destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~CAFIOUtil();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetStatsParser
    ///
    /// @brief  Function to Set the StatsParser
    ///
    /// @param  pStatsParser Pointer to the Stats Parser
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetStatsParser(
        StatsParser* pStatsParser);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyInputSettings
    ///
    /// @brief  Function to Set the default HAL settings
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CopyInputSettings();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCameraInfo
    ///
    /// @brief  Function to Set the camera information
    ///
    /// @param  pCameraInfo Pointer to the camera infomation
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetCameraInfo(
        StatsCameraInfo* pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraInfo
    ///
    /// @brief  Function to Get the camera information
    ///
    /// @param  pCameraInfo Pointer to the camera infomation
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetCameraInfo(
        StatsCameraInfo* pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGyroData
    ///
    /// @brief  Getter function to get gyro data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const AFGyroData GetGyroData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGravityData
    ///
    /// @brief  Getter function to get gravity data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const AFGravityData GetGravityData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTOFData
    ///
    /// @brief  Getter function to get TOF data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const AFTOFData GetTOFData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateAFDefaultGetParamBuffers
    ///
    /// @brief  Prepare GetParam buffers to get algorithm parameter info
    ///
    /// @param  pGetParam Pointer to the list of algorithim output coming from GetParam call
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateAFDefaultGetParamBuffers(
        AFAlgoGetParam* pGetParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoOutputGetParamBuffers
    ///
    /// @brief  provide algo output get param buffer
    ///
    /// @param  pGetParam Pointer to the list of algorithim output
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAlgoOutputGetParamBuffers(
        AFAlgoGetParam* pGetParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateVendorTagGetParam
    ///
    /// @brief  assign memory for get param vendor tag related operation
    ///
    /// @param  getParamType    type of AF GetParameter query
    /// @param  pGetParam       Pointer to the list of algorithim output coming from GetParam call
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateVendorTagGetParam(
        AFAlgoGetParamType  getParamType,
        AFAlgoGetParam*     pGetParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateMemoryVendorTag
    ///
    /// @brief  It goes through all vendor tag list and allocate memory to hold the vendor tag data
    ///
    /// @param  pAlgoGetParam pointer to vendor tag list
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocateMemoryVendorTag(
        AFAlgoGetParam* pAlgoGetParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetOutputs
    ///
    /// @brief  It reset outputs and make them ready for next call to algo
    ///
    /// @param  pOutput     Pointer to core algorithm's output
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ResetOutputs(
        const AFAlgoOutputList* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultSensorResolution
    ///
    /// @brief  Get Default sensor resolution for first frame.
    ///
    /// @param  ppSensorInfo   Pointer to ponter to Sensor input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDefaultSensorResolution(
        AFSensorInfo** ppSensorInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapFocusModeToHALType
    ///
    /// @brief  Function to map AF algorithm focus mode type to HAL focus mode type
    ///
    /// @param  pAFMode             Algorithm focus mode type
    /// @param  pHALAFModeType      HAL focus mode type
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID MapFocusModeToHALType(
        const AFFocusMode*  pAFMode,
        ControlAFModeValues* pHALAFModeType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDebugData
    ///
    /// @brief  Returns the debug data associated with a given request ID
    ///
    /// @param  requestId       Request ID for which to obtain the debug data
    /// @param  ppDebugDataOut  Debug data to be returned for the given request ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetDebugData(
        UINT64      requestId,
        DebugData** ppDebugDataOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiStatsSession
    ///
    /// @brief  Get the stats chi session data.
    ///
    /// @return pointer chi stats session
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiStatsSession* GetChiStatsSession()
    {
        return &m_statsSession;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulatePDInputData
    ///
    /// @brief  Function populates the input parameters to execute PDLibraryProcess
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pPDInput                        Pointer to PD Library input paramstag of the data to get from the pool
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL PopulatePDInputData(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        PDLibInputs*                    pPDInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsEarlyPDPublished
    ///
    /// @brief  Function checks if a Early PD has been published
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsEarlyPDPublished();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateGyroData
    ///
    /// @brief  Retrieves gyro info from NCS interface
    ///
    /// @return CamxResultSuccess if successful
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulateGyroData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateGravityData
    ///
    /// @brief  Retrieves gravity info from NCS interface
    ///
    /// @return CamxResultSuccess if successful
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulateGravityData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateTOFData
    ///
    /// @brief  Retrieves TOF info from TOF interface
    ///
    /// @param  requestId Request ID for which stats is being generated
    ///
    /// @return CamxResultSuccess if successful
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulateTOFData(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadPDLibHandleFromUsecasePool
    ///
    /// @brief  Loads PD Library Handle from the usecase pool
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LoadPDLibHandleFromUsecasePool();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeReadProperties
    ///
    /// @brief  Is PDAF Enabled in current sensor mode.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeReadProperties();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeWriteProperties
    ///
    /// @brief  Is PDAF Enabled in current sensor mode.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeWriteProperties();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadMultiCameraInfo
    ///
    /// @brief  Read DualCameraInformation from FOV controller.
    ///
    /// @param  pStatsCameraInfo   supported sensor modes
    ///
    /// @return Success or Failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadMultiCameraInfo(
        StatsCameraInfo* pStatsCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCropWindowInfo
    ///
    /// @brief  Function to retrieve crop window
    ///
    ///
    /// @param  pStatsRectangle Pointer to crop window info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetCropWindowInfo(
        StatsRectangle*     pStatsRectangle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPeerFocusInfo
    ///
    /// @brief  Publish Property Master info for Peer.
    ///
    /// @param  pPeerInfo  Pointer to Peer information
    ///
    /// @return Success or Failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPeerFocusInfo(
        VOID*  pPeerInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPeerInfo
    ///
    /// @brief  Retrieve the Peer Property from property pool.
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return VOID* or NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetPeerInfo(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOTPData
    ///
    /// @brief  Retreive AF Calibration OTP data at Initialization.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AFCalibrationOTPData* GetOTPData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAFReadProperties
    ///
    /// @brief  Get AF read properties.
    ///
    /// @param  requestIdOffsetFromLastFlush requestId offset from last frame which has been flushed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAFReadProperties(
        UINT64 requestIdOffsetFromLastFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAFPDAFInformation
    ///
    /// @brief  Set PDAF Enable flag.
    ///
    /// @param  pPDAFEnablementConditions   pointer to the structure which holds required conditions to enable
    ///                                     pdaf/dualPDHW/SparsePDHW.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAFPDAFInformation(
        PDAFEnablementConditions* pPDAFEnablementConditions);

protected:

    /// @todo (CAMX-1462): Group related class variable of camxcafioutil into corresponding structure
    Node*                               m_pNode;                                    ///< Pointer to owning node
    MetadataPool*                       m_pDebugDataPool;                           ///< Pointer to the debugdata metadata pool
    HwCameraInfo                        m_hwCameraInfo;                             ///< static camera info
    StatsCameraInfo                     m_cameraInfo;                               ///< Camera id, role and type information
    StatsParser*                        m_pStatsParser;                             ///< ISP stats parser
    AFSensorInfo                        m_sensorInfoInput;                          ///< Holds sensor information input for
                                                                                    ///  AF Algorithm
    AFFocusMode                         m_focusMode;                                ///< Holds AF focus mode input for
                                                                                    ///  AF Algorithm
    AFROIDimension                      m_AFOutputROIDim;                           ///< ROI coordinate
    StatsVendorTagInfoList              m_vendorTagInputList;                       ///< List of Stats vendor tag dependency
    StatsVendorTagInfoList              m_vendorTagOutputList;                      ///< List of Stats output vendor tag
    AFBAFFloatingWindowROIConfig        m_AFOutputBAFFWROIConfig;                   ///< BAF ROI configuration
    AFBAFFloatingWindowROIParam         m_AFOutputFWROIDimensions[MaxFWRegions];    ///< ROI dimensions of each floating windows
    AFBAFFloatingWindowConfig           m_AFOutputBAFFWConfig;                      ///< BAF stats configuration
    OutputBFConfig                      m_BFOut;                                    ///< Holds BAF stats configuration values
    AFAlgoExposureCompensation          m_AFOutputExposureCompensation;             ///< Exposure Compensation Enable
    PDLibWindowConfig                   m_AFOutputPDAFWindowConfig;                 ///< PDAF ROI configuration
    AFLensPositionInfo                  m_AFOutputLensMoveInfo;                     ///< Move Lens information
    AFFOVCInfo                          m_AFFOVCInfo;                               ///< FOVC information
    AFAlgoStatusParam                   m_AFOutputAFStatus;                         ///< Focus status
    AFAlgoFocusValue                    m_AFOutputFocusValue;                       ///< Focus value
    AFGyroData                          m_AFOutputGyroValue;                        ///< Gyro value
    AFGravityData                       m_AFOutputGravityValue;                     ///< Gravity value
    AFTOFData                           m_AFOutputTOFValue;                         ///< TOF sensor data
    AFAlgoSpotLightInfo                 m_AFOutputSpotLightDetected;                ///< Spot light detected enable
    AFAlgoCropMagnificationFactor       m_AFOutputCropMagnificationFactor;          ///< Crop magnification factor
    BOOL                                m_AFOutputIsDepthFocus;                     ///< Depth focus indicator
    AFSoftwareStatisticsFilterConfig    m_AFOutputSoftwareStatsFilterConfig;        ///< Software stats filter configuration
    FLOAT                               m_AFOutputSWIIRCoeffA[MaxSWIIRCoeffA];      ///< Software stats IIR A Coefficients
    FLOAT                               m_AFOutputSWIIRCoeffB[MaxSWIIRCoeffB];      ///< Software stats IIR B Coefficients
    FLOAT                               m_AFOutputSWFIRCoeff[MaxSWFIRCoeff];        ///< Software stats FIR Coefficients
    StatsDataPointer                    m_AFOutputDebugData;                        ///< Debug data passed from algorithm
    AFAlgoOutput                        m_outputArray[AFOutputTypeLastIndex];       ///< Structure to the output parameter for
    AFCalibrationOTPData                m_AFCalibrationOTPData;                     ///< AF Calibration OTP Data
                                                                                    ///  the interface
    AFHALOutput                         m_HALoutputArray[AFHALOutputTypeLastIndex]; ///< Structure to the HAL output parameter
                                                                                    ///  for the interface
    AFAlgoGetParamOutput                m_getParamOutputArray[AFGetParamLastIndex]; ///< Array of algo getparam outputs
    AFAlgoGetParamOutput                m_getParamAlgoOutput[AFGetParamLastIndex];  ///< Array to hold algorithm outputs
    AFAlgoGetParamInput                 m_getParamInputArray[AFGetParamLastIndex];  ///< Array of algo getparam input
    AFAlgoOutputList                    m_outputList;                               ///< Output buffer to get algorithm data
    AFHALOutputList                     m_HALoutputList;                            ///< Output buffer to hold HAL output data
    AFAlgoGetParamOutputList            m_getParamOutputList;                       ///< Output buffer to getParam algorithm
                                                                                    ///  data
    StatsVendorTagList                  m_vendorTagPublishDataList;                 ///< Holds vendor tag data need to be
                                                                                    /// published
    ChiStatsSession                     m_statsSession;                             ///< Holds stats session data for
                                                                                    ///  any vendor tag operation
    AFHardwareCapabiltyKernelSpec       m_kernelInfoInput[MaxAFKernelsSupported];   ///< Holds kernel hardware capability
                                                                                    ///  information input for AF Algorithm
    AFHardwareCapabilityFloatingWindow  m_floatingWindowInfo;                       ///< Holds floating window hardware
                                                                                    ///  capability information
    AFHardwareCapability                m_hardwareCapability;                       ///< Hardware Capability
    UINT                                m_sensorModeIndex;                          ///< Hold current sensor mode
    UINT                                m_maxFocusRegions;                          ///< Max Number of focus regions from
                                                                                    ///  Application
    AFROIInfo                           m_focusRegionInfo;                          ///< Focus Regions from Application
    AFFaceROIInfo                       m_faceROIInfo;                              ///< Face ROI Info
    UnstabilizedROIInformation          m_statsROIInformation;                      ///< Uniform Face ROI Info
    UnstabilizedROIInformation          m_trackROIInformation;                      ///< Uniform Track ROI Info
    ControlAFTriggerValues              m_controlAFTriggerValues;                   ///< AF state transition causes
    AFFocusDistanceInfo                 m_focusDistanceInfo;                        ///< Focus Distance Information
    ControlAFStateValues                m_AFState;                                  ///< Control AF state
    BOOL                                m_isVideoCAFMode;                           ///< Holds info on whether the focus mode
                                                                                    ///  type is Continuous Video
    LensStateValues                     m_lensState;                                /// Control Lens State
    UINT32                              m_sensorActiveArrayWidth;                   ///< Sensor active array width
    UINT32                              m_sensorActiveArrayHeight;                  ///< Sensor active array width
    ControlSceneModeValues              m_controlSceneMode;                         ///< The scene control mode for 3A mode
    CHIPDLib*                           m_pPDLib;                                   ///< Pointer to the instance of PD library
    UINT32                              m_maxPrimaryROIHeight;                      ///< Maximum height of the primary window
    AFPDAFData                          m_PDAFDataInput;                            ///< Hold PDAF data input for the algorithm.
    AFPDAFData                          m_PDAFDataInputs[PDTriggerTypeCount];       ///< Hold PDAF data input for the algorithm.
                                                                                    ///< current frame PDLib output from early
                                                                                    ///< interrupt and PDLiboutput of previous
                                                                                    ///< from end of frame interript
    PDOutput                            m_PDAlgoOutputs[PDTriggerTypeCount];        ///< Hold PD Algo outputs for early and EOF
    UINT32                              m_cameraID;                                 ///< Camera ID
    BOOL                                m_selectionMap[MaxAFFocusMapNum];           ///< Holds AF selection map output
    UINT                                m_confidenceMap[MaxAFFocusMapNum];          ///< Holds AF confidence map output
    UINT                                m_distanceMap[MaxAFFocusMapNum];            ///< Holds AF distance map output
    INT                                 m_defocusMap[MaxAFFocusMapNum];             ///< Holds AF distance map output
    AFFocusMapInfo                      m_AFFocusMapOut;                            ///< AF Focus Map output

    NCSSensor*                          m_pNCSSensorHandleGyro;                     ///< NCS Sensor Gyro handle.
    NCSSensor*                          m_pNCSSensorHandleGravity;                  ///< NCS Sensor Gravity handle.
    NCSService*                         m_pNCSServiceObject;                        ///< NCS Service object pointer.
    TOFSensorIntf*                      m_pTOFInterfaceObject;                      ///< TOF interface object pointer
    UINT32                              m_requestQueueDepth;                        ///< Request Queue Depth

    StatsPropertyReadAndWrite*          m_pStatsAFPropertyReadWrite;                        ///< Stats AF Readwrite instance
    ReadProperty                        m_AFReadProperties[AFReadTypePropertyIDCount];      ///< AF Read Properties
    WriteProperty                       m_AFWriteProperties[NumAFPropertyWriteTags];        ///< AF Write Properties
    AFFrameControl                      m_frameControl;                                     ///< AF Frame control
    AFStatsControl                      m_statsControl;                                     ///< AF Stats control
    AFFrameInformation                  m_AFFrameInfo;                                      ///< AF Frame Info
    AFHALData                           m_AFHALData;                                        ///< AF HAL Data

    AFPDAFDepthMapInfo                  m_depthMapInfo;                             ///< Member to hold depth map information
    BOOL                                m_bGyroErrIndicated;                        ///< Used to prevent flood of error msgs
                                                                                    ///  in case of ongoing gyro errors
    PDAFEnablementConditions*           m_pPDAFEnablementConditions;                ///< pointer to instance of Include
                                                                                    ///< PDAFEnablementConditions
    BOOL                                m_isAFLock;                                 ///< AF Lock is enabled or not
private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupTOFLink
    ///
    /// @brief  Set up a link to the TOF interface from this node
    ///
    /// @param  pChiContext  Pointer to Chi context
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupTOFLink(
        ChiContext* pChiContext);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupNCSLink
    ///
    /// @brief  Set up a link to the NCS service from this node
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupNCSLink();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupNCSLinkForSensor
    ///
    /// @brief  Set up a link to the NCS service for sensor
    ///
    /// @param  sensorType    Sensor type for which NCS service is registered
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupNCSLinkForSensor(
        NCSSensorType  sensorType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateFrameControl
    ///
    /// @brief  Publishes the algorithm GetParam info for frame control to metadata pool
    ///
    /// @param  pGetParam       Pointer to the list of algorithm output coming from GetParam call
    /// @param  pFrameControl   Pointer to Auto Focus frame control output data
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateFrameControl(
        const AFAlgoGetParam*   pGetParam,
        AFFrameControl*         pFrameControl
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateStatsControl
    ///
    /// @brief  Publishes the algorithm GetParam info for stats control to metadata pool
    ///
    /// @param  pGetParam       Pointer to the list of algorithm output coming from GetParam call
    /// @param  pStatsControl   Pointer to Auto Focus stats control output data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateStatsControl(
        const AFAlgoGetParam*   pGetParam,
        AFStatsControl*         pStatsControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateFrameInformation
    ///
    /// @brief  Publishes the algorithm GetParam info for frame control to metadata pool
    ///
    /// @param  pGetParam      Pointer to the list of algorithm output coming from GetParam call
    /// @param  pInformation   Pointer to Auto Focus frame information output data
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateFrameInformation(
        const AFAlgoGetParam*   pGetParam,
        AFFrameInformation*     pInformation
    ) const;

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   /// PopulatePDHWConfiguration
   ///
   /// @brief  Publishes the algorithm GetParam info for frame control to metadata pool
   ///
   /// @param  pPDHWConfig   Pointer to Auto Focus frame information output data
   ///
   /// @return None
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulatePDHWConfiguration(
                    PDHwConfig* pPDHWConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateLCREnable
    ///
    /// @brief  Extract LCR enable from PDLIB tuning
    ///
    /// @param  pLCREnable   Pointer to LCR enable boolean flag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateLCREnable(
        BOOL* pLCREnable);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareAFOutputBuffers
    ///
    /// @brief  Prepare output buffers to get algorithm output info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareAFOutputBuffers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareBAFConfigBuffers
    ///
    /// @brief  Prepare BAF config buffers to get algorithm BAF config
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareBAFConfigBuffers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFrameControl
    ///
    /// @brief  Publishes the algorithm output to the main metadata pool
    ///
    /// @param  pOutput       Pointer to core algorithm's output
    /// @param  pFrameControl Pointer to AF Frame control
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishFrameControl(
        const AFAlgoOutputList* pOutput,
        AFFrameControl*         pFrameControl
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishStatsControl
    ///
    /// @brief  Publishes the algorithm output
    ///
    /// @param  pOutput           Pointer to core algorithm's output
    /// @param  pStatsControl     Pointer to AF stats control
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishStatsControl(
        const AFAlgoOutputList* pOutput,
        AFStatsControl *        pStatsControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFrameInfo
    ///
    /// @brief  Publishes the algorithm output to the main metadata pool
    ///
    /// @param  pOutput          Pointer to core algorithm's output
    /// @param  pAFFrameInfo     Pointer to AF Frame information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishFrameInfo(
        const AFAlgoOutputList* pOutput,
        AFFrameInformation*     pAFFrameInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishHALMetadata
    ///
    /// @brief  Publishes AF output to the metadata pool
    ///
    /// @param  pHALOutput  Pointer to the list of HAL output
    /// @param  pAFHALData  Pointer to the AF HAL Data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishHALMetadata(
        const AFHALOutputList*  pHALOutput,
        AFHALData*              pAFHALData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishVendorTagMetadata
    ///
    /// @brief  Publishes AF output to the metadata pool
    ///
    /// @param  pOutput     Pointer to core algorithm's output
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishVendorTagMetadata(
        const AFAlgoOutputList* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPropertyDebugData
    ///
    /// @brief  Publishes the debug-data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPropertyDebugData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveAECFrameInfo
    ///
    /// @brief  Function to retrieve available AEC  per frame information from Main property pool
    ///
    ///
    /// @param  pAECFrameInfo   Pointer to AEC frame information
    /// @param  pAECInfo        Pointer to AEC structure to be filled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RetrieveAECFrameInfo(
        AECFrameInformation* pAECFrameInfo,
        AFAECInfo*           pAECInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveBFStat
    ///
    /// @brief  Function to retrieve BF stats from property blob BF structure
    ///
    ///
    /// @param  pParsedBFStatsOutput    Pointer to parsed BF statistics output
    /// @param  pBFStat                 BF stat structure to be filled
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveBFStat(
        ParsedBFStatsOutput* pParsedBFStatsOutput,
        AFBAFStatistics*     pBFStat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveBGStat
    ///
    /// @brief  Function to retrieve BG stats from property blob BG structure
    ///
    ///
    /// @param  pISPHDRStats            HDR stats from ISP
    /// @param  pParsedBGStatsOutput    Pointer to parsed BG statistics output
    /// @param  pBGStat                 BG stat structure to be filled
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveBGStat(
        PropertyISPHDRBEStats*  pISPHDRStats,
        ParsedHDRBEStatsOutput* pParsedBGStatsOutput,
        StatsBayerGrid*         pBGStat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveFocusRegions
    ///
    /// @brief  Function to retrieve focus regions
    ///
    ///
    /// @param  pWeightedRegion Pointer to weighted region
    /// @param  pAFROI          Pointer to Focus Regions to be filled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RetrieveFocusRegions(
        WeightedRegion* pWeightedRegion,
        AFROIInfo*      pAFROI);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveFaceRegions
    ///
    /// @brief  Function to retrieve focus regions
    ///
    ///
    /// @param  pFaceROIInfo               Pointer to face info
    /// @param  pAFFaceROI                 Pointer to Focus Regions to be filled
    /// @param  pUnstabilizedFaceROIInfo   pPointer to Focus Regions to be filled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RetrieveFaceRegions(
        FaceROIInformation*             pFaceROIInfo,
        AFFaceROIInfo*                  pAFFaceROI,
        UnstabilizedROIInformation*     pUnstabilizedFaceROIInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdatePropertyMoveLensOutput
    ///
    /// @brief  Function to update Algorithm requested move lens information
    ///
    /// @param  pMoveLensOutput     Pointer to Move lens output from property pool
    /// @param  pLensMoveInfo       Pointer to algorithm output
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdatePropertyMoveLensOutput(
        MoveLensOutput*     pMoveLensOutput,
        AFLensPositionInfo* pLensMoveInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdatePropertyBFConfigOutput
    ///
    /// @brief  Function to assign BAF algo output to property output
    ///
    /// @param  pAFConfig    Pointer to AF output from property pool
    /// @param  pBAFFwConfig Pointer to algorithm output
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdatePropertyBFConfigOutput(
        AFConfigParams*             pAFConfig,
        AFBAFFloatingWindowConfig*  pBAFFwConfig
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdatePropertyROIOutput
    ///
    /// @brief  Function to assign BAF ROI algo output to property output
    ///
    /// @param  pBFConfig       Pointer to AF output from property pool
    /// @param  pBAFFwROIConfig Pointer to algorithm output
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdatePropertyROIOutput(
        BFStatsConfigParams*            pBFConfig,
        AFBAFFloatingWindowROIConfig*   pBAFFwROIConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdatePropertyFocusStatus
    ///
    /// @brief  Function to assign BAF ROI algo output to property output
    ///
    /// @param  pFocusStatus        Pointer to AF output from property pool
    /// @param  pAlgoOutSatus       Pointer to algorithm output
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdatePropertyFocusStatus(
        AFStatusParams*     pFocusStatus,
        AFAlgoStatusParam*  pAlgoOutSatus);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseStats
    ///
    /// @brief  Function to parse input stats
    ///
    /// @param  propertyId          Property pool id
    /// @param  pUnparsedBuffer     Pointer to unparsed ISP stats buffer
    /// @param  requestId           Request for which stats is being generated
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseStats(
        UINT32        propertyId,
        VOID*         pUnparsedBuffer,
        UINT64        requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GroupRegions
    ///
    /// @brief  Function to group ROIs parameters into one ROI
    ///
    /// @param  pConfigBFStats   Pointer to BF Stats configuration
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GroupRegions(
        BFStatsROIConfigType* pConfigBFStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ScaleRegion
    ///
    /// @brief  Function to scale a BFStatsROIConfigType region using current and reference resolution sizes
    ///
    /// @param  pROI                Pointer to ROI dimension
    /// @param  scalingRatio        Scaling ratio
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ScaleRegion(
        RectangleCoordinate* pROI,
        FLOAT                scalingRatio);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseYuvStats
    ///
    /// @brief  Function to parse yuv input stats
    ///
    /// @param  pUnparsedBuffer     Pointer to unparsed YUV buffer
    /// @param  requestId           Request for which stats is being generated
    /// @param  pFormat             Format information for the YUV image.
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseYuvStats(
        VOID*              pUnparsedBuffer,
        UINT64             requestId,
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadSoftwareBFStats
    ///
    /// @brief  Load generate software BF stats function from library
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LoadSoftwareBFStats();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPDLibOutput
    ///
    /// @brief  Publishes the PD Library output to internal metadata pool
    ///
    /// @param  pPDInputData    Pointer to PD Library input data
    /// @param  pPDOutputData   Pointer to PD Library output data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishPDLibOutput(
        PDLibInputs*  pPDInputData,
        PDLibOutputs* pPDOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHWPDConfig
    ///
    /// @brief  Get initial HW PD configuration during start session.
    ///
    /// @param  pPDLib      Pointer to the pointer of the created PD library instance
    /// @param  pHWConfig   Pointer to PDHW config
    ///
    /// @return Success if Sensor data is successfully populated
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetHWPDConfig(
        CHIPDLib*   pPDLib,
        PDHwConfig* pHWConfig);

    CAFIOUtil(const CAFIOUtil&) = delete;               ///< Do not implement copy constructor
    CAFIOUtil& operator=(const CAFIOUtil&) = delete;    ///< Do not implement assignment operator

    ParsedBFStatsOutput*         m_pBFStatsOutput;       ///< BFStats Output

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpAFStats
    ///
    /// @brief  outputs AF stats to the logs
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpAFStats();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpFrameInfo
    ///
    /// @brief  outputs frame info to the logs
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpFrameInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpHWPDConfig
    ///
    /// @brief  outputs HWPD Config info to the logs
    ///
    /// @param  pPDHwConfig   Pointer to PDHW config
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpHWPDConfig(
        PDHwConfig* pPDHwConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLCREnableFromTuning
    ///
    /// @brief  Get LCR enable info during start session.
    ///
    /// @param  pPDLib      Pointer to the pointer of the created PD library instance
    /// @param  pLCREnable  Pointer to boolean for LCR enable/disable
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetLCREnableFromTuning(
        CHIPDLib*   pPDLib,
        BOOL*       pLCREnable);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPDPort
    ///
    /// @brief  check if port is PD\LCR port
    ///
    /// @param  statsType   stat type
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPDPort(
        ISPStatsType statsType);

    CamX::OSLIBRARYHANDLE      m_hBFStatsHandle;                  ///< handle for BF stats
    CREATEGENERATEBFSTATS      m_pBFStats;                        ///< Pointer to BF stats generate function
};

CAMX_NAMESPACE_END

#endif // CAMXCAFIOUTIL_H
