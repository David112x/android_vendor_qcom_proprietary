////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2featurepool.h
/// @brief CHI feature pool manager class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2FEATUREPOOL_H
#define CHIFEATURE2FEATUREPOOL_H

#include <map>
#include <set>

#include "chxincs.h"
#include "chxextensionmodule.h"
#include "chxusecaseutils.h"
#include "chxutils.h"
#include "camxcdktypes.h"
#include "chicommon.h"

#include "chifeature2types.h"
#include "chifeature2base.h"
#include "chifeature2requestobject.h"
#include "chifeature2graph.h"

// NOWHINE FILE NC004c: CHI files wont be having Camx
// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases
// NOWHINE FILE CP004:  Need comparator for map
// NOWHINE FILE GR017:  Need comparator for map
// NOWHINE FILE CP008:  Need comparator for map

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT   MaxFeatureList            = static_cast<UINT>(ChiFeature2Type::MaxFeatureList);
static const INT    MaxNumOfStreamsPerFeature = 10;
#if defined (_LINUX)
static const CHAR SharedLibraryExtension[] = "so";
#else
static const CHAR SharedLibraryExtension[] = "dll";
#endif // _LINUX


struct FeaturePoolInputData
{
    ChiFeature2GraphDesc*               pFeatureGraphDesc;              ///< Associated Feature graphs for the config stream
    ChiFeature2GraphCallbacks*          pFeatureCallbacks;              ///< Feature callbacks
    LogicalCameraInfo*                  pCameraInfo;                    ///< Logical camera info
    const ChiUsecase*                   pUsecaseDescriptor;             ///< Usecase descriptor
    CHISTREAMCONFIGINFO*                pCameraStreamConfig;            ///< Stream config
    ChiMetadataManager*                 pMetadataManager;               ///< Metadata manager for the usecase
    CHIThreadManager*                   pThreadManager;                 ///< Feature Threadmanager
    UINT8                               physicalCameraId;               ///< Physical camera index
    BOOL                                bMultiCameraFeature;            ///< Flag to indicate if it is multi camera graph
    ChiFeature2GraphManagerCallbacks*   pFeatureGraphManagerCallbacks;  ///< Feature Graph Manager Notification callbacks
    ChiFeature2InstanceFlags            featureFlags;                   ///< flags for feature
};

/// @brief  feature configuration
struct Feature2Config
{
    ChiStreamConfigInfo* pCameraStreamConfig;     ///< camera3 stream configuration
    LogicalCameraInfo*   pCameraInfo;             ///< Logical camera info
    VOID*                pSessionParams;          ///< Session parameters
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsChiFeature2PropsLess
///
/// @brief  Returns TRUE if ChiFeature2InstanceId LHS is less then RHS
///
/// @param  rLHS The left-hand  ChiFeature2InstanceId to compare
/// @param  rRHS The Right-hand ChiFeature2InstanceId to compare
///
/// @return TRUE if ChiFeature2Props LHS is less then RHS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CHX_INLINE BOOL IsChiFeature2PropsLess(
    const ChiFeature2InstanceId& rLHS,
    const ChiFeature2InstanceId& rRHS)
{
    BOOL isLHSLessThanRHS = FALSE;

    if (rLHS.featureId < rRHS.featureId)
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.featureId == rRHS.featureId)  &&
             (rLHS.cameraId  <  rRHS.cameraId))
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.featureId  == rRHS.featureId) &&
             (rLHS.cameraId   == rRHS.cameraId)  &&
             (rLHS.instanceId <  rRHS.instanceId))
    {
        isLHSLessThanRHS = TRUE;
    }
    else if ((rLHS.featureId   == rRHS.featureId)  &&
             (rLHS.cameraId    == rRHS.cameraId)   &&
             (rLHS.instanceId  == rRHS.instanceId) &&
             (rLHS.flags.value <  rRHS.flags.value))
    {
        isLHSLessThanRHS = TRUE;
    }

    return isLHSLessThanRHS;
}

/// @brief Comparator function to determine whether ChiFeature2InstanceId LHS is less than RHS
struct ChiFeature2PropsLessComparator :
    public std::binary_function<ChiFeature2InstanceId, ChiFeature2InstanceId, bool>
{
    bool operator()(const ChiFeature2InstanceId LHS, const ChiFeature2InstanceId RHS) const
    {
        return IsChiFeature2PropsLess(LHS, RHS);
    }
};

/// @brief Comparator function to determine whether Chifeature2 name LHS is less than RHS
struct ChiFeature2NameLessComparator :
    public std::binary_function<const CHAR*, const CHAR*, bool>
{
    bool operator()(const CHAR* pLHS, const CHAR* pRHS) const
    {
        BOOL isLHSLessThanRHS = FALSE;

        if (0 > CdkUtils::StrCmp(pRHS, pLHS))
        {
            isLHSLessThanRHS = TRUE;
        }

        return isLHSLessThanRHS;
    }
};

typedef std::map<const CHAR*, CHIFEATURE2OPS, ChiFeature2NameLessComparator> ChiFeature2NameMap;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature pool manager class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiFeaturePoolManager
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFeatureInstance
    ///
    /// @brief  Method to check if feature is already instantiated
    ///
    /// @param  pFeaturePoolInputData   Feature graph descriptor
    /// @param  rpEnabledFeatures       List of enabled features for the selected graph
    ///
    /// @return CDKResultSuccess if feature descriptor available or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateFeatureInstance(
        FeaturePoolInputData*          pFeaturePoolInputData,
        std::vector<ChiFeature2Base*>& rpEnabledFeatures);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SelectFeaturesForGraph
    ///
    /// @brief  Method to create feature graph instance on a request basis
    ///
    /// @param  pFeatureGraphDescForRequest    Feature Graph descriptor for this request.
    /// @param  rEnabledFeatureInstanceReqInfo Features enabled for this graph and the request-specific info associated with
    ///                                        each
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SelectFeaturesForGraph(
        ChiFeature2GraphDesc*                           pFeatureGraphDescForRequest,
        std::vector<ChiFeature2InstanceRequestInfo>&    rEnabledFeatureInstanceReqInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseResource
    ///
    /// @brief  Method to release the feature descriptor back to the feature pool
    ///
    /// @param  pChiFeature2GraphDesc  Feature graph descriptor
    ///
    /// @return CDKResultSuccess if resource is released successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ReleaseResource(
        ChiFeature2GraphDesc* pChiFeature2GraphDesc);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Method to create an instance of feature pool
    ///
    /// @param  pConfig           Feature graph manager config
    /// @param  featureDescSet    feature desc set
    ///
    /// @return ChiFeaturePoolManager object.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeaturePoolManager* Create(
        VOID*                   pConfig,
        std::set<const CHAR*>&  rFeatureDescSet);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief  Method to flush all features in the feature pool
    ///
    /// @param  isSynchronousFlush  Boolean indicating if base should flush synchronously or asynchronously
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Flush(
        BOOL isSynchronousFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SignalFlush
    ///
    /// @brief  Signal flush is done if flush of all features is completed
    ///
    /// @param  featureId   featureId
    ///
    /// @return success if negotiate successfully.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SignalFlush(
        UINT32  featureId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Method to destroy the instance of feature pool
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyGraphManagerDestroyInProgress
    ///
    /// @brief  Notify each feature instance that Feature Graph Manager is destroy in progress.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyGraphManagerDestroyInProgress();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateInstancePropsInGraphDesc
    ///
    /// @brief  This function update instance properties for the feature, like cameraId and featureFlags.
    ///
    /// @param  pFeatureGraphDesc   Feature Graph Descriptor
    /// @param  pFeatureDescriptor  Feature Descriptor
    /// @param  pFeatureInstanceId  Feature Instance ID
    /// @param  rInputPortMap       Reference for input port camera id map
    /// @param  rOutputPortMap      Reference for output port camra id map
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateInstancePropsInGraphDesc(
        ChiFeature2GraphDesc*                  pFeatureGraphDesc,
        const ChiFeature2Descriptor*           pFeatureDescriptor,
        ChiFeature2InstanceId*                 pFeatureInstanceId,
        std::vector<PortIdCameraIndexMapPair>& rInputPortMap,
        std::vector<PortIdCameraIndexMapPair>& rOutputPortMap);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsVideoFrameworkStream
    ///
    /// @brief  This function will indicate if there is a framework video stream
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsVideoFrameworkStream()
    {
        return m_frameworkVideoStream;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetListofFeaturesSupported
    ///
    /// @brief  Method to get the list of features probed
    ///
    /// @return feature list
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiFeature2NameMap& GetListofFeaturesSupported()
    {
        return m_featureNameToOpsMap;
    }

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initializes the feature pool manager
    ///
    /// @param  pConfig  Feature graph manager config
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        VOID* pConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProbeChiFeature2Features
    ///
    /// @brief  Probe chifeature2 features and load feature2Ops from the specified lib folder.
    ///
    /// @param  pLibPath    The lib folder to search
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProbeChiFeature2Features(
        const CHAR* pLibPath);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoFeatureCreate
    ///
    /// @brief  Create feature instance with the specified feature index.
    ///
    /// @param  pFeatureName       Which feature need to be created
    /// @param  pCreateInputInfo   Feature create input info
    ///
    /// @return feature object if successful or NULL in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2Base* DoFeatureCreate(
        const CHAR*                 pFeatureName,
        ChiFeature2CreateInputInfo* pCreateInputInfo);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoStreamNegotiation
    ///
    /// @brief  Create feature input/output stream and negotiation.
    ///
    /// @param  pNegotiationInfo     Input information for negotiation
    /// @param  pNegotiationOutput   Output for stream negotiation
    ///
    /// @return success if negotiate successfully.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DoStreamNegotiation(
        StreamNegotiationInfo*          pNegotiationInfo,
        StreamNegotiationOutput*        pNegotiationOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraIdByPortId
    ///
    /// @brief  Get camera id from map by port id.
    ///
    /// @param  rPortIdMap     The reference pair array for camera id and port id map
    /// @param  portId         Port id
    ///
    /// @return cameraId if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8 GetCameraIdByPortId(
        std::vector<std::pair<UINT8, UINT8>>& rPortIdMap,
        UINT8                                 portId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeaturePoolManager
    ///
    /// @brief  Private constructor for a ChiFeaturePoolManager instance
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeaturePoolManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeaturePoolManager
    ///
    /// @brief  Private destructor for a ChiFeaturePoolManager instance
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~ChiFeaturePoolManager();

    // Do not implement the copy constructor or assignment operator
    ChiFeaturePoolManager(const ChiFeaturePoolManager&) = delete;                ///< Disallow the copy constructor
    ChiFeaturePoolManager& operator= (const ChiFeaturePoolManager&) = delete;    ///< Disallow assignment operator

    FeaturePoolInputData                m_featurePoolInputData;                   ///< Input data required for feature pool
    std::vector<ChiFeature2Base*>       m_pFeatureList;                           ///< List of features
    std::vector<ChiFeature2Base*>       m_pEnabledFeatures;                       ///< List of enabled features
    std::map<ChiFeature2InstanceId,
        ChiFeature2Base*,
        ChiFeature2PropsLessComparator> m_pEnabledFeaturesMap;                    ///< List of enabled features
    ChiStreamConfigInfo*                m_pCameraStreamConfig;                    ///< Camera stream config
    BOOL                                m_frameworkVideoStream;                   ///< is Video stream from framework
    std::vector<ChiStream*>             m_pStreams;                               ///< List of internal streams
    std::vector<ChiStream*>             m_pFeatureStreams;                        ///< Streams allocated as part of a feature's
                                                                                  ///< stream negotiation. Should be freed by
                                                                                  ///< the feature itself.
    ChiMetadataManager*                 m_pMetadataManager;                       ///< Metadata manager
    ChiFeature2NameMap                  m_featureNameToOpsMap;                    ///< FeatureName to Feature2Ops map
    CHISTREAMCONFIGINFO                 m_desiredStreamConfig;                    ///< desired Stream config
    VOID*                               m_pConfig;                                ///< feature graph manager config
    std::set<const CHAR*>               m_featureDescNameSet;                     ///< feature desc set

    Mutex*                              m_pFlushMutex;                            ///< mutex for flush
    Mutex*                              m_pFlushCountMutex;                       ///< mutex for flush feature count
    Condition*                          m_pFlushCondition;                        ///< condition for flush
    UINT32                              m_numFeaturesFlushed;                     ///< counter for features flushed
    BOOL                                m_qcfaUsecase;                             ///< Flag to indicate quadcfa usecase.
};

#endif // CHIFEATURE2FEATUREPOOL_H
