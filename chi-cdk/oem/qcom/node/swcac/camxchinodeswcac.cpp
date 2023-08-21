////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodeswcacnode.cpp
/// @brief Chi node for SW Cac
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <inttypes.h>
#include <system/camera_metadata.h>
#include <stdlib.h>

#include "camxchinodeswcac.h"

#undef  LOG_TAG
#define LOG_TAG "CHISWCAC"

#define PRINT_METADATA TRUE

ChiNodeInterface g_ChiNodeInterface;                                      ///< The instance save the CAMX Chi interface
UINT32           g_vendorTagBase                 = 0;                     ///< Chi assigned runtime vendor tag base for the node

static const UINT32 ChiNodeMajorVersion          = 0;                     ///< The major version of CHI interface
static const UINT32 ChiNodeMinorVersion          = 0;                     ///< The minor version of CHI interface

const CHAR*         pCac3AlgoLibName             = "libmmcamera_cac3";    /// Cac algo library name.
static const CHAR   CacNodeSectionName[]         = "com.qti.node.swcac";  ///< The section name for node
BOOL ChiSwCacNode::m_bIsCacInitDone              = FALSE;                 /// Tells cac_init status.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SwCacNodeGetCaps
///
/// @brief  Implementation of PFNNODEGETCAPS defined in chinode.h
///
/// @param  pCapsInfo   Pointer to a structure that defines the capabilities supported by the node.
///
/// @return Always CDKResultSuccess.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SwCacNodeGetCaps(
    CHINODECAPSINFO* pCapsInfo)
{
    // As SW-CAC does not support any specific capability. Hence returning success always.
    (VOID) pCapsInfo;

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SwCacNodeCreate
///
/// @brief  Implementation of PFNNODECREATE defined in chinode.h
///
/// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SwCacNodeCreate(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult       result  = CDKResultSuccess;
    ChiSwCacNode*   pNode   = NULL;

    if (NULL == pCreateInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pTagTypeInfo is NULL");
    }

    if ((CDKResultSuccess == result) && (pCreateInfo->size < sizeof(CHINODECREATEINFO)))
    {
        LOG_ERROR(CamxLogGroupChi, "CHINODECREATEINFO is smaller than expected");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        pNode = new ChiSwCacNode;
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
/// SwCacNodeDestroy
///
/// @brief  Implementation of PFNNODEDESTROY defined in chinode.h
///
/// @param  pDestroyInfo    Pointer to a structure that defines the session destroy information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SwCacNodeDestroy(
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
            ChiSwCacNode* pNode = static_cast<ChiSwCacNode*>(pDestroyInfo->hNodeSession);
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
/// SwCacNodeQueryBufferInfo
///
/// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
///
/// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SwCacNodeQueryBufferInfo(
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
            ChiSwCacNode* pNode = static_cast<ChiSwCacNode*>(pQueryBufferInfo->hNodeSession);
            result              = pNode->QueryBufferInfo(pQueryBufferInfo);
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
/// SwCacNodeSetBufferInfo
///
/// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
///
/// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SwCacNodeSetBufferInfo(
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
            ChiSwCacNode* pNode = static_cast<ChiSwCacNode*>(pSetBufferInfo->hNodeSession);
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
/// SwCacNodeProcRequest
///
/// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SwCacNodeProcRequest(
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
            ChiSwCacNode* pNode = static_cast<ChiSwCacNode*>(pProcessRequestInfo->hNodeSession);
            result              = pNode->ProcessRequest(pProcessRequestInfo);
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
/// SwCacNodeSetNodeInterface
///
/// @brief  Implementation of PFCHINODESETNODEINTERFACE defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID SwCacNodeSetNodeInterface(
    ChiNodeInterface* pNodeInterface)
{
    CDKResult result = CDKResultSuccess;

    result = ChiNodeUtils::SetNodeInterface(pNodeInterface, CacNodeSectionName,
        &g_ChiNodeInterface, &g_vendorTagBase);
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
            pNodeCallbacks->pGetCapabilities         = SwCacNodeGetCaps;
            pNodeCallbacks->pCreate                  = SwCacNodeCreate;
            pNodeCallbacks->pDestroy                 = SwCacNodeDestroy;
            pNodeCallbacks->pQueryBufferInfo         = SwCacNodeQueryBufferInfo;
            pNodeCallbacks->pSetBufferInfo           = SwCacNodeSetBufferInfo;
            pNodeCallbacks->pProcessRequest          = SwCacNodeProcRequest;
            pNodeCallbacks->pChiNodeSetNodeInterface = SwCacNodeSetNodeInterface;
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
/// ChiSwCacNode::InitializeFuncPtrs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSwCacNode::InitializeFuncPtrs()
{
    CDKResult result                    = CDKResultSuccess;
    CHAR      libFilePath[FILENAME_MAX] = "";
    INT       numCharWritten            = 0;

    numCharWritten = ChiNodeUtils::SNPrintF(libFilePath,
                                            FILENAME_MAX,
                                            "%s%s%s.%s",
                                            CameraComponentLibPath,
                                            PathSeparator,
                                            pCac3AlgoLibName,
                                            SharedLibraryExtension);

    m_hCac3Lib = ChiNodeUtils::LibMapFullName(libFilePath);

    if (NULL == m_hCac3Lib)
    {
        LOG_ERROR(CamxLogGroupChi, "Unable to open %s library", libFilePath);
        result = CDKResultEUnableToLoad;
    }

    if (CDKResultSuccess == result)
    {
        m_hCac3Process = reinterpret_cast<CHICAC3PROCESS>(
            ChiNodeUtils::LibGetAddr(m_hCac3Lib, "cac3_process"));

        if (NULL == m_hCac3Process)
        {
            LOG_ERROR(CamxLogGroupChi, "Unable to find cac3_process");
            result = CDKResultEUnableToLoad;
        }

        if (CDKResultSuccess == result)
        {
            m_hCac3Init = reinterpret_cast<CHICAC3INIT>(
                ChiNodeUtils::LibGetAddr(m_hCac3Lib, "cac3_init"));
            if (NULL == m_hCac3Init)
            {
                LOG_ERROR(CamxLogGroupChi, "Unable to find cac3_init");
                result = CDKResultEUnableToLoad;
            }

            if (CDKResultSuccess == result)
            {
                m_hCac3Deinit = reinterpret_cast<CHICAC3DEINIT>(
                    ChiNodeUtils::LibGetAddr(m_hCac3Lib, "cac3_deinit"));
                if (NULL == m_hCac3Deinit)
                {
                    LOG_ERROR(CamxLogGroupChi, "Unable to find cac3_deinit");
                    result = CDKResultEUnableToLoad;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::DeInitializeFunPtrs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSwCacNode::DeInitializeFunPtrs()
{

    CDKResult result = ChiNodeUtils::LibUnmap(m_hCac3Lib);

    if (CDKResultSuccess == result)
    {
        m_hCac3Deinit  = NULL;
        m_hCac3Process = NULL;
        m_hCac3Init    = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSwCacNode::Initialize(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult result                         = CDKResultSuccess;
    CHAR      swCacState[PROPERTY_VALUE_MAX];

    if (NULL == pCreateInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pCreateInfo is NULL");
    }

    if (CDKResultSuccess == result)
    {
        result = InitializeFuncPtrs();
        if (CDKResultSuccess != result)
        {
            LOG_ERROR(CamxLogGroupChi, "cac3 library not loaded");
            result = CDKResultEInvalidPointer;
        }
    }

    if (CDKResultSuccess == result)
    {
        m_hChiSession    = pCreateInfo->hChiSession;
        m_nodeId         = pCreateInfo->nodeId;
        m_nodeCaps       = pCreateInfo->nodeCaps.nodeCapsMask;
        m_nodeFlags      = pCreateInfo->nodeFlags;
    }

    property_get("persist.vendor.camera.swcacstate", swCacState, "0");
    m_enableSwCacAlways = atoi(swCacState);

    m_processedFrame = 0;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::QueryBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSwCacNode::QueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    ChiNodeUtils::DefaultBufferNegotiation(pQueryBufferInfo);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::SetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSwCacNode::SetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    CDKResult result = CDKResultEFailed;

    if (NULL != pSetBufferInfo)
    {
        m_format.width  = pSetBufferInfo->pFormat->width;
        m_format.height = pSetBufferInfo->pFormat->height;
        result = CDKResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::ConvertCamxImageFormatToCacImageFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CacImageFormat ChiSwCacNode::ConvertCamxImageFormatToCacImageFormat(
    ChiFormat  chiformat)
{
    CacImageFormat cacImageFormat = InvalidImageFormat;

    switch (chiformat)
    {
    case YUV420NV12:
        cacImageFormat = YCbCr;
        break;
    case YUV420NV21:
        cacImageFormat = YCrCb;
        break;
    default:
        break;
    }

    return cacImageFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::InitCacArgument
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSwCacNode::InitCacArgument(
    CHINODEBUFFERHANDLE* phBuffer)
{
    CDKResult result = CDKResultSuccess;
    CacImageFormat cacImageFormat = ConvertCamxImageFormatToCacImageFormat((*phBuffer)->format.format);

    if (InvalidImageFormat == cacImageFormat)
    {
        LOG_ERROR(CamxLogGroupChi, "CAC doesn't support %d CamX image format",
            (*phBuffer)->format.format);
        result = CDKResultEFailed;
    }
    else
    {
        m_cac3Argruments.imageFormat      = cacImageFormat;
        m_cac3Argruments.ionHeapId        = ION_SYSTEM_HEAP_ID;

        m_cac3Argruments.yWidth           = (*phBuffer)->format.formatParams.yuvFormat[0].width;
        m_cac3Argruments.yHeight          = (*phBuffer)->format.formatParams.yuvFormat[0].height;
        m_cac3Argruments.yStride          = (*phBuffer)->format.formatParams.yuvFormat[0].planeStride;

        m_cac3Argruments.uvWidth          = (*phBuffer)->format.formatParams.yuvFormat[1].width;
        m_cac3Argruments.uvHeight         = (*phBuffer)->format.formatParams.yuvFormat[1].height;
        m_cac3Argruments.uvStride         = (*phBuffer)->format.formatParams.yuvFormat[1].planeStride;

        m_cac3Argruments.yOutStride       = (*phBuffer)->format.formatParams.yuvFormat[0].planeStride;
        m_cac3Argruments.uvOutStride      = (*phBuffer)->format.formatParams.yuvFormat[1].planeStride;

        m_cac3Argruments.cac3EnableFlag   = 1;

        // Below parameters are NULL for Synchronous implementation in first stage.
        m_cac3Argruments.userData = NULL;
        m_cac3Argruments.cac3_cb  = NULL;

        // CAC3 parameters
        m_cac3Argruments.detection_TH1    = 10;      // low threshold
        m_cac3Argruments.detection_TH2    = 240;     // high threshold
        m_cac3Argruments.verification_TH1 = 15;      // color threshold for cb
        m_cac3Argruments.verification_TH2 = 12;      // color threshold for cb

        // ptrs
        m_cac3Argruments.pIonVirtualAddr  = (*phBuffer)->pImageList[0].pAddr[0];
        m_cac3Argruments.pLumaData        = (*phBuffer)->pImageList[0].pAddr[0];
        m_cac3Argruments.pChromaData      = (*phBuffer)->pImageList[0].pAddr[1];
        m_cac3Argruments.fd               = (*phBuffer)->pImageList[0].fd[0];

        // m_cac3Argruments.isCached is independent of current cac lib implementation.
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::ApplyCacAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSwCacNode::ApplyCacAlgo(
    CHINODEBUFFERHANDLE* phBuffer)
{
    CDKResult result = CDKResultSuccess;

    if (FALSE == m_bIsCacInitDone)
    {
        if (CDKResultSuccess != (*m_hCac3Init)(ION_SYSTEM_HEAP_ID))
        {
            LOG_ERROR(CamxLogGroupChi, "cac3_init failed");
            result = CDKResultEFailed;
        }
        else
        {
            LOG_VERBOSE(CamxLogGroupChi, "cac3_init done");
            m_bIsCacInitDone = TRUE;
        }
    }
    if (CDKResultSuccess == result)
    {
        result = InitCacArgument(phBuffer);
        if (CDKResultSuccess == result)
        {
            if (0 != (*m_hCac3Process)(&m_cac3Argruments))
            {
                LOG_ERROR(CamxLogGroupChi, "cac3_process failed");
                result = CDKResultEFailed;
            }
            else
            {
                LOG_VERBOSE(CamxLogGroupChi, "Cac3 algo applied");
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::GetCacState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 ChiSwCacNode::GetCacState(
    UINT64 requestId)
{
    VOID* pData = NULL;
    UINT8 CacState = 0;
    pData = ChiNodeUtils::GetMetaData(requestId,
                                      (ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES |
                                          InputMetadataSectionMask),
                                      ChiMetadataDynamic,
                                      &g_ChiNodeInterface,
                                      m_hChiSession);

    if (NULL != pData)
    {
        CacState = *static_cast<UINT8 *>(pData);
    }

    return CacState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSwCacNode::ProcessRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult result = CDKResultSuccess;

    for (UINT32 i = 0; i < pProcessRequestInfo->outputNum; i++)
    {
        if (TRUE == IsBypassableNode())
        {
            UINT8 Cac3State = GetCacState(pProcessRequestInfo->frameNum);

            if (Cac3State || m_enableSwCacAlways)
            {
                if (CDKResultSuccess != ApplyCacAlgo(&pProcessRequestInfo->phInputBuffer[i]))
                {
                    LOG_ERROR(CamxLogGroupChi, "ApplyCacAlgo failed");
                    result = CDKResultEFailed;
                }
                else
                {
                    LOG_INFO(CamxLogGroupChi, "Cac algo applied");
                }
            }
            else
            {
                LOG_INFO(CamxLogGroupChi, "Cac feature is disabled ");
            }
            // As this node is by-passable node, so no need to copy input buffer data to output buffer.
            pProcessRequestInfo->pBypassData[i].isBypassNeeded = TRUE;
            pProcessRequestInfo->pBypassData[i].selectedInputPortIndex = 0;
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Node is not bypassable");
            result = CDKResultEFailed;
        }
    }

    m_processedFrame++;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::ChiSwCacNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiSwCacNode::ChiSwCacNode()
    : m_hChiSession(NULL)
    , m_nodeId(0)
    , m_nodeCaps(0)
    , m_enableSwCacAlways(0)
    , m_processedFrame(0)
{
    memset(&m_cac3Argruments, 0, sizeof(m_cac3Argruments));
    memset(&m_format, 0, sizeof(CHINODEIMAGEFORMAT));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSwCacNode::~ChiSwCacNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiSwCacNode::~ChiSwCacNode()
{
    if (m_bIsCacInitDone == TRUE)
    {
        (*m_hCac3Deinit)();
        m_bIsCacInitDone = FALSE;
    }
    m_hChiSession = NULL;
    DeInitializeFunPtrs();
}

// CAMX_NAMESPACE_END
