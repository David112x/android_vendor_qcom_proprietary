////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcawbioutil.h
/// @brief The class that implements input/output for AWB stats processor class.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCAWBIOUTIL_H
#define CAMXCAWBIOUTIL_H

#include "chiawbinterface.h"
#include "chistatsproperty.h"

#include "camxistatsprocessor.h"
#include "camxstatsparser.h"
#include "camxutils.h"
#include "camxtofsensorintf.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 BGConfigHorizontalRegions           = 64;   ///< Number of horizontal regions configured for BG stats
static const UINT32 BGConfigVerticalRegions             = 48;   ///< Number of vertical regions configured for BG stats
static const UINT32 BGConfigSaturationThreshold         = 240;  ///< Saturation level of BG stats
static const UINT32 BGStatsConsumpBitWidth              = 8;    ///< AWB operation bit width
static const UINT32 AWBAlgoColorTemperateHorizon        = 2300; ///< Color temperate for AWB D50
static const UINT32 AWBAlgoColorTemperateIncandescent   = 2850; ///< Color temperate for AWB D50
static const UINT32 AWBAlgoColorTemperateTL84           = 3800; ///< Color temperate for AWB D50
static const UINT32 AWBAlgoColorTemperateD50            = 5000; ///< Color temperate for AWB D50
static const UINT32 AWBAlgoColorTemperateD65            = 6500; ///< Color temperate for AWB D50

/// @brief This structure holds AWB property dependency List
struct AWBPropertyDependency
{
    UINT64      slotNum;    ///< The request id for the statistics
    PropertyID  propertyID; ///< Property dependencies in this unit
};

/// @brief This structure holds AWB property dependency List
struct AWBPropertyDependencyList
{
    INT32                   propertyCount;                  ///< Number of properties in this unit
    AWBPropertyDependency   properties[MaxStatsProperties]; ///< Property dependencies in this unit
};

/// @brief This structure holds AWB related HAL Data
struct AWBHALData
{
    BYTE              lock;                     ///< AWB Lock state
    UINT8             mode;                     ///< AWB Mode
    UINT8             state;                    ///< AWB state
};

/// @brief List of AWB Read Property tags
static UINT32 AWBPropertyReadTags[] =
{
    PropertyIDParsedAWBBGStatsOutput,      // 0
    PropertyIDISPAWBBGConfig,              // 1
    PropertyIDAECFrameInfo,                // 2
    InputControlAETargetFpsRange,          // 3
    PropertyIDUsecaseLensInfo,             // 4
    InputScalerCropRegion,                 // 5
    InputControlAWBMode,                   // 6
    InputControlSceneMode,                 // 7
    InputControlAWBLock,                   // 8
    InputControlAWBRegions,                // 9
    PropertyIDAECInternal,                 // 10
    PropertyIDAECFrameControl,             // 11
    InputControlCaptureIntent,             // 12
    FlashMode,                             // 13
    PropertyIDAWBFrameControl,             // 14
    PropertyIDAWBFrameInfo,                // 15
    PropertyIDAWBInternal,                 // 16
    PropertyIDAWBStatsControl,             // 17
    PropertyIDDebugDataAll,                // 18
};

/// @brief List of AWB Read Property types
enum AWBPropertyReadType
{
    AWBReadTypeInvalid = -1,
    AWBReadTypePropertyIDParsedAWBBGStatsOutput,      // 0
    AWBReadTypePropertyIDISPAWBBGConfig,              // 1
    AWBReadTypePropertyIDAECFrameInfo,                // 2
    AWBReadTypeInputControlAETargetFpsRange,          // 3
    AWBReadTypePropertyIDUsecaseLensInfo,             // 4
    AWBReadTypeInputScalerCropRegion,                 // 5
    AWBReadTypeInputControlAWBMode,                   // 6
    AWBReadTypeInputControlSceneMode,                 // 7
    AWBReadTypeInputControlAWBLock,                   // 8
    AWBReadTypeInputControlAWBRegions,                // 9
    AWBReadTypePropertyIDAECInternal,                 // 10
    AWBReadTypePropertyIDAECFrameControl,             // 11
    AWBReadTypeInputControlCaptureIntent,             // 12
    AWBReadTypeFlashMode,                             // 13
    AWBReadTypePropertyIDAWBFrameControl,             // 14
    AWBReadTypePropertyIDAWBFrameInfo,                // 15
    AWBReadTypePropertyIDAWBInternal,                 // 16
    AWBReadTypePropertyIDAWBStatsControl,             // 17
    AWBReadTypePropertyIDDebugDataAll,                // 18
    AWBReadTypePropertyIDCount,                       // 19
    AWBReadTypeMax = 0x7FFFFFFF     ///< Anchor to indicate the last item in the defines
};
CAMX_STATIC_ASSERT(AWBReadTypePropertyIDCount == CAMX_ARRAY_SIZE(AWBPropertyReadTags));

/// @brief List of AWB Write Property tags
static UINT32 AWBPropertyWriteTags[] =
{
    PropertyIDCrossAWBStats,
    PropertyIDAWBPeerInfo,
    PropertyIDAWBFrameInfo,
    PropertyIDAWBInternal,
    ControlAWBLock,
    ControlAWBMode,
    ControlAWBState,
    PropertyIDAWBFrameControl,
    PropertyIDAWBStatsControl,

};

/// @brief List of manual mode for awb
enum class PartialMWBMode
{
    Disable = 0, ///< Manual White Balance Mode Disabled
    CCT,         ///< Manual White Balance Mode with CCT input and gains calculated by algo
    Gains,       ///< Manual White Balance Mode with Gains input and cct calculated by algo
};

/// @brief Enum to select output for AWB logs
enum AWBLogMask
{
    AWBStatsControlDump          = 0x1, ///< outputs AWB Stats
    AWBFrameInfoDump             = 0x2, ///< outputs AWB Frame info
    AWBHALDump                   = 0x4, ///< outputs AWB HAL data
};

static const UINT NumAWBPropertyWriteTags = sizeof(AWBPropertyWriteTags) / sizeof(UINT32); ///< Number of AWB write Properties.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The class that implements input/output for AWB stats processor class.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAWBIOUtil
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CAWBIOUtil
    ///
    /// @brief  Constructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAWBIOUtil();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CAWBIOUtil
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~CAWBIOUtil();

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
        const StatsInitializeData*  pInitializeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoProcessInput
    ///
    /// @brief  Gets the required inputs for the core algorithm Process
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    /// @param  pInput                       Pointer to the core input data
    /// @param  processStats                 Read stats for input
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAlgoProcessInput(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoInputList*               pInput,
        BOOL                            processStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoExpectedOutputList
    ///
    /// @brief  Populates the algo output structure with required buffers
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    /// @param  pOutput                      Pointer to the core output data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAlgoExpectedOutputList(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoOutputList*              pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoGetParamInputOutput
    ///
    /// @brief  Gets the required inputs and output buffer for the core algorithm GetParam
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    /// @param  pGetParam                    Pointer to the Algo GetParam structure
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAlgoGetParamInputOutput(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoGetParam*                pGetParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoSetParamInput
    ///
    /// @brief  Gets the required inputs for the core algorithm GetParam
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    /// @param  pInput                       Pointer to the list of SetParam input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAlgoSetParamInput(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFrameAndStatControl
    ///
    /// @brief  Gets the current settings of frameControl and statsControl
    ///
    /// @param  pFrameControl    Frame control to fill
    /// @param  pStatsControl    Stats control to fill
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetFrameAndStatControl(
        AWBFrameControl*   pFrameControl,
        AWBStatsControl*   pStatsControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsAECSettled
    ///
    /// @brief  Check if AEC is settled
    ///
    /// @return TRUE if AEC is settled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsAECSettled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlashFrameControl
    ///
    /// @brief  Gets the current settings of flash frameControl
    ///
    /// @param  pFlashFrameControl  Flash Frame control to fill
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetFlashFrameControl(
        AWBFrameControl*   pFlashFrameControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoFlashSate
    ///
    /// @brief  returns algo's current flash state
    ///
    /// @return algo flash state
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AWBAlgoFlashState GetAlgoFlashSate() {return m_setInputs.flashInfo.flashState;}

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrePublishMetadata
    ///
    /// @brief  Publish the frame controls for initial frames
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrePublishMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishSkippedFrameOutput
    ///
    /// @brief  Publish AWB frame and stats control from previous frame property pool
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishSkippedFrameOutput() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishMetadata
    ///
    /// @brief  Publishes the algorithm output to all metadata pool
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    /// @param  pProcessOutput               Pointer to list of algo process output data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishMetadata(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoOutputList*              pProcessOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishCrossProperty
    ///
    /// @brief  Publish AWB cross pipeline property for dual camera usecase.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishCrossProperty(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPeerInfo
    ///
    /// @brief  Publish AWB peer information property for dual camera usecase.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pPeerInfo                       Pointer to peer information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPeerInfo(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        VOID*                           pPeerInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPeerInfo
    ///
    /// @brief  Retrieve AWB peer information property for dual camera usecase.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetPeerInfo(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InvalidateIO
    ///
    /// @brief  Invalidate all algo input/output array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InvalidateIO();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultBGConfig
    ///
    /// @brief  Get the default AWB BG configuration
    ///
    /// @param  pAWBBGConfigData  AWB BG stats configuration output buffer
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDefaultBGConfig(
        StatsBayerGridBayerExposureConfig* pAWBBGConfigData) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAWBFlashEstimationState
    ///
    /// @brief  Get the current AWB flash estimation progress state
    ///
    /// @return returns the state
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE AWBAlgoFlashEstimationProgress GetAWBFlashEstimationState() const
    {
        return m_algoFlashEstimationState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishExternalCameraMetadata
    ///
    /// @brief  Publishes the mandated camera metadata
    ///
    /// @param  pProcessOutput                  Pointer to list of algo process output data
    /// @param  pAWBHALData                     Pointer to AWB HAL Data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishExternalCameraMetadata(
        AWBAlgoOutputList*              pProcessOutput,
        AWBHALData*                     pAWBHALData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishVendorTagMetadata
    ///
    /// @brief  Publishes AF output to the metadata pool
    ///
    /// @param  pProcessOutput                  Pointer to list of algo process output data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishVendorTagMetadata(
        AWBAlgoOutputList*              pProcessOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFrameControlToMainMetadata
    ///
    /// @brief  Publishes the algorithm output to the main metadata pool
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pFrameControl                   Pointer to AWB Frame Control
    /// @param  pStatsControl                   Pointer to AWB Stats Control
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishFrameControlToMainMetadata(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBFrameControl*                pFrameControl,
        AWBStatsControl*                pStatsControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFrameInformationToMainMetadata
    ///
    /// @brief  Publishes the algorithm output to the main metadata pool
    ///
    /// @param  pFrameInfo pointer to AWB Frame Info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishFrameInformationToMainMetadata(
        AWBFrameInfo*    pFrameInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishToInternalMetadata
    ///
    /// @brief  Publishes the algorithm output to the internal metadata pool
    ///
    /// @param  pOutputInternal pointer to AWB output internal
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishToInternalMetadata(
        AWBOutputInternal*  pOutputInternal);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPropertyDebugData
    ///
    /// @brief  Publishes the debug-data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPropertyDebugData();

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
        AWBAlgoGetParam* pAlgoGetParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiStatsSessionHandle
    ///
    /// @brief  Get the stats chi session data.
    ///
    /// @return pointer chi stats session
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiStatsSession* GetChiStatsSessionHandle()
    {
        return &m_inputs.statsSession;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillIlluminantCalibrationFactor
    ///
    /// @brief  Fill AWB Illuminant Calibration factor data from OTP data strauture to algo set param input structure.
    ///
    /// @param  pWBCalibrationData               White balance calibration OTP data
    /// @param  pIlluminantsCalibrationFactor    Pointer to Illuminants Calibration Factor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillIlluminantCalibrationFactor(
        const WBCalibrationData*                pWBCalibrationData,
        AWBAlgoIlluminantsCalibrationFactor*    pIlluminantsCalibrationFactor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgoChromatix
    ///
    /// @brief  Sets the required inputs parameters for the core algorithm
    ///
    /// @param  pInputTuningModeData    Pointer to Chi Tuning mode
    /// @param  pAlgoSetParamList       Pointer to AWB algo input parameter
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetAlgoChromatix(
        ChiTuningModeParameter* pInputTuningModeData,
        AWBAlgoSetParamList*    pAlgoSetParamList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateCameraInformation
    ///
    /// @brief  Populate camera information
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateCameraInformation(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateTOFData
    ///
    /// @brief  Populate Laser Data
    ///
    /// @param  pData    Pointer to process request information
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulateTOFData(
        DataTOF* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadAWBProperties
    ///
    /// @brief  Sets the required inputs parameters for the core algorithm
    ///
    /// @param  requestIdOffsetFromLastFlush requestId offset from last frame which has been flushed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReadAWBProperties(
        UINT64 requestIdOffsetFromLastFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteAWBProperties
    ///
    /// @brief  Write Data for AWB write property tags
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID WriteAWBProperties();



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAwbAlgo
    ///
    /// @brief  Set AWB Algorithm Handle
    ///
    /// @param  pAlgo         Algo Handle
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAwbAlgo(
        CHIAWBAlgorithm* pAlgo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupTOFLink
    ///
    /// @brief  Set up link to TOF intf
    ///
    /// @param  pChiContext  Pointer to Chi context
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupTOFLink(
        ChiContext* pChiContext);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateTraceEvents
    ///
    /// @brief  Updates the trace events based on the processed stats output
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateTraceEvents();

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
        StatsSensorInfo** ppSensorInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadSetInputParamList
    ///
    /// @brief  Sets input parameters of AWB into parameter list.
    ///
    /// @param  pInputList         Pointer to input parameter list
    /// @param  setParamType       Type of parameter
    /// @param  pAWBSetParam       Pointer to input data
    /// @param  sizeOfInputParam   Size of input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID LoadSetInputParamList(
        AWBAlgoSetParamList* pInputList,
        AWBAlgoSetParamType  setParamType,
        const VOID*          pAWBSetParam,
        UINT32               sizeOfInputParam);

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestNumber
    ///
    /// @brief  Get request number needed by AWB algorithm and add them to the input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pInput                          Pointer to the core input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetRequestNumber(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AWBAlgoInputList*              pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeReadProperties
    ///
    /// @brief  Init and set AWB read property tags.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeReadProperties();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeWriteProperties
    ///
    /// @brief  Init and set AWB write property tags.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeWriteProperties();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAWBBayerGrid
    ///
    /// @brief  Once statistics have been parsed, this function arranges the info into the algorithm structure.
    ///
    /// @param  pBayerGridOutput Pointer to the internal property of parsed output
    /// @param  pStatsConfig     Pointer to the internal property for stats config
    /// @param  pBayerGrid       Pointer to the stats structure used by the algorithm.
    /// @param  pBayerGridROI    Pointer to the stats data ROI used by the algorithm
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAWBBayerGrid(
        ParsedAWBBGStatsOutput* pBayerGridOutput,
        ISPAWBBGStatsConfig*    pStatsConfig,
        StatsBayerGrid*         pBayerGrid,
        StatsRectangle*         pBayerGridROI);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStatistics
    ///
    /// @brief  Get statistics needed by AWB algorithm and add them to the input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pInput                          Pointer to the core input data
    /// @param  processStats                    Read stats for input
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetStatistics(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AWBAlgoInputList*              pInput,
        BOOL                           processStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAECData
    ///
    /// @brief  Get data needed by AWB algorithm from AEC and add them to the input list.
    ///
    /// @param  pInput                       Pointer to the core input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAECData(
        AWBAlgoInputList*              pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveSensorInfo
    ///
    /// @brief  Get sensor info from Property Pool and add them to the AWB set input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveSensorInfo(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveStatsWindowInfo
    ///
    /// @brief  Get stats window info and add them to the AWB set input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveStatsWindowInfo(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveCalibrationData
    ///
    /// @brief  Get various calibration data from Metadata Pool and add them to the AWB set input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveCalibrationData(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveAWBMode
    ///
    /// @brief  Get AWB mode from Metadata Pool and add them to the AWB set input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveAWBMode(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveSceneMode
    ///
    /// @brief  Get scene mode from Metadata Pool and add them to the AWB set input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveSceneMode(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveAWBLockInfo
    ///
    /// @brief  Get AWB lock info from Metadata Pool and add them to the AWB set input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveAWBLockInfo(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveROIInfo
    ///
    /// @brief  Get face and touch ROI info from Metadata Pool and add them to the AWB set input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveROIInfo(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveFlashInfo
    ///
    /// @brief  Get flash info from Metadata Pool and add them to the AWB set input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveFlashInfo(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadDynamicConvergenceSpeed
    ///
    /// @brief  Read dynamic convergence speed from vendor tags and add them to the AWB set input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadDynamicConvergenceSpeed(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveWarmstartInfo
    ///
    /// @brief  Get AWB warm-start info from vendor tags and add them to the AWB set input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveWarmstartInfo(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*            pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveExtensionTriggerInfo
    ///
    /// @brief  Get extension trigger level, e.g. background IR value info(ambient rate)
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pSetInputList                   Pointer to the core set input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveExtensionTriggerInfo(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AWBAlgoSetParamList*           pSetInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVendorTags
    ///
    /// @brief  Get frame number needed by AWB algorithm and add them to the input list.
    ///
    /// @param  pInput                          Pointer to the core input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetVendorTags(
        AWBAlgoInputList*              pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDebugDataBuffer
    ///
    /// @brief  Get the debug data needed by AWB algorithm and add them to the input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pInput                          Pointer to the core input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDebugDataBuffer(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AWBAlgoInputList*              pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiStatsSession
    ///
    /// @brief  Get the stats chi session data.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pInput                          Pointer to the core input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetChiStatsSession(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AWBAlgoInputList*              pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCameraInformation
    ///
    /// @brief  Set camera information to algo input
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pInput                          Pointer to the core input data
    /// @param  processStats                    Flag to check stats processed or not.
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetCameraInformation(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AWBAlgoInputList*              pInput,
        BOOL                           processStats);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillBGConfigurationData
    ///
    /// @brief  Fill AWB BG statistics configuration data, either use algorithm output or use default configuration defined by
    ///         this node.
    ///
    /// @param  pAWBConfigData  Pointer to the stats configuration to be filled
    /// @param  pAWBAlgoConfig  Pointer to the AWB algorithm configuration requested. If null or invalid, default configuration
    ///                         will be fill.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillBGConfigurationData(
        AWBConfig*                                  pAWBConfigData,
        const StatsBayerGridBayerExposureConfig*    pAWBAlgoConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGainsFromCCT
    ///
    /// @brief  Get gains using CCT from Algo
    ///
    /// @param  pStatsProcessRequestDataInfo   Pointer to StatsProcessRequestData
    /// @param  pGains                         Pointer to AWBGainParams Structure
    /// @param  CCT                            Color Temprature
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetGainsFromCCT(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBGainParams* pGains,
        UINT32 CCT);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCCTFromGains
    ///
    /// @brief  Get cct using gains from Algo
    ///
    /// @param  pStatsProcessRequestDataInfo   Pointer to StatsProcessRequestData
    /// @param  pCCT                           Pointer to CCT filled using Algo
    /// @param  pGains                         Pointer to Array of Gains
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCCTFromGains(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        UINT32* pCCT,
        FLOAT* pGains);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PartialMWBOverride
    ///
    /// @brief  Override Manual White balance if enabled
    ///
    /// @param  pStatsProcessRequestDataInfo   Pointer to StatsProcessRequestData
    /// @param  pFrameControl                  Pointer to FrameControl
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PartialMWBOverride(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBFrameControl*                pFrameControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPartialMWBMode
    ///
    /// @brief  Override Manual White balance if enabled
    ///
    /// @return 0 for disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PartialMWBMode GetPartialMWBMode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadVendorTag
    ///
    /// @brief  Read Vendor Tag
    ///
    /// @param  pSectionName  Section Name
    /// @param  pTagName      Tag name
    /// @param  ppArg         Argument
    ///
    /// @return Success or Failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadVendorTag(
        const CHAR* pSectionName,
        const CHAR* pTagName,
        VOID**      ppArg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpAWBStats
    ///
    /// @brief  Dumps AWB stats to log
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpAWBStats();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpFrameInfo
    ///
    /// @brief  Dumps AWB frame info to logs
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpFrameInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpHALData
    ///
    /// @brief  Dumps HAL data to logs
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpHALData();

    CAWBIOUtil(const CAWBIOUtil&)               = delete;   ///< Do not implement copy constructor
    CAWBIOUtil& operator=(const CAWBIOUtil&)    = delete;   ///< Do not implement assignment operator

    ///< @brief Defines the of inputs that will be provided to AWB algorithm
    struct AWBAlgorithmInputs
    {
        UINT64                      requestNumber;      ///< Holds frame number to be use by algorithm
        AWBAlgoExposureInformation  exposureInfo;       ///< Holds exposure information input for AWB Algorithm
        StatsBayerGrid              statsBayerGrid;     ///< Holds the base struct that have the ptr to current stats
        StatsRectangle              statsBayerGridROI;  ///< Holds the ROI used in the configuration of BG stats
        StatsVendorTagList          vendorTagInputList; ///< Holds list of vendor tags data for algo process input
        StatsDataPointer            debugData;          ///< The debug data pointer, if present
        ChiStatsSession             statsSession;       ///< Holds stats session data used for handling vendor tag operations
        StatsCameraInfo             cameraInfo;         ///< Holds camera information
        AWBAlgoGains                gains;              ///< Holds WB Gains
        UINT32                      cct;                ///< Holds color temprature
    };

    ///< @brief Defines the of outputs that will be provided to AWB algorithm
    struct AWBAlgorithmOutputs
    {
        StatsDataPointer                    debugData;                              ///< Holds pointers to debug data
        AWBAlgoGains                        gains;                                  ///< Holds algorithm AWB gain output
        UINT32                              cct;                                    ///< Holds algorithm AWB CCT output
        StatsIlluminantType                 illuminantType;                         ///< Holds algorithm illuminant type result
        StatsIlluminantType                 sampleDecision[AWBAlgoDecisionMapSize]; ///< Holds the array of AWB map decision
        StatsBayerGridBayerExposureConfig   BGConfig;                               ///< Holds the stats configuration requested
                                                                                    ///  by AWB algorithm
        AWBAlgoState                        state;                                  ///< Holds the AWB algorithm state
        AWBAlgoMode                         mode;                                   ///< Holds the AWB algorithm operation mode.
        BOOL                                lock;                                   ///< Holds the AWB algorithm lock status.
        StatsVendorTagList                  vendorTagList;                          ///< Holds base struct for vendor tag list.
        StatsAWBCCMList                     CCMList;                                ///< Holds Algo's CCM output.

        AWBAlgoOutputList                   lastOutputList;                         ///< Holds the payload for get last output

        AWBAlgoGains                        flashGains;                             ///< Holds the estimated gain for main flash
        UINT32                              flashCCT;                               ///< Holds the estimated CCT for main flash
        StatsAWBCCMList                     flashCCMList;                           ///< Holds Algo's CCM output for main flash.
        AWBAlgoOutputList                   flashOutputList;                        ///< Holds the payload for get last output
        VOID*                               pPeerInfo;                              ///< Holds multi camera peer information
                                                                                    ///  pointer from AWB algorithm
        AWBAlgoDecisionInformation          currentDecision;                        ///< Holds AWB algo's decision output
    };

    ///< @brief Defines the inputs of AWB algorithm set method
    struct AWBAlgorithmSetInputs
    {
        INT32                                   debugDataMode;                      ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeDebugDataMode
        StatsSensorInfo                         statsSensorInfo;                    ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeSensorInfo
        StatsWindowInfo                         statsWindowInfo;                    ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeStatsWindow
        StatsTuningData                         statsTuningData;                    ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeChromatixData
        AWBAlgoIlluminantsCalibrationFactor     illuminantsCalibrationFactor;       ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeIlluminantsCalibration-
                                                                                    ///  Factor
        AWBAlgoGeometricalDisparityCalibration  geometricalDisparityCalibration;    ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeGeometricalDisparity-
                                                                                    ///  Calibration
        StatsLEDCalibrationDataInput            LEDCalibrationDataInput;            ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeLEDCalibrationData
        AWBAlgoManualConfiguration              manualConfig;                       ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeManualSettings
        AWBAlgoMode                             algoMode;                           ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeWhiteBalanceMode
        BOOL                                    lock;                               ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeLock
        UINT32                                  sceneMode;                          ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeSceneMode
        StatsFaceInformation                    statsFaceInfo;                      ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeUnstabilizedFaceROI
        StatsWeightedROI                        statsTouchROI;                      ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeTouchROI
        AWBAlgoFlashType                        flashType;                          ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeFlashType
        AWBAlgoFlashInformation                 flashInfo;                          ///< Payload type of AWB algo set param
                                                                                    ///  AWBSetParamTypeFlashData
        VOID*                                   pPeerInfo;                          ///< Holds multi camera peer information
                                                                                    ///  pointer from AWB algorithm
        UnstabilizedROIInformation              unstabilizedFaceInfo;               ///< Payload type of AWB algo set param
                                                                                    ///< AWBSetParamTypeUniformFaceROI
        AWBAlgoWarmstartInformation             warmstartInfo;                      ///< Holds warm-start gain and cct
                                                                                    ///< AWBSetParamTypeWarmstart
        FLOAT                                   dynamicConvergenceSpeed;            ///< Dynamic convergence speed
                                                                                    ///< AWBSetParamTypeDynamicConvergenceSpeed
        AWBAlgoExternalInformation              extensionTriggerInfo;               ///< extension trigger input
                                                                                    ///< AWBAlgoExternalInformation
        UINT8                                   pipelineDelay;                      ///< pipeline delay
                                                                                    ///< UINT8
    };

    Node*                   m_pNode;            ///< Pointer to owning StatsProcessor node
    MetadataPool*           m_pDebugDataPool;   ///< Pointer to debugdata metadata pool
    StatsParser*            m_pStatsParser;     ///< ISP stats parser
    TuningDataManager*      m_pTuningManager;   ///< Pointer to tuning manager
    const SensorMode*       m_pSensorData;      ///< Pointer to Sensor Mode information.
    AWBAlgorithmInputs      m_inputs;           ///< Holds the memory for the input data provided to algorithm.
    AWBAlgorithmOutputs     m_outputs;          ///< Holds the memory for the output data provided to algorithm.
    AWBAlgorithmSetInputs   m_setInputs;        ///< Holds the memory for the input data provided to algorithm set.

    const IFEInputResolution* m_pIFEInput;      ///< Pointer to IFE Input Res

    AWBAlgoInput            m_processInputArray[AWBInputTypeLastIndex];      ///< Array of algo Process input
    AWBAlgoOutput           m_processOutputArray[AWBOutputTypeLastIndex];    ///< Array of algo Process outputs
    AWBAlgoGetParamInput    m_getParamInputArray[AWBGetParamInputTypeLastIndex];         ///< Array of algo getparam inputs
    AWBAlgoGetParamOutput   m_getParamOutputArray[AWBGetParamOutputTypeLastIndex];       ///< Array of algo getparam outputs
    AWBAlgoSetParam         m_setParamInputArray[AWBSetParamTypeLastIndex];         ///< Array of algo setparam inputs

    AWBAlgoFlashEstimationProgress      m_algoFlashEstimationState;     ///< Holds latest algorithm flash estimation state
    StatsVendorTagInfoList              m_vendorTagInputList;           ///< List of Stats vendor tag dependency
    StatsVendorTagInfoList              m_vendorTagInfoOutputputList;   ///< List of Algo's output vendor tags
    BOOL                                m_isOffline;                    ///< Offline/Online stats check
    UINT                                m_numberOfFramesToSkip;         ///< Number of frames to skip after main flash
    AWBAlgoState                        m_lastAWBAlgoState;             ///< indicates AWB state on previous frame

    StatsPropertyReadAndWrite*  m_pStatsAWBPropertyReadWrite;                     ///< Stats Property AWB Readwrite instance
    ReadProperty                m_AWBReadProperties[AWBReadTypePropertyIDCount];  ///< AWB Read Properties
    WriteProperty               m_AWBWriteProperties[NumAWBPropertyWriteTags];    ///< AWB Write Properties
    UINT64                      m_currProcessingRequestId;                        ///< Current processing request Id
    VOID*                       m_pPeerInfo;                                      ///< Pointer to peer info
    AWBFrameControl             m_frameControl;                                   ///< AWB Frame Control
    AWBHALData                  m_AWBHALData;                                     ///< AWB HAL Data
    AWBStatsControl             m_statsControl;                                   ///< AWB Stats control
    AWBFrameInfo                m_frameInfo;                                      ///< AWB Frame Info
    AWBOutputInternal           m_outputInternal;                                 ///< AWB Output data
    const StaticSettings*       m_pStaticSettings;                                ///< Camx static settings
    TOFSensorIntf*              m_pTOFInterfaceObject;                            ///< TOF interface object pointer
    DataTOF                     m_TOFData;                                        ///< TOF sensor data

    CHIAWBAlgorithm*            m_pAWBAlgorithm;        ///< Pointer to instance of AWB algorithm interface
};

CAMX_NAMESPACE_END

#endif // CAMXCAWBIOUTIL_H
