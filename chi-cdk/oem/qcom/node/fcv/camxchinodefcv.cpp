////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodefcv.cpp
/// @brief Chi node for FCV rotation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <inttypes.h>
#include <system/camera_metadata.h>

#include "camxchinodefcv.h"
#include "camxutils.h"


// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads
// NOWHINE FILE NC008: Warning: - Var names should be lower camel case

#undef LOG_TAG
#define LOG_TAG "CHIFCV"

#define PRINT_METADATA FALSE
// =============================================================================================================================
// Start Port to OSUtils section
// =============================================================================================================================
#include <stdarg.h>                 // For variable args
#include <stdio.h>                  // For vsnprintf
#include <math.h>                   // For sin/cos

#if defined (_LINUX)
#include <dlfcn.h>                  // For dynamic linking

const CHAR* pDefaultFastCVLibraryName = "libfastcvopt";

#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WIN32)

ChiNodeInterface g_ChiNodeInterface;    ///< The instance save the CAMX Chi interface
UINT32           g_vendorTagBase = 0;   ///< Chi assigned runtime vendor tag base for the node

/// @todo (CAMX-1854) the major / minor version shall get from CHI
static const UINT32 ChiNodeMajorVersion = 0;    ///< The major version of CHI interface
static const UINT32 ChiNodeMinorVersion = 0;    ///< The minor version of CHI interface


static const CHAR   FCVNodeSectionName[]         = "com.qti.node.fcv";          ///< The section name for node
static const UINT32 ChiNodeCapsFCV               = 1;                                                                            ///

static const UINT32 FCVNodeTagBase               = 0;                        ///< Tag base
static const UINT32 FCVNodeTagSupportedFeature   = FCVNodeTagBase + 0;    ///< Tag for supported features
static const UINT32 FCVNodeTagCurrentMode        = FCVNodeTagBase + 1;    ///< Tag to indicated current operation mode
static const UINT32 FCVNodeTagProcessedFrameNum  = FCVNodeTagBase + 2;    ///< Tag to show processed frame's count
static const UINT32 FCVNodeTagFrameDimension     = FCVNodeTagBase + 3;    ///< Tag to show current's frame dimension

///< Supported vendor tag list, it shall align with the definition in g_VendorTagSectionFCV
static const UINT32 g_VendorTagList[] =
{
    FCVNodeTagSupportedFeature,
    FCVNodeTagCurrentMode,
    FCVNodeTagProcessedFrameNum,
    FCVNodeTagFrameDimension
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGDATA g_VendorTagSectionFCV[] =
{
    { "SupportedFeature",       TYPE_INT32, 1 },
    { "CurrentMode",            TYPE_BYTE,  1 },
    { "ProcessedFrameNumber",   TYPE_INT64, 1 },
    { "FrameDimension",         TYPE_INT32, 2 },
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGSECTIONDATA g_VendorTagFCVSection[] =
{
    {
        FCVNodeSectionName,  0,
        sizeof(g_VendorTagSectionFCV) / sizeof(g_VendorTagSectionFCV[0]), g_VendorTagSectionFCV,
        CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    }
};

///< This is an array of all vendor tag section data
static ChiVendorTagInfo g_VendorTagInfoFCV[] =
{
    {
        &g_VendorTagFCVSection[0],
        sizeof(g_VendorTagFCVSection) / sizeof(g_VendorTagFCVSection[0])
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// VSNPrintF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT VSNPrintF(
    CHAR*       pDst,
    SIZE_T      sizeDst,
    const CHAR* pFormat,
    va_list     argptr)
{
    INT numCharWritten = 0;

#if defined (_LINUX)
    numCharWritten = vsnprintf(pDst, sizeDst, pFormat, argptr);
#elif defined (_WIN32)
    numCharWritten = vsnprintf_s(pDst, sizeDst, _TRUNCATE, pFormat, argptr);
#else
#error Unsupported target defined
#endif
    if ((numCharWritten >= static_cast<INT>(sizeDst)) && (sizeDst > 0))
    {
        // Message length exceeds the buffer limit size
        // CAMX_ASSERT_ALWAYS();
        numCharWritten = -1;
    }
    return numCharWritten;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LibMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OSLIBRARYHANDLE LibMap(
    const CHAR* pLibraryName)
{
    OSLIBRARYHANDLE hLibrary = NULL;

#if defined (_LINUX)
    const UINT bind_flags = RTLD_NOW | RTLD_LOCAL;
    hLibrary = dlopen(pLibraryName, bind_flags);
#elif defined (_WIN32)
    hLibrary = LoadLibrary(pLibraryName);
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WIN32
    if (NULL == hLibrary)
    {
        LOG_ERROR(CamxLogGroupChi, "Error Loading Library: %s", pLibraryName);
    }

    return hLibrary;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LibUnmap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult LibUnmap(
    OSLIBRARYHANDLE hLibrary)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != hLibrary)
    {
#if defined (_LINUX)
        if (0 != dlclose(hLibrary))
        {
            result = CDKResultEFailed;
        }
#elif defined (_WIN32)
        if (FALSE == FreeLibrary(static_cast<HMODULE>(hLibrary)))
        {
            result = CDKResultEFailed;
        }
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WIN32)
    }
    else
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess != result)
    {
        LOG_ERROR(CamxLogGroupCHI, "Failed to close FCV Node Library");
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LibGetAddr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* LibGetAddr(
    OSLIBRARYHANDLE hLibrary,
    const CHAR*     pProcName)
{
    VOID* pProcAddr = NULL;

    if (hLibrary != NULL)
    {
#if defined (_LINUX)
        pProcAddr = dlsym(hLibrary, pProcName);
#elif defined (_WIN32)
        pProcAddr = GetProcAddress(static_cast<HMODULE>(hLibrary), pProcName);
#else
#error Unsupported target defined
#endif // defined(_LINUX) || defined(_WIN32)
    }

    return pProcAddr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SNPrintF
/// @brief Write formatted data from variable argument list to sized buffer
/// @param pDst     Destination buffer
/// @param sizeDst  Size of the destination buffer
/// @param pFormat  Format string, printf style
/// @param ...      Parameters required by format
///
/// @return Number of characters written
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT SNPrintF(
    CHAR*       pDst,
    SIZE_T      sizeDst,
    const CHAR* pFormat,
...)
{
    INT     numCharWritten;
    va_list args;

    va_start(args, pFormat);
    numCharWritten = VSNPrintF(pDst, sizeDst, pFormat, args);
    va_end(args);

    return numCharWritten;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FCVNodeGetCaps
///
/// @brief  Implementation of PFNNODEGETCAPS defined in chinode.h
///
/// @param  pCapsInfo   Pointer to a structure that defines the capabilities supported by the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult FCVNodeGetCaps(
    CHINODECAPSINFO* pCapsInfo)
{
    CDKResult result = CDKResultSuccess;

    if (pCapsInfo == NULL)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pCapsInfo is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pCapsInfo->size >= sizeof(CHINODECAPSINFO))
        {
            pCapsInfo->nodeCapsMask = ChiNodeCapsFCVRotate ;
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
/// FCVNodeQueryVendorTag
///
/// @brief  Implementation of PFNCHIQUERYVENDORTAG defined in chinode.h
///
/// @param  pQueryVendorTag Pointer to a structure that returns the exported vendor tag
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult FCVNodeQueryVendorTag(
    CHIQUERYVENDORTAG* pQueryVendorTag)
{
    CDKResult result = CDKResultSuccess;
    result = ChiNodeUtils::QueryVendorTag(pQueryVendorTag, g_VendorTagInfoFCV);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FCVNodeCreate
///
/// @brief  Implementation of PFNNODECREATE defined in chinode.h
///
/// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult FCVNodeCreate(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult    result  = CDKResultSuccess;
    ChiFCVNode*  pNode   = NULL;

    if (NULL == pCreateInfo)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pTagTypeInfo is NULL");
    }
    else if (pCreateInfo->size < sizeof(CHINODECREATEINFO))
    {
        LOG_ERROR(CamxLogGroupChi, "CHINODECREATEINFO is smaller than expected");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        pNode = new ChiFCVNode;
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
/// FCVNodeDestroy
///
/// @brief  Implementation of PFNNODEDESTROY defined in chinode.h
///
/// @param  pDestroyInfo    Pointer to a structure that defines the session destroy information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult FCVNodeDestroy(
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
            ChiFCVNode* pNode = static_cast<ChiFCVNode*>(pDestroyInfo->hNodeSession);
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
/// FCVNodeQueryBufferInfo
///
/// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
///
/// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult FCVNodeQueryBufferInfo(
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
            ChiFCVNode* pNode    = static_cast<ChiFCVNode*>(pQueryBufferInfo->hNodeSession);
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
/// FCVNodeSetBufferInfo
///
/// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
///
/// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult FCVNodeSetBufferInfo(
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
            ChiFCVNode* pNode = static_cast<ChiFCVNode*>(pSetBufferInfo->hNodeSession);
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
/// FCVNodeProcRequest
///
/// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult FCVNodeProcRequest(
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
            ChiFCVNode* pNode = static_cast<ChiFCVNode*>(pProcessRequestInfo->hNodeSession);
            result            = pNode->ProcessRequest(pProcessRequestInfo);
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
/// FCVNodeSetNodeInterface
///
/// @brief  Implementation of PFCHINODESETNODEINTERFACE defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID FCVNodeSetNodeInterface(
    ChiNodeInterface* pNodeInterface)
{
    CDKResult result = CDKResultSuccess;
    result           = ChiNodeUtils::SetNodeInterface(pNodeInterface, FCVNodeSectionName,
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
            pNodeCallbacks->pGetCapabilities         = FCVNodeGetCaps;
            pNodeCallbacks->pQueryVendorTag          = FCVNodeQueryVendorTag;
            pNodeCallbacks->pCreate                  = FCVNodeCreate;
            pNodeCallbacks->pDestroy                 = FCVNodeDestroy;
            pNodeCallbacks->pQueryBufferInfo         = FCVNodeQueryBufferInfo;
            pNodeCallbacks->pSetBufferInfo           = FCVNodeSetBufferInfo;
            pNodeCallbacks->pProcessRequest          = FCVNodeProcRequest;
            pNodeCallbacks->pChiNodeSetNodeInterface = FCVNodeSetNodeInterface;
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
/// ChiFCVNode::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFCVNode::Initialize(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult result = CDKResultSuccess;
    INT         numCharWritten  = 0;
    CHAR        libFilename[FILENAME_MAX];

    numCharWritten = SNPrintF(libFilename, FILENAME_MAX, "%s%s%s.%s",
                              VendorLibPath, PathSeparator,
                              pDefaultFastCVLibraryName, SharedLibraryExtension);

    m_hfcvLib      = LibMap(libFilename);
    if (NULL == m_hfcvLib)
    {
        result = CDKResultEUnableToLoad;
    }
    else
    {
        m_pfnRemoteRegisterBuf = reinterpret_cast<PFNREMOTEREGISTERBUF>(LibGetAddr(m_hfcvLib, "remote_register_buf"));
        m_pfnfcvScaleDownMNu8 = reinterpret_cast<PFNFCVSCALEDOWNMNU8>(LibGetAddr(m_hfcvLib, "fcvScaleDownMNu8"));
        m_pfnfcvScaleDownMNInterleaveu8 = reinterpret_cast<PFNFCVSCALEDOWNMNINTERLEAVEU8>(LibGetAddr(m_hfcvLib, "fcvScaleDownMNInterleaveu8"));
        m_pfnfcvRotateImageu8 = reinterpret_cast<PFNFCVROTATEIMAGEU8>(LibGetAddr(m_hfcvLib, "fcvRotateImageu8"));
        m_pfnfcvRotateImageInterleavedu8 = reinterpret_cast<PFNFCVROTATEIMAGEINTERLEAVEU8>(LibGetAddr(m_hfcvLib, "fcvRotateImageInterleavedu8"));
        m_pfnfcvSetOperationModeExt = reinterpret_cast<PFNFCVSETOPERATIONMODEEXT>(LibGetAddr(m_hfcvLib, "fcvSetOperationModeExt"));
        m_pfnfcvCleanUp = reinterpret_cast<PFNFCVCLEANUP>(LibGetAddr(m_hfcvLib, "fcvCleanUp"));
    }

    if (NULL != m_pfnfcvSetOperationModeExt)
    {
        LOG_ERROR(CamxLogGroupChi, "Calling fcvSetOperationModeExt");
        m_pfnfcvSetOperationModeExt(FASTCV_OP_EXT_QDSP);
    }

    m_hChiSession    = pCreateInfo->hChiSession;
    m_nodeId         = pCreateInfo->nodeId;
    m_nodeCaps       = pCreateInfo->nodeCaps.nodeCapsMask;
    m_processedFrame = 0;
    LOG_INFO(CamxLogGroupChi, " caps %x", m_nodeCaps);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFCVNode::QueryBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFCVNode::QueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult result                     = CDKResultSuccess;

    // set the nodeCaps later
    switch (m_nodeCaps)
    {
        case ChiNodeCapsFCVRotate:
            result = CDKResultSuccess;
            break;
        default:
            result = CDKResultEFailed;
            break;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < pQueryBufferInfo->numOutputPorts; i++)
        {
            CHINODEBUFFERREQUIREMENT* pInputOptions      = &pQueryBufferInfo->pInputOptions[i].inputBufferOption;
            CHINODEBUFFERREQUIREMENT* pOutputOptions     = &pQueryBufferInfo->pOutputPortQueryInfo[i].outputBufferOption;
            CHINODEBUFFERREQUIREMENT* pOutputRequirement = &pQueryBufferInfo->pOutputPortQueryInfo[i].pBufferRequirement[0];

            pOutputOptions->minW     = ChiNodeUtils::MaxUINT32(pOutputRequirement->minW, pOutputRequirement->minH);
            pOutputOptions->minH     = ChiNodeUtils::MaxUINT32(pOutputRequirement->minW, pOutputRequirement->minH);
            pOutputOptions->maxW     = ChiNodeUtils::MaxUINT32(pOutputRequirement->maxW, pOutputRequirement->maxH);
            pOutputOptions->maxH     = ChiNodeUtils::MaxUINT32(pOutputRequirement->maxW, pOutputRequirement->maxH);
            pOutputOptions->optimalW = ChiNodeUtils::MaxUINT32(pOutputRequirement->optimalW, pOutputRequirement->optimalH);
            pOutputOptions->optimalH = ChiNodeUtils::MaxUINT32(pOutputRequirement->optimalW, pOutputRequirement->optimalH);

            pInputOptions->minW     = pOutputOptions->minW;
            pInputOptions->minH     = pOutputOptions->minH;
            pInputOptions->maxW     = pOutputOptions->maxW;
            pInputOptions->maxH     = pOutputOptions->maxH;
            pInputOptions->optimalW = pOutputOptions->optimalW;
            pInputOptions->optimalH = pOutputOptions->optimalH;

            LOG_INFO(CamxLogGroupChi, "Input min W %d H %d, max W %d h %d,"
                     "optimal w %d h %d", pInputOptions->minW, pInputOptions->minH,
                     pInputOptions->maxW, pInputOptions->maxH, pInputOptions->optimalW,
                     pInputOptions->optimalH);
            LOG_INFO(CamxLogGroupChi, "Output min W %d H %d, max W %d h %d,"
                     "optimal w %d h %d", pOutputOptions->minW, pOutputOptions->minH,
                     pOutputOptions->maxW, pOutputOptions->maxH, pOutputOptions->optimalW,
                     pOutputOptions->optimalH);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFCVNode::SetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFCVNode::SetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    m_format.width  = pSetBufferInfo->pFormat->width;
    m_format.height = pSetBufferInfo->pFormat->height;

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFCVNode::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFCVNode::ProcessRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    LOG_INFO(CamxLogGroupChi, "FCV ProcessRequest");
    for (UINT32 i = 0; i < pProcessRequestInfo->inputNum; i++)
    {
        UpdateOrientation(pProcessRequestInfo->frameNum);
        LOG_INFO(CamxLogGroupChi, " UV: 0/p plane 0 dim: %dx%d stride %d scanline %d",
            pProcessRequestInfo->phOutputBuffer[i]->format.formatParams.yuvFormat[0].width,
            pProcessRequestInfo->phOutputBuffer[i]->format.formatParams.yuvFormat[0].height,
            pProcessRequestInfo->phOutputBuffer[i]->format.formatParams.yuvFormat[0].planeStride,
            pProcessRequestInfo->phOutputBuffer[i]->format.formatParams.yuvFormat[0].sliceHeight);
        LOG_INFO(CamxLogGroupChi, " UV: 0/p plane 1 dim: %dx%d stride %d scanline %d",
            pProcessRequestInfo->phOutputBuffer[i]->format.formatParams.yuvFormat[1].width,
            pProcessRequestInfo->phOutputBuffer[i]->format.formatParams.yuvFormat[1].height,
            pProcessRequestInfo->phOutputBuffer[i]->format.formatParams.yuvFormat[1].planeStride,
            pProcessRequestInfo->phOutputBuffer[i]->format.formatParams.yuvFormat[1].sliceHeight);
        LOG_INFO(CamxLogGroupChi, " UV: i/p plane 0 dim: %dx%d stride %d scanline %d",
            pProcessRequestInfo->phInputBuffer[i]->format.formatParams.yuvFormat[0].width,
            pProcessRequestInfo->phInputBuffer[i]->format.formatParams.yuvFormat[0].height,
            pProcessRequestInfo->phInputBuffer[i]->format.formatParams.yuvFormat[0].planeStride,
            pProcessRequestInfo->phInputBuffer[i]->format.formatParams.yuvFormat[0].sliceHeight);
        LOG_INFO(CamxLogGroupChi, " UV: i/p plane 1 dim: %dx%d stride %d scanline %d",
            pProcessRequestInfo->phInputBuffer[i]->format.formatParams.yuvFormat[1].width,
            pProcessRequestInfo->phInputBuffer[i]->format.formatParams.yuvFormat[1].height,
            pProcessRequestInfo->phInputBuffer[i]->format.formatParams.yuvFormat[1].planeStride,
            pProcessRequestInfo->phInputBuffer[i]->format.formatParams.yuvFormat[1].sliceHeight);

        RotateImage(pProcessRequestInfo->phOutputBuffer[i], pProcessRequestInfo->phInputBuffer[i], m_currentRotation);


        if (Rotate90Degrees == m_currentRotation || Rotate270Degrees == m_currentRotation)
        {
            // Output is currently rotated, swap the width and height values
            m_fullOutputDimensions[0] = pProcessRequestInfo->phInputBuffer[i]->format.height;
            m_fullOutputDimensions[1] = pProcessRequestInfo->phInputBuffer[i]->format.width;
        }
        else
        {
            m_fullOutputDimensions[0] = pProcessRequestInfo->phInputBuffer[i]->format.width;
            m_fullOutputDimensions[1] = pProcessRequestInfo->phInputBuffer[i]->format.height;
        }

        m_processedFrame++;
        UpdateMetaData(pProcessRequestInfo->frameNum);
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFCVNode::ChiFCVNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFCVNode::ChiFCVNode()
    : m_hfcvLib(NULL)
    , m_pfnRemoteRegisterBuf(NULL)
    , m_pfnfcvScaleDownMNu8(NULL)
    , m_pfnfcvScaleDownMNInterleaveu8(NULL)
    , m_pfnfcvRotateImageu8(NULL)
    , m_pfnfcvRotateImageInterleavedu8(NULL)
    , m_pfnfcvSetOperationModeExt(NULL)
    , m_pfnfcvCleanUp(NULL)
    , m_hChiSession(NULL)
    , m_nodeId(0)
    , m_nodeCaps(0)
    , m_currentRotation(Rotate0Degrees)
    , m_processedFrame(0)
{
    memset(&m_format, 0, sizeof(CHINODEIMAGEFORMAT));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFCVNode::~ChiFCVNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFCVNode::~ChiFCVNode()
{
    m_pfnfcvCleanUp();
    LibUnmap(m_hfcvLib);
    m_pfnRemoteRegisterBuf             = NULL;
    m_pfnfcvScaleDownMNu8              = NULL;
    m_pfnfcvScaleDownMNInterleaveu8    = NULL;
    m_pfnfcvRotateImageu8              = NULL;
    m_pfnfcvRotateImageInterleavedu8   = NULL;
    m_pfnfcvSetOperationModeExt        = NULL;
    m_pfnfcvCleanUp                    = NULL;

    m_hChiSession = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFCVNode::RotateImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFCVNode::RotateImage(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput,
    RotationAngle       targetRotation)
{
    fcvStatus status;

    UINT32 src_stride, dst_stride;
    UINT32 src_width, src_height;
    FILE    *fptr   = NULL;
    LOG_INFO(CamxLogGroupChi, "FCV rotate E");
    memcpy(&hOutput->format, &hInput->format, sizeof(CHIIMAGEFORMAT));
    memcpy(&hOutput->planeSize[0], &hInput->planeSize[0], sizeof(hInput->planeSize));
    memcpy(&hOutput->metadataSize[0], &hInput->metadataSize[0], sizeof(hInput->metadataSize));

    hOutput->imageCount     = hInput->imageCount;
    hOutput->numberOfPlanes = hInput->numberOfPlanes;

    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        if ((NULL != m_pfnRemoteRegisterBuf) && (RotationAngle::Rotate0Degrees != targetRotation))
        {
            /* Y-Channel */

            /* Register buffer before rotation */
            // If rotated, output buffer rotation needs to have width and height swapped
            if ((RotationAngle::Rotate90Degrees == targetRotation) ||
                (RotationAngle::Rotate270Degrees == targetRotation))
            {
                src_width = hInput->format.formatParams.yuvFormat[0].width;
                src_height = hInput->format.formatParams.yuvFormat[0].height;
                src_stride = hInput->format.formatParams.yuvFormat[0].planeStride;
                dst_stride = src_height;
            }
            else
            {
                src_width = hInput->format.formatParams.yuvFormat[0].width;
                src_height = hInput->format.formatParams.yuvFormat[0].height;
                src_stride = hInput->format.formatParams.yuvFormat[0].planeStride;
                dst_stride = hOutput->format.formatParams.yuvFormat[0].planeStride;
            }
            m_pfnRemoteRegisterBuf(hInput->pImageList[i].pAddr[0], hInput->planeSize[0], hInput->pImageList[i].fd[0]);
            m_pfnRemoteRegisterBuf(hOutput->pImageList[i].pAddr[0], hOutput->planeSize[0], hOutput->pImageList[i].fd[0]);

            status = m_pfnfcvRotateImageu8(hInput->pImageList[i].pAddr[0], src_width, src_height,
                                           src_stride, hOutput->pImageList[i].pAddr[0],
                                           dst_stride, (fcvRotateDegree)targetRotation);
            LOG_INFO(CamxLogGroupChi, "Y rotation status %d", status);

            /* Un-Register buffer after rotation */
            m_pfnRemoteRegisterBuf(hInput->pImageList[i].pAddr[0], hInput->planeSize[0], -1);
            m_pfnRemoteRegisterBuf(hOutput->pImageList[i].pAddr[0], hOutput->planeSize[0], -1);

            /* UV-Channel */

            if ((RotationAngle::Rotate90Degrees == targetRotation) ||
                (RotationAngle::Rotate270Degrees == targetRotation))
            {
                src_width = (hInput->format.formatParams.yuvFormat[1].width / 2);
                src_height = hInput->format.formatParams.yuvFormat[1].height;
                src_stride = hInput->format.formatParams.yuvFormat[1].planeStride;
                dst_stride = (2 * src_height);
            }
            else /*RotationAngle::Rotate180Degrees == targetRotation*/
            {
                src_width = (hInput->format.formatParams.yuvFormat[1].width >> 1);
                src_height = hInput->format.formatParams.yuvFormat[1].height;
                src_stride = hInput->format.formatParams.yuvFormat[1].planeStride;
                dst_stride = hOutput->format.formatParams.yuvFormat[1].planeStride;
            }

            /* Register buffer before rotation */
            m_pfnRemoteRegisterBuf(hInput->pImageList[i].pAddr[1], hInput->planeSize[1], hInput->pImageList[i].fd[0]);
            m_pfnRemoteRegisterBuf(hOutput->pImageList[i].pAddr[1], hOutput->planeSize[1], hOutput->pImageList[i].fd[0]);

            LOG_VERBOSE(CamxLogGroupChi, "UV: i/p dim: %dx%d", hInput->format.formatParams.yuvFormat[0].width, hInput->format.formatParams.yuvFormat[1].height);
            status = m_pfnfcvRotateImageInterleavedu8(hInput->pImageList[i].pAddr[1], src_width, src_height,
                                                      src_stride, hOutput->pImageList[i].pAddr[1],
                                                      dst_stride, (fcvRotateDegree)targetRotation);
            LOG_INFO(CamxLogGroupChi, "UV rotation status %d", status);

            /* Un-Register buffer after rotation */
            m_pfnRemoteRegisterBuf(hInput->pImageList[i].pAddr[1], hInput->planeSize[1], -1);
            m_pfnRemoteRegisterBuf(hOutput->pImageList[i].pAddr[1], hOutput->planeSize[1], -1);

        }
        else
        {
            LOG_INFO(CamxLogGroupChi, "Start Copy for 0 degree or rotation handle null");
            for (UINT j = 0; j < hOutput->numberOfPlanes; j++)
            {
                memcpy(hOutput->pImageList[i].pAddr[j], hInput->pImageList[i].pAddr[j], hInput->planeSize[j]);
            }
        }
    }
    LOG_INFO(CamxLogGroupChi, "%s X", __func__);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFCVNode::UpdateOrientation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFCVNode::UpdateOrientation(
    UINT64 requestId)
{
    VOID* pData = NULL;
    INT32 orientation = 0;
    pData = ChiNodeUtils::GetMetaData(requestId,
        (ANDROID_JPEG_ORIENTATION | InputMetadataSectionMask),
        ChiMetadataDynamic,
        &g_ChiNodeInterface,
        m_hChiSession);

    if (NULL != pData)
    {
        orientation = *static_cast<INT32 *>(pData);
    }

    // Convert Android's Jpeg Orientation metadata
    switch (orientation)
    {
    case 0:
        m_currentRotation = Rotate0Degrees;
        break;
    case 90:
        m_currentRotation = Rotate90Degrees;
        break;
    case 180:
        m_currentRotation = Rotate180Degrees;
        break;
    case 270:
        m_currentRotation = Rotate270Degrees;
        break;
    default:
        m_currentRotation = Rotate0Degrees;
        LOG_ERROR(CamxLogGroupChi, " Proper Orientation is not set so seeting to default %d",
                  m_currentRotation);
        break;
    }
    LOG_INFO(CamxLogGroupChi, "Orientation %d", m_currentRotation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFCVNode::UpdateMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFCVNode::UpdateMetaData(
    UINT64 requestId)
{
    CHIMETADATAINFO        metadataInfo = {0};
    const UINT32           tagSize      = sizeof(g_VendorTagSectionFCV) / sizeof(g_VendorTagSectionFCV[0]);
    CHITAGDATA             tagData[tagSize];
    UINT32                 tagList[tagSize];

    metadataInfo.size       = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession = m_hChiSession;
    metadataInfo.tagNum     = tagSize;
    metadataInfo.pTagList   = &tagList[0];
    metadataInfo.pTagData   = &tagData[0];

    UINT32 index = 0;

    UINT32  supportedFeature    = ChiNodeCapsFCV;
    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &supportedFeature;
    tagData[index].dataSize     = g_VendorTagSectionFCV[index].numUnits;
    index++;

    UINT32  currentMode         = m_nodeCaps;
    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &currentMode;
    tagData[index].dataSize     = g_VendorTagSectionFCV[index].numUnits;
    index++;

    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &m_processedFrame;
    tagData[index].dataSize     = g_VendorTagSectionFCV[index].numUnits;
    index++;

    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &m_fullOutputDimensions[0];
    tagData[index].dataSize     = g_VendorTagSectionFCV[index].numUnits;

    g_ChiNodeInterface.pSetMetadata(&metadataInfo);

#if PRINT_METADATA
    CHIVENDORTAGBASEINFO vendorTagBase  = {0};
    vendorTagBase.size                  = sizeof(CHIVENDORTAGBASEINFO);
    vendorTagBase.pComponentName        = FCVNodeSectionName;
    vendorTagBase.pTagName              = g_VendorTagSectionFCV[index].pVendorTagName;
    g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);
    LOG_VERBOSE(CamxLogGroupChi, "Vendor Tags value shall be same %x %x",
            g_VendorTagList[index] + g_vendorTagBase, vendorTagBase.vendorTagBase);

    // get the updated metadata from CamX and print them out
    VOID* pData = NULL;
    pData = ChiNodeUtils::GetMetaData(requestId, FCVNodeTagSupportedFeature + g_vendorTagBase, ChiMetadataDynamic,
        &g_ChiNodeInterface, m_hChiSession);
    if (NULL != pData)
    {
        LOG_VERBOSE(CamxLogGroupChi, "supported feature %d supported feature %d",
            *static_cast<UINT32*>(pData), supportedFeature);
    }

    pData = ChiNodeUtils::GetMetaData(requestId, FCVNodeTagCurrentMode + g_vendorTagBase, ChiMetadataDynamic,
        &g_ChiNodeInterface, m_hChiSession);
    if (NULL != pData)
    {
        LOG_VERBOSE(CamxLogGroupChi, "current mode %d current mode %d",
            *static_cast<UINT32*>(pData), currentMode);
    }

    pData = ChiNodeUtils::GetMetaData(requestId, FCVNodeTagProcessedFrameNum + g_vendorTagBase, ChiMetadataDynamic,
        &g_ChiNodeInterface, m_hChiSession);
    if (NULL != pData)
    {
        LOG_VERBOSE(CamxLogGroupChi, "processed frameNum %" PRIu64 " processed frame %" PRIu64,
            *static_cast<UINT64*>(pData), m_processedFrame);
    }

    pData = ChiNodeUtils::GetMetaData(requestId, FCVNodeTagFrameDimension + g_vendorTagBase, ChiMetadataDynamic,
        &g_ChiNodeInterface, m_hChiSession);
    if (NULL != pData)
    {
        UINT32*  local_dimension = (UINT32 *)pData;
        LOG_VERBOSE(CamxLogGroupChi, "frame dimension %d %d frame dimenstion %d %d",
            local_dimension[0], local_dimension[1], dimension[0], dimension[1]);
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

// CAMX_NAMESPACE_END
