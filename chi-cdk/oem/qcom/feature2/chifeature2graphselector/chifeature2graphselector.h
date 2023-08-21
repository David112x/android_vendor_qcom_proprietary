////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graphselector.h
/// @brief CHI feature graph selector base class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2GRAPHSELECTOR_H
#define CHIFEATURE2GRAPHSELECTOR_H

// NOWHINE FILE PR007b: Whiner incorrectly concludes as non-library files
// NOWHINE FILE CP006: used standard libraries for performance improvements
// NOWHINE FILE NC004c: CHI files wont be having Camx
// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases
// NOWHINE FILE CP004:  Need comparator for map
// NOWHINE FILE GR017:  Need comparator for map
// NOWHINE FILE CP008:  Need comparator for map

#include <set>
#include <unordered_map>

#include "chxincs.h"
#include "chxextensionmodule.h"
#include "chxusecaseutils.h"
#include "chxutils.h"
#include "camxcdktypes.h"
#include "chicommon.h"

#include "chifeature2types.h"
#include "chifeature2requestobject.h"
#include "chifeature2graph.h"
#include "chifeature2graphdescriptors.h"
#include "chifeature2featurepool.h"

extern UINT32 g_enableChxLogs;
extern BOOL   g_logRequestMapping;
extern BOOL   g_enableSystemLog;

// NOWHINE FILE NC004c: CHI files wont be having Camx
// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases

static const UINT32 MaxNumberOfGraphs       = 30;
static const UINT32 MaxRequestQueueDepth    = 24;
static const UINT8  DefaultStageSequences   = 1;

static const UINT SINGLE_CAMERA = 50;
static const UINT MULTI_CAMERA  = 51;
static const UINT BOKEH_CAMERA  = 52;
static const UINT FUSION_CAMERA = 53;

/// @brief ControlCaptureIntent
enum ControlCaptureIntentValues
{
    ControlCaptureIntentCustom,                   ///< Capture Intent custom
    ControlCaptureIntentPreview,                  ///< Capture Intent preview
    ControlCaptureIntentStillCapture,             ///< Capture Intent still capture
    ControlCaptureIntentVideoRecord,              ///< Capture Intent video record
    ControlCaptureIntentVideoSnapshot,            ///< Capture Intent video snapshot
    ControlCaptureIntentZeroShutterLag,           ///< Capture Intent zsl
    ControlCaptureIntentManual,                   ///< Capture Intent manual
    ControlCaptureIntentEnd                       ///< Capture Intent end
};

/// @brief NoiseReductionMode
enum NoiseReductionModeValues
{
    NoiseReductionModeOff,                        ///< Noise reduction mode off
    NoiseReductionModeFast,                       ///< Noise reduction mode fast
    NoiseReductionModeHighQuality,                ///< Noise reduction mode high quality
    NoiseReductionModeMinimal,                    ///< Noise reduction mode minimal
    NoiseReductionModeZeroShutterLag,             ///< Noise reduction mode zsl
    NoiseReductionModeEnd                         ///< Noise reduction mode end
};

/// @brief ControlSceneMode
enum ControlSceneModeValues
{
    ControlSceneModeDisabled = 0,                 ///< Control Scene mode disabled
    ControlSceneModeFacePriority,                 ///< Control Scene mode Face Priority
    ControlSceneModeAction,                       ///< Control Scene mode Action
    ControlSceneModePortrait,                     ///< Control Scene mode Portrait
    ControlSceneModeLandscape,                    ///< Control Scene mode Landscape
    ControlSceneModeNight,                        ///< Control Scene mode Night
    ControlSceneModeNightPortrait,                ///< Control Scene mode Night Portrait
    ControlSceneModeTheatre,                      ///< Control Scene mode Theatre
    ControlSceneModeBeach,                        ///< Control Scene mode Beach
    ControlSceneModeSnow,                         ///< Control Scene mode Snow
    ControlSceneModeSunset,                       ///< Control Scene mode Sunset
    ControlSceneModeSteadyphoto,                  ///< Control Scene mode Steady photo
    ControlSceneModeFireworks,                    ///< Control Scene mode Fireworks
    ControlSceneModeSports,                       ///< Control Scene mode Sports
    ControlSceneModeParty,                        ///< Control Scene mode Party
    ControlSceneModeCandlelight,                  ///< Control Scene mode Candle light
    ControlSceneModeBarcode,                      ///< Control Scene mode Barcode
    ControlSceneModeHighSpeedVideo,               ///< Control Scene mode High Speed Video
    ControlSceneModeHDR,                          ///< Control Scene mode Hdr
    ControlSceneModeFacePriorityLowLight,         ///< Control Scene mode Face Priority Low Light
    ControlSceneModeEnd                           ///< Control Scene mode end
};

/// @brief CustomVendorTagValues
enum CustomVendorTagValues
{
    CustomVendorTagDefault         = 0,           ///< Custom Vendor Tag values
    // 1-99 is reserved for oem
    CustomVendorTagRawReprocessing = 100,         ///< Reprocessing is done on RAW frame
    CustomVendorTagBurst           = 101,         ///< Burst shot
    CustomVendorTagMFNR            = 102,         ///< Vendor tag to enable MFNR/MFSR
};

extern const ChiFeature2GraphDesc Bayer2YUVFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc MultiCameraFusionFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc MultiCameraBokehFeatureSuperGraphDescriptor;
extern const ChiFeature2GraphDesc MultiCameraHDRFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc MultiCameraMFNRFusionFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc MultiCameraBokehFeatureSuperGraphDescriptor;
extern const ChiFeature2GraphDesc MultiCameraFusionFeatureSuperGraphDescriptor;
extern const ChiFeature2GraphDesc RealTimeFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTBayer2YUVJPEGFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc MFSRFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTMFSRJPEGFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc MFNRFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTMFNRJPEGFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTBayer2YUVHDRT1JPEGFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTBayer2YUVHDRT1JPEGNativeResolutionFGDescriptor;
extern const ChiFeature2GraphDesc RTBayer2YUVSWMFJPEGFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTBayer2YUVFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTMFSRYUVFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTMFNRYUVFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTMemcpyYUVFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTMFSRHDRT1JPEGFeatureGraphDescriptor;
extern const ChiFeature2GraphDesc RTMFNRHDRT1JPEGFeatureGraphDescriptor;

extern const ChiFeature2Descriptor HDRT1FeatureDescriptor;
extern const ChiFeature2Descriptor Bayer2YuvFeatureDescriptor;
extern const ChiFeature2Descriptor RealTimeFeatureDescriptor;
extern const ChiFeature2Descriptor RealTimeFeatureWithSWRemosaicDescriptor;
extern const ChiFeature2Descriptor StubFeatureDescriptor;
extern const ChiFeature2Descriptor JPEGFeatureDescriptor;
extern const ChiFeature2Descriptor MFSRFeatureDescriptor;
extern const ChiFeature2Descriptor MFNRFeatureDescriptor;
extern const ChiFeature2Descriptor AnchorSyncFeatureDescriptor;
extern const ChiFeature2Descriptor DemuxFeatureDescriptor;
extern const ChiFeature2Descriptor SWMFFeatureDescriptor;
extern const ChiFeature2Descriptor RawHDRFeatureDescriptor;
extern const ChiFeature2Descriptor BokehFeatureDescriptor;
extern const ChiFeature2Descriptor FusionFeatureDescriptor;
extern const ChiFeature2Descriptor FrameSelectFeatureDescriptor;
extern const ChiFeature2Descriptor MemcpyFeatureDescriptor;
extern const ChiFeature2Descriptor JPEGFeatureDescriptorGPU;
extern const ChiFeature2Descriptor SerializerFeatureDescriptor;
extern const ChiFeature2Descriptor FormatConvertorFeatureDescriptor;

typedef std::set<std::pair<UINT, std::set<const CHAR *>>> cameraIdNameSet;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2WrapperCallbacks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ChiFeature2WrapperCallbacks
{
    /// @brief FGM callback method to send back the capture results to chi usecase
    void(*ProcessCaptureResultCbToUsecase)(
        CHICAPTURERESULT*   pCaptureResult,
        VOID*               pPrivateCallbackData);

    /// @brief Asynchronous notification callback method from FGM
    CDKResult(*NotifyMessageToUsecase)(
        const ChiFeature2Messages*  pMessageDesc,
        VOID*                       pPrivateCallbackData);

    /// @brief FGM callback method to send back the partial capture results to the chi usecase
    void(*ProcessPartialCaptureResultCbToUsecase)(
        CHIPARTIALCAPTURERESULT* pCaptureResult,
        VOID*                    pPrivateCallbackData);

    VOID* pPrivateCallbackData; ///< The data passed back to the feature wrapper during a callback
};

/// @brief feature graph configuration
struct FeatureGraphManagerConfig
{
    ChiStreamConfigInfo*           pCameraStreamConfig;         ///< camera3 stream configuration
    LogicalCameraInfo*             pCameraInfo;                 ///< Logical camera info
    VOID*                          pSessionParams;              ///< Session parameters
    ChiFeature2WrapperCallbacks*   pFeatureWrapperCallbacks;    ///< Feature graph manager callbacks
    const ChiUsecase*              pUsecaseDescriptor;          ///< Pointer to usecase XML
    ChiMetadataManager*            pMetadataManager;            ///< Metadata manager for the usecase
    UINT32                         inputOutputType;             ///< Config stream input out type
};

/// @brief Keys to select list of feature graphs for given stream configuration
struct ChiFeature2FGDKeysForClonedMap
{
    UINT                           cameraId;                ///< Physical camera Id
    const CHAR*                    pDescriptorName;         ///< name of the feature graph descriptor
    ChiFeature2InstanceFlags       featureFlags;            ///< feature2 flags
    BOOL                           bMultiCamera;            ///< Flag to indicate if it is multi camera graph
};

struct FeatureGraphSelectorConfig
{
    UINT               numFeatureGraphDescriptors; ///< Number of FG descriptors
    ChiFeature2NameMap featureNameToOpsMap;        ///< feature Id map
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsChiFeature2KeysLess
///
/// @brief  Returns TRUE if FGDKeysForClonedMap LHS is less then RHS
///
/// @param  rLHS The left-hand  FGDKeysForClonedMap to compare
/// @param  rRHS The Right-hand FGDKeysForClonedMap to compare
///
/// @return TRUE if ChiFeature2Props LHS is less then RHS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsChiFeature2KeysLess(
    const ChiFeature2FGDKeysForClonedMap& rLHS,
    const ChiFeature2FGDKeysForClonedMap& rRHS)
{
    BOOL isLHSLessThanRHS = FALSE;

    if (rLHS.cameraId < rRHS.cameraId)
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.cameraId == rRHS.cameraId) &&
             (0 < CdkUtils::StrCmp(rLHS.pDescriptorName, rRHS.pDescriptorName)))
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.cameraId == rRHS.cameraId) &&
             (!CdkUtils::StrCmp(rLHS.pDescriptorName, rRHS.pDescriptorName)) &&
             (rLHS.featureFlags.value < rRHS.featureFlags.value))
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.cameraId == rRHS.cameraId) &&
             (!CdkUtils::StrCmp(rLHS.pDescriptorName, rRHS.pDescriptorName)) &&
             (rLHS.featureFlags.value == rRHS.featureFlags.value) &&
             (rLHS.bMultiCamera < rRHS.bMultiCamera))
    {
        isLHSLessThanRHS = TRUE;
    }

    return isLHSLessThanRHS;
}

/// @brief Comparator function to determine whether FGDKeysForClonedMap LHS is less than RHS
struct ChiFeature2KeysLessComparator :
    public std::binary_function<ChiFeature2FGDKeysForClonedMap, ChiFeature2FGDKeysForClonedMap, bool>
{
    bool operator()(const ChiFeature2FGDKeysForClonedMap LHS, const ChiFeature2FGDKeysForClonedMap RHS) const
    {
        return IsChiFeature2KeysLess(LHS, RHS);
    }
};

/// @brief Keys to select particular feature graph for given capture request
struct FGDKeysForTable
{
    const CHAR*      pDescriptorName;                ///< Feature Descriptor Name
    UINT32           cameraId;                       ///< camera Id for just mapping
    std::set<UINT32> captureIntent;                  ///< Capture Intent
    std::set<UINT32> sceneMode;                      ///< Scene mode camera_metadata_tag typecasted
    std::set<UINT32> noiseReductionMode;             ///< Noise reduction mode camera_metadata_tag typecasted
    std::set<UINT32> customVendorTag;                ///< Custom vendor tag
    UINT             opsMode;                        ///< ops mode - HFR or not
};

/// @brief Keys to select particular feature graph for given capture request
struct FGDKeysForCaptureRequest
{
    UINT32                          cameraId;           ///< Physical camera Id
    UINT32                          captureIntent;      ///< Capture Intent
    UINT32                          sceneMode;          ///< Scene mode camera_metadata_tag typecasted
    UINT32                          noiseReductionMode; ///< Noise reduction mode camera_metadata_tag typecasted
    UINT32                          customVendorTag;    ///< Custom vendor tag
    UINT32                          physicalCameraId;   ///< Physical Camera ID
    ChiFeature2InstanceFlags        flags;              ///< feature flags
    const Feature2ControllerResult* pMCCResult;         ///< MCC result
};

/// @brief List of all the graph descriptor selection tables
struct GraphDescriptorTables
{
    cameraIdNameSet*                              pCameraIdDescriptorNameSet;    ///< cameraId Descriptor name set
    std::map<const CHAR *, ChiFeature2GraphDesc>* pFeatureGraphDescriptorsMap;   ///< graph descriptor name to descriptor map
    std::vector<FGDKeysForTable>*                 pFeatureGraphDescKeysMap;      ///< keys to descriptor map
};

typedef std::map<ChiFeature2FGDKeysForClonedMap, ChiFeature2GraphDesc*, ChiFeature2KeysLessComparator> keysToCloneDescMap;
typedef std::map<CHAR*, ChiFeature2GraphDesc*, ChiFeature2StringLessComparator> pruneKeyToGraphDescMap;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature graph selector base class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiFeature2GraphSelector
{

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SelectFeatureGraphforRequest
    ///
    /// @brief  Method to create an instance of the feature graph descriptor per physical device.
    ///
    /// @param  pChiUsecaseRequestObject Usecase request object
    /// @param  pMetadataFrameNumberMap  result metadata map for each frame number
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ChiFeature2GraphDesc* SelectFeatureGraphforRequest(
        ChiFeature2UsecaseRequestObject* pChiUsecaseRequestObject,
        std::map<UINT32, ChiMetadata*>   pMetadataFrameNumberMap);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeatureGraphMapforConfig
    ///
    /// @brief  method to get a feature graph on a request basis
    ///
    /// @param  pConfig                 Config Information to get list of feature graph.
    /// @param  rSelectorOutput         feature graph selector config data
    /// @param  pGraphDescriptorTables  All the graph tables required to pick graph descriptors
    ///
    /// @return ChiFeature2GraphDesc Map of feature graphs supported for this config
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual keysToCloneDescMap GetFeatureGraphMapforConfig(
        FeatureGraphManagerConfig*  pConfig,
        FeatureGraphSelectorConfig& rSelectorOutput,
        GraphDescriptorTables*      pGraphDescriptorTables);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddCustomFeatureGraphNodeHints
    ///
    /// @brief  Method to create feature graph instance on a request basis
    ///
    /// @param  pRequestObject              Usecase Request Object
    /// @param  rFeatureInstanceReqInfoList List of enabled features and associated request info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AddCustomFeatureGraphNodeHints(
        ChiFeature2UsecaseRequestObject*             pRequestObject,
        std::vector<ChiFeature2InstanceRequestInfo>& rFeatureInstanceReqInfoList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCaptureIntentForSnapshot
    ///
    /// @brief  Method to check if capture intent is for snapshot
    ///
    /// @param  pKeysForRequest     Information about the Feature graphs.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsCaptureIntentForSnapshot(
        const FGDKeysForCaptureRequest* pKeysForRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Method to destroy.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump
    ///
    /// @brief  Dump logs
    ///
    /// @param  pGraphDescriptorTables table pointer.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Dump(
        GraphDescriptorTables* pGraphDescriptorTables);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGraphDescriptorTables
    ///
    /// @brief  Method to get the set of all tables required to select graph descriptor.
    ///
    /// @return return the map of keys to graph descriptor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    GraphDescriptorTables* GetGraphDescriptorTables()
    {
        m_graphDescriptorTables.pCameraIdDescriptorNameSet  = &m_cameraIdDescriptorNameSet;
        m_graphDescriptorTables.pFeatureGraphDescKeysMap    = &m_FDGKeysMap;
        m_graphDescriptorTables.pFeatureGraphDescriptorsMap = &m_featureGraphDescriptorsMap;

        return &m_graphDescriptorTables;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateInstancePropsForGraph
    ///
    /// @brief  Method to update camera id and intance id for multi camera cloned graph.
    ///
    /// @param  pGraphDesc  The pointer of graph descriptor
    /// @param  instanceId  The instanceId of the graph
    /// @param  cameraId    The camera index of the graph
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateInstancePropsForGraph(
        ChiFeature2GraphDesc* pGraphDesc,
        UINT32                instanceId,
        UINT32                cameraId);
protected:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initializes the feature graph selector base & trigger derived feature's initialization.
    ///
    /// @param  pConfig  Config such as stream config, system settings, session parameters, sensor config.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        FeatureGraphManagerConfig* pConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateAllTables
    ///
    /// @brief  Method to populate the keys map, <cameraid, descriptorName>.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateAllTables();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SelectFeatureGraphforRequestFromTable
    ///
    /// @brief  Method to create an instance of the feature graph descriptor per physical device.
    ///
    /// @param  pChiUsecaseRequestObject Usecase request object
    /// @param  pMetadataFrameNumberMap  result metadata map for each frame number
    /// @param  pGraphDescriptorTables   all the tables required to select a feature graph
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphDesc* SelectFeatureGraphforRequestFromTable(
        ChiFeature2UsecaseRequestObject* pChiUsecaseRequestObject,
        std::map<UINT32, ChiMetadata*>   pMetadataFrameNumberMap,
        GraphDescriptorTables*           pGraphDescriptorTables);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// VerifyAndSetStageSequenceHint
    ///
    /// @brief  Method to set the stage sequence hint. This function verifies that we have set the m_stageSequenceHint
    ///         vector correctly. If definining a custom usecase, must use flowtype1 to execute request or mimic its
    ///         logic. Size of m_stageSequenceHint is assumed to be the number of times we want to repeat the stage.
    ///         Each entry in the vector represents the number of frames we will request for that sequence of the stage.
    ///         This means the vector entries must add up to equal the the number of frames requested for the entire request
    ///
    /// @param  pHint   The hint for the request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID VerifyAndSetStageSequenceHint(
        ChiFeature2Hint* pHint);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPruneRulesAndIdentifier
    ///
    /// @brief  Method to get prune rules to prune feature graph and identifier for caching
    ///
    /// @param  pKeysForRequest     Information about the Feature graphs.
    /// @param  rOutPruneRules      Output prune rules
    /// @param  pOutPruneIDString   Output identifier string to use for pruned descriptor caching
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult GetPruneRulesAndIdentifier(
        const FGDKeysForCaptureRequest*         pKeysForRequest,
        std::vector<ChiFeature2PruneRule>&      rOutPruneRules,
        CHAR*                                   pOutPruneIDString);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2GraphSelector
    ///
    /// @brief  Default constructor.
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphSelector() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2GraphSelector
    ///
    /// @brief  Destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFeature2GraphSelector();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BuildCameraIdSet
    ///
    /// @brief  Method to populate the cameraId map
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID BuildCameraIdSet();

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CloneFeatureGraphDescriptor
    ///
    /// @brief  Method to clone feature graph descriptor.
    ///
    /// @param  pFeatureGraphDescriptor Information about the Feature graphs to be cloned.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphDesc* CloneFeatureGraphDescriptor(
        const ChiFeature2GraphDesc* pFeatureGraphDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPrunedFeatureGraph
    ///
    /// @brief  Method to prune super graph to different graph based on some keys, such as scene mode, noise level, mcc.
    ///
    /// @param  pFeatureGraphDescriptor Parent feature graph need to be pruned.
    /// @param  pKeysForRequest         Information about the Feature graphs to be pruned.
    ///
    /// @return The pointer of pruned feature graph
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphDesc* GetPrunedFeatureGraph(
        const ChiFeature2GraphDesc*     pFeatureGraphDescriptor,
        const FGDKeysForCaptureRequest* pKeysForRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCaptureIntentForRequest
    ///
    /// @brief  Get Capture Intent based on the input app metadata.
    ///
    /// @param  pAppSettings  App metadata.
    /// @param  pChiRequest   Pointer to the ChiCaptureRequest of this usecase request object.
    ///
    /// @return return the capture intent value.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetCaptureIntentForRequest(
        ChiMetadata*       pAppSettings,
        CHICAPTUREREQUEST* pChiRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNoiseReductionMode
    ///
    /// @brief  Get Noise reduction mode based on the input app metadata.
    ///
    /// @param  pAppSettings  App metadata.
    ///
    /// @return return the capture intent value.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetNoiseReductionMode(
        ChiMetadata* pAppSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSceneMode
    ///
    /// @brief  Get Capture Intent based on the input app metadata.
    ///
    /// @param  pAppSettings  App metadata.
    ///
    /// @return return the capture intent value.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetSceneMode(
        ChiMetadata* pAppSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCustomVendorTag
    ///
    /// @brief  Get custom vendor tag.
    ///
    /// @param  pAppSettings        App metadata.
    /// @param  pKeysForRequest     key combinations for request
    ///
    /// @return return the vendor tag value.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetCustomVendorTag(
        ChiMetadata* pAppSettings,
        const FGDKeysForCaptureRequest* pKeysForRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBurstShot
    ///
    /// @brief  Method to check if burst shot is enabled.
    ///
    /// @return return the vendor tag value.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsBurstShot()
    {
        return m_isBurstShotEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllFeatureGraphDescriptors
    ///
    /// @brief  Get custom vendor tag.
    ///
    /// @param  pConfig                 config data
    /// @param  pGraphDescriptorTables  table of cameraId and feature descriptors.
    /// @param  rFeatureNameToOpsMap    featureName map
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAllFeatureGraphDescriptors(
        FeatureGraphManagerConfig*             pConfig,
        GraphDescriptorTables*                 pGraphDescriptorTables,
        ChiFeature2NameMap&                    rFeatureNameToOpsMap);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SearchFeatureGraphinTable
    ///
    /// @brief  Search a feature graph for a particular key combination
    ///
    /// @param  pKeysForRequest         key combinations for request
    /// @param  pFDGKeysMap             keys table
    ///
    /// @return return the vendor tag value.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphDesc* SearchFeatureGraphinTable(
        FGDKeysForCaptureRequest* pKeysForRequest,
        std::vector<FGDKeysForTable>* pFDGKeysMap);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeatureHint
    ///
    /// @brief  Get the feature hints.
    ///
    /// @param  featureId       Feature Id
    /// @param  pRequestObject  Usecase request object
    /// @param  pCustomHints    Custom hints for the feature
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetFeatureHint(
        ChiFeature2Type                  featureId,
        ChiFeature2UsecaseRequestObject* pRequestObject,
        ChiFeature2Hint*                 pCustomHints);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNZSLSnapshotFlag
    ///
    /// @brief  This function checks whether to use zsl or non-zsl snapshot for current snapshot request,
    ///         based on input setting from app and result metadata from camx pipeline.
    ///
    /// @param  pAppSetting   Metadata input setting from application
    /// @param  pResultMeta   Metadata result from camx pipeline
    ///
    /// @return ChiFeature2InstanceFlags Instance flags for non-zsl snapshot
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2InstanceFlags GetNZSLSnapshotFlag(
        ChiMetadata* pAppSetting,
        ChiMetadata* pResultMeta);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNonZSLSnapshotGraphNeeded
    ///
    /// @brief  This function checks if non-zsl snapshot feature graphs are needed
    ///
    /// @param  pConfig        Config such as stream config, system settings, session parameters, sensor config.
    /// @param  pGraphName     The feature graph name
    /// @param  phyCamIdx      Physical camera index for multi camera logical camera
    /// @param  pInstanceFlags Output feature instance flags
    ///
    /// @return TRUE if require non-zsl snaphot, FALSE otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsNonZSLSnapshotGraphNeeded(
        const FeatureGraphManagerConfig* pConfig,
        const CHAR*                      pGraphName,
        UINT32                           phyCamIdx,
        ChiFeature2InstanceFlags*        pInstanceFlags);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CallCloneGraphDescriptor
    ///
    /// @brief  This function calls the clone feature graph descriptor and stores it in a map
    ///
    /// @param  rPhyCamDesc         Keys required to create a cloned copy of feature graph descriptor .
    /// @param  rFeatureGraphDesc   feature graph descriptor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID CallCloneGraphDescriptor(
        ChiFeature2FGDKeysForClonedMap  &rPhyCamDesc,
        ChiFeature2GraphDesc &rFeatureGraphDesc)
    {
        m_clonedFeatureGraphDescriptorsMap.insert(
        { rPhyCamDesc, CloneFeatureGraphDescriptor(&rFeatureGraphDesc) });
        m_pFeature2GraphDescs.push_back(m_clonedFeatureGraphDescriptorsMap[rPhyCamDesc]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CloneExtSrcExtSinkLinkDesc
    ///
    /// @brief  Clone external source and sink link to destination descriptor.
    ///
    /// @param  pSourceGraphDesc             Source feature descriptor
    /// @param  pDestGraphDesc               Destination feature descriptor
    ///
    /// @return CDKResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CloneExtSrcExtSinkLinkDesc(
        const ChiFeature2GraphDesc* pSourceGraphDesc,
        ChiFeature2GraphDesc*       pDestGraphDesc);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PruneInternalGraphLinks
    ///
    /// @brief  Prune internal graph links with respect to prune rules provided.
    ///
    /// @param  pFeatureGraphDesc            Source feature descriptor
    /// @param  rPruneRulesMap               Pruning rules for this descriptor
    ///
    /// @return Pruned feature graph descriptor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphDesc* PruneInternalGraphLinks(
        const ChiFeature2GraphDesc*                   pFeatureGraphDesc,
        const std::vector<ChiFeature2PruneRule>&      rPruneRules);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PruneSinkGraphLinks
    ///
    /// @brief  Prune sink graph links with respect to prune rules provided.
    ///
    /// @param  pFeatureGraphDesc            Source feature descriptor
    /// @param  pFinalFeatureGraphDesc       The pointer for final graph descriptor
    /// @param  rPruneRulesMap               Pruning rules for this descriptor
    ///
    /// @return CDKResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PruneSinkGraphLinks(
        const ChiFeature2GraphDesc*                   pFeatureGraphDesc,
        ChiFeature2GraphDesc*                         pFinalFeatureGraphDesc,
        const std::vector<ChiFeature2PruneRule>&      rPruneRules);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PruneSourceGraphLinks
    ///
    /// @brief  Prune source graph links with respect to prune rules provided.
    ///
    /// @param  pFeatureGraphDesc            Source feature descriptor
    /// @param  pFinalFeatureGraphDesc       The pointer for final graph descriptor
    /// @param  rPruneRulesMap               Pruning rules for this descriptor
    ///
    /// @return CDKResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PruneSourceGraphLinks(
        const ChiFeature2GraphDesc*                   pFeatureGraphDesc,
        ChiFeature2GraphDesc*                         pFinalFeatureGraphDesc,
        const std::vector<ChiFeature2PruneRule>&      rPruneRules);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BuildInternalLinkPruneRule
    ///
    /// @brief  Generate internal link prune rule based feature graph keys for capture request.
    ///
    /// @param  pKeysForRequest     The pointer of feature graph keys for capture request
    /// @param  physicalCameraIndex The index of physical camera
    /// @param  rPruneRule          The reference of prune rule
    ///
    /// @return CDKResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult BuildInternalLinkPruneRule(
        const FGDKeysForCaptureRequest* pKeysForRequest,
        UINT8                           physicalCameraIndex,
        ChiFeature2PruneRule&           rPruneRule);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BuildSinkLinkPruneRule
    ///
    /// @brief  Generate sink link prune rule based feature graph keys for capture request.
    ///
    /// @param  pKeysForRequest     The pointer of feature graph keys for capture request
    /// @param  physicalCameraIndex The index of physical camera
    /// @param  rPruneRule          The reference of prune rule
    ///
    /// @return CDKResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult BuildSinkLinkPruneRule(
        const FGDKeysForCaptureRequest* pKeysForRequest,
        UINT8                           physicalCameraIndex,
        ChiFeature2PruneRule&           rPruneRule);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeGraphDescResources
    ///
    /// @brief  Free graph descriptor resources on error
    ///
    /// @param  pFeatureGraphDesc            Source feature descriptor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID FreeGraphDescResources(
        ChiFeature2GraphDesc* pFeatureGraphDesc);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CloneFeature2InstanceDesc
    ///
    /// @brief  Clone feature instance descriptor for source to destination descriptor.
    ///
    /// @param  pSourceFeature2InstanceDesc             Source feature instance descriptor
    /// @param  pDestFeature2InstanceDesc               Destination feature instance descriptor
    ///
    /// @return CDKResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CloneFeature2InstanceDesc(
        ChiFeature2InstanceDesc*             pSourceFeature2InstanceDesc,
        ChiFeature2InstanceDesc*             pDestFeature2InstanceDesc);

    const LogicalCameraInfo*                     m_pCameraInfo;                      ///< Pointer to logical camera info
    std::vector<ChiFeature2GraphDesc*>           m_pFeature2GraphDescs;              ///< Feature graph descriptors
    keysToCloneDescMap                           m_clonedFeatureGraphDescriptorsMap; ///< Cloned featureDescriptor map
    std::vector<FGDKeysForTable>                 m_FDGKeysMap;                       ///< map of keys to graph descriptor
    cameraIdNameSet                              m_cameraIdDescriptorNameSet;        ///< cameraIdset and Descriptor set pair
    std::map<const CHAR*, ChiFeature2GraphDesc>  m_featureGraphDescriptorsMap;       ///< map of featurename to descriptor
    GraphDescriptorTables                        m_graphDescriptorTables;            ///< list of all the tables
    CHITAGSOPS                                   m_vendorTagOps;                     ///< Vendor Tag Ops
    BOOL                                         m_isQCFAUsecase;                    ///< Non-zsl snapshot is required for
                                                                                     ///  qcfa sensor and qcfa usecase
    ChiRemosaicType                              m_remosaicType;                     ///< SW remosaic or HW remosaic
    BOOL                                         m_is4KYUVOut;                       ///< 4K YUV out to use different graph
    BOOL                                         m_isJpegSnapshot;                   ///< is Jpeg snapshot included
    BOOL                                         m_isRawInput;                       ///< raw reprocess
    std::vector<UINT8>                           m_stageSequenceHint;                ///< stage sequence hint
    pruneKeyToGraphDescMap                       m_prunedFeatureGraphDescriptorsMap; ///< Pruned key to feature graph map
    UINT32                                       m_inputOutputType;                  ///< Config stream input out type
    BOOL                                         m_isBurstShotEnabled;               ///< long shot enabled or not
    std::map<std::set<UINT>, UINT>               m_cameraIdMap;                      ///< camera id map set
    ChiFeature2GraphSelector(const ChiFeature2GraphSelector&) = delete;              ///< Disallow the copy constructor
    ChiFeature2GraphSelector& operator= (const ChiFeature2GraphSelector&) = delete;  ///< Disallow assignment operator

};

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

    /// @brief Pack data structures to ensure consistent layout.
#pragma pack(push, 8)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief feature2 ops APIs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // @brief to create feature graph selector
    typedef ChiFeature2GraphSelector* (*PFNCREATEFGS)(
        FeatureGraphManagerConfig* pCreateInputInfo,
        std::set<const CHAR*>&     rFeatureDescNameSet);

    // @brief to query the capabilities of this graph selector
    typedef VOID* (*PFNDOQUERYCAPSFGS)(
        VOID*                 pPrivateData);

    // @brief to select feature graph for the capture request
    typedef ChiFeature2GraphDesc* (*PFNSELECTFGDFORREQUEST)(
        ChiFeature2UsecaseRequestObject* pChiUsecaseRequestObject,
        std::map<UINT32, ChiMetadata*>   pMetadataFrameNumberMap,
        VOID*                            pPrivateData);

    // @brief to get list of feature graphs supported for the stream config
    typedef keysToCloneDescMap(*PFNGETFGDLISTFORCONFIG)(
        FeatureGraphManagerConfig*  pConfig,
        FeatureGraphSelectorConfig& rSelectorOutput,
        VOID*                       pPrivateData);

    // @brief to add custom hints for graph nodes for each request
    typedef VOID (*PFNADDCUSTOMHINTS)(
        ChiFeature2UsecaseRequestObject*             pRequestObject,
        std::vector<ChiFeature2InstanceRequestInfo>& rFeatureInstanceReqInfo,
        VOID*                                        pPrivateData);

    // @brief to destroy the feature graph selector
    typedef VOID (*PFNDODESTROY)(
        VOID* pPrivateData);

    /// @brief feature2 graph selector ops
    struct CHIFEATURE2GRAPHSELECTOROPS
    {
        UINT32                 size;                  ///< Size of this structure
        PFNCREATEFGS           pCreate;               ///< Create this feature graph selector
        PFNDOQUERYCAPSFGS      pQueryCaps;            ///< Query the capabilities
        PFNSELECTFGDFORREQUEST pSelectFGD;            ///< Select feature graph descriptor
        PFNGETFGDLISTFORCONFIG pGetFGDListForConfig;  ///< Get the list of FGDs for the config
        PFNADDCUSTOMHINTS      pAddCustomHints;       ///< Add custom hints
        PFNDODESTROY           pDestroy;              ///< Destroy the selector
    };

    // @brief function pointer for entry functions
    typedef VOID(*PCHIFEATURE2GRAPHSELECTOROPSENTRY)(
        CHIFEATURE2GRAPHSELECTOROPS* pChiFeature2GraphSelectorOps);

    // @brief function to map entry functions
    CDK_VISIBILITY_PUBLIC VOID ChiFeature2GraphSelectorOpsEntry(
        CHIFEATURE2GRAPHSELECTOROPS* pChiFeature2GraphSelectorOps);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHIFEATURE2GRAPHSELECTOR_H
