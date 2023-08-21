////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinoderemosaic.cpp
/// @brief Chi node for remosaic
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <inttypes.h>
#include <system/camera_metadata.h>

#include "camxchinoderemosaic.h"
#include "camxchinodeutil.h"
#include "chiaecinterface.h"
#include "chistatsproperty.h"
#include "chistatspropertydefines.h"


// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads
// NOWHINE FILE NC008: Warning: - Var names should be lower camel case

#undef LOG_TAG
#define LOG_TAG "CHIREMOSAIC"

#define PRINT_METADATA FALSE

ChiNodeInterface g_ChiNodeInterface;    ///< The instance save the CAMX Chi interface
UINT32           g_vendorTagBase = 0;   ///< Chi assigned runtime vendor tag base for the node

/// @todo (CAMX-1854) the major / minor version shall get from CHI
static const UINT32 ChiNodeMajorVersion = 0;    ///< The major version of CHI interface
static const UINT32 ChiNodeMinorVersion = 0;    ///< The minor version of CHI interface

static const UINT32 ChiNodeCapsRemosaic     = 1;                    ///< Simply copy the image
static const UINT32 ChiNodeCapsHalfScale    = 1 << 1;               ///< 1/2 downsize
static const UINT32 ChiNodeCapsQuarterScale = 1 << 2;               ///< 1/4 downsize

static const CHAR   RemosaicNodeSectionName[]         = "com.qti.node.remosaic";             ///< The section name for node

static const UINT32 RemosaicNodeTagBase               = 0;                        ///< Tag base
static const UINT32 RemosaicNodeTagSupportedFeature   = RemosaicNodeTagBase + 0;    ///< Tag for supported features
static const UINT32 RemosaicNodeTagCurrentMode        = RemosaicNodeTagBase + 1;    ///< Tag to indicated current operation mode
static const UINT32 RemosaicNodeTagProcessedFrameNum  = RemosaicNodeTagBase + 2;    ///< Tag to show processed frame's count
static const UINT32 RemosaicNodeTagFrameDimension     = RemosaicNodeTagBase + 3;    ///< Tag to show current's frame dimension

///< Supported vendor tag list, it shall align with the definition in g_VendorTagSectionRemosaic
static const UINT32 g_VendorTagList[] =
{
    RemosaicNodeTagSupportedFeature,
    RemosaicNodeTagCurrentMode,
    RemosaicNodeTagProcessedFrameNum,
    RemosaicNodeTagFrameDimension
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGDATA g_VendorTagSectionRemosaic[] =
{
    { "SupportedFeature",       TYPE_INT32, 1 },
    { "CurrentMode",            TYPE_BYTE,  1 },
    { "ProcessedFrameNumber",   TYPE_INT64, 1 },
    { "FrameDimension",         TYPE_INT32, 2 },
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGSECTIONDATA g_VendorTagRemosaicSection[] =
{
    {
        RemosaicNodeSectionName,  0,
        sizeof(g_VendorTagSectionRemosaic) / sizeof(g_VendorTagSectionRemosaic[0]), g_VendorTagSectionRemosaic,
        CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    }
};

///< This is an array of all vendor tag section data
static ChiVendorTagInfo g_VendorTagInfoRemosaic[] =
{
    {
        &g_VendorTagRemosaicSection[0],
        sizeof(g_VendorTagRemosaicSection) / sizeof(g_VendorTagRemosaicSection[0])
    }
};

static RemosaicRequiredVendorTags g_vendorTagIds = { 0 };

static const UINT32 REMOSAIC_FLUSH_RESPONSE_TIME = 500;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHIDefaultBufferNegotiation
///
/// @brief  Implement the default buffer negotiation logic.
///
/// @param  pQueryBufferInfo   Pointer to a structure that has the information of required output/input buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHIDefaultBufferNegotiation(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    UINT      optimalInputWidth          = 0;
    UINT      optimalInputHeight         = 0;
    UINT32    minInputWidth              = 0;
    UINT32    minInputHeight             = 0;
    UINT32    maxInputWidth              = 0xffff;
    UINT32    maxInputHeight             = 0xffff;
    UINT      perOutputPortOptimalWidth  = 0;
    UINT      perOutputPortOptimalHeight = 0;
    UINT32    perOutputPortMinWidth      = 0;
    UINT32    perOutputPortMinHeight     = 0;
    UINT32    perOutputPortMaxWidth      = 0xffff;
    UINT32    perOutputPortMaxHeight     = 0xffff;

    for (UINT outputIndex = 0; outputIndex < pQueryBufferInfo->numOutputPorts ; outputIndex++)
    {
        ChiOutputPortQueryBufferInfo* pOutputPort = &pQueryBufferInfo->pOutputPortQueryInfo[outputIndex];

        perOutputPortOptimalWidth  = 0;
        perOutputPortOptimalHeight = 0;

        for (UINT input = 0; input < pOutputPort->numConnectedInputPorts; input++)
        {
            ChiNodeBufferRequirement* pInputPortRequirement = &pOutputPort->pBufferRequirement[input];

            // Optimal width per port is the super resolution of all the connected destination ports' optimal needs.
            perOutputPortOptimalWidth  = (pInputPortRequirement->optimalW  >= perOutputPortOptimalWidth) ?
                                          pInputPortRequirement->optimalW  : perOutputPortOptimalWidth;
            perOutputPortOptimalHeight = (pInputPortRequirement->optimalH  >= perOutputPortOptimalHeight) ?
                                          pInputPortRequirement->optimalH  : perOutputPortOptimalHeight;
            perOutputPortMinWidth      = (pInputPortRequirement->minW      >= perOutputPortMinWidth) ?
                                          pInputPortRequirement->minW      : perOutputPortMinWidth;
            perOutputPortMinHeight     = (pInputPortRequirement->minH      >= perOutputPortMinHeight) ?
                                          pInputPortRequirement->minH      : perOutputPortMinHeight;
            perOutputPortMaxWidth      = (pInputPortRequirement->maxW      <= perOutputPortMaxWidth) ?
                                          pInputPortRequirement->maxW      : perOutputPortMaxWidth;
            perOutputPortMaxHeight     = (pInputPortRequirement->maxH      <= perOutputPortMaxHeight) ?
                                          pInputPortRequirement->maxH      : perOutputPortMaxHeight;
        }

        // Ensure optimal dimensions are within min and max dimensions. There are chances that the optimal dimension goes
        // over the max. Correct for the same
        perOutputPortOptimalWidth = (((perOutputPortOptimalWidth)  <= (perOutputPortMinWidth)) ? (perOutputPortMinWidth) :
                                     (((perOutputPortOptimalWidth) >= (perOutputPortMaxWidth)) ? (perOutputPortMaxWidth) :
                                     (perOutputPortOptimalWidth)));

        perOutputPortOptimalHeight = (((perOutputPortOptimalHeight)  <= (perOutputPortMinHeight)) ?
                                      (perOutputPortMinHeight)                                    :
                                      (((perOutputPortOptimalHeight) >= (perOutputPortMaxHeight)) ?
                                      (perOutputPortMaxHeight)                                    :
                                      (perOutputPortOptimalHeight)));

        pOutputPort->outputBufferOption.minW     = perOutputPortMinWidth;
        pOutputPort->outputBufferOption.minH     = perOutputPortMinHeight;
        pOutputPort->outputBufferOption.maxW     = perOutputPortMaxWidth;
        pOutputPort->outputBufferOption.maxH     = perOutputPortMaxHeight;
        pOutputPort->outputBufferOption.optimalW = perOutputPortOptimalWidth;
        pOutputPort->outputBufferOption.optimalH = perOutputPortOptimalHeight;

        optimalInputWidth  = ((perOutputPortOptimalWidth > optimalInputWidth)  ?
                               perOutputPortOptimalWidth : optimalInputWidth);
        optimalInputHeight = ((perOutputPortOptimalHeight > optimalInputHeight) ?
                               perOutputPortOptimalHeight : optimalInputHeight);
        minInputWidth      = ((perOutputPortMinWidth > minInputWidth) ?
                               perOutputPortMinWidth : minInputWidth);
        minInputHeight     = ((perOutputPortMinHeight > minInputHeight) ?
                               perOutputPortMinHeight : minInputHeight);
        maxInputWidth      = ((perOutputPortMaxWidth < maxInputWidth)   ? perOutputPortMaxWidth  : maxInputWidth);
        maxInputHeight     = ((perOutputPortMaxHeight < maxInputHeight) ? perOutputPortMaxHeight : maxInputHeight);

        LOG_VERBOSE(CamxLogGroupChi, "Input:  (w x h) optimal %d x %d min %d x %d max %d x %d", optimalInputWidth,
                    optimalInputHeight, minInputWidth, minInputHeight, maxInputWidth, maxInputHeight);
        LOG_VERBOSE(CamxLogGroupChi, "Output: (w x h) optimal %d x %d min %d x %d max %d x %d", perOutputPortOptimalWidth,
                    perOutputPortOptimalHeight, perOutputPortMinWidth, perOutputPortMinHeight,
                    perOutputPortMaxWidth, perOutputPortMaxHeight);
    }

    for (UINT inputIndex = 0; inputIndex < pQueryBufferInfo->numInputPorts ; inputIndex++)
    {
        ChiInputPortQueryBufferInfo* pInputOptions = &pQueryBufferInfo->pInputOptions[inputIndex];

        pInputOptions->inputBufferOption.minW     = minInputWidth;
        pInputOptions->inputBufferOption.minH     = minInputHeight;
        pInputOptions->inputBufferOption.maxW     = maxInputWidth;
        pInputOptions->inputBufferOption.maxH     = maxInputHeight;
        pInputOptions->inputBufferOption.optimalW = optimalInputWidth;
        pInputOptions->inputBufferOption.optimalH = optimalInputHeight;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MemCpyNodeGetCaps
///
/// @brief  Implementation of PFNNODEGETCAPS defined in chinode.h
///
/// @param  pCapsInfo   Pointer to a structure that defines the capabilities supported by the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult RemosaicNodeGetCaps(
    CHINODECAPSINFO* pCapsInfo)
{
    CDKResult result = CDKResultSuccess;

    if (pCapsInfo == NULL)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pCapsInfo is NULL");
        return result;
    }

    if (CDKResultSuccess != result)
    {
        if (pCapsInfo->size >= sizeof(CHINODECAPSINFO))
        {
            pCapsInfo->nodeCapsMask = ChiNodeCapsRemosaic | ChiNodeCapsHalfScale | ChiNodeCapsQuarterScale;
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
/// RemosaicNodeQueryVendorTag
///
/// @brief  Implementation of PFNCHIQUERYVENDORTAG defined in chinode.h
///
/// @param  pQueryVendorTag Pointer to a structure that returns the exported vendor tag
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult RemosaicNodeQueryVendorTag(
    CHIQUERYVENDORTAG* pQueryVendorTag)
{
    CDKResult result = CDKResultSuccess;
    if (NULL == pQueryVendorTag)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalide argument: pQueryVendorTag is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pQueryVendorTag->size >= sizeof(CHIQUERYVENDORTAG))
        {
            pQueryVendorTag->pVendorTagInfo = g_VendorTagInfoRemosaic;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "pQueryVendorTag is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RemosaicNodeCreate
///
/// @brief  Implementation of PFNNODECREATE defined in chinode.h
///
/// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult RemosaicNodeCreate(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult       result  = CDKResultSuccess;
    ChiRemosaicNode*  pNode = NULL;

    if (NULL == pCreateInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pTagTypeInfo is NULL");
        return result;
    }

    if (pCreateInfo->size < sizeof(CHINODECREATEINFO))
    {
        LOG_ERROR(CamxLogGroupChi, "CHINODECREATEINFO is smaller than expected");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        pNode = new ChiRemosaicNode;
        if (pNode == NULL)
        {
            result = CDKResultENoMemory;
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

    if (CDKResultSuccess != result)
    {
        delete pNode;
        pNode = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RemosaicNodeDestroy
///
/// @brief  Implementation of PFNNODEDESTROY defined in chinode.h
///
/// @param  pDestroyInfo    Pointer to a structure that defines the session destroy information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult RemosaicNodeDestroy(
    CHINODEDESTROYINFO* pDestroyInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pDestroyInfo) || (NULL == pDestroyInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pDestroyInfo is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pDestroyInfo->size >= sizeof(CHINODEDESTROYINFO))
        {
            ChiRemosaicNode* pNode = static_cast<ChiRemosaicNode*>(pDestroyInfo->hNodeSession);
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
/// RemosaicNodeQueryBufferInfo
///
/// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
///
/// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult RemosaicNodeQueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pQueryBufferInfo) || (NULL == pQueryBufferInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument");
    }

    if (CDKResultSuccess == result)
    {
        if (pQueryBufferInfo->size >= sizeof(CHINODEQUERYBUFFERINFO))
        {
            ChiRemosaicNode* pNode = static_cast<ChiRemosaicNode*>(pQueryBufferInfo->hNodeSession);
            result                 = pNode->QueryBufferInfo(pQueryBufferInfo);
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
/// RemosaicNodeSetBufferInfo
///
/// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
///
/// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult RemosaicNodeSetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pSetBufferInfo) || (NULL == pSetBufferInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument");
    }

    if (CDKResultSuccess == result)
    {
        if (pSetBufferInfo->size >= sizeof(CHINODESETBUFFERPROPERTIESINFO))
        {
            ChiRemosaicNode* pNode = static_cast<ChiRemosaicNode*>(pSetBufferInfo->hNodeSession);
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
/// RemosaicNodeProcRequest
///
/// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult RemosaicNodeProcRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pProcessRequestInfo) || (NULL == pProcessRequestInfo->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument");
    }

    if (CDKResultSuccess == result)
    {
        if (pProcessRequestInfo->size >= sizeof(CHINODEPROCESSREQUESTINFO))
        {
            ChiRemosaicNode* pNode = static_cast<ChiRemosaicNode*>(pProcessRequestInfo->hNodeSession);
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
/// RemosaicNodeSetNodeInterface
///
/// @brief  Implementation of PFCHINODESETNODEINTERFACE defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID RemosaicNodeSetNodeInterface(
    ChiNodeInterface* pNodeInterface)
{
    CDKResult result = CDKResultSuccess;
    result = ChiNodeUtils::SetNodeInterface(pNodeInterface, RemosaicNodeSectionName,
        &g_ChiNodeInterface, &g_vendorTagBase);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RemosaicNodeGetFlushResponseTime
///
/// @brief  Implementation of PFNNODEFLUSHRESPONSEINFO defined in chinode.h
///
/// @param  pInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult RemosaicNodeGetFlushResponseTime(
    CHINODERESPONSEINFO* pInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pInfo) || (NULL == pInfo->hChiSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument");
    }

    if (CDKResultSuccess == result)
    {
        if (pInfo->size == sizeof(CHINODERESPONSEINFO))
        {
            pInfo->responseTimeInMillisec = REMOSAIC_FLUSH_RESPONSE_TIME;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODERESPONSEINFO is smaller than expected");
            result = CDKResultEFailed;
        }

        LOG_INFO(CamxLogGroupChi, "flush response time: %d", static_cast<INT32>(pInfo->responseTimeInMillisec));
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
            pNodeCallbacks->pGetCapabilities         = RemosaicNodeGetCaps;
            pNodeCallbacks->pQueryVendorTag          = RemosaicNodeQueryVendorTag;
            pNodeCallbacks->pCreate                  = RemosaicNodeCreate;
            pNodeCallbacks->pDestroy                 = RemosaicNodeDestroy;
            pNodeCallbacks->pQueryBufferInfo         = RemosaicNodeQueryBufferInfo;
            pNodeCallbacks->pSetBufferInfo           = RemosaicNodeSetBufferInfo;
            pNodeCallbacks->pProcessRequest          = RemosaicNodeProcRequest;
            pNodeCallbacks->pChiNodeSetNodeInterface = RemosaicNodeSetNodeInterface;
            pNodeCallbacks->pGetFlushResponse        = RemosaicNodeGetFlushResponseTime;
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
        LOG_ERROR(CamxLogGroupChi, "Invalid Argument: %p", pNodeCallbacks);
    }
}
#ifdef __cplusplus
}
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiRemosaicNode::Initialize(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult result = CDKResultSuccess;

    /// @todo (CAMX-1854) Check for Node Capabilities using NodeCapsMask
    m_hChiSession    = pCreateInfo->hChiSession;
    m_nodeId         = pCreateInfo->nodeId;
    m_nodeCaps       = pCreateInfo->nodeCaps.nodeCapsMask;
    m_nodeFlags      = pCreateInfo->nodeFlags;
    m_processedFrame = 0;

    result = QueryVendorTagIds();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::QueryBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiRemosaicNode::QueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult result = CDKResultSuccess;
    (void) pQueryBufferInfo;

    // set the nodeCaps later
    switch (m_nodeCaps)
    {
        case ChiNodeCapsRemosaic:
            break;
        case ChiNodeCapsHalfScale:
        case ChiNodeCapsQuarterScale:
        default:
            break;
    }

    if (CDKResultSuccess == result)
    {
        CHIDefaultBufferNegotiation(pQueryBufferInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::SetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiRemosaicNode::SetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    m_format.width  = pSetBufferInfo->pFormat->width;
    m_format.height = pSetBufferInfo->pFormat->height;

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiRemosaicNode::ProcessRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult result           = CDKResultSuccess;
    BOOL      useInputMetaPool = TRUE;

    if (TRUE == IsRealTime())
    {
        useInputMetaPool = FALSE;
    }

    result = GetMetaDataFromMetaDataPool(pProcessRequestInfo, useInputMetaPool);

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < pProcessRequestInfo->inputNum; i++)
        {
            // Cache invalidate on input buffers
            g_ChiNodeInterface.pCacheOps(pProcessRequestInfo->phInputBuffer[i], TRUE, FALSE);

            DoRemosaic(pProcessRequestInfo->phOutputBuffer[i], pProcessRequestInfo->phInputBuffer[i]);

            // Cache clean on ouput buffer
            g_ChiNodeInterface.pCacheOps(pProcessRequestInfo->phOutputBuffer[i], FALSE, TRUE);
        }

        m_processedFrame++;
        UpdateMetaData(pProcessRequestInfo->frameNum);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::ChiRemosaicNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiRemosaicNode::ChiRemosaicNode()
    : m_hChiSession(NULL)
    , m_nodeId(0)
    , m_nodeCaps(0)
    , m_processedFrame(0)
{
    memset(&m_format, 0, sizeof(CHINODEIMAGEFORMAT));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::~ChiRemosaicNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiRemosaicNode::~ChiRemosaicNode()
{
    m_hChiSession = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::DoRemosaic
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiRemosaicNode::DoRemosaic(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    LOG_INFO(CamxLogGroupChi, "%s, [INPUT], format:%d, wxh:%dx%d, stridexscanline:%dx%d, plane[0] size:%zu",
        __func__,
        hInput->format.format,
        hInput->format.width,
        hInput->format.height,
        hInput->format.formatParams.rawFormat.stride,
        hInput->format.formatParams.rawFormat.sliceHeight,
        hInput->planeSize[0]);

    LOG_INFO(CamxLogGroupChi, "%s, [OUTPUT], format:%d, wxh:%dx%d, stridexscanline:%dx%d, plane[0] size:%zu",
        __func__,
        hOutput->format.format,
        hOutput->format.width,
        hOutput->format.height,
        hOutput->format.formatParams.rawFormat.stride,
        hOutput->format.formatParams.rawFormat.sliceHeight,
        hOutput->planeSize[0]);

    memcpy(&hOutput->metadataSize[0], &hInput->metadataSize[0], sizeof(hInput->metadataSize));
    hOutput->imageCount = hInput->imageCount;

    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        for (UINT j = 0; j < hOutput->numberOfPlanes; j++)
        {
            UCHAR* pIn   = hInput->pImageList[i].pAddr[j];
            UCHAR* pOut  = hOutput->pImageList[i].pAddr[j];
            INT32  count = (hInput->format.width * hInput->format.height * 5) / 4;

            if ((hOutput->format.width  == hInput->format.width)  &&
                (hOutput->format.height == hInput->format.height) &&
                (hOutput->format.format == hInput->format.format))
            {
                LOG_INFO(CamxLogGroupChi, "just memcpy for swremosiac");
                // simply do memcpy if input/output have same size and format
                memcpy(pOut, pIn, hOutput->planeSize[0]);
            }
            else
            {
                LOG_INFO(CamxLogGroupChi, "unpacking for swremosiac");
                LOG_INFO(CamxLogGroupChi, "Quad CFA dummy unpacking (mipi10 --> plain16)");

                for (INT32 i = 0; i < count; i = i + 5)
                {
                    *pOut = ((*(pIn + 4)) & 0b00000011) | ((*pIn & 0b00111111) << 2);
                    pOut++;
                    *pOut = ((*pIn) & 0b11000000) >> 6;
                    pOut++;

                    *pOut = (((*(pIn + 4)) & 0b00001100) >> 2) | ((*(pIn + 1) & 0b00111111) << 2);
                    pOut++;
                    *pOut = ((*(pIn + 1)) & 0b11000000) >> 6;
                    pOut++;

                    *pOut = (((*(pIn + 4)) & 0b00110000) >> 4) | ((*(pIn + 2) & 0b00111111) << 2);
                    pOut++;
                    *pOut = ((*(pIn + 2)) & 0b11000000) >> 6;
                    pOut++;

                    *pOut = (((*(pIn + 4)) & 0b11000000) >> 6) | ((*(pIn + 3) & 0b00111111) << 2);
                    pOut++;
                    *pOut = ((*(pIn + 3)) & 0b11000000) >> 6;
                    pOut++;

                    pIn = pIn + 5;
                }
            }
        }
    }

    LOG_INFO(CamxLogGroupChi, "%s, SW Remosaic Done", __func__);

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::QueryVendorTagIds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiRemosaicNode::QueryVendorTagIds()
{
    CDKResult            result        = CDKResultSuccess;
    CHIVENDORTAGBASEINFO vendorTagBase = { 0 };

    result = ChiNodeUtils::GetVendorTagBase("org.quic.camera2.statsconfigs",
                                            "AECFrameControl",
                                            &g_ChiNodeInterface,
                                            &vendorTagBase);
    if (CDKResultSuccess == result)
    {
        g_vendorTagIds.AECFrameControlTagId = vendorTagBase.vendorTagBase;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to get AECFrameControlTagId Vendor Tag Id");
    }

    result = ChiNodeUtils::GetVendorTagBase("org.quic.camera2.statsconfigs",
                                            "AWBFrameControl",
                                            &g_ChiNodeInterface,
                                            &vendorTagBase);
    if (CDKResultSuccess == result)
    {
        g_vendorTagIds.AWBFrameControlTagId = vendorTagBase.vendorTagBase;
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to get AWBFrameControl Vendor Tag Id");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::GetMetaDataFromMetaDataPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiRemosaicNode::GetMetaDataFromMetaDataPool(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo,
    BOOL                       useInputPool)
{
    CDKResult            result     = CDKResultSuccess;
    VOID*                pData      = NULL;
    UINT32               tagId      = 0;
    AECFrameControl*     aecControl = NULL;
    AWBFrameControl*     awbControl = NULL;

    if (TRUE == useInputPool)
    {
        tagId = g_vendorTagIds.AECFrameControlTagId | InputMetadataSectionMask;
    }
    else
    {
        tagId = g_vendorTagIds.AECFrameControlTagId;
    }

    pData = ChiNodeUtils::GetMetaData(
        pProcessRequestInfo->frameNum,
        tagId,
        ChiMetadataDynamic,
        &g_ChiNodeInterface,
        m_hChiSession);

    aecControl = (AECFrameControl*)pData;
    if (NULL != aecControl)
    {
        LOG_VERBOSE(CamxLogGroupChi, "aec output info: luxIndex:%f, ET:%" PRIu64 ", gain:%f",
            aecControl->luxIndex,
            aecControl->exposureInfo[AECAlgoExposureSafe].exposureTime,
            aecControl->exposureInfo[AECAlgoExposureSafe].linearGain);
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "fail to get aec info from metadata");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        if (TRUE == useInputPool)
        {
            tagId = g_vendorTagIds.AWBFrameControlTagId | InputMetadataSectionMask;
        }
        else
        {
            tagId = g_vendorTagIds.AWBFrameControlTagId;
        }

        pData = ChiNodeUtils::GetMetaData(
            pProcessRequestInfo->frameNum,
            tagId,
            ChiMetadataDynamic,
            &g_ChiNodeInterface,
            m_hChiSession);

        awbControl = (AWBFrameControl*)pData;
        if (NULL != awbControl)
        {
            LOG_VERBOSE(CamxLogGroupChi, "awb output info: cct:%d, r_gain:%f, g_gain:%f, b_gain:%f",
                awbControl->colorTemperature,
                awbControl->AWBGains.rGain,
                awbControl->AWBGains.gGain,
                awbControl->AWBGains.bGain);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "fail to get awb info from metadata");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::UpdateMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiRemosaicNode::UpdateMetaData(
    UINT64 requestId)
{
    CHIMETADATAINFO        metadataInfo = {0};
    const UINT32           tagSize      = sizeof(g_VendorTagSectionRemosaic) / sizeof(g_VendorTagSectionRemosaic[0]);
    CHITAGDATA             tagData[tagSize];
    UINT32                 tagList[tagSize];

    metadataInfo.size       = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession = m_hChiSession;
    metadataInfo.tagNum     = tagSize;
    metadataInfo.pTagList   = &tagList[0];
    metadataInfo.pTagData   = &tagData[0];

    UINT32 index = 0;

    UINT32  supportedFeature    = ChiNodeCapsRemosaic;
    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &supportedFeature;
    tagData[index].dataSize     = g_VendorTagSectionRemosaic[index].numUnits;
    index++;

    UINT32  currentMode         = m_nodeCaps;
    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &currentMode;
    tagData[index].dataSize     = g_VendorTagSectionRemosaic[index].numUnits;
    index++;

    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &m_processedFrame;
    tagData[index].dataSize     = g_VendorTagSectionRemosaic[index].numUnits;
    index++;

    UINT32 dimension[2];
    dimension[0]                = m_format.width;
    dimension[1]                = m_format.height;
    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &dimension[0];
    tagData[index].dataSize     = g_VendorTagSectionRemosaic[index].numUnits;

    g_ChiNodeInterface.pSetMetadata(&metadataInfo);

#if PRINT_METADATA
    // get the updated metadata from CamX and print them out
    VOID* pData = NULL;
    pData = GetMetaData(requestId, RemosaicNodeTagSupportedFeature + g_vendorTagBase);
    if (NULL != pData)
    {
        LOG_VERBOSE(CamxLogGroupChi, "supported feature %d supported feature %d",
            *static_cast<UINT32*>(pData), supportedFeature);
    }

    pData = GetMetaData(requestId, RemosaicNodeTagCurrentMode + g_vendorTagBase);
    if (NULL != pData)
    {
        LOG_VERBOSE(CamxLogGroupChi, "current mode %d current mode %d",
            *static_cast<UINT32*>(pData), currentMode);
    }

    pData = GetMetaData(requestId, RemosaicNodeTagProcessedFrameNum + g_vendorTagBase);
    if (NULL != pData)
    {
        LOG_VERBOSE(CamxLogGroupChi, "processed frameNum %" PRIu64 " processed frame %" PRIu64,
            *static_cast<UINT64*>(pData), m_processedFrame);
    }

    pData = GetMetaData(requestId, RemosaicNodeTagFrameDimension + g_vendorTagBase);
    if (NULL != pData)
    {
        UINT32*  local_dimension = (UINT32 *)pData;
        LOG_VERBOSE(CamxLogGroupChi, "frame dimension %d %d frame dimenstion %d %d",
            local_dimension[0], local_dimension[1], dimension[0], dimension[1]);
    }

    // sample of getting Android metadata from CamX
    pData = GetMetaData(requestId, ANDROID_SENSOR_TIMESTAMP);
    if (NULL != pData)
    {
        INT64 timestamp = *static_cast<INT64 *>(pData);
        LOG_VERBOSE(CamxLogGroupChi, "timestamp for current frame %" PRIi64, timestamp);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiRemosaicNode::GetMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiRemosaicNode::GetMetaData(
    UINT64 requestId,
    UINT32 tagId)
{
    CHIMETADATAINFO        metadataInfo = {0};
    const UINT32           tagSize      = 1;
    CHITAGDATA             tagData      = {0};
    UINT32                 tagList      = tagId;

    tagData.requestId       = requestId;

    metadataInfo.size       = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession = m_hChiSession;
    metadataInfo.tagNum     = tagSize;
    metadataInfo.pTagList   = &tagList;
    metadataInfo.pTagData   = &tagData;

    g_ChiNodeInterface.pGetMetadata(&metadataInfo);
    return tagData.pData;
}

// CAMX_NAMESPACE_END
