////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 - 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxaecengine.h
/// @brief The class that defines the AECEngine for AEC.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXAECENGINE_H
#define CAMXAECENGINE_H

#include "chiaecinterface.h"
#include "chiispstatsdefs.h"
#include "chistatsproperty.h"

#include "camxistatsprocessor.h"
#include "camxutils.h"

CAMX_NAMESPACE_BEGIN

static const UINT32  AERegionSize    = 5;    ///< The size of array for the AE region set from HAL
static const UINT32  AEFPSRangeSize  = 2;    ///< The size of array for the AE FPS range set from HAL


/// @brief Defines the state for the AEC state machine
enum class AECState
{
    Inactive,       ///< Initial AEC State
    Manual,         ///< AEC in manual mode, what if semi-half?
    Converging,     ///< AEC in searching
    Converged,      ///< AEC converged
    Flash,          ///< AEC in flash state
    LEDCalibration  ///< AEC Dual LED Calibration & Tuning
};

/// @brief Defines the sub-states for AECState::LEDCalibration state machine. We cycle through these states for
///        each individual measurement point
enum class LEDCalibrationState
{
    Ready,              ///< Ready to configure LEDs and start the next measurement
    Collecting,         ///< Collecting measurements for the next calibration/tuning point
    PartialComplete,    ///< Measurements done for the calibration/tuning point
    Complete            ///< Measurements are complete for the current calibration/tuning point
};

/// @brief Instant AEC modes
enum
{
    InstantAecModeNormal,           ///< AEC Startup mode: Normal Convergence
    InstantAecModeAggressive,       ///< AEC Startup mode: Aggressive Convergence
    InstantAecModeFastConvergence   ///< AEC Startup mode: Fast Convergence
}InstantAecMode;


/// @brief The string name of the type of the AE Engine State. Must be in order of AECState.
#if __GNUC__
static const CHAR* CamxAECEngineStateStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CamxAECEngineStateStrings[] =
#endif // _GNUC__
{
    "Inactive",
    "Manual",
    "Converging",
    "Converged",
    "Flash",
    "LEDCalibration"
};

/// @brief The string name of the type of the LED Calibration State. Must be in order of LEDCalibrationState.
#if __GNUC__
static const CHAR* CamxLEDCalibrationState[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CamxLEDCalibrationState[] =
#endif // _GNUC__
{
    "Ready",
    "Collecting",
    "PartialComplete",
    "Complete"
};

/// LED inline calibration file
static const CHAR pLEDInlineCalibrationFilename[] = "LEDInlineCalibrationData.bin";

/// @brief The string name of the type of the PreFlash State. Must be in order of PreFlashState.
#if __GNUC__
static const CHAR* CamxAECPreFlashStateStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CamxAECPreFlashStateStrings[] =
#endif // _GNUC__
{
    "Inactive",
    "Start",
    "TriggerFD",
    "TriggerAF",
    "TriggerAWB",
    "CompleteLED",
    "CompleteNoLED",
    "RER"
};

/// @brief The string name of the type of the AEC Engine Commands. Must be in order of PreFlashState.
#if __GNUC__
static const CHAR* CamxAECEngineCommandStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CamxAECEngineCommandStrings[] =
#endif // _GNUC__
{
    "StartDriver",                  ///< Start the AEC Driver
    "StopDriver",                   ///< Stop the AEC Driver
    "ConfigDriver",                 ///< Configure the AEC driver's run mode, like if we are starting in
    "SetChromatix",                 ///< Set the chromatix tuning data to algorithm
    "StartStreaming",               ///< Set the AEC pipeline in streaming mode, return the start exposure params
    "SetPerFrameControlParam",      ///< Passed all the frame settings like ISO, ROI, etc
    "SetSettingMgr",                ///< For handling all AEC settings from setting manager, to enable/disable features etc
    "ConfigAECTestMode",            ///< Config the AEC algorithm run in specified test mode
    "ProcessStats",                 ///< Process the bayer/histogram stats
    "ProcessGYROStats",             ///< Process the gyro stats
    "SetNodesUpdate",               ///< Set the update from internal nodes, such as AF, AWB, ASD, AFD, Sensor, ISP, etc
    "ProcessHardwareInfo",          ///< Process the hardwaare information
    "ProcessCropWindow",            ///< Process the stats crop window
    "GetVendorTagFromAlgo",         ///< Get the Vendor Tag info from Algo
    "GetPubVendorTagFromAlgo",      ///< Get the Vendor Tag info from Algo
    "SetPipelineDelay",             ///< Set the system latency
    "GetDefaultValues",             ///< Get Default Values from the algo
    "GetLEDCalibrationConfig",      ///< Get LED Calibration config
    "LoadLEDCalibrationData",       ///< Load LED Calibration config
    "LoadLEDInlineCalibrationData", ///< Load LED Inline Calibration config
    "SetDCCalibrationData",         ///< Set Dual Camera Calibration Data
    "SetCameraInformation",         ///< Set Camera information
    "SetFPSRange",                  ///< Set FPS Range
    "AECCommandMax"                 ///< Max AEC command
};

/// @brief Defines the AEC test modes
enum class AECTestMode
{
    AECScan,            ///< Run AEC in scan mode
    DualLEDCalibration  ///< Run AEC for dual LED calibration
};

/// @brief Defines the command supported in AEC Engine. AECCommand is used as the interface to caller. All of the operations
/// needed in node side will be done through AECCommand, such as ProcessStats.
/// Any change in below enum list, there is a need to update at CamxAECEngineCommandStrings
enum class AECCommand
{
    StartDriver,                    ///< Start the AEC Driver
                                    ///  Payload: N/A
    StopDriver,                     ///< Stop the AEC Driver
                                    ///  Payload: N/A
    ConfigDriver,                   ///< Configure the AEC driver's run mode, like if we are starting in
                                    ///  (PREVIEW_NORMAL, PREVIEW_FAST_AEC/INSTANT_AEC, PREVIEW_VIDEO etc)
                                    ///  Payload: N/A
    SetChromatix,                   ///< Set the chromatix tuning data to algorithm
                                    ///  Payload: TBD
    StartStreaming,                 ///< Set the AEC pipeline in streaming mode, return the start exposure params
                                    ///  Payload: input:  AECEngineOperationMode
                                    ///           output: AECEngineFrameControl
    SetPerFrameControlParam,        ///< Passed all the frame settings like ISO, ROI, etc
                                    ///  Payload: AECEnginePerFrameParam
    SetSettingMgr,                  ///< For handling all AEC settings from setting manager, to enable/disable features etc
                                    ///  Payload: SettingManager(TBD)
    ConfigAECTestMode,              ///< Config the AEC algorithm run in specified test mode
                                    ///  Payload: AECTestMode
    ProcessStats,                   ///< Process the bayer/histogram stats
                                    ///  Payload: AECAlgoInputs
    ProcessGYROStats,               ///< Process the gyro stats
                                    ///  Payload: TBD
    SetNodesUpdate,                 ///< Set the update from internal nodes, such as AF, AWB, ASD, AFD, Sensor, ISP, etc
    ProcessHardwareInfo,            ///< Process the hardwaare information
                                    ///  Payload: input: AECEngineHWInfo
    ProcessCropWindow,              ///< Process the stats crop window
                                    ///  input: StatsRectangle
                                    ///  Payload: TBD
    GetVendorTagFromAlgo,           ///< Get the Vendor Tag info from Algo
                                    ///  Payload: TBD
    GetPubVendorTagFromAlgo,        ///< Get the Vendor Tag info from Algo
                                    ///  Payload: TBD
    SetPipelineDelay,               ///< Set the system latency
                                    ///  Payload: UINT8
    GetDefaultValues,               ///< Get Default Values from the algo
                                    ///  Payload: None
    GetLEDCalibrationConfig,        ///< Get LED Calibration config
                                    ///  Payload: None
    LoadLEDCalibrationData,         ///< Load LED Calibration config
                                    ///  Payload: None
    LoadLEDInlineCalibrationData,   ///< Load dynamic inline LED Calibration config
                                    ///  Payload: None
    SetDCCalibrationData,           ///< Set Dual Camera Calibration Data
                                    ///  Payload: AECDCCalibrationInfo
    SetCameraInformation,           ///< Set Camera information
                                    ///  Payload: m_cameraInfo
    SetFPSRange,                    ///< Set FPSRange to algorithm
                                    ///  Paylod: AECAlgoInputs
    AECCommandMax                   ///< Anchor to indicate the last item in the defines

};

/// @brief Defines the manual mode for aec control
enum class ISOExposureTimePriorityMode
{
    DisablePriority = -1,       ///< Disable ISO/ExpTime Priority
    ISOPriority,                ///< ISO priority
    ExposureTimePriority,       ///< Exposure time Priority
    GainPriority,                ///< Gain priority
};

/// @brief Defines the ISO modes
enum class ISOMode
{
    ISOModeAuto,                ///< ISO mode Auto
    ISOModeDeblur,              ///< ISO mode Deblur
    ISOMode100,                 ///< ISO value 100
    ISOMode200,                 ///< ISO value 200
    ISOMode400,                 ///< ISO value 400
    ISOMode800,                 ///< ISO value 800
    ISOMode1600,                ///< ISO value 1600
    ISOMode3200,                ///< ISO value 3200
    ISOModeAbsolute             ///< Absolute ISO Value
};

/// @brief Defines the HDR types
enum class HDRTypeValues
{
    HDRDisabled,               ///< HDR mode disabled
    ISPHDR,                    ///< HDR mode synthesized by ISP
    SensorHDR,                 ///< HDR mode synthesized by sensor
};

/// @brief Defines the input for processing the stats
struct AECEngineAlgorithmInput
{
    UINT64              requestId;                      ///< The request id for the statistics
    UINT64              requestIdOffsetFromLastFlush;   ///< The number of request since last flash
    StatsBayerGrid      bayerGrid;                      ///< The Bayer Grid statistics data
    StatsBayerHist      bayerHist;                      ///< The Bayer Histogram statistics data
    StatsBayerHist      hdrBHist;                       ///< The HDR Bayer Histogram statistics data
    HDR3ExposureStats   HDR3ExposureStatsData;          ///< The 3-exposure HDR statistics data from sensor RDI
    StatsVendorTagList  vendorTagInputList;             ///< The Vendor Tag statistics data
    StatsDataPointer    debugData;                      ///< The debug data pointer, if present
    ChiStatsSession     statsSession;                   ///< Holds stats session data used for handling vendor tag operations
    StatsCameraInfo     cameraInfo;                     ///< Holds camera information
    BOOL                bIsTorchAEC;                    ///< Torch mode flag
    StatsRectangle      bayerGridROI;                   ///< The region where the bayer grid statistics data is provided
    StatsIHist          imgHist;                        ///< The image Histogram Statistics data.
};

/// @brief Defines the information which are output from the algorithm, and those information are used to control the frame
struct AECEngineFrameControl
{
    AECAlgoFrameControl                 frameControl;                       ///< The frame control information from algorithm
    AECAlgoFrameControl                 mainFlashFrameControl;              ///< Main flash frame control information
                                                                            ///  from algorithm, required for AWB internal
                                                                            ///  when AWB starts
    AECAlgoAPEXData                     apexData;                           ///< The APEX data output from the algorithm
    UINT32                              LEDCurrents[StatisticsMaxNumOfLED]; ///< The LED currents for LED snapshot
    AECAlgoExposureData                 exposureData[AECAlgoExposureCount]; ///< The exposure parameter to control the next
                                                                            ///  frame's exposure
    StatsBayerGridBayerExposureConfig   statsConfig;                        ///< Future frame AEC configuration; consumed by ISP
    StatsBayerHistogramConfig           statsBHISTConfig;                   ///< Future frame AEC BHist configuration;
                                                                            /// consumed by ISP
    AECAlgoAdditionalInfo               engineAdditionalControl;            ///< The additional control information output
                                                                            ///  from algorithm after processing the stats
};

/// @brief Defines the output for processing the stats
struct AECEngineAlgorithmOutput
{
    AECAlgoFrameInfo        frameInfo;                  ///< The frame information output from algorithm
                                                        ///  after processing the stats
    AECEngineFrameControl   engineFrameControl;         ///< The frame control information output from algorithm
                                                        ///  after processing the stats
                                                        ///  This carries output only for Preview and Preflash
    VOID*                   pPeerInfo;                  ///< Peer Information
};

/// @brief Contains information for flash measurement (calibration & tuning)
struct AECEngineFlashMeasurement
{
    AECAlgoFlashMeasurementConfig   config;             ///< Dual LED calibration & tuning configuration
    AECAlgoFlashMeasKindType        mode;               ///< Dual LED calibration & tuning mode
    AECAlgoFlashMeasurementResult   result;             ///< Measurement result; this can be updated per-frame
    INT32                           completedCount;     ///< Count of the completed measurements. This is initialized to 0
    UINT32 *                        pLEDCurTable;       ///< Array of LED currents
    VOID *                          pDynamicData;       ///< Dynamic Inline Calibration Data
    UINT32                          dynamicDataSize;    ///< size of dynamic data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Defines the metadata inputs from HAL, needed to be used by AEC
/// following metadata from HAL will be defined
///   ControlAEExposureCompensation,    INT32
///   ControlAELock,                    BYTE
///   ControlAEMode,                    BYTE
///   AEMeteringMode,                   INT32
///   AEBracketMode,                    BYTE
///   touchROI,                         UINT32 x 2
///   ControlAETargetFpsRange,          INT32 x 2
///   ControlAEPrecaptureTrigger,       BYTE
///   ControlAFTrigger,                 BYTE
///   ControlCaptureIntent,             BYTE
///   ControlMode,                      BYTE
///   controlSceneMode                  BYTE
///   FlashMode,                        BYTE
///   SensorExposureTime,               INT64
///   SensorSensitivity,                INT32
///   ControlAEAntibandingModeValues,   BYTE
///   ISOExpTimePriorityMode,           INT32
///   ISOorExposureTimePriorityValue,   INT64
///   ISOValue,                         INT32
///   faceROI                           UINT32 x 13 + UINT64
///   sensorFlashState,                 BYTE
///   pPeerInfo,                        UINT32
///   videoHDRType,                     INT32
///   controlZslEnable,                 INT32
///   frameDuration,                    UINT64
///   controlPostRawSensitivityBoost,   INT32
///   gain,                             FLOAT
///   customExpTable,                   BOOL + UINT32 + 2 x FLOAT + ( 2 x FLOAT + UINT64 + INT32 ) x MaxTableKnees
///   customMeteringTable,              FLOAT x MaxMeteringTableSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct AECEngineHALParam
{
    INT32                            AECompensation;                     ///< The AEC exposure compensation setting
    ControlAELockValues              AELock;                             ///< The AEC lock setting
    ControlAEModeValues              AEMode;                             ///< The AEC mode setting
    INT32                            AEMeteringMode;                     ///< The AEC metering Mode setting
    BYTE                             AEBracketMode;                      ///< The AE Bracket Mode setting
    AECAlgoROI                       touchROI;                           ///< The AEC touch ROI setting
    RangeINT32                       FPSRange;                           ///< The AEC running FPS range setting
    ControlAEPrecaptureTriggerValues AETrigger;                          ///< The AEC trigger value
    ControlAFTriggerValues           AFTrigger;                          ///< The AF trigger value, needed to monitor this
                                                                         ///  value for LED AF case
    ControlCaptureIntentValues       captureIntent;                      ///< The capture intent value on each request
    ControlModeValues                controlMode;                        ///< The master control mode for 3A mode
    ControlSceneModeValues           controlSceneMode;                   ///< The scene control mode for 3A mode
    FlashModeValues                  flashMode;                          ///< The AEC flash running mode.
    INT64                            exposureTime;                       ///< The exposure time in nanoseconds when used in
                                                                         ///  manual mode
    INT32                            sensitivity;                        ///< The ISO value used in manual mode
    ControlAEAntibandingModeValues   AEAntibandingModeValue;             ///< The Antibanding mode
    ControlAEAntibandingModeValues   flickerMode;                        ///< The detected/Applied flicker mode
    ISOExposureTimePriorityMode      ISOExposureTimePriortyMode;         ///< ISO priority/Exp time indication from HAL
    INT64                            ISOorExposureTimePriorityValue;     ///< Value of ISO/Exposure Time from HAL
    INT32                            ISOValue;                           ///< Absolute ISO Value
    FaceROIInformation               faceROI;                            ///< Detected Face ROIs
    FlashStateValues                 sensorFlashState;                   ///< Value of sensor flash firing status
    VOID*                            pPeerInfo;                          ///< Peer Algorithm Information
    HDRTypeValues                    videoHDRType;                       ///< Video HDR type
    INT32                            controlZslEnable;                   ///< ZSL Enable flag
    UINT64                           frameDuration;                      ///< Frame Duration
    INT32                            controlPostRawSensitivityBoost;     ///< Raw sensitivity boost control
    FLOAT                            gain;                               ///< Absolute Gain Value
    AECCustomExposureTable           customExposureTable;                ///< Custom exposure table for vendor tag
    AECCustomMeteringTable           customMeteringTable;                ///< Custom metering table for vendor tag
    BOOL                             skipPreflashTriggers;               ///< Skip preflash triggers for reqId < PipelineDelay
    BOOL                             disableADRC;                        ///< flag to disable ADRC
    FLOAT                            convergenceSpeed;                   ///< dynamic convergence speed
    FLOAT                            warmStartSensitivity[AECAlgoExposureCount];    ///< warm start Short exposure sensitivity
    TrackerROIInformation            trackerROI;                         ///< The AEC tracker ROI setting
    INT32                            instantAECMode;                     ///< Instant AEC Mode: 0-Normal;1-Aggressive;2-Fast
    UINT64                           frameId;                            ///< frame ID
    SeamlessInSensorState            seamlessInSensorState;              ///< Flag to indicate seamless in-sensor control state
};

/// @brief Defines the updates from other nodes
struct AECEngineNodesUpdate
{
    AFOutput*           pAFOutput;              ///< The AF updates

    AWBFrameControl*    pAWBFrameControl;       ///< The AWB frame control updates
    AWBFrameInfo*       pAWBFrameInfo;          ///< The AWB frame info updates
    AWBOutputInternal*  pAWBOutputInternal;     ///< The AWB output internal updates
    BOOL                isAWBModeAuto;          ///< The AWB control mode
    BOOL                isRERDone;              ///< Flag to indicate completion of RER sequence by Sensor Node
};

/// @brief Defines hardware information to be set to the algorithm
struct AECEngineHWInfo
{
    StatsSensorInfo sensorInfo;                     ///< Sensor information
    StatsCapability statsCapabilities;              ///< Stats capabilities
    BOOL            isDual;                         ///< Is dual camera
    BOOL            isFixedFocus;                   ///< Sensor focus capability flag
};

/// @brief Defines initial startup exposure mode and setting to be set to the algorithm
struct AECEngineInitStreamConfig
{
    AECAlgoOperationModeType streamingType;                     ///< The algorithm streaming type
    FLOAT                    sensitivity[AECAlgoExposureCount]; ///< Initial sensitivity information
};

/// @brief  Defines the input param for AECCommand, it's a union, for each AECCommand, it shall define it's input payload
///         For any non-primitive type added to this union has to be pointer
union AECCommandInputParam
{
    StatsTuningData*            pTuningData;        ///< Pointer to the tuning data to pass into the algorithm
    StatsStreamInitConfig*      pInitStreamConfig;  ///< The algorithm initial streaming config
    AECEngineHALParam*          pHALParam;          ///< The per request's HAL parameters
    AECEngineAlgorithmInput*    pAlgorithmInput;    ///< The input for algorithm to process the statistics
    AECEngineNodesUpdate*       pNodesUpdate;       ///< The updates from other nodes
    AECEngineHWInfo*            pHardwareInfo;      ///< Hardware information
    StatsRectangle*             pCropWindow;        ///< Stats crop window
    UINT8                       systemLatency;      ///< System Latency
    AECAlgoDCCalibrationInfo    dcCalibrationInfo;  ///< Dual Camera OTP Calibration data coming from sensor
    StatsGyroInfo               gyroInfo;           ///< Gyro information for AEC
};

/// @brief Defines the output from processing the stats, it include output from algorithm as well as states of the AEC engine
struct AECEngineProcessStatsOutput
{
    // output populated by AEC Algorithm
    AECEngineAlgorithmOutput    algorithmOutput;        ///< the output from algorithm

    // the state for HAL
    ControlAEModeValues         AEMode;                 ///< AEC mode
    WeightedRegion              AERegion;               ///< AEC touch region
    ControlAEStateValues        AEState;                ///< AEC state
    ControlModeValues           controlMode;            ///< 3A control mode
    INT32                       AECompensation;         ///< Current exposure compensation index
    AECAlgoEVCapabilities       AECEVCapabilities;      ///< AEC Exposure Compensation Capabilities
    PrecapTriggers              AEPrecapTrigger;        ///< Trigger a precapture metering sequence
    ControlAELockValues         AELock;                 ///< The AEC lock setting
    PreFlashState               preFlashState;          ///< The state in preflash case
    RangeINT32                  currentFPSRange;        ///< Curent FPS range
    CalibrationFlashState       calibFlashState;        ///< The state in calibflash case

    // Holds base struct for vendor tag list
    StatsVendorTagList          vendorTagList;          ///< Holds base struct for vendor tag list
};

/// @brief  Defines the output param for AECCommand, it's a union, for each AECCommand, it shall define it's output payload
///         For any non-primitive type added to this union has to be pointer
union AECCommandOutputParam
{
    AECEngineProcessStatsOutput*    pProcessStatsOutput;    ///< The output from algorithm and engine for the following command:
                                                            ///     AECCommand::ProcessStats
    AECEngineFrameControl*          pFrameControl;          ///< For the command to get start/snapshot exposure parameter for:
                                                            ///     AECCommand::StartStreaming
                                                            ///     AECCommand::SnapshotTrigger
    StatsVendorTagInfoList*         pVendorTagInfoList;     ///< vendor tag list information of depending vendor tags
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The utility class that implements utility functions used in CAECEngine
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAECEngineUtility
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyExposureSetToExposureData
    ///
    /// @brief  Copy the AECAlgoExposureSet output from algorithm into AECAlgoExposureData
    ///
    /// @param  pData the AECAlgoExposureData array, will copy data from pSet
    /// @param  size  the size of the pData array
    /// @param  pSet  the AECAlgoExposureSet populated by the algorithm
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CopyExposureSetToExposureData(
        AECAlgoExposureData* pData,
        INT32                size,
        AECAlgoExposureSet*  pSet);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetExposureDataByType
    ///
    /// @brief  get one single entry of AECAlgoExposureData from AECAlgoExposureSet by specified AECAlgoExposureType
    ///
    /// @param  pExposureSet   pointer to AECAlgoExposureSet
    /// @param  type           the type of AECAlgoExposureData to look in AECAlgoExposureSet
    ///
    /// @return the AECAlgoExposureData which is set for AECAlgoExposureType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static AECAlgoExposureData* GetExposureDataByType(
        AECAlgoExposureSet* pExposureSet,
        AECAlgoExposureType type);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeAlgorithmInput
    ///
    /// @brief  Initialize the AECAlgoInput array, set each member's inputType to AECAlgoInputInvalid
    ///
    /// @param  pInputs     The pointer to AECAlgoInput array
    /// @param  size        The element number in the pInputs array
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult InitializeAlgorithmInput(
        AECAlgoInput*   pInputs,
        UINT32          size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeAlgorithmOutput
    ///
    /// @brief  Initialize the AECAlgoOutput array, set each member's outputType to AECAlgoOutputInvalid
    ///
    /// @param  pOutputs    The pointer to AECAlgoOutput array
    /// @param  size        The element number in the pOutputs array
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult InitializeAlgorithmOutput(
        AECAlgoOutput* pOutputs,
        UINT32         size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgorithmInputEntry
    ///
    /// @brief  Set one single input entry to the AECAlgoInput array
    ///
    /// @param  pInputs     The pointer to AECAlgoInput array
    /// @param  inputType   The input type for the value to set
    /// @param  inputSize   The size of the data pointed by pValue
    /// @param  pValue      The actual value set to the pInputs
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetAlgorithmInputEntry(
        AECAlgoInput*    pInputs,
        AECAlgoInputType inputType,
        UINT32           inputSize,
        VOID*            pValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgorithmOutputEntry
    ///
    /// @brief  Set one single input entry to the AECAlgoInput array
    ///
    /// @param  pOutputs    The pointer to AECAlgoInput array
    /// @param  outputType  The input type for the value to set
    /// @param  outputSize  The size of the data pointed by pValue
    /// @param  pValue      The actual value set to the pOutputs
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetAlgorithmOutputEntry(
        AECAlgoOutput*    pOutputs,
        AECAlgoOutputType outputType,
        UINT32            outputSize,
        VOID*             pValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLEDMode
    ///
    /// @brief  Use the input from HAL to determine the LED mode set to algorithm, the function uses the FlashModeValues,
    ///         ControlAEModeValues etc to infer the LED mode to algorithm
    ///
    /// @param  pHALParam   The pointer to AECEngineHALParam which has the inputs from HAL
    ///
    /// @return AECAlgoLEDModeType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static AECAlgoLEDModeType GetLEDMode(
        const AECEngineHALParam* pHALParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAECAlgorithmMode
    ///
    /// @brief  Use the input from HAL to determine the AEC algo mode set to algorithm, the function uses the ControlModeValues
    ///         ControlAEModeValues,etc to infer the AECAlgoModeType mode to algorithm
    ///
    /// @param  pHALParam   The pointer to AECEngineHALParam which has the inputs from HAL
    ///
    /// @return AECAlgoModeType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static AECAlgoModeType GetAECAlgorithmMode(
        AECEngineHALParam*  pHALParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NormalizeROI
    ///
    /// @brief  Normalize the ROI within the range of [0..1]. The algorithm only takes normalized ROI, so all of region sent
    ///         to algorithm has to be normalized.
    ///
    /// @param  pStatsWindowDimension   The full stats windows size, the function normalize the pHALROI based on this window's
    ///                                 dimension
    /// @param  pHALROI                 The ROI from HAL to be normalized
    /// @param  pNormalizedROI          The normalized ROI which has the range of [0..1]
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult NormalizeROI(
        ::StatsDimension* pStatsWindowDimension,
        INT32*            pHALROI,
        AECAlgoROI*       pNormalizedROI);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadSetParamList
    ///
    /// @brief  Set input from AECEngineHALParam to AECAlgoSetParam.
    ///
    /// @param  pSetParam       The AECAlgoSetParam where the function save the AE compensation value to algorithm
    /// @param  pAECSetParam    The pointer to pAECSetParam Value.
    /// @param  type            The type of AEC SetParam
    /// @param  size            The Size of Value of AEC SetParam
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID LoadSetParamList(
        AECAlgoSetParam*    pSetParam,
        VOID*               pAECSetParam,
        AECAlgoSetParamType type,
        SIZE_T              size);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetTrackerROIToSetParamList
    ///
    /// @brief  Set the AE ROI input from AECEngineHALParam to the array of AECAlgoSetParam, the array of
    ///         AECAlgoSetParam will be sent to algorithm
    ///
    /// @param  pSetParam         The array of AECAlgoSetParam where the function save the AE ROI value to algorithm
    /// @param  pHALParam         The pointer to AECEngineHALParam where the function retrieves the AE ROI value
    /// @param  pROIList          The pointer to AECAlgoROIList
    /// @param  pStatsSensorInfo  The pointer to StatsSensorInfo
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetTrackerROIToSetParamList(
        AECAlgoSetParam*               pSetParam,
        AECEngineHALParam*             pHALParam,
        UnstabilizedROIInformation*    pROIList,
        StatsSensorInfo*               pStatsSensorInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetROIToSetParamList
    ///
    /// @brief  Set the AE ROI input from AECEngineHALParam to the array of AECAlgoSetParam, the array of
    ///         AECAlgoSetParam will be sent to algorithm
    ///
    /// @param  pSetParam       The array of AECAlgoSetParam where the function save the AE ROI value to algorithm
    /// @param  pHALParam       The pointer to AECEngineHALParam where the function retrieves the AE ROI value
    /// @param  pROIList        The pointer to AECAlgoROIList
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetROIToSetParamList(
        AECAlgoSetParam*   pSetParam,
        AECEngineHALParam* pHALParam,
        AECAlgoROIList*    pROIList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFaceROIToSetParamList
    ///
    /// @brief  Set the AE ROI input from AECEngineHALParam to the array of AECAlgoSetParam, the array of
    ///         AECAlgoSetParam will be sent to algorithm
    ///
    /// @param  pSetParam         The array of AECAlgoSetParam where the function save the AE ROI value to algorithm
    /// @param  pHALParam         The pointer to AECEngineHALParam where the function retrieves the AE ROI value
    /// @param  pROIList          The pointer to AECAlgoROIList
    /// @param  pStatsSensorInfo  The pointer to StatsSensorInfo
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetFaceROIToSetParamList(
        AECAlgoSetParam*   pSetParam,
        AECEngineHALParam* pHALParam,
        AECAlgoROIList*    pROIList,
        StatsSensorInfo*   pStatsSensorInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUniformFaceROIToSetParamList
    ///
    /// @brief  Set the AE ROI input from AECEngineHALParam to the array of AECAlgoSetParam, the array of
    ///         AECAlgoSetParam will be sent to algorithm
    ///
    /// @param  pSetParam                         The array of AECAlgoSetParam where the function save the AE ROI value to
    ///                                           algorithm
    /// @param  pHALParam                         The pointer to AECEngineHALParam where the function retrieves the AE ROI
    ///                                           value
    /// @param  pUnstabilizedFaceROIInformation   The pointer to UnstabilizedROIInformation
    /// @param  pStatsSensorInfo                  The pointer to pStatsSensorInfo
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetUniformFaceROIToSetParamList(
        AECAlgoSetParam*            pSetParam,
        AECEngineHALParam*          pHALParam,
        UnstabilizedROIInformation* pUnstabilizedFaceROIInformation,
        StatsSensorInfo*            pStatsSensorInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFPSRangeToSetParamList
    ///
    /// @brief  Set the AE FPS range input from AECEngineHALParam to the array of AECAlgoSetParam, the array of
    ///         AECAlgoSetParam will be sent to algorithm
    ///
    /// @param  pHALParam   The pointer to AECEngineHALParam where the function retrieves the AE FPS range value
    /// @param  index       The index of AECAlgoSetParam to save the value
    /// @param  pSetParam   The array of AECAlgoSetParam where the function save the AE FPS range value to algorithm
    /// @param  pFPSRange   The pointer to AECAlgoFPSRange to save the AE FPS value
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetFPSRangeToSetParamList(
        const AECEngineHALParam* pHALParam,
        const INT32              index,
        AECAlgoSetParam*         pSetParam,
        AECAlgoFPSRange*         pFPSRange);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLEDModeToSetParamList
    ///
    /// @brief  Set the AE LED mode from AECEngineHALParam to the array of AECAlgoSetParam, the array of
    ///         AECAlgoSetParam will be sent to algorithm
    ///
    /// @param  pHALParam   The pointer to AECEngineHALParam where the function retrieves the AE LED mode
    /// @param  index       The index of AECAlgoSetParam to save the value
    /// @param  pSetParam   The array of AECAlgoSetParam where the function save the AE LED mode value to algorithm
    /// @param  pLEDMode    The pointer to AECAlgoLEDModeType to save the AE LED mode value
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetLEDModeToSetParamList(
        const AECEngineHALParam* pHALParam,
        const INT32              index,
        AECAlgoSetParam*         pSetParam,
        AECAlgoLEDModeType*      pLEDMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetManualSettingToSetParamList
    ///
    /// @brief  Set the AE manual setting input from AECEngineHALParam to the array of AECAlgoSetParam, the array of
    ///         AECAlgoSetParam will be sent to algorithm
    ///
    /// @param  pHALParam   The pointer to AECEngineHALParam where the function retrieves the AE manual setting value
    /// @param  index       The index of AECAlgoSetParam to save the value
    /// @param  pSetParam   The array of AECAlgoSetParam where the function save the AE manual setting value to algorithm
    /// @param  pManual     The pointer to AECAlgoManualSetting to save the AE manual setting
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetManualSettingToSetParamList(
        AECEngineHALParam* pHALParam,
        const INT32              index,
        AECAlgoSetParam*         pSetParam,
        AECAlgoManualSetting*    pManual);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLockToSetParamList
    ///
    /// @brief  Set the AE lock setting input from AECEngineHALParam to the array of AECAlgoSetParam, the array of
    ///         AECAlgoSetParam will be sent to algorithm
    ///
    /// @param  pHALParam   The pointer to AECEngineHALParam where the function retrieves the AE manual setting value
    /// @param  index       The index of AECAlgoSetParam to save the value
    /// @param  pSetParam   The array of AECAlgoSetParam where the function save the AE manual setting value to algorithm
    /// @param  pLock       The pointer to AECAlgoLockType to save the AE lock setting
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult   SetLockToSetParamList(
        const AECEngineHALParam* pHALParam,
        const INT32              index,
        AECAlgoSetParam*         pSetParam,
        AECAlgoLockType*         pLock);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAECAlgoDiagConfig
    ///
    /// @brief  Set the AE Diagnostic config setting input from Static settings to the array of AECAlgoSetParam, the array of
    ///         AECAlgoSetParam will be sent to algorithm
    ///
    /// @param  pDiagConfig The pointer to AECAlgoDiagConfig where the function retrieves the AE manual setting value
    /// @param  pSetParam   The array of AECAlgoSetParam where the function save the AE manual setting value to algorithm
    /// @param  state       The flag indicating aec states.
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SetAECAlgoDiagConfig(
        AECAlgoDiagConfig*       pDiagConfig,
        AECAlgoSetParam*         pSetParam,
        AECState                 state);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The class that implements CAECEngine
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAECEngine
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create CAECEngine object
    ///
    /// @param  pfnCreate     Function Pointer to create Algorithm instance
    /// @param  pStatsSession Pointer to Stats session
    /// @param  pCameraInfo   Pointer to camera information
    ///
    /// @return Created CAECEngine pointer if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAECEngine* Create(
        CREATEAEC        pfnCreate,
        ChiStatsSession* pStatsSession,
        StatsCameraInfo* pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Method to delete an instance of CAECEngine
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Uninitialize
    ///
    /// @brief  This method uninitializes the stats core library.
    ///
    /// @param  overrideCameraClose Actual camera close
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Uninitialize(
        UINT overrideCameraClose);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleCommand
    ///
    /// @brief  This is the main method used by caller to drive the AECEngine. Caller sets the command, input and output.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult HandleCommand(
        AECCommand             command,
        AECCommandInputParam*  pInput,
        AECCommandOutputParam* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAECState
    ///
    /// @brief  Get the AECState
    ///
    /// @return The current value of AECState
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AECState GetAECState();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLEDCalibrationState
    ///
    /// @brief  Get the LEDCalibrationState
    ///
    /// @return The current value of LEDCalibrationState
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    LEDCalibrationState GetLEDCalibrationState();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAECPreFlashState
    ///
    /// @brief  Get the AECPreFlashState
    ///
    /// @return The current value of AECPreFlashState
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PreFlashState GetAECPreFlashState();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsZSLDisabled
    ///
    /// @brief  Return ZSL status (enable/disable)
    ///
    /// @return TRUE if ZSL is disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsZSLDisabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsQuerySnapshotExposure
    ///
    /// @brief  Return a boolean which indicate if querySnapshotExposure is needed or not
    ///
    /// @return TRUE if querySnapshotExposure is needed, false otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsQuerySnapshotExposure();

    AECEngineHALParam     m_HALParam;                     ///< save the HAL parameter, used to report back the state
    StatsStreamInitConfig m_statsStreamInitConfig;        ///< Save initial configuration settings for stats
    BOOL                  m_warmStartDone;                ///< Flag to indicate if warm start settings has been used.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFlashInfluencedStats
    ///
    /// @brief   sets the flag to indicate whether the stats for snapshot frame is influenced by flash
    ///
    /// @param   isFlashInfluencedStats                sets the flag to TRUE/FALSE
    ///
    /// @return  None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline VOID SetFlashInfluencedStats(
        BOOL isFlashInfluencedStats)
    {
        m_isFlashInfluencedStats = isFlashInfluencedStats;
    }

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CAECEngine
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAECEngine();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CAECEngine
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~CAECEngine();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  This method initializes the stats core library.
    ///
    /// @param  pfnCreate     Function Pointer to create Algorithm instance
    /// @param  pStatsSession Pointer to Stats Session.
    /// @param  pCameraInfo   Pointer to camera information
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        CREATEAEC        pfnCreate,
        ChiStatsSession* pStatsSession,
        StatsCameraInfo* pCameraInfo);

    CAECEngine(const CAECEngine&)            = delete;   ///< Do not implement copy constructor
    CAECEngine& operator=(const CAECEngine&) = delete;   ///< Do not implement assignment operator

    ///< Defines the trigger for the flash state machine
    enum class FlashTrigger
    {
        Invalid,        ///< No trigger for flash state machine
        AE,             ///< AE triggers for the flash state machine
        LEDAF           ///< LEDAF triggers for the flash state machine
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachine
    ///
    /// @brief      This is the entry to change the AEC state machine. The trigger for state machine change is either from
    ///             pInput or pOutput.
    ///
    /// @param      command Refer to AECCommand for the supported command
    /// @param      pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param      pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @remarks    Transition chart
    ///             INACTIVE
    ///             START STREAMING -> CONVERGING   INPUT
    ///
    ///             MANUAL
    ///             TO AUTO -> CONVERGING   INPUT
    ///             STOP    -> INACTIVE     INPUT
    ///
    ///             LOCK
    ///             TO AUTO -> CONVERGING   INPUT/ALGO
    ///             STOP    -> INACTIVE     INPUT
    ///
    ///             CONVERGING:
    ///             SETTLED -> CONVERGED                                ALGO
    ///             MANUAL  -> MANUAL                                   INPUT
    ///             LOCK    -> LOCK                                     INPUT/ALGO
    ///             TRIGGER -> FLASH(LOW LIGHT)                         INPUT
    ///                     -> TRIGGER SETTLED(no LED needed, no ops)   INPUT
    ///             STOP    -> INACTIVE                                 INPUT
    ///
    ///             CONVERGED
    ///             SEARCHING   -> CONVERGING               ALGO
    ///             MANUAL      -> MANUAL                   INPUT
    ///             LOCK        -> LOCK                     INPUT/ALGO
    ///             TRIGGER     -> FLASH (LOW LIGHT)        INPUT
    ///                         -> NO OPS (NO LED NEEDED)
    ///             STOP        -> INACTIVE INPUT           INPUT
    ///
    ///             INFLASH
    ///             STOP        -> CANCEL
    ///
    ///             FLASH_INACTIVE
    ///             TRIGGER     -> START        INPUT
    ///
    ///             FLASH_START
    ///             CANCEL          -> FLASH_COMPLETED_NO_LED   INPUT
    ///             ALGO STARTED    -> FLASH_CONVERGING         ALGO
    ///
    ///             FLASH_CONVERGING
    ///             CANCEL          -> FLASH_COMPLETED_NO_LED   INPUT
    ///             CONVERGED       -> DO FD/AF                 ALGO
    ///
    ///             DO_FD/DO_AF
    ///             CANCEL          -> FLASH_COMPLETED_NO_LED   INPUT
    ///             COMPLETED       -> FLASH_COMPLETE_LED       AEC ALGO/AF ALGO
    ///
    ///             FLASH_COMPLETED_NO_LED/FLASH_COMPLETE_LED
    ///                             -> FLASH_INACTIVE
    ///
    /// @return     CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AECStateMachine(
        AECCommand             command,
        AECCommandInputParam*  pInput,
        AECCommandOutputParam* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineInactive
    ///
    /// @brief  This is the entry to change the AEC state machine when AEC is in Inactive state.
    ///         The trigger for state machine change is either from pInput or pOutput.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AECStateMachineInactive(
        AECCommand             command,
        AECCommandInputParam*  pInput,
        AECCommandOutputParam* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineManual
    ///
    /// @brief  This is the entry to change the AEC state machine when AEC is in Manual state.
    ///         The trigger for state machine change is either from pInput or pOutput.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AECStateMachineManual(
        AECCommand             command,
        AECCommandInputParam*  pInput,
        AECCommandOutputParam* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineConvergedAndConverging
    ///
    /// @brief  This is the entry to change the AEC state machine when AEC is in Converged/Converging state.
    ///         The trigger for state machine change is either from pInput or pOutput.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  AECStateMachineConvergedAndConverging(
        AECCommand              command,
        AECCommandInputParam*   pInput,
        AECCommandOutputParam*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineFlash
    ///
    /// @brief      This is the entry to change the AEC state machine when AEC is in Flash state.
    ///             The trigger for state machine change is either from pInput or pOutput.
    ///
    /// @param      command Refer to AECCommand for the supported command
    /// @param      pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param      pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return     CamxResultSuccess upon success.
    ///
    /// @remarks    Transition chart
    ///             PreFlashState::Inactive
    ///             AE/AF trigger -> PreFlashState::Start   INPUT
    ///
    ///             PreFlashState::Start
    ///             AE/AF TRIGGER CANCEL    -> PreFlashState::CompleteNoLED                   INPUT
    ///             AF TRIGGER              -> Update flash mode to LED AF                    INPUT
    ///             AEC SETTLED             -> PreFlashState::TriggerFD/TriggerAF/TriggerAWB  AEC ALGO
    ///             STOP DRIVER             -> PreFlashState::Inactive, AECState::Inactive    INPUT
    ///
    ///             PreFlashState::TriggerFD
    ///             AE/AF TRIGGER CANCEL    -> PreFlashState::CompleteNoLED                 INPUT
    ///             AF TRIGGER              -> Update flash mode to LED AF                  INPUT
    ///             AEC FD DONE             -> PreFlashState::TriggerAF/TriggerAWB          AEC ALGO
    ///             STOP DRIVER             -> PreFlashState::Inactive, AECState::Inactive  INPUT
    ///
    ///             PreFlashState::TriggerAF
    ///             AE/AF TRIGGER CANCEL    -> PreFlashState::CompleteNoLED                 INPUT
    ///             AF DONE                 -> PreFlashState::TriggerAWB                    AF ALGO
    ///             STOP DRIVER             -> PreFlashState::Inactive, AECState::Inactive  INPUT
    ///
    ///             PreFlashState::TriggerAWB
    ///             AE/AF TRIGGER CANCEL    -> PreFlashState::CompleteNoLED                 INPUT
    ///             AWB DONE                -> PreFlashState::CompleteLED                   AWB ALGO
    ///             STOP DRIVER             -> PreFlashState::Inactive, AECState::Inactive  INPUT
    ///
    ///             PreFlashState::CompleteLED
    ///             AE/AF TRIGGER CANCEL    -> PreFlashState::CompleteNoLED                 INPUT
    ///             SNAPSHOT REQUEST        -> PreFlashState::Inactive                      INPUT
    ///             STOP DRIVER             -> PreFlashState::Inactive, AECState::Inactive  INPUT
    ///
    ///             PreFlashState::CompleteNoLED
    ///             ON NEXT STATS(TBD)      -> PreFlashState::Inactive                      INPUT
    ///             STOP DRIVER             -> PreFlashState::Inactive, AECState::Inactive  INPUT
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  AECStateMachineFlash(
        AECCommand              command,
        AECCommandInputParam*   pInput,
        AECCommandOutputParam*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineLEDCalibration
    ///
    /// @brief      This is the entry to change the AEC state machine when AEC is in LEDCalibration state.
    ///             The trigger for state machine change is either from pInput or pOutput.
    ///
    /// @param      command Refer to AECCommand for the supported command
    /// @param      pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param      pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return     CamxResultSuccess upon success.
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  AECStateMachineLEDCalibration(
        AECCommand              command,
        AECCommandInputParam*   pInput,
        AECCommandOutputParam*  pOutput);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineFlashStart
    ///
    /// @brief  This is the entry to change the AEC state machine when AEC is in AECState::Flash state, and the
    ///         AECPreFlashState is PreFlashState::Start. The trigger for state machine change is either from pInput or pOutput.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  AECStateMachineFlashStart(
        AECCommand              command,
        AECCommandInputParam*   pInput,
        AECCommandOutputParam*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineFlashTriggerFD
    ///
    /// @brief  This is the entry to change the AEC state machine when AEC is in AECState::Flash state, and the
    ///         AECPreFlashState is PreFlashState::TriggerFD. The trigger for state machine change is either from pInput or
    ///         pOutput.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  AECStateMachineFlashTriggerFD(
        AECCommand              command,
        AECCommandInputParam*   pInput,
        AECCommandOutputParam*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineFlashTriggerAF
    ///
    /// @brief  This is the entry to change the AEC state machine when AEC is in AECState::Flash state, and the
    ///         AECPreFlashState is PreFlashState::TriggerAF. The trigger for state machine change is either from pInput or
    ///         pOutput.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  AECStateMachineFlashTriggerAF(
        AECCommand              command,
        AECCommandInputParam*   pInput,
        AECCommandOutputParam*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineFlashTriggerAWB
    ///
    /// @brief  This is the entry to change the AEC state machine when AEC is in AECState::Flash state, and the
    ///         AECPreFlashState is PreFlashState::TriggerAWB. The trigger for state machine change is either from pInput or
    ///         pOutput.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  AECStateMachineFlashTriggerAWB(
        AECCommand              command,
        AECCommandInputParam*   pInput,
        AECCommandOutputParam*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineFlashCompleteLED
    ///
    /// @brief  This is the entry to change the AEC state machine when AEC is in AECState::Flash state, and the
    ///         AECPreFlashState is PreFlashState::CompleteLED. The trigger for state machine change is either from pInput or
    ///         pOutput.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  AECStateMachineFlashCompleteLED(
        AECCommand              command,
        AECCommandInputParam*   pInput,
        AECCommandOutputParam*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineFlashCompleteNoLED
    ///
    /// @brief  This is the entry to change the AEC state machine when AEC is in AECState::Flash state, and the
    ///         AECPreFlashState is PreFlashState::CompleteNoLED. The trigger for state machine change is either from pInput or
    ///         pOutput.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  AECStateMachineFlashCompleteNoLED(
        AECCommand              command,
        AECCommandInputParam*   pInput,
        AECCommandOutputParam*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AECStateMachineFlashRER
    ///
    /// @brief  This is the entry to change the AEC state machine when AEC is in AECState::Flash state, and the
    ///         AECPreFlashState is PreFlashState::RER. The trigger for state machine change is either from pInput or
    ///         pOutput.
    ///
    /// @param  command Refer to AECCommand for the supported command
    /// @param  pInput  Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pOutput Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  AECStateMachineFlashRER(
        AECCommand              command,
        AECCommandInputParam*   pInput,
        AECCommandOutputParam*  pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessStats
    ///
    /// @brief  This is the entry run the AECCommand ProcessStats.
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessStats(
        AECCommandInputParam*  pCommandInput,
        AECCommandOutputParam* pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessStatsAlgo
    ///
    /// @brief  Process the stats with algorithm
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return TRUE if process stats returns success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  ProcessStatsAlgo(
        AECCommandInputParam*   pCommandInput,
        AECCommandOutputParam*  pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessStatsCaptureRequest
    ///
    /// @brief  Process stats with request for still capture
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return TRUE if process stats returns success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  ProcessStatsCaptureRequest(
        AECCommandInputParam*   pCommandInput,
        AECCommandOutputParam*  pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StartDriver
    ///
    /// @brief  This is the entry run the AECCommand StartDriver.
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  StartDriver(
        AECCommandInputParam*   pCommandInput,
        AECCommandOutputParam*  pCommandOutput);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StopDriver
    ///
    /// @brief  This is the entry run the AECCommand StopDriver.
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  StopDriver(
        AECCommandInputParam*   pCommandInput,
        AECCommandOutputParam*  pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigDriver
    ///
    /// @brief  This is the entry run the AECCommand ConfigDriver.
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  ConfigDriver(
        AECCommandInputParam*   pCommandInput,
        AECCommandOutputParam*  pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OverrideAlgoSetting
    ///
    /// @brief  Read the input setting and override the current settings.
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult OverrideAlgoSetting();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetChromatix
    ///
    /// @brief  This is the entry run the AECCommand SetChromatix.
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  SetChromatix(
        AECCommandInputParam*   pCommandInput,
        AECCommandOutputParam*  pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StartStreaming
    ///
    /// @brief  This is the entry run the AECCommand StartStreaming.
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pFrameControl   The frame control information from algorithm
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  StartStreaming(
        AECCommandInputParam*    pCommandInput,
        AECEngineFrameControl*   pFrameControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPerFrameControlParam
    ///
    /// @brief  This is the entry run the AECCommand SetPerFrameControlParam.
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  SetPerFrameControlParam(
        AECCommandInputParam*   pCommandInput,
        AECCommandOutputParam*  pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetNodesUpdate
    ///
    /// @brief  Sets the nodes update to the algorithm
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetNodesUpdate(
        AECCommandInputParam*  pCommandInput,
        AECCommandOutputParam* pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessHardwareInfo
    ///
    /// @brief  Sets the hardware configuration to the algorithm
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessHardwareInfo(
        AECCommandInputParam*  pCommandInput,
        AECCommandOutputParam* pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessCropWindow
    ///
    /// @brief  Sets the stats crop window to the algorithm
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessCropWindow(
        AECCommandInputParam*  pCommandInput,
        AECCommandOutputParam* pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetParamForVendorTag
    ///
    /// @brief  This function encapsulate the logic of call AECGetParam with return type of StatsVendorTagInfoList
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetParamForVendorTag(
        AECCommandInputParam*  pCommandInput,
        AECCommandOutputParam* pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetParamForPubVendorTag
    ///
    /// @brief  This is the entry run the AECCommand SetPerFrameControlParam.
    ///
    /// @param  pCommandInput   Refer to AECCommand for the AECCommandInputParam payload
    /// @param  pCommandOutput  Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetParamForPubVendorTag(
        AECCommandInputParam*  pCommandInput,
        AECCommandOutputParam* pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSingleParamToAlgorithm
    ///
    /// @brief  This function wraps the logic when only need set one single parameter to algorithm
    ///
    /// @param  paramType   The AECAlgoSetParamType of parameter will be set to algorithm
    /// @param  pParam      The pointer to the parameter will be set
    /// @param  paramSize   The size of the parameter
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  SetSingleParamToAlgorithm(
        AECAlgoSetParamType paramType,
        VOID*               pParam,
        UINT32              paramSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetParamFromAlgorithm
    ///
    /// @brief  Call the algorithm's interface AECGetParam to query data by paramType
    ///
    /// @param  paramType       The type of data to query
    /// @param  pInputList      The input data list associated with the paramType. Refer to AECAlgoGetParamType to get the
    ///                         detail of payload for pInput
    /// @param  pOutputList     The AECAlgoGetParamOutput, each member of this structure shall be initialized. Refer to
    ///                         AECAlgoGetParamType to get the detail of each output type
    /// @param  numberOfOutputs The number of output in AECAlgoGetParamOutput
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  GetParamFromAlgorithm(
        AECAlgoGetParamType       paramType,
        AECAlgoGetParamInputList* pInputList,
        AECAlgoGetParamOutput*    pOutputList,
        UINT32                    numberOfOutputs);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoParamForFrameControlType
    ///
    /// @brief  This function encapsulate the logic of call AECGetParam with return type of AECEngineFrameControl
    ///         such as AECAlgoGetParamSnapshotExposure, AECAlgoGetParamStartExposure
    ///
    /// @param  paramType         The type of data to query
    /// @param  pInputList        Pointer to input data list
    /// @param  pFrameControl     The pointer to AECEngineFrameControl, algorithm will fill data into this structure
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  GetAlgoParamForFrameControlType(
        AECAlgoGetParamType       paramType,
        AECAlgoGetParamInputList* pInputList,
        AECEngineFrameControl*    pFrameControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoParamForMultiCamera
    ///
    /// @brief  This function encapsulate the logic of collecting information from master
    ///
    /// @param  ppPeerInfo  Peer Information
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAlgoParamForMultiCamera(
        VOID** ppPeerInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoParamForEvCapabilities
    ///
    /// @brief  This function encapsulate the logic of collecting information for EV capabilities
    ///
    /// @param  pEvCapabilities  pointer for Ev Capabilities
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAlgoParamForEvCapabilities(
        VOID* pEvCapabilities);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoParamForSnapshotType
    ///
    /// @brief  This function encapsulate the logic of collecting information for snapshot type
    ///
    /// @param  pSnapshotType  pointer for snapshot type
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAlgoParamForSnapshotType(
        VOID* pSnapshotType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoParamForVendorTagType
    ///
    /// @brief  This function encapsulate the logic of call AECGetParam with return type of StatsVendorTagInfoList
    ///
    /// @param  paramType            The type of data to query
    /// @param  pVendorTagInfoList   The pointer to VendorTagInfoList.
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAlgoParamForVendorTagType(
        AECAlgoGetParamType     paramType,
        StatsVendorTagInfoList* pVendorTagInfoList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoParamForDependVendorTagType
    ///
    /// @brief  This function encapsulate the logic of call AECGetParam with return type of StatsVendorTagInfoList
    ///
    /// @param  paramType            The type of data to query
    /// @param  pVendorTagInfoList   The pointer to VendorTagInfoList.
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAlgoParamForDependVendorTagType(
        AECAlgoGetParamType     paramType,
        StatsVendorTagInfoList* pVendorTagInfoList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAECState
    ///
    /// @brief  Set the m_AECState, all operation on m_AECState shall be called through this method
    ///
    /// @param  state   The new state will be set to m_AECState
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  SetAECState(
        AECState state);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLEDCalibrationState
    ///
    /// @brief  Set the m_LEDCalibrationState, all operation on m_LEDCalibrationState shall be called through this method
    ///
    /// @param  state   The new state will be set to m_LEDCalibrationState
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  SetLEDCalibrationState(
        LEDCalibrationState state);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAECPreFlashState
    ///
    /// @brief  Set the m_AECPreFlashState, all operation on m_AECPreFlashState shall be called through this method
    ///
    /// @param  state   The new state will be set to m_AECPreFlashState
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  SetAECPreFlashState(
        PreFlashState    state);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateAEEngineOutputResults
    ///
    /// @brief  This method updates all the aec output from engine side to be consumed by aec stats processor
    ///
    /// @param  pCommandOutput     Refer to AECCommand for the AECCommandOutputParam payload
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateAEEngineOutputResults(
        AECCommandOutputParam*    pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpAEEngineOutputResults
    ///
    /// @brief  This method updates all the aec output from engine side to be consumed by aec stats processor
    ///
    /// @param  pTriggerName       Pointer to a debug string that needs to be printed along with dump
    /// @param  pAlgoOutput        Pointer to AEC algorithm output
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DumpAEEngineOutputResults(
        const CHAR* pTriggerName,
        AECEngineAlgorithmOutput* pAlgoOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlashTrigger
    ///
    /// @brief  Get the flash trigger
    ///
    /// @return The flash trigger value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FlashTrigger    GetFlashTrigger();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFlashTrigger
    ///
    /// @brief  Set the flash trigger value, the higher value has higher priority. For example, if receive both LED AF & AE
    ///         trigger, no matter what order the triggers are received, the trigger will be LEDAF
    ///
    /// @param  trigger The FlashTrigger to set
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult      SetFlashTrigger(
        FlashTrigger trigger);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsLEDAFNeeded
    ///
    /// @brief  Check if need to run the LED AF sequence on AF trigger
    ///
    /// @return TRUE if LED AF sequence is needed, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL        IsLEDAFNeeded();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsHalPreFlashSettingEnabled
    ///
    /// @brief  Check if HAL setting is enabled for flash operation
    ///
    /// @return TRUE if HAL flash setting is ALWAYS or AUTO, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL        IsHalPreFlashSettingEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsTouchFlashEnabled
    ///
    /// @brief  Check if touch ROI is enabled
    ///
    /// @return TRUE if touch ROI is enabled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL        IsTouchFlashEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CancelPreFlash
    ///
    /// @brief  Cancel the pre flash sequence
    ///
    /// @return CamxResultSuccess upon the success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  CancelPreFlash();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TransitToNextPreFlashState
    ///
    /// @brief  Determine the next PreFlashState per current PreFlashState
    ///
    /// @param  curState the current state of PreFlashState
    ///
    /// @return CamxResultSuccess upon the success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  TransitToNextPreFlashState(
        PreFlashState curState);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsAECSettled
    ///
    /// @brief  Check if the AEC is converged/settled
    ///
    /// @param  pStatsOutput the output of processing the stats from algorithm
    ///
    /// @return TRUE if the AEC is converged/settled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsAECSettled(
        AECEngineProcessStatsOutput* pStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsAFSettled
    ///
    /// @brief  Check if the AF is settled
    ///
    /// @param  pNodesUpdate the output of other nodes which includes the output from AF
    ///
    /// @return TRUE if the AF is focused, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL    IsAFSettled(
        AECEngineNodesUpdate* pNodesUpdate);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsAWBSettled
    ///
    /// @brief  Check if the AWB is converged/settled
    ///
    /// @param  pNodesUpdate the output of other nodes which includes the output from AWB
    ///
    /// @return TRUE if the AWB is converged/settled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL        IsAWBSettled(
        AECEngineNodesUpdate* pNodesUpdate);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCaptureRequest
    ///
    /// @brief  Check current request is a still capture request
    ///
    /// @param  pHALParam   The pointer to AECEngineHALParam
    ///
    /// @return TRUE if is a still capture request, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL        IsCaptureRequest(
        AECEngineHALParam* pHALParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsAELocked
    ///
    /// @brief  Check if the AE is locked by HAL
    ///
    /// @return TRUE if the AE is Locked, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL        IsAELocked();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPreflashComplete
    ///
    /// @brief  Check current request is a still capture request
    ///
    /// @return TRUE if is a still capture request, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL        IsPreflashComplete();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetOperationModeToAlgo
    ///
    /// @brief  Set operation mode to algorithm
    ///
    /// @param  operationMode   the operation mode will be set to algorithm
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult  SetOperationModeToAlgo(
        AECAlgoOperationModeType operationMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetControlAEState
    ///
    /// @brief  This method returns equivalent control AE State that we need to expose externally for AEC internal state
    ///
    /// @return ControlAEStateValues External AE State value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ControlAEStateValues GetControlAEState();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateAEStateBasedOnAlgo
    ///
    /// @brief  Update the AE State based on AEC algo output
    ///
    /// @param  pStatsOutput the output of processing the stats from algorithm
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateAEStateBasedOnAlgo(
        AECEngineProcessStatsOutput* pStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetAEState
    ///
    /// @brief  Resets all states back to normal, after capture snapshot if required
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ResetAEState();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RestoreStreaming
    ///
    /// @brief  Restores back to preview when preflash is complete
    ///
    /// @param  pOutput The output of the AEC algo
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RestoreStreaming(
        AECCommandOutputParam* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TimedWaitForSettle
    ///
    /// @brief  Generic Function to wait for AF or AWB settle
    ///
    /// @param  waitFramesLimit Max No. of frames that need to be waited before transitioning to next Preflash State
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL TimedWaitForSettle(
        UINT32 waitFramesLimit);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StoreLEDCalibrationData
    ///
    /// @brief  Stores the dual LED calibration and tuning results
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID StoreLEDCalibrationData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StoreLEDInlineCalibrationData
    ///
    /// @brief  Stores the dynamic inline LED calibration data and tuning results
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID StoreLEDInlineCalibrationData();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadLEDCalibrationData
    ///
    /// @brief  Loads the dual LED calibration and tuning results
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LoadLEDCalibrationData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadLEDInlineCalibrationData
    ///
    /// @brief  Loads the dynamic LED inline calibration data and tuning results
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LoadLEDInlineCalibrationData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigLEDCalibration
    ///
    /// @brief  Configures the LED calibration based on the internal state.
    ///
    /// @param  pFrameControl The Engine frame control to update with the calibration currents from the algoritm
    /// @param  pOutput       The process stats output
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigLEDCalibration(
        AECEngineFrameControl* pFrameControl,
        AECEngineProcessStatsOutput* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLEDCalibrationConfig
    ///
    /// @brief  Retreives the LED Calibration configuration
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetLEDCalibrationConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlashSnapshotGains
    ///
    /// @brief  Gets the flash snapshot gains from Algo
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFlashSnapshotGains();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAECDebugData
    ///
    /// @brief  Gets the AEC Debug data from Algo
    ///
    /// @param  pCommandInput    Input to AEC algo
    /// @param  pCommandOutput   Output of AEC algo
    ///
    /// @return CamxResultSuccess upon success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAECDebugData(
        AECCommandInputParam*  pCommandInput,
        AECCommandOutputParam* pCommandOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFlashGainsAvailable
    ///
    /// @brief  Checks if flash gains are available for flash snapshot
    ///
    /// @return TRUE if flash gains are valid, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsFlashGainsAvailable();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Private members
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    CHIAECAlgorithm*            m_pAECAlgorithm;                ///< the instance of AEC algorithm
    const StaticSettings*       m_pStaticSettings;              ///< Camx static settings
    AECState                    m_AECState;                     ///< the AECEngine state
    PreFlashState               m_AECPreFlashState;             ///< the sub state when AECEngine in flash state
    BOOL                        m_AECPreFlashSkipChecking;      ///< skips checkings node update during preflash
    CalibrationFlashState       m_calibFlashState;              ///< the LED calibration substates
    LEDCalibrationState         m_LEDCalibrationState;          ///< the LED calibration & tuning substates
    CamX::OSLIBRARYHANDLE       m_hHandle;                      ///< handle for custom algo.
    AECAlgoOperationModeType    m_operationMode;                ///< the current operation mode in algorithm
    AECEngineAlgorithmOutput    m_algoLastOutput;               ///< Last output from algo
    FlashTrigger                m_flashTrigger;                 ///< the trigger for the flash state machine
    BOOL                        m_LEDFDDelay;                   ///< Flag to indicate if need wait more frames for FD under
                                                                ///  preflash
    BOOL                        m_LEDAFRequired;                ///< Flag to indicate if LED assisted is needed
    AECAlgoSnapshotType         m_snapshotIndicator;            ///< Snapshot indicator that shows flash or normal snapshot
    UINT8                       m_preflashFrameWaitCount;       ///< AF settle count under preflash
    UINT32                      m_preFlashMaxFrameWaitLimitAF;  ///< Max frames to wait for AF to settle under preflash
    UINT32                      m_preFlashMaxFrameWaitLimitAWB; ///< Max frames to wait for AWB to settle under preflash
    BOOL                        m_disableAFAWBpreFlash;         ///< Flag to disable AF and AWB states in preflash sequence
    UINT                        m_numberOfFramesToSkip;         ///< Number of frames to skip after main flash
    BOOL                        m_isPrecaptureInProgress;       ///< Flag to indicate if precapture sequence is going on
    StatsCameraInfo             m_cameraInfo;                   ///< camera id, role and type information
    AECEngineFrameControl       m_flashFrameControl;            ///< The main flash frame control information output from algo
                                                                ///  This carries output only for MAIN FLASH SNAPSHOT
    UINT32                      m_AECConvergenceStartTime;      /// AEC Convergence start time measure Param
    AECState                    m_AECLastState;                 /// Last AEC convergence state
    AECEngineFlashMeasurement   m_LEDCalibration;               ///< Flash measurement information for dual LED
    BOOL                        m_RERDone;                      ///< Flag to indicate if RER sequence is complete
    UINT8                       m_precaptureWaitFrames;         ///< No. of frames for precapture after AE Trigger
    BOOL                        m_isFlashCaptureIntent;         ///< Indicate whether is flash capture intent or not
    BOOL                        m_isFixedFocus;                 ///< Indicate whether camera is fixed focus
    StatsSensorInfo             m_sensorInfo;                   ///< AEC sensor information
    UINT                        m_maxPipelineDelay;             ///< Max pipeline delay configured
    BOOL                        m_isFlashInfluencedStats;       ///< flag to indicate whether the stats for snapshot frame is
                                                                ///< influenced by flash
    BOOL                        m_isDynamicInlineCalibration;   ///< flag to indicate whether dynamic inline calibration is
                                                                ///< enabled or not
};

CAMX_NAMESPACE_END

#endif // CAMXAECENGINE_H
