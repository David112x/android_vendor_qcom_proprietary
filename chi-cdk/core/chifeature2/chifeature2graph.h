////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graph.h
/// @brief CHI feature graph declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2GRAPH_H
#define CHIFEATURE2GRAPH_H

#include <vector>

#include "chifeature2base.h"
#include "chifeature2types.h"
#include "chifeature2utils.h"
#include "chifeature2requestobject.h"
#include "chifeature2usecaserequestobject.h"

// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases
// NOWHINE FILE NC004c: Need whiner update: CHI files will use chi
// NOWHINE FILE CP004:  Need comparator for map
// NOWHINE FILE GR017:  Need comparator for map
// NOWHINE FILE CP008:  Need comparator for map

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Feature Graph Types
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ChiFeature2GraphNode;

/// @brief The container for the port that describes a link connected to an input port on a Feature2GraphNode in a feature graph
///        and that receives buffers/metadata from the client of the graph
struct ChiFeature2GraphExtSrcLinkDesc
{
    ChiFeature2GlobalPortInstanceId portId; ///< The globally unique id of the sink (input) port the link is connected to
};

/// @brief The container for the source and sink ports that describe a link between two Feature2GraphNodes in a feature graph
struct ChiFeature2GraphInternalLinkDesc
{
    ChiFeature2GlobalPortInstanceId  srcPortId;      ///< The globally unique id of the source (output) port of the link
    ChiFeature2GlobalPortInstanceId  sinkPortId;     ///< The globally unique id of the sink (input) port of the link
    ChiFeature2PrunableVariant       pruneVariant;   ///< Information about pruning the feature descriptor
};

/// @brief The container for the port that describes a link connected to an output port on a Feature2GraphNode in a feature
///        graph and that outputs buffers/metadata to the client of the graph
struct ChiFeature2GraphExtSinkLinkDesc
{
    ChiFeature2GlobalPortInstanceId portId; ///< The globally unique id of the source (output) port the link is connected to
    ChiFeature2PrunableVariant      pruneVariant;   ///< Information about pruning the feature descriptor
};

/// @brief The container for the data describing the feature graph
struct ChiFeature2GraphDesc
{
    const CHAR*                       pFeatureGraphName;    ///< Feature Graph name
    UINT32                            numFeatureInstances;  ///< The total number of feature instances
    ChiFeature2InstanceDesc*          pFeatureInstances;    ///< An array containing all feature instances
    UINT32                            numExtSrcLinks;       ///< The number of links in pSourceLinks
    ChiFeature2GraphExtSrcLinkDesc*   pExtSrcLinks;         ///< An array containing links connected to all source ports
    UINT32                            numInternalLinks;     ///< The number of links in pInternalLinks
    ChiFeature2GraphInternalLinkDesc* pInternalLinks;       ///< An array containing links connected to all internal ports
    UINT32                            numExtSinkLinks;      ///< The number of links in pSinkLinks
    ChiFeature2GraphExtSinkLinkDesc*  pExtSinkLinks;        ///< An array containing links connected to all sink ports
    BOOL                              isMultiCameraGraph;   ///< Flag indicating whether this is multi camera graph
};

/// @brief The state of the feature graph link
enum class ChiFeature2GraphLinkState
{
    NotVisited,         ///< Link has not been visited, so state is unknown
    Disabled,           ///< Link is not currently used by the feature graph
    OutputPending       ///< Link is active and pending output
};

/// @brief Comparator function to determine whether global port instance ID LHS is less than RHS
struct globalPortInstanceIdLessComparator :
    public std::binary_function<ChiFeature2GlobalPortInstanceId, ChiFeature2GlobalPortInstanceId, bool>
{
    bool operator()(const ChiFeature2GlobalPortInstanceId LHS, const ChiFeature2GlobalPortInstanceId RHS) const
    {
        return IsGlobalPortInstanceIdLess(LHS, RHS);
    }
};

/// @brief Maps a graph node to its corresponding feature request object
struct ChiFeature2RequestObjectMap
{
    ChiFeature2GraphNode*       pGraphNode;         ///< The graph node to which the FRO belongs
    ChiFeature2RequestObject*   pFeatureRequestObj; ///< The active FRO of the feature
};

/// @brief Container that associates a global instance port id to a link index
struct ChiFeature2PortToLinkIndexMap
{
    ChiFeature2GlobalPortInstanceId portId;     ///< The port id that corresponds to the link index
    UINT32                          linkIndex;  ///< The index into the m_linkData array
};

/// @brief The Feature2 graph node wraps the Feature2 object instance and contains linkage information
struct ChiFeature2GraphNode
{
    /// The id of the feature associated with this node
    UINT32 featureId;

    /// The instance properties associated with this feature
    ChiFeature2InstanceProps instanceProps;

    /// Pointer to the Feature2 base class object that this graph node wraps
    ChiFeature2Base* pFeatureBaseObj;

    /// List of outstanding feature request objects currently being processed.
    /// Limitations:
    ///  - Only the most recently added FRO will be used when accessing the FRO from the node (.back())
    ///  - Older FROs are only tracked in order to delete them when the graph is destroyed
    ///  - Older FROs may only be used when referred to by an upstream FRO (for processing results/metadata)
    std::vector<ChiFeature2RequestObject*> pFeatureRequestObjList;

    /// Maps graph nodes to their downstream feature request objects. The downstream FROs are associated with the FRO of this
    /// node that will be created next.
    std::vector<ChiFeature2RequestObjectMap> downstreamFeatureRequestObjMap;

    /// A two-dimensional table containing each request and each port per request for feature request object creation
    std::vector<std::vector<ChiFeature2RequestOutputInfo>> requestTable;

    /// Maps an input port id to an index in the feature graph's link data table connecting to the given input port
    std::vector<ChiFeature2PortToLinkIndexMap> portToInputLinkIndexMap;

    /// Maps an output port id to an index in the feature graph's link data table connecting to the given output port
    std::vector<ChiFeature2PortToLinkIndexMap> portToOutputLinkIndexMap;

    /// Feature hint
    ChiFeature2Hint featureHint;
};

/// @brief The container that associates all data (indices and dependency info) for a single request on a link
struct ChiFeature2GraphLinkRequestMap
{
    UINT8   requestIndex;               ///< The index of this request
    UINT8   batchIndex;                 ///< The batch this request is associated with
    UINT8   dependencyIndex;            ///< The dependency index of this request
    BOOL    inputDependencyReleased;    ///< Flag indicating whether this input dependency has been released
};

/// @brief The container for all data associated with a link in a feature graph
struct ChiFeature2GraphLinkData
{
    ChiFeature2GraphLinkType    linkType;       ///< Indicates the type of link this is: source, internal, or sink

    union
    {
        ChiFeature2GraphExtSrcLinkDesc      extSrcLinkDesc;     ///< The graph-external source link that this container wraps
        ChiFeature2GraphInternalLinkDesc    internalLinkDesc;   ///< The graph-internal link that this container wraps
        ChiFeature2GraphExtSinkLinkDesc     extSinkLinkDesc;    ///< The graph-external sink link that this container wraps
    } linkDesc;

    ChiFeature2GraphLinkState   linkState;      ///< Whether the link is active or inactive

    ChiFeature2GraphNode*       pSrcGraphNode;  ///< The source node for this link
    ChiFeature2GraphNode*       pSinkGraphNode; ///< The sink node for this link

    std::vector<ChiFeature2GraphLinkRequestMap> linkRequestTable;   ///< A list of structs associating a link's indices and
                                                                    ///  dependency info. The size will be numRequests which is
                                                                    ///  equal to numBatches * numDependencies.

    UINT8               numBatches;                     ///< The number of input batches of requests
    std::vector<UINT8>  numDependenciesPerBatchList;    ///< The number of input dependencies per batch of requests
    UINT8               numDependenciesReleased;        ///< The total number of input dependencies released. Once this becomes
                                                        ///  the same as the size of link request table, the link can be moved
                                                        ///  to the Disabled state.
};

/// @brief Container that associates a global instance port id to a ChiStream
struct ChiFeature2PortIdToChiStreamMap
{
    ChiFeature2GlobalPortInstanceId portId;     ///< The global instance port id that corresponds to the ChiStream
    ChiStream*                      pChiStream; ///< The ChiStream that corresponds to the global instance port id
    union
    {
        BOOL    inputConfigSet;         ///< Indicates the input configuration has been set on the external source port
        BOOL    outputResultNotified;  ///< Indicates the output result has been delivered from the external sink port
    } portFlags;
};

/// @brief Feature graph input creation information
struct ChiFeature2GraphCreateInputInfo
{
    ChiFeature2GraphManagerCallbacks*   pFeatureGraphManagerCallbacks;  ///< Callback functions for the feature graph's manager
    ChiFeature2GraphDesc*               pFeatureGraphDesc;              ///< The feature graph descriptor used to build the
                                                                        ///  feature graph object.

    std::vector<ChiFeature2InstanceRequestInfo>     featureInstanceReqInfoList;     ///< An array containing all of the enabled
                                                                                    ///  feature instances and the associated
                                                                                    ///  request info for each
    std::vector<ChiFeature2PortIdToChiStreamMap>    extSrcPortIdToChiStreamMap;     ///< A list of external source port ids
                                                                                    ///  associated with input ChiStreams
    std::vector<ChiFeature2PortIdToChiStreamMap>    extSinkPortIdToChiStreamMap;    ///< A list of external sink port ids
                                                                                    ///  associated with ChiStreams

    ChiFeature2UsecaseRequestObject* pUsecaseRequestObject;  ///< Associated URO with the graph
};

/// @brief Feature graph callback data
struct ChiFeature2GraphCallbackData
{
    ChiFeature2Graph*                           pFeatureGraph;                  ///< Pointer to feature graph instance
    ChiFeature2GraphNode*                       pGraphNode;                     ///< Pointer to feature graph node
    std::vector<ChiFeature2RequestObjectMap>    downstreamFeatureRequestObjMap; ///< Maps downstream graph nodes to their
                                                                                ///  corresponding downstream FROs
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature graph class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFeature2Graph
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create feature graph object
    ///
    /// @param  pFeatureGraphCreateInputInfo Feature graph input creation information
    ///
    /// @return Pointer to new instance of ChiFeature2Graph
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2Graph* Create(
        ChiFeature2GraphCreateInputInfo* pFeatureGraphCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy feature graph object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  The method to commence request processing in the feature graph. This method will be called twice per request,
    ///         the first time being used to collect input dependencies and the second time preparing the features for
    ///         execution.
    ///
    /// @param  pUsecaseRequestObj  Data from usecase required in order to process the request
    ///
    /// @return CDKResultSuccess if successful; otherwise failure result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteProcessRequest(
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObj);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMessage
    ///
    /// @brief  Process message callback from a feature in graph
    ///
    /// @param  pFeatureRequestObj      The feature request object to operate on, including the feature graph context
    /// @param  pMessages               The message data to process, including the list of ports corresponding to the message
    ///
    /// @return CDKResultSuccess if successful; otherwise failure result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult ProcessMessage(
        ChiFeature2RequestObject*   pFeatureRequestObj,
        ChiFeature2Messages*        pMessages);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief  Flush requests from all features in the graph.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Flush();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump
    ///
    /// @brief  This method is used by CHI to print debugging state for the feature graph instance. It will be called when using
    ///         the dumpsys tool or capturing a bugreport.
    ///
    /// @param  fd The file descriptor which can be used to write debugging text using dprintf() or write().
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Dump(
        INT fd
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeatureGraphName
    ///
    /// @brief  This method returns the feature graph name
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const CHAR* GetFeatureGraphName()
    {
        return m_pFeatureGraphName;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IdentifierString
    ///
    /// @brief  returns the FRO Identifer that has the URO, Feature info
    ///
    /// @return identifier
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE const CHAR* IdentifierString() const
    {
        return m_identifierString;
    }

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2Graph
    ///
    /// @brief  Default constructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2Graph() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2Graph
    ///
    /// @brief  Virtual Destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFeature2Graph();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateFeatureGraphDescriptor
    ///
    /// @brief  Ensures the feature graph descriptor is valid
    ///
    /// @param  pFeatureGraphCreateInputInfo Feature graph input creation information containing the feature graph descriptor
    ///
    /// @return TRUE if valid; FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL ValidateFeatureGraphDescriptor(
        ChiFeature2GraphCreateInputInfo* pFeatureGraphCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize a newly-created feature graph object
    ///
    /// @param  pFeatureGraphCreateInputInfo Feature graph input creation information
    ///
    /// @return CDKResultSuccess if successful; otherwise failure result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        ChiFeature2GraphCreateInputInfo* pFeatureGraphCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WalkAllExtSrcLinks
    ///
    /// @brief  Walks all external source links to populate external buffer/metadata info and submits requests to each feature
    ///         to continue request processing.
    ///
    /// @param  pUsecaseRequestObj   Data from usecase required in order to process the request
    ///
    /// @return CDKResultSuccess if successful; otherwise failure result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult WalkAllExtSrcLinks(
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObj);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WalkAllExtSinkLinks
    ///
    /// @brief  Walks all external sink links depth first to
    ///         1) Register downstream callback data (e.g. feature request object) on each link
    ///         2) GetInputDependencies for each feature
    ///         3) Submit requests to each feature to commence request processing
    ///
    /// @param  pUsecaseRequestObj  Data from usecase required in order to process the request
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult WalkAllExtSinkLinks(
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObj);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WalkBackFromLink
    ///
    /// @brief  Walks back to the source node of the given link and based on output link states, recursively walks back or
    ///         submits a request to the upstream feature.
    ///
    /// @param  pUsecaseRequestObj  The usecase request object associated with this feature request.
    /// @param  pOutputLinkData     The data for the link connected to the output port to walk back from.
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult WalkBackFromLink(
        ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj,
        ChiFeature2GraphLinkData*           pOutputLinkData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessUsecaseRequestObjectInputStreamData
    ///
    /// @brief  Processes and stores input data for the usecase request object, to be used upon return from WalkAllSinkLinks.
    ///
    /// @param  pInputLinkData  The data for the link connected to the port that generated an event/output.
    /// @param  extSrcPortId    The port id of the external source input port.
    /// @param  hSettings       Pointer to settings. Can be NULL for default settings.
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessUsecaseRequestObjectInputStreamData(
        ChiFeature2GraphLinkData*        pInputLinkData,
        ChiFeature2GlobalPortInstanceId  extSrcPortId,
        CHIMETAHANDLE                    hSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessInputDepsForUpstreamFeatureRequest
    ///
    /// @brief  Processes input dependencies for the upstream feature request
    ///
    /// @param  pUsecaseRequestObj  The usecase request object associated with this feature request.
    /// @param  pLinkData           The data for the link connected to the port that generated an event/output.
    /// @param  requestIndex        The request index currently being processed
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessInputDepsForUpstreamFeatureRequest(
        ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj,
        ChiFeature2GraphLinkData*           pLinkData,
        UINT8                               requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessUpstreamFeatureRequest
    ///
    /// @brief  Based on output link states, submits a request to the upstream feature.
    ///
    /// @param  pUsecaseRequestObj  The usecase request object associated with this feature request.
    /// @param  pGraphNode          The feature graph node on which to submit the feature request object.
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessUpstreamFeatureRequest(
        ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj,
        ChiFeature2GraphNode*               pGraphNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessGetInputDependencyMessage
    ///
    /// @brief  Processes the GetInputDependency message
    ///
    /// @param  pGraphNode  The graph node that generated the GetInputDependency message
    /// @param  pMessages   The message data to process, including the list of ports corresponding to the message
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessGetInputDependencyMessage(
        ChiFeature2GraphNode*   pGraphNode,
        ChiFeature2Messages*    pMessages);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessReleaseInputDependencyMessage
    ///
    /// @brief  Processes the ReleaseInputDependency message
    ///
    /// @param  pGraphNode  The graph node that generated the ReleaseInputDependency message
    /// @param  pMessages   The message data to process, including the list of ports corresponding to the message
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessReleaseInputDependencyMessage(
        ChiFeature2GraphNode*   pGraphNode,
        ChiFeature2Messages*    pMessages);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessResultMetadataMessage
    ///
    /// @brief  Processes the Result and Metadata messages
    ///
    /// @param  pGraphNode                      The graph node that generated the Result and Metadata message
    /// @param  rDownstreamFeatureRequestObjMap Maps graph nodes to downstream FROs. Used to notify the correct FRO of the
    ///                                         result/metadata.
    /// @param  pMessages                       The message data to process, including the list of ports corresponding to
    ///                                         the message
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessResultMetadataMessage(
        ChiFeature2GraphNode*                       pGraphNode,
        std::vector<ChiFeature2RequestObjectMap>&   rDownstreamFeatureRequestObjMap,
        ChiFeature2Messages*                        pMessages);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessPartialMetadataMessage
    ///
    /// @brief  Processes the Partial Metadata messages
    ///
    /// @param  pGraphNode  The graph node that generated the Partial Metadata message
    /// @param  pMessages   The message data to process, including the list of ports corresponding to the message
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessPartialMetadataMessage(
        ChiFeature2GraphNode*   pGraphNode,
        ChiFeature2Messages*    pMessages);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessProcessingDependenciesCompleteMessage
    ///
    /// @brief  Processes the ProcessingDependenciesComplete message notification. This notification indicates that the
    ///         graph node is done processing and we want to notify each upstream node connected to this node that the
    ///         corresponding ports on this upstream node will not send any further GerInputDependency messages.
    ///
    /// @param  pGraphNode  The graph node that generated the Processing ProcessingDependenciesComplete message
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessProcessingDependenciesCompleteMessage(
        ChiFeature2GraphNode* pGraphNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapInputPortIdToInputLinkData
    ///
    /// @brief  Maps the input port id to the corresponding input link data
    ///
    /// @param  pInputPortId    The input port id to correlate to a link data structure
    /// @param  pGraphNode      The graph node corresponding to the port
    ///
    /// @return Pointer to ChiFeature2GraphLinkData entry
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphLinkData* MapInputPortIdToInputLinkData(
        ChiFeature2GlobalPortInstanceId*    pInputPortId,
        ChiFeature2GraphNode*               pGraphNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapOutputPortIdToOutputLinkData
    ///
    /// @brief  Maps the output port id to the corresponding output link data
    ///
    /// @param  pOutputPortId   The output port id to correlate to a link data structure
    /// @param  pGraphNode      The graph node corresponding to the port
    ///
    /// @return Pointer to ChiFeature2GraphLinkData entry
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2GraphLinkData* MapOutputPortIdToOutputLinkData(
        ChiFeature2GlobalPortInstanceId*    pOutputPortId,
        ChiFeature2GraphNode*               pGraphNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAllOutputLinksDisabled
    ///
    /// @brief  Checks if all links are in the Disabled state.
    ///
    /// @param  pGraphNode   A pointer to the feature graph node on which to check all output links.
    ///
    /// @return TRUE if all output links are in the Disabled state; FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckAllOutputLinksDisabled(
        ChiFeature2GraphNode* pGraphNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAllOutputLinksReadyToProcessRequest
    ///
    /// @brief  Checks if at least one output link from the given node is in the OutputPending state and all the remaining
    ///         output links are in the OutputPending or Disabled state.
    ///
    /// @param  pGraphNode   A pointer to the feature graph node on which to check all output links.
    ///
    /// @return TRUE if at least one output link from the given node is in the OutputPending state and all the remaining
    ///         output links for the given node are in the OutputPending or Disabled state; FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckAllOutputLinksReadyToProcessRequest(
        ChiFeature2GraphNode* pGraphNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStreamBuffer
    ///
    /// @brief  Get pointer to stream buffer from target buffer info handle
    ///
    /// @param  handle              Target buffer info handle containing the associated stream buffer
    /// @param  key                 Unique key identifying the ChiStreamBuffer
    /// @param  ppChiStreamBuffer   Pointer to underlying ChiStreamBuffer
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetStreamBuffer(
        CHITARGETBUFFERINFOHANDLE   handle,
        UINT64                      key,
        CHISTREAMBUFFER**           ppChiStreamBuffer) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputSettingsForInputBuffer
    ///
    /// @brief  Get input settings on given input port
    ///
    /// @param  pExtSrcPortId        The port id of the external source input port.
    /// @param  pFeatureRequestObj   The featurerequestobject to operate on
    /// @param  batchIndex           batch index to retrieve setting
    /// @param  dependencyIndex      dependency index to retrieve setting
    ///
    /// @return CHIMETAHANDLE   on requested port
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHIMETAHANDLE GetInputSettingsForInputBuffer(
        ChiFeature2GlobalPortInstanceId*    pExtSrcPortId,
        ChiFeature2RequestObject*           pFeatureRequestObj,
        UINT8                               batchIndex,
        UINT8                               dependencyIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessChiMessage
    ///
    /// @brief  Process CHI message from a feature in graph. CHI message includes CHIERRORMESSAGE, CHISHUTTERMESSAGE,
    ///         CHISOFMESSAGE and CHIMETABUFFERDONEMESSAGE. Also determines if message type needs to be propagagted
    ///         directly to the graph manager or through the graph
    ///
    /// @param  pFeatureRequestObj      The feature request object to operate on, including the feature graph context
    /// @param  pMessages               The message data to process, including the list of ports corresponding to the message
    ///
    /// @return CDKResultSuccess if successful; otherwise failure result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult ProcessChiMessage(
        ChiFeature2RequestObject* pFeatureRequestObj,
        ChiFeature2Messages*      pMessages);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFeatureMessage
    ///
    /// @brief  Process feature message from a feature in graph
    ///
    /// @param  pFeatureRequestObj      The feature request object to operate on, including the feature graph context
    /// @param  pMessages               The message data to process, including the list of ports corresponding to the message
    ///
    /// @return CDKResultSuccess if successful; otherwise failure result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult ProcessFeatureMessage(
        ChiFeature2RequestObject* pFeatureRequestObj,
        ChiFeature2Messages*      pMessages);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessErrorMessage
    ///
    /// @brief  Process CHI error message from a feature in graph. Note that this function only handles errors that
    ///         are specific to an FRO, device and recovery errors are handled separately within ProcessChiMessage
    ///
    /// @param  pGraphNode                      The graph node associated with the error message
    /// @param  pMessages                       The message data to process, including the list of ports corresponding to the
    ///                                         message
    /// @param  rDownstreamFeatureRequestObjMap Maps graph nodes to downstream FROs. Used to notify the correct FRO of the
    ///                                         error.
    /// @param  frameNumber                     Framework number associated with the error message
    ///
    /// @return CDKResultSuccess if successful; otherwise failure result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessErrorMessage(
        ChiFeature2GraphNode*                       pGraphNode,
        ChiFeature2Messages*                        pMessages,
        std::vector<ChiFeature2RequestObjectMap>&   rDownstreamFeatureRequestObjMap,
        UINT64                                      frameNumber);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PropagateErrorToDownstreamNode
    ///
    /// @brief  Propagate a non fatal error to the graph's downstream node
    ///
    /// @param  pGraphNode                      The graph node associated with the error message
    /// @param  pMessages                       The message data to process, including the list of ports corresponding to the
    ///                                         message
    /// @param  rDownstreamFeatureRequestObjMap Maps graph nodes to downstream FROs. Used to notify the correct FRO of the
    ///                                         error.
    /// @param  frameNumber                     Framework number associated with the error message
    ///
    /// @return CDKResultSuccess if successful; otherwise failure result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PropagateErrorToDownstreamNode(
        ChiFeature2GraphNode*                       pGraphNode,
        ChiFeature2Messages*                        pMessages,
        std::vector<ChiFeature2RequestObjectMap>&   rDownstreamFeatureRequestObjMap,
        UINT64                                      frameNumber);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoesNotificationTypeHaveFRO
    ///
    /// @brief  Is an error type specific to a feature request object? Helper function to help determine what callback
    ///         needs to be used for notification handling
    ///
    /// @param  pMessages   The message data to process, including the list of ports corresponding to the message
    ///
    /// @return TRUE if the error is specific to an FRO, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHX_INLINE BOOL DoesNotificationTypeHaveFRO(
        ChiFeature2Messages* pMessages)
    {
        BOOL result = TRUE;

        if (((ChiMessageTypeError         == pMessages->chiNotification.pChiMessages->messageType) &&
             ((MessageCodeDevice          ==
               pMessages->chiNotification.pChiMessages->message.errorMessage.errorMessageCode) ||
              (MessageCodeTriggerRecovery ==
               pMessages->chiNotification.pChiMessages->message.errorMessage.errorMessageCode))) ||
            (ChiMessageTypeSof            == pMessages->chiNotification.pChiMessages->messageType) ||
            (ChiMessageTypeMetaBufferDone == pMessages->chiNotification.pChiMessages->messageType))
        {
            result = FALSE;
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiStreamForRequestedPort
    ///
    /// @brief  Get input Chi stream of this link and check if the stream is requested from downstream feature
    ///
    /// @param  rInputPortId   The reference of input port id of this link
    /// @param  ppChiStream    Pointer to underlying ChiStream
    /// @param  rNumInputs     The reference of inputs number on this stream
    ///
    /// @return CDKResultSuccess if successful; otherwise failure result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetChiStreamForRequestedPort(
        ChiFeature2GlobalPortInstanceId& rInputPortId,
        CHISTREAM**                      ppChiStream,
        UINT32&                          rNumInputs) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AreAllFROsProcessingComplete
    ///
    /// @brief  Determine whether all FROs in the graph are done processing
    ///
    /// @return TRUE if all FROs in the graph are done processing, false otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AreAllFROsProcessingComplete();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Copy constructor and assignment operator
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2Graph(const ChiFeature2Graph&)               = delete; ///< Disallow the copy constructor
    ChiFeature2Graph& operator= (const ChiFeature2Graph&)   = delete; ///< Disallow assignment operator

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const CHAR*                             m_pFeatureGraphName;            ///< Feature Graph name.
    std::vector<ChiFeature2GraphLinkData>   m_linkData;                     ///< The list of all link data
    std::vector<ChiFeature2GraphLinkData*>  m_pExtSrcLinkData;              ///< The list of pointers to all source link data
    std::vector<ChiFeature2GraphLinkData*>  m_pInternalLinkData;            ///< The list of pointers to all internal link data
    std::vector<ChiFeature2GraphLinkData*>  m_pExtSinkLinkData;             ///< The list of pointers to all sink link data

    std::vector<ChiFeature2GraphNode*>      m_pFeatureGraphNodes;           ///< The list of all feature graph nodes
    ChiFeature2UsecaseRequestObject*        m_pUsecaseRequestObj;           ///< The usecase request object associated with this
                                                                            ///  feature graph

    std::vector<ChiFeature2GraphCallbackData*>   m_pFeatureGraphCallbackData;    ///< Feature graph callback data given to the
                                                                                 ///  client
    ChiFeature2GraphManagerCallbacks             m_featureGraphManagerCallbacks; ///< Callbacks to the feature graph manager
    std::vector<ChiFeature2PortIdToChiStreamMap> m_extSrcPortIdToChiStreamMap;   ///< A list of input port ids
                                                                                 ///  associated with input ChiStreams
    std::vector<ChiFeature2PortIdToChiStreamMap> m_extSinkPortIdToChiStreamMap;  ///< A list of output port ids
                                                                                 ///  associated with output ChiStreams

    std::vector<ChiFeature2UsecaseRequestObjectExtSrcStreamData> m_usecaseRequestObjInputStreamData;    ///< Aggregated input
                                                                                                        ///  stream data for
                                                                                                        ///  usecase request
                                                                                                        ///  object

    CHAR                    m_identifierString[MaxLength256];   ///< Identifier for graph that consists of URO ID & graph name

};

#endif // CHIFEATURE2GRAPH_H
