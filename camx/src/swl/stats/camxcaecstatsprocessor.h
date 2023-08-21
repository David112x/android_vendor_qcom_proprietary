////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcaecstatsprocessor.h
/// @brief The class that implements IStatsProcessor for AEC.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCAECSTATSPROCESSOR_H
#define CAMXCAECSTATSPROCESSOR_H

#include "chiaecinterface.h"
#include "chistatsproperty.h"
#include "camxtuningdatamanager.h"
#include "camxaecengine.h"
#include "camxistatsprocessor.h"
#include "camxlist.h"
#include "camxstatsparser.h"
#include "camxutils.h"
#include "parametertuningtypes.h"
#include "camxncsservice.h"
#include "camxncssensordata.h"



CAMX_NAMESPACE_BEGIN

static const INT AECSettleUnknown                  = -1;     ///< Invalid/Unknown AEC settle state - Initial
static const INT AECSettleFalse                    = 0;      ///< AEC settle flag is false from algo
static const INT AECSettled                        = 1;      ///< AEC settle flag is true  from algo
static const INT AECompensation0                   = 0;      ///< AEC Compensation for Invalid
static const INT AECompensation_Beach_Snow         = 6;      ///< AEC Compensation for Beach Snow scene
static const INT AECompensation_Sunset_CandleLight = -6;     ///< AEC Compensation for Sunset and Candle light
static const INT AESensitivity_Sports              = 400;    ///< AEC Compensation for Sports
static const UINT32 AECAlgoGetMaxInputParamCount   =  2;     ///< Maximum Number of input params for Get
static const UINT16 AECExposureProgramManual       = 1;      ///< Manual Exposure Program
static const UINT16 AECExposureProgramNormal       = 2;      ///< Normal Exposure Program

/// @brief List of AEC Read Property tags
static UINT32 AECPropertyReadTags[] =
{
    PropertyIDISPHDRBEConfig,                  // 0
    PropertyIDParsedHDRBEStatsOutput,          // 1
    PropertyIDParsedHDRBHistStatsOutput,       // 2
    PropertyIDParsedBHistStatsOutput,          // 3
    FlashMode,                                 // 4
    PropertyIDDebugDataAll,                    // 5
    InputControlAEExposureCompensation,        // 6
    InputControlAELock,                        // 7
    InputControlAEMode,                        // 8
    InputControlAEPrecaptureTrigger,           // 9
    InputControlAFTrigger,                     // 10
    InputControlCaptureIntent,                 // 11
    InputControlMode,                          // 12
    InputControlSceneMode,                     // 13
    InputFlashMode,                            // 14
    InputSensorExposureTime,                   // 15
    InputSensorSensitivity,                    // 16
    InputControlAEAntibandingMode,             // 17
    InputControlAERegions,                     // 18
    FlashState,                                // 19
    InputControlAETargetFpsRange,              // 20
    InputControlZslEnable,                     // 21
    SensorFrameDuration,                       // 22
    InputControlPostRawSensitivityBoost,       // 23
    InputScalerCropRegion,                     // 24
    PropertyIDAFDFrameInfo,                    // 25
    PropertyIDAFFrameInfo,                     // 26
    PropertyIDAWBInternal,                     // 27
    PropertyIDAWBFrameInfo,                    // 28
    InputControlAWBMode,                       // 29
    PropertyIDRERCompleted,                    // 30
    PropertyIDUsecaseLensInfo,                 // 31
    PropertyIDParsedIHistStatsOutput,          // 32
};


/// @brief List of AEC Read Property types
enum AECPropertyReadType
{
    AECReadTypeInvalid = -1,
    AECReadTypePropertyIDISPHDRBEConfig,                  // 0
    AECReadTypePropertyIDParsedHDRBEStatsOutput,          // 1
    AECReadTypePropertyIDParsedHDRBHistStatsOutput,       // 2
    AECReadTypePropertyIDParsedBHistStatsOutput,          // 3
    AECReadTypeFlashMode,                                 // 4
    AECReadTypePropertyIDDebugDataAll,                    // 5
    AECReadTypeInputControlAEExposureCompensation,        // 6
    AECReadTypeInputControlAELock,                        // 7
    AECReadTypeInputControlAEMode,                        // 8
    AECReadTypeInputControlAEPrecaptureTrigger,           // 9
    AECReadTypeInputControlAFTrigger,                     // 10
    AECReadTypeInputControlCaptureIntent,                 // 11
    AECReadTypeInputControlMode,                          // 12
    AECReadTypeInputControlSceneMode,                     // 13
    AECReadTypeInputFlashMode,                            // 14
    AECReadTypeInputSensorExposureTime,                   // 15
    AECReadTypeInputSensorSensitivity,                    // 16
    AECReadTypeInputControlAEAntibandingMode,             // 17
    AECReadTypeInputControlAERegions,                     // 18
    AECReadTypeFlashState,                                // 19
    AECReadTypeInputControlAETargetFpsRange,              // 20
    AECReadTypeInputControlZslEnable,                     // 21
    AECReadTypeSensorFrameDuration,                       // 22
    AECReadTypeInputControlPostRawSensitivityBoost,       // 23
    AECReadTypeInputScalerCropRegion,                     // 24
    AECReadTypePropertyIDAFDFrameInfo,                    // 25
    AECReadTypePropertyIDAFFrameInfo,                     // 26
    AECReadTypePropertyIDAWBInternal,                     // 27
    AECReadTypePropertyIDAWBFrameInfo,                    // 28
    AECReadTypeInputControlAWBMode,                       // 29
    AECReadTypePropertyIDRERCompleted,                    // 30
    AECReadTypePropertyIDLensInfo,                        // 31
    AECReadTypePropertyIDParsedIHistStatsOutput,          // 32
    AECReadTypePropertyIDCount,                           // 33
    AECReadTypeMax = 0x7FFFFFFF     ///< Anchor to indicate the last item in the defines
};

/// @brief Enum to select output for AEC logs
enum AECLogMask
{
    AECStatsControlDump           = 0x1, ///< outputs AEC Stats
    AECFrameInfoDump              = 0x2, ///< outputs AEC Frame Info
    AECHALDump                    = 0x4  ///< outputs AEC HAL Data
};

CAMX_STATIC_ASSERT(AECReadTypePropertyIDCount == CAMX_ARRAY_SIZE(AECPropertyReadTags));

/// @brief List of AEC Write Property tags
static UINT32 AECPropertyWriteTags[] =
{
    PropertyIDAECFrameControl,
    PropertyIDAECStatsControl,
    PropertyIDAECPeerInfo,
    PropertyIDAECFrameInfo,
    ControlAEExposureCompensation,
    ControlAELock,
    ControlAEMode,
    ControlAERegions,
    ControlAEState,
    ControlAEPrecaptureTrigger,
    ControlMode,
    ControlAECompensationStep,
    StatisticsSceneFlicker,
    ControlAEAntibandingMode,
    ControlAETargetFpsRange,
    PropertyIDAECInternal,
    PropertyIDCrossAECStats
};

static const UINT NumAECPropertyWriteTags = sizeof(AECPropertyWriteTags) / sizeof(UINT32); ///< Number of AEC write Properties.
static const UINT NumAECVendorTagsPublish = 11; ///< Number of AEC vendor tag going to publish

/// @brief This structure holds AWB property dependency List
struct AECPropertyDependency
{
    UINT64                 slotNum;                          ///< The request id for the statistics
    PropertyID             propertyID;                       ///< Property dependencies in this unit
};

/// @brief This structure holds AEC property dependency List
struct AECPropertyDependencyList
{
    INT32                   propertyCount;                  ///< Number of properties in this unit
    AECPropertyDependency   properties[MaxStatsProperties]; ///< Property dependencies in this unit
};

/// @brief This structure holds AEC related vendor tag
struct AECVendorTagList
{
    const CHAR*   pSectionName;      ///< pSectionName
    const CHAR*   pTagName;          ///< pTagName
    VOID*         pData;             ///< data
    UINT32        size;              ///< size
};

/// @brief This structure holds AEC related HAL Data
struct AECHALData
{
    INT32             aeCompensation;           ///< Pointer to AECompensation
    BOOL              aeLockFlag;               ///< AE lock flag
    UINT8             mode;                     ///< AE mode
    WeightedRegion    aeRegion;                 ///< AE Region
    UINT8             aeState;                  ///< AE state
    UINT8             triggerValue;             ///< AE Pre-capture trigger value
    ControlModeValues controlMode;              ///< 3A control mode
    Rational          exposureCompensationStep; ///< Exposure compensation step
    UINT8             flickerMode;              ///< Scene flicker mode
    UINT8             aeAntiBandingMode;        ///< AEC antibanding mode
    RangeINT32        currentFPSRange;          ///< Curent FPS range
};

/// @brief function pointer to stats parser
typedef VOID (*StatsParseFuncPtr_t)(UINT8*, VOID*, UINT, UINT);

/// Forward Declarations
class Node;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The class that implements Gyro data for AEC.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAECGyro
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Used to initialize the class.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize();

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
    /// PopulateGyroData
    ///
    /// @brief  Retrieves gyro info from NCS interface
    ///
    /// @param  pInput      Set gyro info into pInput for AECCommandInputParam usage
    /// @param  minFPS      Minimum Supported FPS by the Sensor
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulateGyroData(
        AECCommandInputParam* pInput,
        INT32                 minFPS);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CAECGyro
    ///
    /// @brief  default constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAECGyro();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CAECGyro
    ///
    /// @brief  destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~CAECGyro();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAECGyro(const CAECGyro&)               = delete;   ///< Do not implement copy constructor
    CAECGyro& operator=(const CAECGyro&)    = delete;   ///< Do not implement assignment operator

    NCSSensor*                  m_pNCSSensorHandleGyro;                     ///< NCS Sensor handle.
    NCSService*                 m_pNCSServiceObject;                        ///< NCS Service object pointer.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The class that implements IStatsProcessor for AEC.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAECStatsProcessor final : public IStatsProcessor
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create the object for CAECStatsProcessor.
    ///
    /// @param  ppAECStatsProcessor Pointer to CAECStatsProcessor
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        IStatsProcessor** ppAECStatsProcessor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Used to initialize the class.
    ///
    /// @param  pInitializeData Pointer to initial settings
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult Initialize(
        const StatsInitializeData* pInitializeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Executes the algorithm for a request.
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ExecuteProcessRequest(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// VendorTagListAllocation
    ///
    /// @brief  Function to fetch vendor tag list and allocate memory for them
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult VendorTagListAllocation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddOEMDependencies
    ///
    /// @brief  Get the list of OEM dependencies from all stats processors.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pStatsDependency                Pointer to list of property dependencies
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddOEMDependencies(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        StatsDependency*                pStatsDependency);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDependencies
    ///
    /// @brief  Get the the list of dependencies from all stats processors.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pStatsDependency                Pointer to stats dependencies
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetDependencies(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        StatsDependency*               pStatsDependency);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPublishList
    ///
    /// @brief  Get the the list of tags published by all stats processors.
    ///
    /// @param  maxTagArraySize  Maximum size of pTagArray
    /// @param  pTagArray        Array of tags that are published by the stats processor.
    /// @param  pTagCount        Number of tags published by the stats processor
    /// @param  pPartialTagCount Number of Partialtags published by the stats processor
    ///
    /// @return CamxResultSuccess if successful, return failure if the number of tags to be published exceeds maxTagArraySize
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetPublishList(
        const UINT32    maxTagArraySize,
        UINT32*         pTagArray,
        UINT32*         pTagCount,
        UINT32*         pPartialTagCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetStatsParseFuncPtr
    ///
    /// @brief  Set stats parser function pointer
    ///
    /// @param  pStatsParseFuncPtr function pointer of stats parser
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetStatsParseFuncPtr(
        VOID* pStatsParseFuncPtr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetVideoHDRInformation
    ///
    /// @brief  Set VideoHDRInformation
    ///
    /// @param  isVideoHDREnabled               Indicate it is video HDR mode or not
    /// @param  HDR3ExposureType                Flag of 3-Exposure HDR type
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetVideoHDRInformation(
        BOOL             isVideoHDREnabled,
        HDR3ExposureType HDR3ExposureType);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CAECStatsProcessor
    ///
    /// @brief  constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAECStatsProcessor();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CAECStatsProcessor
    ///
    /// @brief  destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~CAECStatsProcessor();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OverwriteAECOutput
    ///
    /// @brief  Overwrite the value from algorithm output
    ///
    /// @param  pOutput Pointer to AEC Engine's output
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult OverwriteAECOutput(
        AECEngineProcessStatsOutput* pOutput
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVendorTags
    ///
    /// @brief  Gets the required inputs for the core algorithm
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    /// @param  pInput                       Pointer to the core input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetVendorTags(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AECEngineAlgorithmInput*       pInput);

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
        AECEngineAlgorithmInput*       pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlgoInput
    ///
    /// @brief  Gets the required inputs for the core algorithm
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    /// @param  pInput                       Pointer to the core input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAlgoInput(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AECEngineAlgorithmInput*       pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDebugDataPointer
    ///
    /// @brief  Sets the debug data pointer to the algorithm
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    /// @param  pInput                       Pointer to the core input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID SetDebugDataPointer(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AECEngineAlgorithmInput*       pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCameraInformation
    ///
    /// @brief  Sets the camera information to the algorithm
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    /// @param  pInput                       Pointer to the core input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetCameraInformation(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AECEngineAlgorithmInput*       pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareAlgorithmOutput
    ///
    /// @brief  Prepare the data used by algorithm to output it's result for processing the stats
    ///
    /// @param  pOutput   Pointer to AECEngineProcessStatsOutput
    /// @param  requestId ID of the request being serviced
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PrepareAlgorithmOutput(
        AECEngineProcessStatsOutput* pOutput,
        UINT64                       requestId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgoChromatix
    ///
    /// @brief  Sets the required inputs parameters for the core algorithm
    ///
    /// @param  pInputTuningModeData Pointer to Tuning mode data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetAlgoChromatix(
        ChiTuningModeParameter* pInputTuningModeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgoSetParams
    ///
    /// @brief  Sets the required inputs parameters for the core algorithm
    ///
    /// @param  pHALParam The pointer AECEngineHALParam to save the HAL parameters
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetAlgoSetParams(
        AECEngineHALParam* pHALParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishVendorTagMetadata
    ///
    /// @brief  Publishes AEC Vendor tag output to the metadata pool
    ///
    /// @param  pVendorTagOutput                List of vendor tag outputs to be published
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishVendorTagMetadata(
        StatsVendorTagList*             pVendorTagOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateMemoryVendorTag
    ///
    /// @brief  It goes through all vendor tag list and allocate memory to hold the vendor tag data
    ///
    /// @param  pVendorTagInfoList pointer to vendor tag list
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocateMemoryVendorTag(
        StatsVendorTagInfoList* pVendorTagInfoList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishMetadata
    ///
    /// @brief  Publishes all necessary camera metadata to all needed pools
    ///
    /// @param  pOutput                      Pointer to core algorithm's output
    /// @param  pHALParam                    Pointer AECEngineHALParam to save the HAL parameters
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishMetadata(
        AECEngineProcessStatsOutput* pOutput,
        AECEngineHALParam*           pHALParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgoRDIStatsValue
    ///
    /// @brief  Set the RDI statistic data to pBHistStatsOutput
    ///
    /// @param  pRDIStatsOutput      Pointer to the RDI Stats Data Output from sensor
    /// @param  RDIStatsBufferSize   Buffer Size of RDI Stats from sensor
    /// @param  pRDIStatsData        Pointer to the Stats data to be used as algorithm input
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetAlgoRDIStatsValue(
        VOID*              pRDIStatsOutput,
        SIZE_T             RDIStatsBufferSize,
        HDR3ExposureStats* pRDIStatsData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPropertyPoolFrameControl
    ///
    /// @brief  Publishes the algorithm output to control future frames to the main metadata pool
    ///
    /// @param  pEngineOutput   Pointer to core algorithm's output
    /// @param  pFrameControl   AEC Frame control being published
    /// @param  pStatsControl   AEC Stats control being published
    /// @param  ppPeerInfo      Pointer to published AEC peer info
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPropertyPoolFrameControl(
        AECEngineProcessStatsOutput*    pEngineOutput,
        AECFrameControl*                pFrameControl,
        AECStatsControl*                pStatsControl,
        VOID**                          ppPeerInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPropertyPoolAdditionalFrameControl
    ///
    /// @brief  Publishes the algorithm additional output to control future frames to the main metadata pool
    ///
    /// @param  pOutput                      Pointer to core algorithm's output
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPropertyPoolAdditionalFrameControl(
        AECEngineProcessStatsOutput*   pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPropertyPoolFrameInfo
    ///
    /// @brief  Publishes the algorithm output describing the frame to the main metadata pool
    ///
    /// @param  pOutput                     Pointer to core algorithm's output
    /// @param  pHALParam                   Pointer AECEngineHALParam to save the HAL parameters
    /// @param  pFrameInfo                  Pointer to AEC Frame Information for the current frame
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPropertyPoolFrameInfo(
        AECEngineProcessStatsOutput*   pOutput,
        AECEngineHALParam*             pHALParam,
        AECFrameInformation*           pFrameInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishExternalCameraMetadata
    ///
    /// @brief  Publishes the mandated camera metadata
    ///
    /// @param  pOutput                  Pointer to core algorithm's output
    /// @param  pAECHALData              Pointer to core algorithm's output
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishExternalCameraMetadata(
        AECEngineProcessStatsOutput*   pOutput,
        AECHALData*                    pAECHALData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPropertyPoolInternal
    ///
    /// @brief  Publishes the algorithm output to the internal metadata pool
    ///
    /// @param  pOutput                      Pointer to core algorithm's output
    /// @param  pOutputInternal              Pointer to AEC internal output data being publishedd
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPropertyPoolInternal(
        AECEngineProcessStatsOutput*   pOutput,
        AECOutputInternal*             pOutputInternal);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPropertyDebugData
    ///
    /// @brief  Publishes the debug-data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPropertyDebugData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPreRequestOutput
    ///
    /// @brief  Publishes the output for first two slots. This will be done even before the first
    ///         process request comes. The output from stats node will be used by the dependent nodes.
    ///
    /// @param  pUsecasePool Pointer to the usecase pool
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPreRequestOutput(
        MetadataPool* pUsecasePool);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgoBayerGridValue
    ///
    /// @brief  Set the Bayer Grid statistic data to pBayerGrid for the algorithm to consume
    ///
    /// @param  pISPHDRStats    Pointer to the internal property blob to read from
    /// @param  pHDRBEOutput    Pointer to the internal property blob to read from
    /// @param  pBayerGrid      The Bayer Grid statistic data to set the values to
    /// @param  pBayerGridROI   The Bayer Grid ROI were the statistics are provided
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetAlgoBayerGridValue(
    PropertyISPHDRBEStats*  pISPHDRStats,
    ParsedHDRBEStatsOutput* pHDRBEOutput,
    StatsBayerGrid*         pBayerGrid,
    StatsRectangle*         pBayerGridROI);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgoBayerHDRBHistValue
    ///
    /// @brief  Set the Bayer Histogram statistic data for the algorithm to consume
    ///
    /// @param  pHDRBHistStatsOutput    Pointer to the ParsedHDRBhistStatsOutput to read from
    /// @param  pBayerHistogram         Pointer to the Bayer Histogram statistic data to write to as algorithm input
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetAlgoBayerHDRBHistValue(
        ParsedHDRBHistStatsOutput*  pHDRBHistStatsOutput,
        StatsBayerHist*             pBayerHistogram);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgoBayerHistValue
    ///
    /// @brief  Set the Bayer Histogram statistic data to pBHistStatsOutput
    ///
    /// @param  pBHistStatsOutput    Pointer to the ParsedBHistStatsOutput from ISP
    /// @param  pBayerHistogram      Pointer to the Bayer Histogram statistic data to be used as algorithm input
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetAlgoBayerHistValue(
        ParsedBHistStatsOutput*  pBHistStatsOutput,
        StatsBayerHist*          pBayerHistogram);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAlgoImageHistValue
    ///
    /// @brief  Set the Bayer Histogram statistic data to pIHistStatsOutput
    ///
    /// @param  pIHistStatsOutput    Pointer to the ParsedIHistStatsOutput from ISP
    /// @param  pImageHistogram      Pointer to the Image Histogram statistic data to be used as algorithm input
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetAlgoImageHistValue(
        ParsedIHistStatsOutput*  pIHistStatsOutput,
        StatsIHist*              pImageHistogram);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulatePropPoolFrameControl
    ///
    /// @brief  Populates the algorithm output for the frame control on main metadata pool
    ///
    /// @param  pOutput                 Pointer to core algorithm's output
    /// @param  pFrameControl           Pointer to AEC frame control within the main metadata pool
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulatePropPoolFrameControl(
        AECEngineProcessStatsOutput* pOutput,
        AECFrameControl*             pFrameControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulatePropPoolPeerControl
    ///
    /// @brief  Populates the algorithm output for the Peer Info control on main metadata pool
    ///
    /// @param  pOutput         Pointer to core algorithm's output
    /// @param  ppPeerInfo      Pointer to Pointer of Peer Info.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulatePropPoolPeerControl(
        AECEngineProcessStatsOutput* pOutput,
        VOID**                       ppPeerInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulatePropPoolStatsControl
    ///
    /// @brief  Populates the algorithm output for the Stats control on main metadata pool
    ///
    /// @param  pOutput         Pointer to core algorithm's output
    /// @param  pFrameControl   Pointer to AEC Stats control within the main metadata pool
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulatePropPoolStatsControl(
        AECEngineProcessStatsOutput* pOutput,
        AECStatsControl*             pFrameControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulatePropPoolFrameInfo
    ///
    /// @brief  Populates the algorithm output for the frame information on main metadata pool
    ///
    /// @param  pOutput     Pointer to core algorithm's output
    /// @param  pFrameInfo  Pointer to AEC frame information within the main metadata pool
    /// @param  pHALParam   Pointer AECEngineHALParam to save the HAL parameters
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulatePropPoolFrameInfo(
        AECEngineProcessStatsOutput* pOutput,
        AECFrameInformation*         pFrameInfo,
        AECEngineHALParam*           pHALParam);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulatePropPoolInternal
    ///
    /// @brief  Populates the algorithm output to the internal metadata pool
    ///
    /// @param  pOutput         Pointer to core algorithm's output
    /// @param  pInternalOutput Pointer to AEC internal output
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulatePropPoolInternal(
        AECEngineProcessStatsOutput* pOutput,
        AECOutputInternal*           pInternalOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadHALAECParam
    ///
    /// @brief  Read the HAL's input for AEC
    ///
    /// @param  pHALParam                    Pointer for AECEngineHALParam to save the HAL parameters
    /// @param  pStatsProcessRequestDataInfo Pointer to Stats Process Request Data Info
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadHALAECParam(
       AECEngineHALParam*             pHALParam,
       const StatsProcessRequestData* pStatsProcessRequestDataInfo
       ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveFlashStatsInfo
    ///
    /// @brief  retrieves flash info for the stats of capture frame influenced by main flash
    ///
    /// @param  pStatsProcessRequestDataInfo             Pointer to Stats Process Request Data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RetrieveFlashStatsInfo(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureAESyncLockParam
    ///
    /// @brief  Set AE Sync lock if advertised by the pipeline for the particular request
    ///
    /// @param  pHALParam   pointer for AECEngineHALParam to save the HAL parameters
    /// @param  requestID   Current Request ID
    /// @param  role        Current Stats Algo role
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureAESyncLockParam(
        AECEngineHALParam*             pHALParam,
        UINT64                         requestID,
        StatsAlgoRole                  role);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHardwareInfo
    ///
    /// @brief  Fills in the hardware information
    ///
    /// @param  pHardwareInfo       The hardware capabilities information to be filled in
    /// @param  pHardwareContext    The hardware context
    /// @param  cameraID            The camera ID for the sensor in interest
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetHardwareInfo(
        AECEngineHWInfo*    pHardwareInfo,
        HwContext*          pHardwareContext,
        INT32               cameraID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPeerInfo
    ///
    /// @brief  Fills in the Peer information
    ///
    /// @param  pStatsProcessRequestDataInfo  Pointer to Stats Process Request Data Info
    /// @param  ppPeerInfo                    Pointer of Pointer of Peer Information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPeerInfo(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        VOID**                         ppPeerInfo
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCropWindow
    ///
    /// @brief  Fills in the stats crop window
    ///
    /// @param  pCropWindow     The stats crop window
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCropWindow(
        StatsRectangle* pCropWindow
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateTraceEvents
    ///
    /// @brief  Updates the trace events based on the processed stats output
    ///
    /// @param  pProcessStatsOutput  The output of processed stats
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateTraceEvents(
        AECEngineProcessStatsOutput* pProcessStatsOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetStatsConfigFromAlgoConfig
    ///
    /// @brief  This method sets the complete stats configuration based on algorithm's stats configuration
    ///
    /// @param  pAlgoStatsConfig       Statistics configuration received from the algorithm
    /// @param  pAlgoBHISTStatsConfig  BHIST Statistics configuration received from the algorithm
    /// @param  pStatsConfig           Output statistics configuration to be set for
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetStatsConfigFromAlgoConfig(
        StatsBayerGridBayerExposureConfig*  pAlgoStatsConfig,
        StatsBayerHistogramConfig*          pAlgoBHISTStatsConfig,
        AECConfig*                          pStatsConfig) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetTouchROISettings
    ///
    /// @brief  Read the HAL's input for best shot mode and configure settigns to AEC
    ///
    /// @param  pHALParam     The pointer AECEngineHALParam to save the HAL parameters
    /// @param  pTouchROIInfo The pointer WeightedRectangle to save the HAL parameters
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetTouchROISettings(
        AECEngineHALParam*      pHALParam,
        WeightedRectangle*      pTouchROIInfo
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFaceROISettings
    ///
    /// @brief  Read the HAL's input for best shot mode and configure settigns to AEC
    ///
    /// @param  pHALParam    The pointer AECEngineHALParam to save the HAL parameters
    /// @param  pFaceROIInfo The pointer PropertyFaceROIInfo to save the HAL parameters
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetFaceROISettings(
        AECEngineHALParam*     pHALParam,
        FaceROIInformation*    pFaceROIInfo
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetTrackerROISettings
    ///
    /// @brief  Read the HAL's input for best shot mode and configure settigns to AEC
    ///
    /// @param  pHALParam       The pointer AECEngineHALParam to save the HAL parameters
    /// @param  pTrackerROIInfo The pointer PropertyTrackerROIInfo to save the HAL parameters
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetTrackerROISettings(
        AECEngineHALParam*        pHALParam,
        TrackerROIInformation*    pTrackerROIInfo
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBestShotModeSettings
    ///
    /// @brief  Read the HAL's input for best shot mode and configure settigns to AEC
    ///
    /// @param  pHALParam The pointer AECEngineHALParam to save the HAL parameters
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetBestShotModeSettings(
        AECEngineHALParam* pHALParam
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlashInfoType
    ///
    /// @brief  Convert the Algo flash type (PRE/MAIN) to AEC Property pool type
    ///
    /// @param  flashState Flash type passed from Algo
    ///
    /// @return flashType converted to property pool type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AECFlashInfoType GetFlashInfoType(
        AECAlgoFlashStateType flashState
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetISOExpPriorityValue
    ///
    /// @brief  Set ISO or Exposure Time priority based on priority param from HAL
    ///
    /// @param  pHALParam Pointer to AECEngineHALParam where the function retrieves the ISO/Exposure Time priority
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetISOExpPriorityValue(
        AECEngineHALParam* pHALParam
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadAFDMode
    ///
    /// @brief  Read the HAL's Anti Banding Mode input and AFD output and configure AFD mode to AEC
    ///
    /// @param  pHALParam The pointer AECEngineHALParam to save the HAL parameters
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadAFDMode(
        AECEngineHALParam* pHALParam
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadStatsNodesUpdates
    ///
    /// @brief  Gets the updates from other Nodes: AF, AWB, AFD, ASD for previous frame
    ///
    /// @param  pAECNodesUpdate  Pointer to AEC input structure
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReadStatsNodesUpdates(
        AECEngineNodesUpdate* pAECNodesUpdate);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeReadProperties
    ///
    /// @brief  Init and set AEC Read Properties
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeReadProperties();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeWriteProperties
    ///
    /// @brief  Init and set AEC Write Properties
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeWriteProperties();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeVendorTagPublishList
    ///
    /// @brief  Init and set AEC vendor tag list
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeVendorTagPublishList();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetInitialTuning
    ///
    /// @brief  Set intial tuning Mode.
    ///
    /// @param  pInitializeData Pointer to initial settings
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetInitialTuning(
        const StatsInitializeData* pInitializeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadInputVendorTag
    ///
    /// @brief  Read the input Vendor tags for AEC
    ///
    /// @param  pHALParam Pointer for AECEngineHALParam to save the HAL parameters
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReadInputVendorTag(
        AECEngineHALParam*             pHALParam)const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSeamlessInSensorHDR3ExpSnapshot
    ///
    /// @brief  Get the in-sensor HDR 3 exposure start / enabled state
    ///
    /// @return return TRUE if seamless in-sensor HDR 3 exposure is enalbed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSeamlessInSensorHDR3ExpSnapshot()const
    {
        return ((SeamlessInSensorState::InSensorHDR3ExpStart   == m_HALParam.seamlessInSensorState) ||
                (SeamlessInSensorState::InSensorHDR3ExpEnabled == m_HALParam.seamlessInSensorState) ||
                (SeamlessInSensorState::InSensorHDR3ExpStop    == m_HALParam.seamlessInSensorState))? TRUE: FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGyroDataFromNCS
    ///
    /// @brief  Get the Gyro data for AEC from NCS
    ///
    /// @param  pInput  Pointer for AECCommandInputParam to pass into the function and bring Gyro data out
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetGyroDataFromNCS(
        AECCommandInputParam* pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpAECStats
    ///
    /// @brief  Dumps AEC stats to log
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpAECStats();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpFrameInfo
    ///
    /// @brief  Dumps AEC frame info to log
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpFrameInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpHALData
    ///
    /// @brief  Dumps AEC HAL data to log file
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpHALData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OverrideInSensorHDR3ExpOutputMeta
    ///
    /// @brief  Override the in-sensor HDR 3 exposure output meta while taking seamless snapshot
    ///
    /// @param  pFrameControlOut Pointer to frame control meta
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID OverrideInSensorHDR3ExpOutputMeta(
        AECFrameControl*    pFrameControlOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CanSkipAEProcessing
    ///
    /// @brief  Checks if algo processing can be skipped
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return TRUE if algo processing can be skipped, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CanSkipAEProcessing(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DetermineStabilizationMargins
    ///
    /// @brief  Get the stabilization margins depending on IFE crop dimension and video/preview dimension
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DetermineStabilizationMargins();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateWindowForStabilization
    ///
    /// @brief  Updates the stats ROI window after applying stabilization margin
    ///
    /// @param  pStatsROI stats window ROI
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateWindowForStabilization(
        StatsRectangle* pStatsROI) const;

    CAECStatsProcessor(const CAECStatsProcessor&)               = delete;   ///< Do not implement copy constructor
    CAECStatsProcessor& operator=(const CAECStatsProcessor&)    = delete;   ///< Do not implement assignment operator

    Node*                        m_pNode;               ///< Pointer to owning StatsProcessor node
    MetadataPool*                m_pDebugDataPool;      ///< Pointer to debugdata metadata pool
    CAECEngine*                  m_pAECEngine;          ///< Pointer to CAECEngine instance
    AECEngineHWInfo              m_hardwareInfo;        ///< AEC engine hardware info
    const StaticSettings*        m_pStaticSettings;     ///< Camx Static settings
    CAECGyro                     m_AECGyro;             ///< Gyro Data for AEC

    INT                          m_lastSettledState;    ///< Indicate if AEC is settled in last request
    AECEngineAlgorithmInput      m_algoInput;           ///< Input for algorithm when processing the data
    AECEngineProcessStatsOutput  m_engineStatsOutput;   ///< Output used by AEC Engine to save the output from algorithm
    CREATEAEC                    m_pfnCreate;           ///< Function Pointer to create Algorithm instance

    StatsVendorTagInfoList      m_vendorTagInputList;           ///< List of Stats vendor tag dependency
    StatsVendorTagInfoList      m_vendorTagInfoOutputputList;   ///< List of Algo's output vendor tags
    StatsVendorTagList          m_algoVendorTagOutputList;      ///< Pointer to Algo VendorTagOutputlist
    StatsTuningData             m_tuningData;                   ///< Tuning data set
    UINT64                      m_currProcessingRequestId;      ///< Current processing request Id
    BOOL                        m_skipProcessing;               ///< algo processing skip flag
    BOOL                        m_isFixedFocus;                 ///< Fixed focus camera flag
    UINT                        m_cameraId;                     ///< Camera Id
    AECCommandOutputParam       m_outputParam;                  ///< Algo output param
    AECEngineHALParam           m_HALParam;                     ///< input HAL Param
    const StatsInitializeData*  m_pStatsInitializeData;         ///< Pointer to initial settings

    StatsPropertyReadAndWrite*  m_pStatsAECPropertyReadWrite;                       ///< Stats Readwrite instance for AEC
    AECFrameControl             m_frameControl;                                     ///< AEC frame control
    AECStatsControl             m_statsControl;                                     ///< AEC stats control
    VOID*                       m_pPeerInfo;                                        ///< Peer info
    AECFrameInformation         m_frameInfo;                                        ///< AEC frame info
    AECOutputInternal           m_outputInternal;                                   ///< AEC internal output
    ReadProperty                m_AECReadProperties[AECReadTypePropertyIDCount];    ///< AEC Read Properties
    WriteProperty               m_AECWriteProperties[NumAECPropertyWriteTags];      ///< AEC Write Properties
    AECHALData                  m_aeHALData;                                        ///< AEC HAL Data
    HDRTypeValues               m_HDRType;                                          ///< HDR type
    StatsHDR3ExposureDataType*  m_pRDIStatsDataBuffer;                              ///< RDI HDR stats data buffer after parsed
    HDR3ExposureType            m_HDR3ExposureType;                                 ///< flag of 3-Exposure HDR type
    StatsParseFuncPtr_t         m_pfnStatsParse;                                    ///< pointer to sensor stats parse function
    BOOL                        m_isZZHDRSupported;                                 ///< ZZHDR mode supported
    BOOL                        m_isSHDRSupported;                                  ///< SHDR mode supported
    FLOAT                       m_prevLuxIndex;                                     ///< Lux index of last non in-sensor preview
    FLOAT                       m_compenADRCGain;                                   ///< Preivous Compensation ADRC gain
    BOOL                        m_isInSensorHDR3ExpSnapshot;                        ///< Preivous flag for in-sensor HDR 3 exp
    InSensorHDR3ExpTriggerInfo  m_prevInSensorHDR3ExpTriggerInfo;                   ///< Preivous in-sensor HDR 3 exp info
    FLOAT                       m_aecSensitivity[ExposureIndexCount];               ///< Sensitivity for publish
    UINT64                      m_aecExposureTime[ExposureIndexCount];              ///< Exposure time for publish
    FLOAT                       m_aecLinearGain[ExposureIndexCount];                ///< Linear Gain for publish
    FLOAT                       m_aecLuxIndex;                                      ///< Lux index for publish
    SensorRegisterControl       m_sensorControl;                                    ///< Sensor register control
    DebugData                   m_debugData;                                        ///< DebugData for publish
    PublishVendorTag            m_aecVendorTagsPublish[NumAECVendorTagsPublish];    ///< AEC vendor tag publish list
    AECAlgoDCCalibrationInfo    m_dcCalibrationInfo;                                ///< Dual Camera OTP Calibration data coming
                                                                                    ///< from sensor
    AECAlgoROI                  m_aecProcessedROI;                                  ///< ROI considered for processing
    StabilizationMargin         m_stabilizationMargin;                              ///< Holds the current stabilization margin
};

CAMX_NAMESPACE_END

#endif // CAMXCAECSTATSPROCESSOR_H
