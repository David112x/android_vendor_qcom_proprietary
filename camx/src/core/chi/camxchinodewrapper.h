////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodewrapper.h
/// @brief Chi node wrapper class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODEWRAPPER_H
#define CAMXCHINODEWRAPPER_H

#include "camxchi.h"
#include "camxdefs.h"
#include "camxmem.h"
#include "camxnode.h"
#include "camxtypes.h"
#include "chinode.h"
#include "camxncssensor.h"
#include "camxncsservice.h"

CAMX_NAMESPACE_BEGIN

struct ChiFence;

struct ChiNodeWrapperCSlHwInfo
{
    BOOL              requireCSLAccess;                              ///< Require CSL Access
    BOOL              requireOutputBuffers;                          ///< Require Output buffers
    BOOL              requireScratchBuffer;                          ///< Indicates if this node require Scratch Buffers
    CSLDeviceHandle   hDevice;                                       ///< CSL Device handle
    UINT32            CSLHwResourceID;                               ///< CSL HW Node resource ID
    CmdBufferManager* pPacketManager;                                ///< IQ Packet buffer manager
    Packet*           pPacket;                                       ///< CSL packet
    CmdBufferManager* pCommandBufferManager[ChiNodeMaxCSLCmdBuffer]; ///< Command buffer manager
    UINT32            numCommandBufferManager;                       ///< Number of Command buffer manager
    CmdBuffer*        pCmdBuffer[ChiNodeMaxCSLCmdBuffer];            ///< CmdBuffer array
    UINT32            sizeCmdBufferData[ChiNodeMaxCSLCmdBuffer];     ///< Data to be written into each command buffer
    CSLBufferInfo*    pScratchMemoryBuffer;                          ///< Scratch Buffer
};

const UINT32 CANARY          = 0xCA113D17;
const UINT   RequestWaitTime = 5000; // Maximum delay of 5 seconds in chi nodes for request processing

struct ChiNodeBufferHandleWrapper
{
    CHIIMAGELIST        hBuffer;       ///< Must be the first element of this struct
    UINT32              canary;        ///< Canary to detect memory corruption
    ImageBuffer*        pImageBuffer;  ///< Pointer to the original image buffer
};

CAMX_STATIC_ASSERT(0 == offsetof(ChiNodeBufferHandleWrapper, hBuffer));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the Chi node wrapper class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiNodeWrapper final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create ChiNodeWrapper Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete ChiNodeWrapper object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiNodeWrapper* Create(
        const NodeCreateInputData*  pCreateInputData,
        NodeCreateOutputData*       pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupNCSLink
    ///
    /// @brief  Setup an NCS link
    ///
    /// @param  pSensorConfig pointer to the sensor config structure
    ///
    /// @return Pointer to NCS Sensor object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static NCSSensor* SetupNCSLink(
        NCSSensorConfig* pSensorConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroyNCSLink
    ///
    /// @brief  Destroy NCS link
    ///
    /// @param  pSensor pointer to the sensor handle
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult DestroyNCSLink(
        VOID* pSensor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetMetadata
    ///
    /// @brief  The implemenation for PFNCHIGETMETADATA defined in chinode.h
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNGetMetadata(
        CHIMETADATAINFO* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetMultiCamDynamicMetaByCamId
    ///
    /// @brief  The implemenation for PFNCHIGETMULTICAMDYNAMICMETABYCAMID defined in chinode.h
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the node.
    /// @param  cameraId        CameraId associated with the metadata from which the list of tags must be fetched. This camera
    ///                         id must be posted with metadata. InvalidCameraId if dont care
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNGetMultiCamDynamicMetaByCamId(
        CHIMETADATAINFO* pMetadataInfo,
        UINT32           cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetSupportedPSMetadataList
    ///
    /// @brief  Gets a list of per-port metadata supported by the Driver
    ///
    /// @param  pMetadataIdArray   Pointer to a structure that defines the array of metadata Ids
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNGetSupportedPSMetadataList(
        CHIMETADATAIDARRAY* pMetadataIdArray);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetPSMetadata
    ///
    /// @brief  Gets a list of metadata information based on metadata tags.
    ///
    ///         The tag must be a per-port metadata defined by driver. This can be queried through
    ///         PFNCHIGETSUPPORTEDPSMETADATALIST
    ///         If the metadata information associated with the tag is not published,
    ///         Chi returns those tags as unpublished when this function returns. The component can add them in dependency
    ///         reporting.
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by component.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNGetPSMetadata(
        CHIPSMETADATA* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNSetPSMetadata
    ///
    /// @brief  Sets a list of metadata information based on metadata tags.
    ///
    ///         The tag must be a per-port metadata defined by driver. This can be queried through
    ///         PFNCHIGETSUPPORTEDPSMETADATALIST
    ///         When published, Chi driver will notify all other component that reported these tags as dependencies.
    ///
    /// @param  pMetadataInfo Pointer to a structure that defines the list of metadata information published by component.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNSetPSMetadata(
        CHIPSMETADATA* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNPublishPSMetadata
    ///
    /// @brief  Publishes a list of metadata information based on metadata tags.
    ///
    ///         The tag must be a per-port metadata defined by driver. This can be queried through
    ///         PFNCHIGETSUPPORTEDPSMETADATALIST
    ///         When published, Chi driver will notify all other component that reported
    ///         these tags as dependencies.
    ///
    /// @param  metadataId   Metadata identifier
    /// @param  pBypassInfo  If non-NULL, contains the map of input output ports to fetch the metadata
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNPublishPSMetadata(
        UINT32                   metadataId,
        CHIPSMETADATABYPASSINFO* pBypassInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetStaticMetadata
    ///
    /// @brief  The implemenation for PFNNODEGETSTATICMETADATA defined in chinode.h
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the node.
    /// @param  cameraId        Camera Id for this request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNGetStaticMetadata(
        CHIMETADATAINFO* pMetadataInfo,
        UINT32           cameraId);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNIsPSMetadataPublished
    ///
    /// @brief  The implemenation for PFNISPSMETADATAPUBLISHED defined in chinode.h
    ///
    /// @param  pPSTagInfo Pointer to a structure that define port specific tag info
    ///
    /// @return True if published, otherwise False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL FNIsPSMetadataPublished(
        CHIPSTAGINFO* pPSTagInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNSetMetadata
    ///
    /// @brief  The implemenation for PFNNODESETMETADATA defined in chinode.h
    ///
    /// @param  pMetadataInfo Pointer to a structure that defines the list of metadata information published by the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNSetMetadata(
        CHIMETADATAINFO* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetVendorTagBase
    ///
    /// @brief  The implemenation for PFNNODEGETVENDORTAGBASE defined in chinode.h
    ///
    /// @param  pVendorTagBaseInfo  Pointer to a structure that defines the run-time Chi assigned vendor tag base.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNGetVendorTagBase(
        CHIVENDORTAGBASEINFO* pVendorTagBaseInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetTuningmanager
    ///
    /// @brief  The implementation for PFNNODEGETTUNINGMANAGER defined in chinode.h
    ///
    /// @param  hChiSession  Chi driver handle that node can use to callback into Chi
    /// @param  pDataRequest Pointer to data request
    ///
    /// @return Handle to the tuning manager.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* FNGetTuningmanager(
        CHIHANDLE       hChiSession,
        CHIDATAREQUEST* pDataRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNProcRequestDone
    ///
    /// @brief  The implemenation for PFNCHINODEPROCREQUESTDONE defined in chinode.h
    ///
    /// @param  pInfo   Pointer to the structure containing information about the request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNProcRequestDone(
        CHINODEPROCESSREQUESTDONEINFO* pInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNProcMetadataDone
    ///
    /// @brief  The implemenation for PFNCHINODEPROCMETADATADONE defined in chinode.h
    ///
    /// @param  pInfo   Pointer to the structure containing information about the request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNProcMetadataDone(
        CHINODEPROCESSMETADATADONEINFO* pInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNCreateFence
    ///
    /// @brief  The implemenation for PFNCHINODECREATEFENCE defined in chinode.h
    ///
    /// @param  hChiSession Chi driver handle that node can use to callback into Chi
    /// @param  pInfo       Pointer to the structure containing information about the fence.
    /// @param  phChiFence  Pointer to Chi fence handle to be filled.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNCreateFence(
        CHIHANDLE               hChiSession,
        CHIFENCECREATEPARAMS*   pInfo,
        CHIFENCEHANDLE*         phChiFence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNReleaseFence
    ///
    /// @brief  The implemenation for PFNCHINODERELEASEFENCE defined in chinode.h
    ///
    /// @param  hChiSession Chi driver handle that node can use to callback into Chi
    /// @param  hChiFence   Handle to Chi fence to be released
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNReleaseFence(
        CHIHANDLE       hChiSession,
        CHIFENCEHANDLE  hChiFence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetDataSource
    ///
    /// @brief  The implemenation for PFNGETDATASOURCE defined in chinode.h
    ///
    /// @param  hChiSession        Chi session hadle
    /// @param  phDataSourceHandle Data source handle
    /// @param  pDataSourceConfig  Data source config
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNGetDataSource(
        CHIHANDLE            hChiSession,
        CHIDATASOURCE*       phDataSourceHandle,
        CHIDATASOURCECONFIG* pDataSourceConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetData
    ///
    /// @brief  The implemenation for PFNGETDATA defined in chinode.h
    ///
    /// @param  hChiSession       Chi session handle
    /// @param  hDataSourceHandle Data source handle
    /// @param  pDataRequest      Data request payload
    /// @param  pSize             Pointer to be filled in with data size
    ///
    /// @return Pointer to the data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* FNGetData(
        CHIHANDLE            hChiSession,
        CHIDATASOURCEHANDLE  hDataSourceHandle,
        CHIDATAREQUEST*      pDataRequest,
        UINT*                pSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNPutData
    ///
    /// @brief  The implemenation for PFNPUTDATA defined in chinode.h
    ///
    /// @param  hChiSession       Chi session handle
    /// @param  hDataSourceHandle Data source handle
    /// @param  hData             Data handle
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNPutData(
        CHIHANDLE            hChiSession,
        CHIDATASOURCEHANDLE  hDataSourceHandle,
        CHIDATAHANDLE        hData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetDataNCS
    ///
    /// @brief  API to get NCS data
    ///
    /// @param  hChiSession       Chi session handle
    /// @param  hDataSourceHandle Data source handle
    /// @param  pDataRequest      Data request handle
    /// @param  pSize             Size of the data
    ///
    /// @return Pointer to the data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* FNGetDataNCS(
        CHIHANDLE            hChiSession,
        CHIDATASOURCEHANDLE  hDataSourceHandle,
        CHIDATAREQUEST*      pDataRequest,
        UINT*                pSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNPutDataSource
    ///
    /// @brief  The implemenation for PFNGETDATA defined in chinode.h
    ///
    /// @param  hChiSession        Chi Session handle
    /// @param  hDataSourceHandle  Data source handle
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNPutDataSource(
        CHIHANDLE           hChiSession,
        CHIDATASOURCEHANDLE hDataSourceHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNWaitFenceAsync
    ///
    /// @brief  The implemenation for PFNCHINODEWAITFENCEASYNC defined in chinode.h
    ///
    /// @param  hChiSession Chi driver handle that node can use to callback into Chi
    /// @param  pCallbackFn Callback function
    /// @param  hChiFence   Handle to Chi fence to be released
    /// @param  pData       User data pointer
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNWaitFenceAsync(
        CHIHANDLE           hChiSession,
        PFNCHIFENCECALLBACK pCallbackFn,
        CHIFENCEHANDLE      hChiFence,
        VOID*               pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNSignalFence
    ///
    /// @brief  The implemenation for PFNCHINODESIGNALFENCE defined in chinode.h
    ///
    /// @param  hChiSession  Chi driver handle that node can use to callback into Chi
    /// @param  hChiFence    Handle to Chi fence to be released
    /// @param  statusResult Fence signalled status result
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNSignalFence(
        CHIHANDLE           hChiSession,
        CHIFENCEHANDLE      hChiFence,
        CDKResult           statusResult);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetFenceStatus
    ///
    /// @brief  The implemenation for PFNGETFENCESTATUS defined in chinode.h
    ///
    /// @param  hChiSession Chi driver handle that node can use to callback into Chi
    /// @param  hChiFence   Handle to Chi fence to be queried for status
    /// @param  pResult     Pointer to the CDKResult pointer to be filled with fence status
    ///                     CDKResultSuccess        - If the fence is signalled with Success
    ///                     CDKResultEFailed        - If the fence is signalled with Failure
    ///                     CDKResultEInvalidState  - If the fence is not yet signalled
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNGetFenceStatus(
        CHIHANDLE               hChiSession,
        CHIFENCEHANDLE          hChiFence,
        CDKResult*              pResult);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ComponentName
    ///
    /// @brief  Return the Chi node's component name string
    ///
    /// @return Component name
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CHAR* ComponentName() const
    {
        return m_pComponentName;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNBufferManagerCreate
    ///
    /// @brief  Creates an image buffer manager with given name
    ///
    /// @param  pBufferManagerName String of Buffer manager's name
    /// @param  pCreateData        Pointer to BufferManager create data structure
    ///
    /// @return A wrapped CHINODEBUFFERHANDLE if success or NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHIBUFFERMANAGERHANDLE FNBufferManagerCreate(
        const CHAR*                     pBufferManagerName,
        CHINodeBufferManagerCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNBufferManagerDestroy
    ///
    /// @brief  Destroy an image buffer manager
    ///
    /// @param  hBufferManager Buffer manager handle to destroy
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID FNBufferManagerDestroy(
        CHIBUFFERMANAGERHANDLE hBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNBufferManagerGetImageBuffer
    ///
    /// @brief  Obtains an image buffer from a provided buffer manager handle
    ///
    /// @param  hBufferManager Buffer manager handle from which to obtain an image buffer
    ///
    /// @return A wrapped CHINODEBUFFERHANDLE if success or NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHINODEBUFFERHANDLE FNBufferManagerGetImageBuffer(
        CHIBUFFERMANAGERHANDLE hBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNBufferManagerReleaseReference
    ///
    /// @brief  Release an image buffer reference from a provided image buffer. Caller is responsible to provide the associated
    ///         buffer manager to prove ownership over buffer.
    ///
    /// @param  hBufferManager    Chi driver handle that node can use to callback into Chi
    /// @param  hNodeBufferHandle Buffer handle to release
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNBufferManagerReleaseReference(
        CHIBUFFERMANAGERHANDLE hBufferManager,
        CHINODEBUFFERHANDLE    hNodeBufferHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNGetFlushInfo
    ///
    /// @brief  The implementation for Get flush info
    ///
    /// @param  hChiSession Chi driver handle that node can use to callback into Chi
    /// @param  pFlushInfo  Pointer to the flush info
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNGetFlushInfo(
        CHIHANDLE     hChiSession,
        CHIFLUSHINFO* pFlushInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FNCacheOps
    ///
    /// @brief  The implemenation for PFNCACHEOPS defined in chinode.h
    ///
    /// @param  hChiBuffer   Chi buffer handle
    /// @param  invalidata   invalidate cache
    /// @param  clean        clean cache
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FNCacheOps(
        CHINODEBUFFERHANDLE hChiBuffer,
        BOOL                invalidata,
        BOOL                clean);

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFinalizeInitialization
    ///
    /// @brief  Method to finalize the initialization of the node in the pipeline
    ///
    /// @param  pFinalizeInitializationData Finalize data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeFinalizeInitialization(
        FinalizeInitializationData* pFinalizeInitializationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeInitialize
    ///
    /// @brief  Initialize the sw processing object
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeInitialize(
        const NodeCreateInputData*  pCreateInputData,
        NodeCreateOutputData*       pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFinalizeInputRequirement
    ///
    /// @brief  Implemented by derived nodes to determine its input buffer requirement based on all the output buffer
    ///         requirements
    ///
    /// @param  pBufferNegotiationData  Negotiation data for all output ports of a node
    ///
    /// @return Success if the negotiation was successful, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeFinalizeInputRequirement(
        BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FinalizeBufferProperties
    ///
    /// @brief  Finalize the buffer properties of each output port
    ///
    /// @param  pBufferNegotiationData Buffer negotiation data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID FinalizeBufferProperties(
        BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostPipelineCreate
    ///
    /// @brief  virtual method to be called at NotifyTopologyCreated time; node should take care of updates and initialize
    ///         blocks that has dependency on other nodes in the topology at this time.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PostPipelineCreate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Pure virtual method to trigger process request for the stats processing node object.
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ExecuteProcessRequest(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFenceCallback
    ///
    /// @brief  Chi fence callback
    ///
    /// @param  hChiFence   Handle to Chi fence this callback is called for
    /// @param  pUserData   User data provided when waiting on the fence
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ChiFenceCallback(
        CHIFENCEHANDLE  hChiFence,
        VOID*           pUserData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFenceDependencyCallback
    ///
    /// @brief  Chi fence callback for fence dependency
    ///
    /// @param  hChiFence   Handle to Chi fence this callback is called for
    /// @param  pUserData   User data provided when waiting on the fence
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ChiFenceDependencyCallback(
        CHIFENCEHANDLE  hChiFence,
        VOID*           pUserData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareStreamOn
    ///
    /// @brief  virtual method to that will be called before streamOn command is sent to HW. HW nodes may use
    ///         this hook to do any preparation, or per-configure_stream one-time configuration.
    ///         This is generally called in FinalizePipeline, i.e within a lifetime of pipeline, this is called only once.
    ///         Actual StreamOn may happen much later based on Activate Pipeline. Nodes can use this to do any one time
    ///         setup that is needed before stream.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PrepareStreamOn();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnStreamOn
    ///
    /// @brief  virtual method to that will be called after streamOn command is sent to HW. HW nodes may use
    ///         this hook to do any stream on configuration. This is generally called everytime ActivatePipeline is called.
    ///         Nodes may use this to setup things that are required while streaming. For exa, any resources that are needed
    ///         only during streaming can be allocated here. Make sure to do light weight operations here as this might delay
    ///         processing of the first request.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult OnStreamOn();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnStreamOff
    ///
    /// @brief  virtual method to that will be called before streamOff command is sent to HW. HW nodes may use
    ///         this hook to do any preparation. This is generally called on every Deactivate Pipeline.
    ///         Nodes may use this to release things that are not required at the end of streaming. For exa, any resources
    ///         that are not needed after stream-on can be released here. Make sure to do light weight operations here as
    ///         releasing here may result in re-allocating resources in OnStreamOn.
    ///
    /// @param  modeBitmask Stream off mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult OnStreamOff(
        CHIDEACTIVATEPIPELINEMODE modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiNodeWrapper
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiNodeWrapper();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiNodeWrapper
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiNodeWrapper();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataPublishList
    ///
    /// @brief  Method to query the publish list from the node
    ///
    /// @param  pPublistTagList List of tags published by the node
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult QueryMetadataPublishList(
        NodeMetadataList* pPublistTagList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CancelRequest
    ///
    /// @brief  Method to release/clear internal resources that derive node might have allocated/reserved for a request
    ///
    /// @param  requestId Request for which cmdbuffers are complete
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult CancelRequest(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushResponseTimeInMs
    ///
    /// @brief  Return worst-case response time of the node for flush call
    ///
    /// @return worst-case response time in milliseconds.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT64 GetFlushResponseTimeInMs();

private:
    ChiNodeWrapper(const ChiNodeWrapper&) = delete;                                       ///< Disallow the copy constructor
    ChiNodeWrapper& operator=(const ChiNodeWrapper&) = delete;                            ///< Disallow assignment operator

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupAndCommitHWData
    ///
    /// @brief  Setup packet and commit the filled HW data
    ///
    /// @param  pInfo        CHI Processrequest Info
    /// @param  requestID    Request ID
    /// @param  hwPacketType Packet type to commit
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupAndCommitHWData(
        CHINODEPROCESSREQUESTINFO* pInfo,
        UINT64                     requestID,
        ChiCSLHwPacketContextType  hwPacketType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CommitAndSubmitPacket
    ///
    /// @brief  Helper method to commit Command Buffer and Packet buffers and do CSL submit
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CommitAndSubmitPacket();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddCmdBufferReference
    ///
    /// @brief  Helper method to add command buffer reference to CSL packet
    ///
    /// @param  requestId  Request number to which command buffer refrence needs to be added
    /// @param  opcode     CSL opcode init/Update
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddCmdBufferReference(
        UINT64 requestId,
        UINT32 opcode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireDevice
    ///
    /// @brief  Helper method to acquire HW device
    ///
    /// @param  pCSLInfo CSL HW Info
    ///
    /// @return CamxResult CamxResultSuccess on success or appropriate error
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AcquireDevice(
        CHICSLHWINFO* pCSLInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseDevice
    ///
    /// @brief  Helper method to release IFE device
    ///
    /// @return CamxResult CamxResultSuccess on success or appropriate error
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdAndScratchBuffers
    ///
    /// @brief  Helper method to initialize command manager, allocate command buffers and scratch buffer
    ///
    /// @param  pCSLInfo Pointer to the CSL info
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateCmdAndScratchBuffers(
        CHICSLHWINFO* pCSLInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMulticamDynamicMetaByCamId
    ///
    /// @brief  Gets a list of metadata information based on metadata tags by camera id for multicamera metadata.
    ///         The tag can be an Android tag or a vendor tag. If the metadata information associated with the tag is not
    ///         published, Chi returns those tags as unpublished when this function returns. The node can add them in the
    ///         dependency reporting during ChiNodeProcRequest().
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the node.
    /// @param  cameraId        CameraId of the metadata from which the list of tags must be fetched.This camera id must
    ///                         be set while posting metadata. InvalidCameraId if dont care. InvalidCameraId if dont care
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetMulticamDynamicMetaByCamId(
        CHIMETADATAINFO* pMetadataInfo,
        UINT32           cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDynamicMetadata
    ///
    /// @brief  Gets a list of dynamic metadata information based on metadata tags.
    ///         If the metadata information associated with the tag is not published, Chi returns those tags as unpublished
    ///         when this function returns. The node can add them in the dependency reporting during ChiNodeProcRequest().
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the node.
    /// @param  cameraId        CameraId of the metadata from which the list of tags must be fetched. This camera id must
    ///                         be set while posting metadata. InvalidCameraId if dont care
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetDynamicMetadata(
        CHIMETADATAINFO* pMetadataInfo,
        UINT32           cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadata
    ///
    /// @brief  Gets a list of metadata information based on metadata tags.
    ///         The tag can be an Android tag or a vendor tag. If the metadata information associated with the tag is not
    ///         published, Chi returns those tags as unpublished when this function returns. The node can add them in the
    ///         dependency reporting during ChiNodeProcRequest().
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetMetadata(
        CHIMETADATAINFO* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticMetadata
    ///
    /// @brief  Gets a list of metadata information based on metadata tags.
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the node.
    /// @param  cameraId        Camera Id for this request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetStaticMetadata(
        CHIMETADATAINFO* pMetadataInfo,
        UINT32           cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMetadata
    ///
    /// @brief  Sets a list of metadata information based on metadata tags.
    ///
    ///         The tag can be an Android tag or a vendor tag. When published, Chi driver will notify all other nodes that
    ///         reported these tags as dependencies.
    ///
    /// @param  pMetadataInfo Pointer to a structure that defines the list of metadata information published by the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetMetadata(
        CHIMETADATAINFO* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPortMetadata
    ///
    /// @brief  Gets a list of metadata information based on metadata tags.
    ///
    ///         The tag must be a per-port metadata defined by driver. This can be queried through
    ///         PFNCHIGETSUPPORTEDPSMETADATALIST
    ///         If the metadata information associated with the tag is not published,
    ///         Chi returns those tags as unpublished when this function returns. The component can add them in dependency
    ///         reporting.
    ///
    /// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by component.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetPortMetadata(
        CHIPSMETADATA* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPortMetadata
    ///
    /// @brief  Sets a list of metadata information based on metadata tags.
    ///
    ///         The tag must be a per-port metadata defined by driver. This can be queried through
    ///         PFNCHIGETSUPPORTEDPSMETADATALIST
    ///         When published, Chi driver will notify all other component that reported these tags as dependencies.
    ///
    /// @param  pMetadataInfo Pointer to a structure that defines the list of metadata information published by component.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetPortMetadata(
        CHIPSMETADATA* pMetadataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPortMetadata
    ///
    /// @brief  Publish port-link metadata for all ports
    ///
    /// @param  tagId        Metadata identifier
    /// @param  pBypassInfo  If non-NULL, contains the map of input output ports to fetch the metadata
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPortMetadata(
        UINT32                     tagId,
        CHIPSMETADATABYPASSINFO* pBypassInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCSLHwInfoAndAcquire
    ///
    /// @brief  The implementation for Set CSL info
    ///
    /// @param  pCSLInfo    Pointer to the CSL info
    ///
    /// @return CamxResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetCSLHwInfoAndAcquire(
        CHICSLHWINFO* pCSLInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVendorTagBase
    ///
    /// @brief  Get the base of vendor tags of other nodes.
    ///
    ///         The tag can be from nodes including the Chi default node or other third-party custom nodes on which this node is
    ///         dependent for request processing. The node uses this base and the actual tag offset to derive the
    ///         tag ID(= base + offset) to report the dependencies.
    ///
    /// @param  pVendorTagBaseInfo  Pointer to a structure that defines the run-time Chi assigned vendor tag base.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetVendorTagBase(
        CHIVENDORTAGBASEINFO* pVendorTagBaseInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcRequestDone
    ///
    /// @brief  Reports the status of request processing.
    ///
    ///         The node calls this function into Chi driver to report the status as success or failure. The fences that are
    ///         associated with the request are signaled accordingly by Chi for this request.
    ///
    /// @param  pInfo   Pointer to the structure containing information about the request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcRequestDone(
        CHINODEPROCESSREQUESTDONEINFO* pInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcMetadataDone
    ///
    /// @brief  Reports the status of processing metadata.
    ///
    ///         The node calls this function into Chi driver to report the status as success or failure of metadata
    ///
    /// @param  pInfo   Pointer to the structure containing information about the request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcMetadataDone(
        CHINODEPROCESSMETADATADONEINFO* pInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFence
    ///
    /// @brief  Creates a Chi fence give the input parameters.
    ///
    /// @param  pInfo       Pointer to the structure containing information about the fence.
    /// @param  phChiFence  Pointer to Chi fence handle to be filled.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateFence(
        CHIFENCECREATEPARAMS*   pInfo,
        CHIFENCEHANDLE*         phChiFence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseFence
    ///
    /// @brief  Releases a Chi fence.
    ///
    /// @param  hChiFence   Handle to Chi fence to be released
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ReleaseFence(
        CHIFENCEHANDLE  hChiFence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateChiBufferHandlePool
    ///
    /// @brief  Allocate the pool for CHIBUFFERHANDLE
    ///
    /// @param  size    The size of CHIBUFFERHANDLE pool
    ///
    /// @return CamResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHINODEBUFFERHANDLE* AllocateChiBufferHandlePool(
        INT32 size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseChiBufferHandlePool
    ///
    /// @brief  Free the allocated pool for CHIBUFFERHANDLE
    ///
    /// @param  phBufferHandle Free the allocated pool in phBufferHandle
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseChiBufferHandlePool(
        CHINODEBUFFERHANDLE* phBufferHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImageBufferToChiBuffer
    ///
    /// @brief  Populate ChiImage buffer information from camx ImageBuffer object
    ///
    /// @param  pImageBuffer            The ImageBuffer instance to be converted [DO NOT MODIFY]
    /// @param  pChiImageBuffer         Pointer to ChiImageList to populate Image Buffer information
    /// @param  bPopulateBufferHandles  Whether to populate buffer handle information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ImageBufferToChiBuffer(
        ImageBuffer*        pImageBuffer,
        CHIIMAGELIST*       pChiImageBuffer,
        BOOL                bPopulateBufferHandles);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImageFormatToChiFormat
    ///
    /// @brief  Populate Chi format information from camx Format
    ///
    /// @param  pFormat     Pointer to Camx format to be populated
    /// @param  pChiFormat  Pointer to ChiImageFormat to populate Format information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ImageFormatToChiFormat(
        const ImageFormat * pFormat,
        CHIIMAGEFORMAT*     pChiFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiNodeCapsMask
    ///
    /// @brief  Get ChiNode capabilities mask from NodeProerties
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pNodeCreateInfo   Pointer to a structure that defines create session information for the node.
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetChiNodeCapsMask(
        const NodeCreateInputData*  pCreateInputData,
        CHINODECREATEINFO*          pNodeCreateInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CDKResultToCamxResult
    ///
    /// @brief  Convert CDKResult to CamxResult
    ///
    /// @param  result The CDKResult
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CDKResultToCamxResult(
        CDKResult result);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CamxResultToCDKResult
    ///
    /// @brief  Convert camxResult to CDKResult
    ///
    /// @param  result The CDKResult
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult CamxResultToCDKResult(
        CamxResult result);

    struct PerRequestData
    {
        BOOL      isDelayedRequestDone;         ///< Delayed request done
        UINT      numFences;                    ///< Num fences
        CSLFence* phFence[MaxOutputBuffers];    ///< Fences
    };

    PerRequestData          m_perRequestData[MaxRequestQueueDepth];     ///< Per request data
    ChiNodeCallbacks        m_nodeCallbacks;                            ///< The callback functions from the CHI node
    CHIHANDLE               m_hNodeSession;                             ///< The node session identifier return by
                                                                        ///  creating the CHI node
    UINT                    m_numInputPort;                             ///< The number of input port
    CHINODEBUFFERHANDLE*    m_phInputBuffer;                            ///< The pointer to array of CHIBUFFERHANDLE for
                                                                        ///  input buffer, the size of array will be
                                                                        ///  determined by m_numInputPort
    UINT                    m_numOutputPort;                            ///< The number of output port
    CHINODEBUFFERHANDLE*    m_phOutputBuffer;                           ///< The pointer to array of CHIBUFFERHANDLE
                                                                        ///  for output buffer, the size of array will
                                                                        ///  be determined by m_numOutputPort
    CHINODEBYPASSINFO*      m_pBypassData;                              ///< Info for bypassable output ports
    const Metadata*         m_pStaticMetadata;                          ///< The static metadatapool
    ChiContext*             m_pChiContext;                              ///< The pointer to ChiContext
    ChiNodeCapsInfo         m_nodeCapsMask;                             ///< Node caps mask
    CHAR*                   m_pComponentName;                           ///< Component name
    UINT                    m_instancePropertyCapsMask;                 ///< GPU Node Instance Property Capabilities mask
    BOOL                    m_canNodeSetBufferDependency;               ///< Flag to indicate if node can set dependency
    ChiNodeWrapperCSlHwInfo m_CSLHwInfo;                                ///< CSL Hw Info
    Mutex*                  m_pRequestLock;                            ///< Mutex to protect requestInProgress
    Condition*              m_pRequestProgress;                        ///< condition to singal when the request done
    BOOL                    m_requestInProgress[MaxRequestQueueDepth]; ///< Request is in progress
    BOOL                    m_flushInProgress;                         ///< Flush is in progress
};

CAMX_NAMESPACE_END

#endif // CAMXCHINODEWRAPPER_H
