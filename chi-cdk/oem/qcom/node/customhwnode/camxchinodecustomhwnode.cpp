////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodecustomhwnode.cpp
/// @brief Chi node for cutomhwnode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <inttypes.h>
#include <media/cam_custom.h>
#include <system/camera_metadata.h>

#include "camxchinodecustomhwnode.h"

// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads
// NOWHINE FILE NC008: Warning: - Var names should be lower camel case

#undef LOG_TAG
#define LOG_TAG "CHICUSTOMHWNODE"

#define PRINT_METADATA FALSE

ChiNodeInterface g_ChiNodeInterface;    ///< The instance save the CAMX Chi interface
UINT32           g_vendorTagBase = 0;   ///< Chi assigned runtime vendor tag base for the node

/// @todo (CAMX-1854) the major / minor version shall get from CHI
static const UINT32 ChiNodeMajorVersion = 0;    ///< The major version of CHI interface
static const UINT32 ChiNodeMinorVersion = 0;    ///< The minor version of CHI interface

static const UINT32 ChiNodeCapsCustomHwNode        = 1;                       ///< Custom HW Node
static const UINT32 CustomHwNodeNodeTagCurrentMode = 1;                       ///< Tag to indicated current operation mode
static const CHAR   CustomHwNodeNodeSectionName[]  = "com.qti.node.customhw"; ///< The section name for node

///< Supported vendor tag list, it shall align with the definition in g_VendorTagSectionCustomHwNode
static const UINT32 g_VendorTagList[] =
{
    CustomHwNodeNodeTagCurrentMode,
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGDATA g_VendorTagSectionCustomHwNode[] =
{
    { "CurrentMode", TYPE_INT32,  1 },
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGSECTIONDATA g_VendorTagCustomHwNodeSection[] =
{
    {
        CustomHwNodeNodeSectionName,  0,
        sizeof(g_VendorTagSectionCustomHwNode) / sizeof(g_VendorTagSectionCustomHwNode[0]), g_VendorTagSectionCustomHwNode,
        CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    }
};

///< This is an array of all vendor tag section data
static ChiVendorTagInfo g_VendorTagInfoCustomHwNode[] =
{
    {
        &g_VendorTagCustomHwNodeSection[0],
        sizeof(g_VendorTagCustomHwNodeSection) / sizeof(g_VendorTagCustomHwNodeSection[0])
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeGetCaps
///
/// @brief  Implementation of PFNNODEGETCAPS defined in chinode.h
///
/// @param  pCapsInfo   Pointer to a structure that defines the capabilities supported by the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CustomHwNodeNodeGetCaps(
    CHINODECAPSINFO* pCapsInfo)
{
    CDKResult result = CDKResultSuccess;

    if (pCapsInfo == NULL)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pCapsInfo is NULL");
    }
    else
    {
        if (pCapsInfo->size >= sizeof(CHINODECAPSINFO))
        {
            // Add and extend the support for multiple capabilities
            pCapsInfo->nodeCapsMask = ChiNodeCapsCustomHwNode;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODECAPSINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeQueryVendorTag
///
/// @brief  Implementation of PFNCHIQUERYVENDORTAG defined in chinode.h
///
/// @param  pQueryVendorTag Pointer to a structure that returns the exported vendor tag
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CustomHwNodeNodeQueryVendorTag(
    CHIQUERYVENDORTAG* pQueryVendorTag)
{
    CDKResult result = CDKResultSuccess;
    result = ChiNodeUtils::QueryVendorTag(pQueryVendorTag, g_VendorTagInfoCustomHwNode);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeCreate
///
/// @brief  Implementation of PFNNODECREATE defined in chinode.h
///
/// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CustomHwNodeNodeCreate(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult        result = CDKResultSuccess;
    ChiCustomHwNode* pNode  = NULL;

    if (NULL == pCreateInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pCreateInfo is NULL");
    }

    if ((CDKResultSuccess == result) && (pCreateInfo->size < sizeof(CHINODECREATEINFO)))
    {
        LOG_ERROR(CamxLogGroupChi, "CHINODECREATEINFO is smaller than expected");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        pNode = new ChiCustomHwNode;
        if (pNode == NULL)
        {
            result = CDKResultENoMemory;
            LOG_ERROR(CamxLogGroupChi, "Not enough memory to create CHI Custom Node");
        }
    }

    if (CDKResultSuccess == result)
    {
        result = pNode->Initialize(pCreateInfo);
    }

    if (CDKResultSuccess == result)
    {
        pCreateInfo->phNodeSession = reinterpret_cast<CHIHANDLE*>(pNode);
    }
    else
    {
        delete pNode;
        pNode = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeDestroy
///
/// @brief  Implementation of PFNNODEDESTROY defined in chinode.h
///
/// @param  pDestroyInfo    Pointer to a structure that defines the session destroy information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CustomHwNodeNodeDestroy(
    CHINODEDESTROYINFO* pDestroyInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pDestroyInfo) || (NULL == pDestroyInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument [pDestroyInfo: %p | pDestroyInfo->hNodeSession is NULL]", pDestroyInfo);
    }
    else
    {
        if (pDestroyInfo->size >= sizeof(CHINODEDESTROYINFO))
        {
            ChiCustomHwNode* pNode     = static_cast<ChiCustomHwNode*>(pDestroyInfo->hNodeSession);
            delete pNode;

            pNode                      = NULL;
            pDestroyInfo->hNodeSession = NULL;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEDESTROYINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeQueryBufferInfo
///
/// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
///
/// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CustomHwNodeNodeQueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pQueryBufferInfo) || (NULL == pQueryBufferInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument [pQueryBufferInfo: %p | pQueryBufferInfo->hNodeSession is NULL]",
            pQueryBufferInfo);
    }
    else
    {
        if (pQueryBufferInfo->size >= sizeof(CHINODEQUERYBUFFERINFO))
        {
            ChiCustomHwNode* pNode = static_cast<ChiCustomHwNode*>(pQueryBufferInfo->hNodeSession);
            result               = pNode->QueryBufferInfo(pQueryBufferInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEQUERYBUFFERINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeSetBufferInfo
///
/// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
///
/// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CustomHwNodeNodeSetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pSetBufferInfo) || (NULL == pSetBufferInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument [pSetBufferInfo: %p | pSetBufferInfo->hNodeSession is NULL]",
            pSetBufferInfo);
    }
    else
    {
        if (pSetBufferInfo->size >= sizeof(CHINODESETBUFFERPROPERTIESINFO))
        {
            ChiCustomHwNode* pNode = static_cast<ChiCustomHwNode*>(pSetBufferInfo->hNodeSession);
            result = pNode->SetBufferInfo(pSetBufferInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODESETBUFFERPROPERTIESINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeProcRequest
///
/// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CustomHwNodeNodeProcRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pProcessRequestInfo) || (NULL == pProcessRequestInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument [pProcessRequestInfo: %p | pProcessRequestInfo->hNodeSession is NULL]",
            pProcessRequestInfo);
    }
    else
    {
        if (pProcessRequestInfo->size >= sizeof(CHINODEPROCESSREQUESTINFO))
        {
            ChiCustomHwNode* pNode = static_cast<ChiCustomHwNode*>(pProcessRequestInfo->hNodeSession);
            result               = pNode->ProcessRequest(pProcessRequestInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEPROCESSREQUESTINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeSetNodeInterface
///
/// @brief  Implementation of PFCHINODESETNODEINTERFACE defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return VOID.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID CustomHwNodeNodeSetNodeInterface(
    ChiNodeInterface* pNodeInterface)
{
    CDKResult result = CDKResultSuccess;

    result = ChiNodeUtils::SetNodeInterface(pNodeInterface, CustomHwNodeNodeSectionName,
        &g_ChiNodeInterface, &g_vendorTagBase);

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupChi, "Set Node Interface failed");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeFlushRequest
///
/// @brief  Implementation of PFNNODEFLUSH defined in chinode.h
///
/// @param  pInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CustomHwNodeNodeFlushRequest(
    CHINODEFLUSHREQUESTINFO* pInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pInfo) || (NULL == pInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument [pInfo: %p | pInfo->hNodeSession is NULL]", pInfo);
    }
    else
    {
        if (pInfo->size >= sizeof(CHINODEFLUSHREQUESTINFO))
        {
            ChiCustomHwNode* pNode = static_cast<ChiCustomHwNode*>(pInfo->hNodeSession);
            result = pNode->FlushRequest(pInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEFLUSHREQUESTINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeFillHwData
///
/// @brief  Implementation of PFNNODEFILLPERREQUESTHWDATA defined in chinode.h
///
/// @param  pProcessHWData Pointer to process HW data
/// @param  pInfo          Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CustomHwNodeNodeFillHwData(
    CHINODEFILLHWDATA*         pProcessHWData,
    CHINODEPROCESSREQUESTINFO* pInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pProcessHWData) || (NULL == pProcessHWData->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument [pProcessHWData: %p | pProcessHWData->hNodeSession is NULL]",
            pProcessHWData);
    }
    else
    {
        if (pProcessHWData->size >= sizeof(CHINODEFILLHWDATA))
        {
            ChiCustomHwNode* pNode = static_cast<ChiCustomHwNode*>(pProcessHWData->hNodeSession);
            result = pNode->FillHWIOConfig(pProcessHWData, pInfo);

            if (CDKResultSuccess == result)
            {
                 result = pNode->FillCommandBufferData(pProcessHWData);
             }
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEFILLHWDATA is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodePrepareStreamOn
///
/// @brief  Implementation of PFNCHIPREPARESTREAMON defined in chinode.h
///
/// @param  pInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult CustomHwNodePrepareStreamOn(
    CHINODEPREPARESTREAMONINFO* pInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pInfo) || (NULL == pInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument [pInfo: %p | pInfo->hNodeSession is NULL]", pInfo);
    }
    else
    {
        if (pInfo->size == sizeof(CHINODEPREPARESTREAMONINFO))
        {
            ChiCustomHwNode* pNode = static_cast<ChiCustomHwNode*>(pInfo->hNodeSession);
            result = pNode->PrepareCSLHwInfo(&(pInfo->CSLHwInfo), &(pInfo->portInfo));
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEPREPARESTREAMONINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

   return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CustomHwNodeNodeGetFlushResponseTime
///
/// @brief  Implementation of PFNNODEFLUSHRESPONSEINFO defined in chinode.h
///
/// @param  pInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult CustomHwNodeNodeGetFlushResponseTime(
    CHINODERESPONSEINFO* pInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pInfo) || (NULL == pInfo->hChiSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument [pInfo: %p | pInfo->hChiSession is NULL]", pInfo);
    }
    else
    {
        if (pInfo->size == sizeof(CHINODERESPONSEINFO))
        {
            ChiCustomHwNode* pNode = static_cast<ChiCustomHwNode*>(pInfo->hChiSession);
            result = pNode->ComputeResponseTime(pInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODERESPONSEINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiNodeEntry
///
/// @brief  Entry point called by the Chi driver to initialize the custom node.
///
/// This function must be exported by every <library>.so in order for driver to initialize the Node. This function is called
/// during the camera server initialization, which occurs during HAL process start. In addition to communicating the necessary
/// function pointers between Chi and external nodes, this function allows a node to do any initialization work that it
/// would typically do at process init. Anything done here should not be specific to a session, and any variables stored in
/// the node must be protected against multiple sessions accessing it at the same time.
///
/// @param pNodeCallbacks  Pointer to a structure that defines callbacks that the Chi driver sends to the node.
///                        The node must fill in these function pointers.
///
/// @return VOID.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDK_VISIBILITY_PUBLIC VOID ChiNodeEntry(
    CHINODECALLBACKS* pNodeCallbacks)
{
    if (NULL != pNodeCallbacks)
    {
        if (pNodeCallbacks->majorVersion == ChiNodeMajorVersion &&
            pNodeCallbacks->size >= sizeof(CHINODECALLBACKS))
        {
            pNodeCallbacks->majorVersion             = ChiNodeMajorVersion;
            pNodeCallbacks->minorVersion             = ChiNodeMinorVersion;
            pNodeCallbacks->pGetCapabilities         = CustomHwNodeNodeGetCaps;
            pNodeCallbacks->pQueryVendorTag          = CustomHwNodeNodeQueryVendorTag;
            pNodeCallbacks->pCreate                  = CustomHwNodeNodeCreate;
            pNodeCallbacks->pDestroy                 = CustomHwNodeNodeDestroy;
            pNodeCallbacks->pQueryBufferInfo         = CustomHwNodeNodeQueryBufferInfo;
            pNodeCallbacks->pSetBufferInfo           = CustomHwNodeNodeSetBufferInfo;
            pNodeCallbacks->pProcessRequest          = CustomHwNodeNodeProcRequest;
            pNodeCallbacks->pChiNodeSetNodeInterface = CustomHwNodeNodeSetNodeInterface;
            pNodeCallbacks->pFlushRequest            = CustomHwNodeNodeFlushRequest;
            pNodeCallbacks->pGetFlushResponse        = CustomHwNodeNodeGetFlushResponseTime;
            pNodeCallbacks->pFillHwdata              = CustomHwNodeNodeFillHwData;
            pNodeCallbacks->pPrepareStreamOn         = CustomHwNodePrepareStreamOn;

        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Chi API major version doesn't match (%d:%d) vs (%d:%d)",
                pNodeCallbacks->majorVersion, pNodeCallbacks->minorVersion,
                ChiNodeMajorVersion, ChiNodeMinorVersion);
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Invalid Argument: pNodeCallbacks is NULL");
    }
}
#ifdef __cplusplus
}
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiCustomHwNode::Initialize(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pCreateInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pCreateInfo is NULL");
    }
    else
    {
        /// @todo (CAMX-1854) Check for Node Capabilities using NodeCapsMask
        m_hChiSession   = pCreateInfo->hChiSession;
        m_nodeId        = pCreateInfo->nodeId;
        m_nodeCaps      = pCreateInfo->nodeCaps.nodeCapsMask;
        m_nodeFlags     = pCreateInfo->nodeFlags;
        m_pipelineDelay = pCreateInfo->pipelineDelay;
    }

    m_processedFrame = 0;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::QueryBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiCustomHwNode::QueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult result = CDKResultSuccess;

    // set the nodeCaps later
    switch (m_nodeCaps)
    {
        case ChiNodeCapsCustomHwNode:
        default:
            break;
    }

    if (CDKResultSuccess == result)
    {
        ChiNodeUtils::DefaultBufferNegotiation(pQueryBufferInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::SetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiCustomHwNode::SetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    m_outputFormat.width  = pSetBufferInfo->pFormat->width;
    m_outputFormat.height = pSetBufferInfo->pFormat->height;

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CheckDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiCustomHwNode::CheckDependency(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    BOOL satisfied = TRUE;

    /*
     * The following code illustrate the usage of tag dependency. In the case of Chi node's processing has dependency on other
     * tags and the tags are not published yet, the Chi node could set the pDependency in pProcessRequestInfo and return
     * immediately. The framework will call the ExecuteProcessRequest again once the dependency is met.
     */

    if (pProcessRequestInfo->frameNum > m_pipelineDelay)
    {
        UINT16             dependencyCount = 0;
        CHIDEPENDENCYINFO* pDependencyInfo = pProcessRequestInfo->pDependency;

        // Adding dependency on frame duration. Will need to calculate FPS based on frame duration
        VOID* pData = NULL;
        UINT32 tag  = ANDROID_SENSOR_FRAME_DURATION;
        pData       = ChiNodeUtils::GetMetaData(pProcessRequestInfo->frameNum, tag, ChiMetadataDynamic,
                                            &g_ChiNodeInterface, m_hChiSession);
        // if the dependent tag is not published yet, set it as dependency
        if (NULL == pData)
        {
            pDependencyInfo->properties[dependencyCount] = tag;
            pDependencyInfo->offsets[dependencyCount]    = 1;
            dependencyCount++;
        }
        else
        {
            // Calculate FPS based on frame duration which is in nano seconds
            m_currentFPS = 1 / NanoToSec(*static_cast<FLOAT*>(pData));
        }

        // If any dependency is required, add code here to add dependency

        if (dependencyCount > 0)
        {
            pDependencyInfo->count        = dependencyCount;
            pDependencyInfo->hNodeSession = m_hChiSession;

            // uses processSequenceId to determine what state it is in.
            pProcessRequestInfo->pDependency->processSequenceId                 = 1;

            // if this node has dependency on input output buffers enable the ccode below
            // pProcessRequestInfo->pDependency->hasIOBufferAvailabilityDependency = TRUE;

            satisfied = FALSE;
        }
    }

    return satisfied;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiCustomHwNode::ProcessRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    BOOL satisfied = FALSE;

    if (IsRealTime())
    {
        satisfied = CheckDependency(pProcessRequestInfo);
    }
    else
    {
        satisfied = TRUE;
    }

    if (TRUE == satisfied)
    {
        for (UINT32 i = 0; i < pProcessRequestInfo->outputNum; i++)
        {
            BOOL         flushInProgress = FALSE;
            CHIFLUSHINFO chiFlushInfo;

            // GetFlushInfo obtains information from the session if flush is in progress
            // Custom Node can read this information and behave appropriately, such as aborting
            // processing of the request.
            g_ChiNodeInterface.pGetFlushInfo(m_hChiSession, &chiFlushInfo);
            flushInProgress = chiFlushInfo.flushInProgress;

            // If flush is in progress, abort processing of the request
            if (TRUE == flushInProgress)
            {
                LOG_INFO(CamxLogGroupChi, "Flush is in Progress for the session, aborting");
                break;
            }

            pProcessRequestInfo->requireCmdBuffers = TRUE;

            /// Add code to handle CustomHw Node Output Buffers if required
            if (TRUE == IsBypassableNode())
            {
                pProcessRequestInfo->pBypassData[i].isBypassNeeded = TRUE;
            }
        }

        m_processedFrame++;
        UpdateMetaData(pProcessRequestInfo->frameNum);
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::ChiCustomHwNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiCustomHwNode::ChiCustomHwNode()
    : m_hChiSession(NULL)
    , m_nodeId(0)
    , m_nodeCaps(0)
    , m_processedFrame(0)
{
    memset(&m_outputFormat, 0, sizeof(CHINODEIMAGEFORMAT));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::~ChiCustomHwNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiCustomHwNode::~ChiCustomHwNode()
{
    m_hChiSession = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::UpdateMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiCustomHwNode::UpdateMetaData(
    UINT64 requestId)
{
    CHIMETADATAINFO        metadataInfo     = {0};
    const UINT32           tagSize          = sizeof(g_VendorTagSectionCustomHwNode) / sizeof(g_VendorTagSectionCustomHwNode[0]);
    CHITAGDATA             tagData[tagSize] = { {0} };
    UINT32                 tagList[tagSize];

    metadataInfo.size       = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession = m_hChiSession;
    metadataInfo.tagNum     = tagSize;
    metadataInfo.pTagList   = &tagList[0];
    metadataInfo.pTagData   = &tagData[0];

    UINT32 index = 0;

    UINT32  currentMode      = m_nodeCaps;
    tagList[index]           = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size      = sizeof(CHITAGDATA);
    tagData[index].requestId = requestId;
    tagData[index].pData     = &currentMode;
    tagData[index].dataSize  = g_VendorTagSectionCustomHwNode[index].numUnits;
    index++;

    g_ChiNodeInterface.pSetMetadata(&metadataInfo);

    BOOL         flushInProgress      = FALSE;
    UINT64       lastFlushedRequestId = 0;
    UINT64       requestIdOffset      = 0;
    CHIFLUSHINFO chiFlushInfo;

    //  GetFlushInfo obtains information from the session if flush is in progress.
    //  Custom Node can read this information and behave appropriately, such as aborting
    //  the request processing during flush.
    //
    //  The lastFlushedRequestId is the request id of the highest request that was flushed during the flush operation.
    //  After a flush operation, the first requests after a flush may need special handling to avoid
    //  setting dependencies on previously flushed requests (similar to how first requests in a pipeline
    //  avoid setting dependencies at the beginning of a usecase). We use lastFlushedRequestId to identify
    //  the first requests after the flush call.
    //
    //  Example:
    //    Flush operation flushes requests 4, 5, 6, 7. lastFlushedRequestId is set to 7.
    //    After flush, the next incoming request is request id 8. Request id 8 needs to be handled like request id 1.
    //    Using the lastFlushedRequestId, we can take the offset from the current request id.
    //    If the offset is equal to 1, we know that request id 8 is the first request after flush.
    //
    //                        CurrentRequestId - lastFlushedRequestId = Offset
    //                                       8 - 7                    = 1
    //
    //  Utiliy function to compute this offset: ChiNodeUtils::GetRequestIdOffset(requestId, lastFlushedRequestId)

    g_ChiNodeInterface.pGetFlushInfo(m_hChiSession, &chiFlushInfo);
    flushInProgress      = chiFlushInfo.flushInProgress;
    lastFlushedRequestId = chiFlushInfo.lastFlushedRequestId;
    requestIdOffset      = ChiNodeUtils::GetRequestIdOffset(requestId, lastFlushedRequestId);

    // If flush is in progress, abort processing
    if (TRUE == flushInProgress)
    {
        LOG_INFO(CamxLogGroupChi, "Flush is in Progress for the session, aborting");
    }
    else if (requestId != lastFlushedRequestId)
    {
        if (0 < lastFlushedRequestId)
        {
            LOG_VERBOSE(CamxLogGroupChi, "Last Request Id flushed %" PRIu64, lastFlushedRequestId);
        }
#if PRINT_METADATA
        CHIVENDORTAGBASEINFO vendorTagBase = { 0 };
        vendorTagBase.size = sizeof(CHIVENDORTAGBASEINFO);
        vendorTagBase.pComponentName = CustomHwNodeNodeSectionName;
        vendorTagBase.pTagName = g_VendorTagSectionCustomHwNode[index].pVendorTagName;
        g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);
        LOG_VERBOSE(CamxLogGroupChi, "Vendor Tags value shall be same %x %x",
            g_VendorTagList[index] + g_vendorTagBase, vendorTagBase.vendorTagBase);

        // get the updated metadata from CamX and print them out
        VOID* pData = NULL;
        pData = ChiNodeUtils::GetMetaData(requestId, CustomHwNodeNodeTagCurrentMode + g_vendorTagBase, ChiMetadataDynamic,
            &g_ChiNodeInterface, m_hChiSession);
        if (NULL != pData)
        {
            LOG_VERBOSE(CamxLogGroupChi, "current mode %d current mode %d",
                *static_cast<UINT32*>(pData), currentMode);
        }

        // sample of getting Android metadata from CamX
        pData = ChiNodeUtils::GetMetaData(requestId, ANDROID_SENSOR_TIMESTAMP, ChiMetadataDynamic,
            &g_ChiNodeInterface, m_hChiSession);
        if (NULL != pData)
        {
            INT64 timestamp = *static_cast<INT64 *>(pData);
            LOG_VERBOSE(CamxLogGroupChi, "timestamp for current frame %" PRIi64, timestamp);
        }

        // sample of getting static Android metadata from CamX
        pData = ChiNodeUtils::GetMetaData(requestId, ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES, ChiMetadataStatic,
            &g_ChiNodeInterface, m_hChiSession);
        if (NULL != pData)
        {
            UINT8*  aeModes = (UINT8 *)pData;
            LOG_VERBOSE(CamxLogGroupChi, "ae banding modes %d %d %d %d", aeModes[0], aeModes[1], aeModes[2], aeModes[3]);
        }
#endif
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::FlushRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiCustomHwNode::FlushRequest(
    CHINODEFLUSHREQUESTINFO* pFlushRequestInfo)
{
    // any internal resouces reserved for the request can be freed up here
    LOG_INFO(CamxLogGroupChi, "Flush request Id %" PRIu64 " from node", pFlushRequestInfo->frameNum);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::ComputeResponseTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiCustomHwNode::ComputeResponseTime(
    CHINODERESPONSEINFO* pInfo)
{
    CHI_UNREFERENCED_PARAM(pInfo);

    // Each node needs to publish a response time (milliseconds) for how long it takes to
    // stop processing requests after flush call.
    // During flush, session will consider this reponse time and will wait for nodes to flush gracefully.
    // This response time can be computed dynamically based on the type of the node,
    // otherwise default reponse time can be used.
    pInfo->responseTimeInMillisec = DEFAULT_FLUSH_RESPONSE_TIME;

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::SetupChannelResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiCustomHwNode::SetupChannelResource(
    struct cam_custom_in_port_info* pInputResource,
    UINT32*                         pInputPortInfo)
{
    CDKResult result    = CDKResultSuccess;
    UINT32    vcdtCount = 0;
    UINT32    index     = 0;

    if ((NULL != pInputResource) &&  (NULL != pInputPortInfo))
    {

        // Get the Sensor mode information from vendor tag
        CHIVENDORTAGBASEINFO vendorTagBaseInfo = { 0 };
        VOID*                pMetaData         = NULL;

        // Get the "sensor_mode_info" vendor tag base
        result = ChiNodeUtils::GetVendorTagBase("org.codeaurora.qcamera3.sensor_meta_data",
                                                "sensor_mode_info",
                                                &g_ChiNodeInterface,
                                                &vendorTagBaseInfo);

        if (CDKResultSuccess == result)
        {
            pMetaData = ChiNodeUtils::GetMetaData(0,
                                                  (vendorTagBaseInfo.vendorTagBase | UsecaseMetadataSectionMask),
                                                  ChiMetadataDynamic,
                                                  &g_ChiNodeInterface,
                                                  m_hChiSession);

            if (NULL != pMetaData)
            {
                ChiSensorModeInfo* pSensorModeInfo = reinterpret_cast<ChiSensorModeInfo*>(pMetaData);

                LOG_VERBOSE(CamxLogGroupChi, "Sensor Output Dimensions %ux%u", pSensorModeInfo->frameDimension.width,
                            pSensorModeInfo->frameDimension.height);

                /// @todo (CAMX-795) Following field needs to from sensor usecase data
                switch (pSensorModeInfo->CSIPHYId)
                {
                    case 0:
                        pInputResource->res_type = CustomHwInputPHY0;
                        break;
                    case 1:
                        pInputResource->res_type = CustomHwInputPHY1;
                        break;
                    case 2:
                        pInputResource->res_type = CustomHwInputPHY2;
                        break;
                    case 3:
                        pInputResource->res_type = CustomHwInputPHY3;
                        break;
                    default:
                        LOG_ERROR(CamxLogGroupChi, "CSIPHY channel out of range value %d", pSensorModeInfo->CSIPHYId);
                        break;
                }

                pInputResource->lane_type = pSensorModeInfo->is3Phase ? CustomLaneTypeCPHY : CustomLaneTypeDPHY;
                pInputResource->lane_num  = pSensorModeInfo->laneCount;

                for (index = 0; index < pSensorModeInfo->streamConfigCount; index++)
                {
                    if (ChiSensorStreamType::StreamImage == pSensorModeInfo->streamConfig[index].streamtype)
                    {
                        pInputResource->vc[vcdtCount] = pSensorModeInfo->streamConfig[index].vc;
                        pInputResource->dt[vcdtCount] = pSensorModeInfo->streamConfig[index].dt;
                        vcdtCount++;
                        // This is an example, add the right format based on custom HW node
                        pInputResource->format = CustomHwNodeFormatMIPIRaw10;
                    }
                    else if (ChiSensorStreamType::StreamMeta == pSensorModeInfo->streamConfig[index].streamtype)
                    {
                        pInputResource->vc[vcdtCount] = pSensorModeInfo->streamConfig[index].vc;
                        pInputResource->dt[vcdtCount] = pSensorModeInfo->streamConfig[index].dt;
                        vcdtCount++;
                        // This is an example, add the right format based on custom HW node
                        pInputResource->format = CustomHwNodeFormatPlain8;
                    }
                    else if ((ChiSensorStreamType::StreamBlob == pSensorModeInfo->streamConfig[index].streamtype) ||
                             (ChiSensorStreamType::StreamPDAF == pSensorModeInfo->streamConfig[index].streamtype) ||
                             (ChiSensorStreamType::StreamHDR  == pSensorModeInfo->streamConfig[index].streamtype))
                    {
                        continue;
                    }
                    else
                    {
                        result = CDKResultEFailed;
                        LOG_ERROR(CamxLogGroupChi, "Invalid StreamType");
                    }

                }

                pInputResource->num_valid_vc_dt = index;
                pInputResource->left_start      = pSensorModeInfo->cropInfo.left;
                pInputResource->left_stop       = pSensorModeInfo->cropInfo.width;
                pInputResource->line_start      = pSensorModeInfo->cropInfo.top;
                pInputResource->line_stop       = pSensorModeInfo->cropInfo.height;
                pInputResource->height          = pSensorModeInfo->cropInfo.height;
                pInputResource->left_width      = pSensorModeInfo->cropInfo.width;
                pInputResource->pixel_clk       = pSensorModeInfo->outPixelClock;
                pInputResource->lane_cfg        = pSensorModeInfo->laneCfg;

                // To be filled by implentor based on format
                pInputResource->num_bytes_out   = 0x7;

            }
            else
            {
                LOG_ERROR(CamxLogGroupChi, "Error getting depth input dimension");
                result = CDKResultEFailed;
            }
        }


        if (CDKResultSuccess == result)
        {
            pMetaData = ChiNodeUtils::GetMetaData(0,
                                                  ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
                                                  ChiMetadataStatic,
                                                  &g_ChiNodeInterface,
                                                  m_hChiSession);

            if (NULL != pMetaData)
            {
                pInputResource->test_pattern = TranslateColorFilterPatternToCustomHwNodePattern(
                       *static_cast<camera_metadata_enum_android_sensor_info_color_filter_arrangement*>(pMetaData));
            }
            else
            {
                result = CDKResultEFailed;
                LOG_ERROR(CamxLogGroupChi, "Failed to get sensor color filter arrangement");
            }
        }

        // To be filled by the Custom HW node implmentor based on provided out port information
        if (CDKResultSuccess == result)
        {
            pInputResource->usage_type           = 0;
            pInputResource->custom_info1         = 0;
            pInputResource->custom_info2         = 0;
            pInputResource->num_out_res          = 1;

            pInputResource->data[0].res_type     = 0;
            pInputResource->data[0].format       = 0;
            pInputResource->data[0].custom_info1 = 0;
            pInputResource->data[0].custom_info2 = 0;
            pInputResource->data[0].custom_info3 = 0;
        }
    }
    else
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Input Resource NULL [pInputResource: %p, pInputPortInfo: %p]",
            pInputResource, pInputPortInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCustomHwNode::PrepareCSLHwInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiCustomHwNode::PrepareCSLHwInfo(
    CHICSLHWINFO*    pInfo,
    CHINODEPORTINFO* pPortInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL != pInfo) && (NULL != pPortInfo))
    {
        ChiNodeUtils::Memset(&m_customResource, 0, sizeof(m_customResource));
        // Fill this if there is an data is to be sent while acquiring the hw device
        pInfo->hwInitData   = static_cast<VOID*>(&m_customResource);
        pInfo->sizeInitData = sizeof(cam_custom_resource);

        // Save Input/Output Port Info
        ChiNodeUtils::Memcpy(&m_portInfo, pPortInfo, sizeof(CHINODEPORTINFO));

        for (UINT32 index = 0; index < pPortInfo->numInputPorts; index++)
        {
            if (CDKResultSuccess == result)
            {
                ChiNodeUtils::Memset(&m_inputResource, 0, sizeof(m_inputResource));
                result = SetupChannelResource(&m_inputResource, &pPortInfo->inputPortId[index]);

                // Identifies the resource in HW that is to be acquired
                m_customResource[index].resource_id = 0;
                m_customResource[index].length      = sizeof(cam_custom_in_port_info)  +
                                                     (sizeof(cam_custom_out_port_info) * (m_inputResource.num_out_res - 1));
                m_customResource[index].handle_type = CAM_HANDLE_USER_POINTER;
                m_customResource[index].res_hdl     = reinterpret_cast<UINT64>(&m_inputResource);
                pInfo->sizeInitData                += m_customResource[index].length;
            }
        }

        if (CDKResultSuccess == result)
        {
            // To access CSL API an config for the CSL must be provided
            // Indicates if CSL Accessis required, if this is a hw node, this will be TRUE
            pInfo->requireCSLAccess       = TRUE;

            // Need to obtain this info from custom UAPI
            pInfo->CSLHwResourceID        = 1;
            // Number of command buffers Required
            pInfo->numCmdBuffers          = 1;
            //Number of buffers for each command Buffe
            pInfo->numberOfBuffer         = 16;
            // Size of each command buffer data that this node will fill in bytes
            pInfo->sizeOfCommandBuffer[0] = sizeof(cam_custom_cmd_buf_type_1);

            // no output buffers required
            pInfo->requireOutputBuffers   = FALSE;

            pInfo->numResources           = 1;

            // Iff node requires a scratch buffer
            pInfo->requireScratchBuffer   = TRUE;

            // In bytes
            pInfo->sizeOfScratchBuffer    = 5120;
        }
        else
        {
            result = CDKResultEFailed;
            LOG_ERROR(CamxLogGroupChi, "Couldn't create CSL Info");
        }
    }
    else
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid Arg [pInfo: %p, pPortInfo: %p]", pInfo, pPortInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiCustomHwNode::TranslateColorFilterPatternToCustomHwNodePattern
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiCustomHwNode::TranslateColorFilterPatternToCustomHwNodePattern(
    camera_metadata_enum_android_sensor_info_color_filter_arrangement colorFilterArrangementValue)
{
    UINT32 CustomHwNodePattern = 0;

    switch (colorFilterArrangementValue)
    {
        case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB:
            CustomHwNodePattern = CustomHwNodePatternBayerRGRGRG;
            break;

        case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG:
            CustomHwNodePattern = CustomHwNodePatternBayerGRGRGR;
            break;

        case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG:
            CustomHwNodePattern = CustomHwNodePatternBayerGBGBGB;
            break;

        case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR:
            CustomHwNodePattern = CustomHwNodePatternBayerBGBGBG;
            break;

        default:
            CustomHwNodePattern = CustomHwNodePatternBayerRGRGRG;

            LOG_ERROR(CamxLogGroupCHI,
                      "Unable to translate SensorInfoColorFilterArrangementValue = %d to CustomHwNode Bayer pattern."
                      "Assuming CustomHwNodePatternBayerRGRGRG!",
                      colorFilterArrangementValue);
            break;
    }

    return CustomHwNodePattern;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiCustomHwNode::FillHWIOConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiCustomHwNode::FillHWIOConfig(
    CHINODEFILLHWDATA*         pProcessHWData,
    CHINODEPROCESSREQUESTINFO* pInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pProcessHWData) || (NULL == pInfo))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument [pProcessHWData: %p, pInfo: %p]", pProcessHWData, pInfo);
    }
    else
    {
        UINT32 index = 0;

        pProcessHWData->numHWIOConfig = 0;

        for (index = 0; index < pInfo->inputNum; index++)
        {
            if (NULL != pInfo->phInputBuffer[index])
            {
                if (ChiNodeCustomHwInputPortFetchEngine == pInfo->phInputBuffer[index]->portId)
                {
                    pProcessHWData->numHWIOConfig++;
                    pProcessHWData->hwIOConfig[index].bufferDirection  = CHICUSTOMHWBUFFERDIRECTION::Input;

                    // To be filled by implentor based on UAPI and HW capabilities
                    pProcessHWData->hwIOConfig[index].imageBuffer      = pInfo->phInputBuffer[index]->imageHandle;
                    pProcessHWData->hwIOConfig[index].channelID        = CustomHwFetchEngine;
                    pProcessHWData->hwIOConfig[index].pfenceHandle     = pInfo->phInputBuffer[index]->pfenceHandle;
                    pProcessHWData->hwIOConfig[index].frameDropPattern = 0;
                    pProcessHWData->hwIOConfig[index].frameDropPeriod  = 0;
                    pProcessHWData->hwIOConfig[index].subsamplePattern = 0;
                    pProcessHWData->hwIOConfig[index].subsamplePeriod  = 0;
                }
            }
        }

        for (index = 0; index < pInfo->outputNum; index++)
        {
            if (NULL != pInfo->phInputBuffer[index])
            {
                if (ChiNodeCustomHwOutputPortPixel == pInfo->phOutputBuffer[index]->portId)
                {
                    pProcessHWData->numHWIOConfig++;
                    pProcessHWData->hwIOConfig[index].bufferDirection  = CHICUSTOMHWBUFFERDIRECTION::Input;

                    // To be filled by implentor based on UAPI and HW capabilities
                    pProcessHWData->hwIOConfig[index].imageBuffer      = pInfo->phOutputBuffer[index]->imageHandle;
                    pProcessHWData->hwIOConfig[index].channelID        = ChiNodeCustomOutputWMPixel;
                    pProcessHWData->hwIOConfig[index].pfenceHandle     = pInfo->phOutputBuffer[index]->pfenceHandle;
                    pProcessHWData->hwIOConfig[index].frameDropPattern = 0;
                    pProcessHWData->hwIOConfig[index].frameDropPeriod  = 0;
                    pProcessHWData->hwIOConfig[index].subsamplePattern = 0;
                    pProcessHWData->hwIOConfig[index].subsamplePeriod  = 0;
                }
                else if (ChiNodeCustomHwOutputPortRDI == pInfo->phOutputBuffer[index]->portId)
                {
                    pProcessHWData->numHWIOConfig++;
                    pProcessHWData->hwIOConfig[index].bufferDirection  = CHICUSTOMHWBUFFERDIRECTION::Input;

                    // To be filled by implentor based on UAPI and HW capabilities
                    pProcessHWData->hwIOConfig[index].imageBuffer      = pInfo->phOutputBuffer[index]->imageHandle;
                    pProcessHWData->hwIOConfig[index].channelID        = ChiNodeCustomOutputWMRDI;
                    pProcessHWData->hwIOConfig[index].pfenceHandle     = pInfo->phOutputBuffer[index]->pfenceHandle;
                    pProcessHWData->hwIOConfig[index].frameDropPattern = 0;
                    pProcessHWData->hwIOConfig[index].frameDropPeriod  = 0;
                    pProcessHWData->hwIOConfig[index].subsamplePattern = 0;
                    pProcessHWData->hwIOConfig[index].subsamplePeriod  = 0;
                }
                else if (ChiNodeCustomHwOutputPortMeta == pInfo->phOutputBuffer[index]->portId)
                {
                    pProcessHWData->numHWIOConfig++;
                    pProcessHWData->hwIOConfig[index].bufferDirection  = CHICUSTOMHWBUFFERDIRECTION::Input;

                    // To be filled by implentor based on UAPI and HW capabilities
                    pProcessHWData->hwIOConfig[index].imageBuffer      = pInfo->phOutputBuffer[index]->imageHandle;
                    pProcessHWData->hwIOConfig[index].channelID        = ChiNodeCustomOutputWMMeta;
                    pProcessHWData->hwIOConfig[index].pfenceHandle     = pInfo->phOutputBuffer[index]->pfenceHandle;
                    pProcessHWData->hwIOConfig[index].frameDropPattern = 0;
                    pProcessHWData->hwIOConfig[index].frameDropPeriod  = 0;
                    pProcessHWData->hwIOConfig[index].subsamplePattern = 0;
                    pProcessHWData->hwIOConfig[index].subsamplePeriod  = 0;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiCustomHwNode::FillCommandBufferData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiCustomHwNode::FillCommandBufferData(
    CHINODEFILLHWDATA* pProcessHWData)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pProcessHWData)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pProcessHWData is NULL");
    }
    else
    {
        cam_custom_cmd_buf_type_1 data = {0};
        data.custom_info = pProcessHWData->frameNum;

        // Add code to fill in the Hw data based on custom UAPI
        for (UINT32 i = 0; i < pProcessHWData->numCmdBufferAddr; i++)
        {
            ChiNodeUtils::Memcpy(pProcessHWData->pCmdMemoryAdd[i], &data, sizeof(cam_custom_cmd_buf_type_1));
        }

        if (ChiCSLHwPacketInit == pProcessHWData->packetContext)
        {
            pProcessHWData->opCode = CAM_CUSTOM_PACKET_INIT_DEV;
        }
        else
        {
            pProcessHWData->opCode = CAM_CUSTOM_PACKET_UPDATE_DEV;
        }
    }

    return result;
}

// CAMX_NAMESPACE_END
