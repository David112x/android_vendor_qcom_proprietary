////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graphmanager.h
/// @brief CHI feature graph manager class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2GRAPHMANAGER_H
#define CHIFEATURE2GRAPHMANAGER_H

#include <map>

#include "chxincs.h"
#include "chxextensionmodule.h"
#include "chxusecaseutils.h"
#include "chxutils.h"
#include "camxcdktypes.h"
#include "chicommon.h"

#include "chifeature2types.h"
#include "chifeature2graphselector.h"
#include "chifeature2graphselectoroem.h"
#include "chifeature2base.h"
#include "chifeature2requestobject.h"
#include "chifeature2graph.h"
#include "chifeature2featurepool.h"
#include "chithreadmanager.h"

// NOWHINE FILE NC004c: CHI files wont be having Camx
// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward Declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFeaturePoolManager;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const INT    MaxFeaturesPerUsecase = 10;
static const UINT   MaxFGMSessions        = 10;

/// @brief Feature graph manager private data
struct FeatureGraphManagerPrivateData
{
    VOID*  pFeatureGraphManager;             ///< feature Graph Manager
    VOID*  pChiFeature2UsecaseRequestObject; ///< feature usecase request object

};

/// @brief Feature graph manager session data for each configure streams
struct FeatureGraphManagerSessionData
{
    UINT                   sessionId;                                  ///< FGM session Id
    ChiFeature2Base*       pEnabledFeatures[MaxFeaturesPerUsecase];    ///< Enabled Features for this session
    ChiFeature2GraphDesc** ppFeatureGraphDesc;                          ///< Associated Feature graphs for the config stream
};

typedef std::pair<UINT32, const CHAR *> cameraIdDescNamePair;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature graph manager class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFeature2GraphManager
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create feature graph manager instance
    ///
    /// @param  pConfig Config Information required to create a feature graph.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2GraphManager* Create(
        FeatureGraphManagerConfig* pConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief  Flush feature graph manager components
    ///
    /// @return CDKResultSuccess if successful, error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Flush();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy feature graph manager object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Method to trigger process request for the feature graph.
    ///
    /// @param  pRequest Usecase request object data
    ///
    /// @return CDKResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteProcessRequest(
        ChiFeature2UsecaseRequestObject* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnFeatureObjComplete
    ///
    /// @brief  Method to do task on feature object complete state.
    ///
    /// @param  pUsecaseRequestObj Usecase request object data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID OnFeatureObjComplete(
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObj);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterFGMCallbacks
    ///
    /// @brief  Set the FGM callbacks
    ///
    /// @param  pFeatureWrapperCallbacks Feature Wrapper Callbacks
    ///
    /// @return CDKResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult RegisterFGMCallbacks(
        ChiFeature2WrapperCallbacks* pFeatureWrapperCallbacks)
    {
        CDKResult result = CDKResultSuccess;

        if (NULL != pFeatureWrapperCallbacks)
        {
            m_featureWrapperCallbacks = *pFeatureWrapperCallbacks;
        }
        else
        {
            result = CDKResultEInvalidArg;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump
    ///
    /// @brief  This method is used by CHI to print debugging info for FGM. It will be
    ///         called when using the dumpsys tool or capturing a bugreport.
    ///
    /// @param  fd                 The file descriptor which can be used to write debugging text using dprintf() or write().
    /// @param  pUsecaseRequestObj The request object to be dumped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Dump(
        INT                                 fd,
        ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj) const;

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initializes the feature graph manager base
    ///
    /// @param  pConfig Config such as stream config, system settings, session parameters, sensor config.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        FeatureGraphManagerConfig* pConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadFeatureGraphSelectorOps
    ///
    /// @brief  Method to load FeatureGraphSelector entry OPs for lib file,
    ///         priority to load oem libs, if failed, load default qcom one.
    ///
    /// @param  pFGSOps The result FeatureGraphSelector Ops.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult LoadFeatureGraphSelectorOps(
        CHIFEATURE2GRAPHSELECTOROPS* pFGSOps);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFeatureGraphSelector
    ///
    /// @brief  Method to create an instance of the feature graph Selector based on config
    ///
    /// @param  pConfig Config such as stream config, system settings, session parameters, sensor config.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphSelector* CreateFeatureGraphSelector(
        FeatureGraphManagerConfig* pConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessUsecaseRequestObject
    ///
    /// @brief  Method to create usecase request object from the usecase request
    ///
    /// @param  pRequest      Usecase request data
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessUsecaseRequestObject(
        ChiFeature2UsecaseRequestObject* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2GraphManager
    ///
    /// @brief  Default constructor.
    ///
    /// @return VOID.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphManager() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2GraphManager
    ///
    /// @brief  Destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~ChiFeature2GraphManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFeatureGraphResultCb
    ///
    /// @brief  Callback for a result from the feature Graph
    ///
    /// @param  pResult              Capture result from the feature graph
    /// @param  pUsecaseRequestObj   Usecase Request Object associated with this callback
    /// @param  pPrivateCallbackData Private callback data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ProcessFeatureGraphResultCb(
        CHICAPTURERESULT*                pResult,
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObj,
        VOID*                            pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFeatureGraphPartialCaptureResultCb
    ///
    /// @brief  Callback for Partial Capture result from the feature Graph
    ///
    /// @param  pCaptureResult       Partial capture result from the feature graph
    /// @param  pPrivateCallbackData Private callback data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ProcessFeatureGraphPartialCaptureResultCb(
        CHIPARTIALCAPTURERESULT* pCaptureResult,
        VOID*                    pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFeatureGraphMessageCb
    ///
    /// @brief  Callback for a message from the feature Graph
    ///
    /// @param  pMessageDesc         Message from feature graph
    /// @param  pPrivateCallbackData Private callback data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult ProcessFeatureGraphMessageCb(
        const ChiFeature2Messages*          pMessageDesc,
        VOID*                               pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFeatureGraphFlushCb
    ///
    /// @brief  Callback for flush from the feature Graph
    ///
    /// @param  featureId            featureId
    /// @param  result               result of success or failure
    /// @param  pPrivateCallbackData Private callback data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ProcessFeatureGraphFlushCb(
        UINT32      featureId,
        CDKResult   result,
        VOID*       pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapInputPortToInputStream
    ///
    /// @brief  Map chistreams to global input port ids
    ///
    /// @param  pFeatureGraphDesc                   Feature Graph Descriptor
    /// @param  inputStreams                        List of input framework streams
    /// @param  globalPortIdToChiStreamMapForInput  port id to stream mapping for all inputs
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID MapInputPortToInputStream(
        ChiFeature2GraphDesc*                        pFeatureGraphDesc,
        std::vector<ChiStream*>                      &inputStreams,
        std::vector<ChiFeature2PortIdToChiStreamMap> &globalPortIdToChiStreamMapForInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapOutputPortToOutputStream
    ///
    /// @brief  Map output chistreams to global output port ids
    ///
    /// @param  pFeatureGraphDesc                   Feature Graph Descriptor
    /// @param  outputStreams                        List of output framework streams
    /// @param  globalPortIdToChiStreamMapForOutput port id to stream mapping for all outputs
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID MapOutputPortToOutputStream(
        ChiFeature2GraphDesc*                        pFeatureGraphDesc,
        std::vector<ChiStream*>                      &outputStreams,
        std::vector<ChiFeature2PortIdToChiStreamMap> &globalPortIdToChiStreamMapForOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFeatureGraph
    ///
    /// @brief  Method to create feature graph instance on a request basis
    ///
    /// @param  pRequestObject               Usecase Request Object
    /// @param  pFeatureGraphDescForRequest  Feature Graph descriptor for this request.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2Graph* CreateFeatureGraph(
        ChiFeature2UsecaseRequestObject* pRequestObject,
        ChiFeature2GraphDesc*            pFeatureGraphDescForRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CloneStreamConfig
    ///
    /// @brief  Method to clone stream config
    ///
    /// @param  pSrcCameraStreamConfig     camera stream config
    /// @param  pDstCameraStreamConfig     cloned camera stream config
    /// @param  numStreams                 number of streams to be allocated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID CloneStreamConfig(
        ChiStreamConfigInfo* pSrcCameraStreamConfig,
        ChiStreamConfigInfo* pDstCameraStreamConfig,
        UINT                 numStreams)
    {
        pDstCameraStreamConfig->numStreams = pSrcCameraStreamConfig->numStreams;
        ChxUtils::Memcpy(&pDstCameraStreamConfig->pSessionSettings, &pSrcCameraStreamConfig->pSessionSettings, sizeof(VOID*));

        pDstCameraStreamConfig->pChiStreams = static_cast<CHISTREAM**>(ChxUtils::Calloc(sizeof(CHISTREAM) *
                                                                                        numStreams));

        if (NULL != pDstCameraStreamConfig->pChiStreams)
        {
            for (UINT streamIdx = 0; streamIdx < pSrcCameraStreamConfig->numStreams; streamIdx++)
            {
                pDstCameraStreamConfig->pChiStreams[streamIdx] = pSrcCameraStreamConfig->pChiStreams[streamIdx];
            }
        }
        else
        {
            CHX_LOG_ERROR("Failed to clone stream config");
        }
    }

    ChiFeature2UsecaseRequestObject* m_pChiUsecaseRequestObject[MaxRequestQueueDepth];  ///< Feature2 request object
    std::vector<ChiFeature2Base*>    m_pEnabledFeatures;                                ///< List of enabled features
    ChiFeature2GraphSelector*        m_pFeatureGraphSelector;                           ///< Feature graph selector object
    ChiFeature2GraphManagerCallbacks m_featureGraphCallbacks[MaxRequestQueueDepth];     ///< Feature Graph Manager callbacks

    ChiFeature2GraphCallbacks        m_featureCallbacks;                                ///< Feature Graph callbacks
    ChiFeature2WrapperCallbacks      m_featureWrapperCallbacks;                         ///< Feature wrapper callbacks
    keysToCloneDescMap               m_pFeatureGraphDescMapForConfig;                   ///< Feature Graph container

    std::map<UINT32, ChiFeature2Graph*>   m_pFeatureGraphFrameNumMap;                   ///< Feature graphs URO map
    std::vector<ChiFeature2GraphDesc *>   m_pFeatureGraphDescsForConfig;                ///< List of feature graph descriptors
                                                                                        ///  for each config stream
    ChiFeaturePoolManager*                m_pFeaturePoolManager;                        ///< Feature pool manager
    UINT                                  m_numFeatureGraphDescriptors;                 ///< Number of FG descriptors for
                                                                                        ///  each config
    CHIThreadManager*                     m_pFeatureThreadManager;                      ///< Feature Threadmanager
    ChiStreamConfigInfo                   m_cameraStreamConfig;                         ///< Camera stream config
    LogicalCameraInfo*                    m_pLogicalCameraInfo;                         ///< Logical Camera Info

    ChiFeature2GraphCreateInputInfo m_featureGraphCreateInputInfo[MaxRequestQueueDepth]; ///< Feature Graphs input config
    std::map<UINT32, FeatureGraphManagerPrivateData*>  m_pChiFGMPrivateDataMap;          ///< Graph Manager Private data

    CHIFEATURE2GRAPHSELECTOROPS          m_feature2GraphSelectorOps;                    ///< Graph Selector ops for dynamic
                                                                                        ///  loading of lib
    std::map<UINT32, ChiMetadata*>       m_metadataFrameNumberMap;                      ///< store Metadata for a frame number
    ChiMetadataManager*                  m_pChiMetadataManager;                         ///< metadata manager
    FeatureGraphManagerPrivateData*      m_pChiFeature2GraphManagerPrivateData;         ///< Graph manager private data

    std::set<const CHAR*>                m_featureDescNameSet;                          ///< feature descriptor set
    BOOL                                 m_isGPURotationNeeded;                         ///< flag to check if GPU rotation is
                                                                                        ///< required in the JPEG pipeline

    ChiFeature2GraphManager(const ChiFeature2GraphManager&) = delete;                   ///< Disallow the copy constructor
    ChiFeature2GraphManager& operator= (const ChiFeature2GraphManager&) = delete;       ///< Disallow assignment operator

    Mutex*                               m_pMetaFrameLock;                              ///< Lock to synchronize map access
};

#endif // CHIFEATURE2GRAPHMANAGER_H
