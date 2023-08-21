////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodeswregistration.cpp
/// @brief Chi node for swregistration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <inttypes.h>
#include <system/camera_metadata.h>
#include <chiipedefs.h>
#include "camxchinodeswregistration.h"
#include "camxhal3metadatatags.h"
#include "camxhal3metadatatagtypes.h"

#undef LOG_TAG
#define LOG_TAG "CHISWREG"

const CHAR* pDefaultSwRegistraionibraryName = "libswregistrationalgo";

// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads
// NOWHINE FILE NC008: Warning: - Var names should be lower camel case

#define PRINT_METADATA FALSE

#define CHISWREGANCHORPORTID 10                                  // SWREG node Anchor input port
#define CHISWREGREFPORTID    11                                  // SWREG node Reference input port

ChiNodeInterface       g_ChiNodeInterface;                       ///< The instance save the CAMX Chi interface
UINT32                 g_vendorTagBase                    = 0;   ///< Chi assigned runtime vendor tag base for the node
const UINT32           g_swregistrationMinInputPorts      = 2;   ///< Minimum of Number of input ports
const UINT32           g_swregistrationNumOfMotionVectors = 9;   ///< Minimum of Number Motion Vectors

/// @todo (CAMX-1854) the major / minor version shall get from CHI
static const UINT32 ChiNodeMajorVersion = 0;    ///< The major version of CHI interface
static const UINT32 ChiNodeMinorVersion = 0;    ///< The minor version of CHI interface

static const UINT32 ChiNodeCapsSWRegistration       = 1;                    ///< execute sw reg algo

static const CHAR   SWRegistrationNodeSectionName[]         = "org.quic.camera2.ipeicaconfigs";   ///< section name for swreg

static const CHAR   ICACapabilitiesSectionName[] = "org.codeaurora.qcamera3.platformCapabilities"; ///< section name
                                                                                                  // for ICA Capabilities

///< This is an array of all vendor tag section data
static CHIVENDORTAGDATA g_VendorTagSectionSWRegistration[] =
{
    { "ICAInPerspectiveTransform",       0, sizeof(IPEICAPerspectiveTransform) },

};

///< This is an array of ICA Capabilties vendor tag section data
static CHIVENDORTAGDATA g_VendorTagSectionICACapabilities[] =
{
    { "IPEICACapabilities",              0, sizeof(CamX::IPEICACapability) },

};

static const CHAR   ICARefPerspectiveTag[] = "ICARefPerspectiveTransform";


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SWRegistrationNodeGetCaps
///
/// @brief  Implementation of PFNNODEGETCAPS defined in chinode.h
///
/// @param  pCapsInfo   Pointer to a structure that defines the capabilities supported by the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SWRegistrationNodeGetCaps(
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
            pCapsInfo->nodeCapsMask = ChiNodeCapsSWRegistration ;
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
/// SWRegistrationNodeCreate
///
/// @brief  Implementation of PFNNODECREATE defined in chinode.h
///
/// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SWRegistrationNodeCreate(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult       result  = CDKResultSuccess;
    ChiSWRegistrationNode*  pNode   = NULL;

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
        pNode = new ChiSWRegistrationNode;
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
/// SWRegistrationNodeDestroy
///
/// @brief  Implementation of PFNNODEDESTROY defined in chinode.h
///
/// @param  pDestroyInfo    Pointer to a structure that defines the session destroy information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SWRegistrationNodeDestroy(
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
            ChiSWRegistrationNode* pNode = static_cast<ChiSWRegistrationNode*>(pDestroyInfo->hNodeSession);
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
/// SWRegistrationNodeQueryBufferInfo
///
/// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
///
/// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SWRegistrationNodeQueryBufferInfo(
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
            ChiSWRegistrationNode* pNode = static_cast<ChiSWRegistrationNode*>(pQueryBufferInfo->hNodeSession);
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
/// SWRegistrationNodeProcRequest
///
/// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult SWRegistrationNodeProcRequest(
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
            ChiSWRegistrationNode* pNode = static_cast<ChiSWRegistrationNode*>(pProcessRequestInfo->hNodeSession);
            result                       = pNode->ProcessRequest(pProcessRequestInfo);
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
/// SWRegistrationNodeSetNodeInterface
///
/// @brief  Implementation of PFCHINODESETNODEINTERFACE defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID SWRegistrationNodeSetNodeInterface(
    ChiNodeInterface* pNodeInterface)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pNodeInterface)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pNodeInterface is NULL");
    }

    if (CDKResultSuccess == result)
    {
        if (pNodeInterface->size >= sizeof(ChiNodeInterface))
        {
            g_ChiNodeInterface.pProcessRequestDone  = pNodeInterface->pProcessRequestDone;
            g_ChiNodeInterface.pSetMetadata         = pNodeInterface->pSetMetadata;
            g_ChiNodeInterface.pGetVendorTagBase    = pNodeInterface->pGetVendorTagBase;
            g_ChiNodeInterface.pGetMetadata         = pNodeInterface->pGetMetadata;

            // get the vendor tag base
            CHIVENDORTAGBASEINFO vendorTagBase = { 0 };
            vendorTagBase.size                 = sizeof(CHIVENDORTAGBASEINFO);
            vendorTagBase.pComponentName       = SWRegistrationNodeSectionName;

            result = g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);
            if (CDKResultSuccess == result)
            {
                g_vendorTagBase = vendorTagBase.vendorTagBase;
            }

        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEPROCESSREQUESTINFO is smaller than expected");
            result = CDKResultEFailed;
        }
    }
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
            pNodeCallbacks->pGetCapabilities         = SWRegistrationNodeGetCaps;
            pNodeCallbacks->pCreate                  = SWRegistrationNodeCreate;
            pNodeCallbacks->pDestroy                 = SWRegistrationNodeDestroy;
            pNodeCallbacks->pQueryBufferInfo         = SWRegistrationNodeQueryBufferInfo;
            pNodeCallbacks->pProcessRequest          = SWRegistrationNodeProcRequest;
            pNodeCallbacks->pChiNodeSetNodeInterface = SWRegistrationNodeSetNodeInterface;
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
/// ChiSWRegistrationNode::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSWRegistrationNode::Initialize(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult   result          = CDKResultSuccess;
    VOID*       pAddr           = NULL;
    INT         numCharWritten  = 0;
    CHAR        libFilename[FILENAME_MAX];

    // CAMX_ASSERT(NULL != pCreateInfo->hChiSession);
    m_hChiSession    = pCreateInfo->hChiSession;
    m_nodeId         = pCreateInfo->nodeId;
    m_nodeCaps       = pCreateInfo->nodeCaps.nodeCapsMask;
    m_processedFrame = 0;

    numCharWritten = ChiNodeUtils::SNPrintF(libFilename, FILENAME_MAX, "%s%s%s.%s",
        VendorLibPath, PathSeparator, pDefaultSwRegistraionibraryName, SharedLibraryExtension);

    m_handle = ChiNodeUtils::LibMapFullName(libFilename);

    if (NULL != m_handle)
    {
        pAddr = ChiNodeUtils::LibGetAddr(m_handle, "register_mf");
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "FATAL: Cannot open swreg module library");
    }

    if (NULL == pAddr)
    {
        result = CDKResultEUnableToLoad;
        LOG_ERROR(CamxLogGroupChi, "Unable to get swreg Entry point!");
    }
    else
    {
        m_pSWRegistration = reinterpret_cast<SWRegistrationalgorithm>(pAddr);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSWRegistrationNode::QueryBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSWRegistrationNode::QueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CAMX_UNREFERENCED_PARAM(pQueryBufferInfo);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSWRegistrationNode::SetInputBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiSWRegistrationNode::SetInputBufferInfo(
    CHINODEBUFFERHANDLE hInput)
{
    m_format.width = hInput->format.width;
    m_format.height = hInput->format.height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSWRegistrationNode::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiSWRegistrationNode::ProcessRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult result = CDKResultSuccess;
    CHINODEBUFFERHANDLE     hpInputHandle[g_swregistrationMinInputPorts] = {0};    ///< To store Pointers of input buffers
    FLOAT motionvectors[g_swregistrationNumOfMotionVectors] = { 0 };
    UINT  algoconfidence                                    = 0;

    // CAMX_ASSERT(g_swregistrationMinInputPorts == pProcessRequestInfo->inputNum);
    for (UINT32 i = 0; i < pProcessRequestInfo->inputNum; i++)
    {
        if (CHISWREGANCHORPORTID == (pProcessRequestInfo->phInputBuffer[i]->portId))
        {
            hpInputHandle[0] = pProcessRequestInfo->phInputBuffer[i];
            SetInputBufferInfo(hpInputHandle[0]);
        }
        else if (CHISWREGREFPORTID == (pProcessRequestInfo->phInputBuffer[i]->portId))
        {
            hpInputHandle[1] = pProcessRequestInfo->phInputBuffer[i];
            SetInputBufferInfo(hpInputHandle[1]);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "Invalid SWREG in port: %d",
                    pProcessRequestInfo->phInputBuffer[i]->portId);
            result = CDKResultEFailed;
        }
    }

    // Call the SWRegistration algo
    // need to find proper input params
    // call swregistration algo with 2 Images [Anchor and Ref]
    if ((NULL != hpInputHandle[0]) && (NULL != hpInputHandle[1]))
    {
        RegImage anchorImg = {
            hpInputHandle[0]->pImageList[0].pAddr[0],
            hpInputHandle[0]->format.width,
            hpInputHandle[0]->format.height,
            hpInputHandle[0]->format.formatParams.yuvFormat[0].planeStride,
            8,
            0
        };
        RegImage refImg = {
            hpInputHandle[1]->pImageList[0].pAddr[0],
            hpInputHandle[1]->format.width,
            hpInputHandle[1]->format.height,
            hpInputHandle[1]->format.formatParams.yuvFormat[0].planeStride,
            8,
            0
        };
        RegPostprocessing postProc = { 0, 0, m_format.width / 2, m_format.height / 2, 1, 1 };

        LOG_VERBOSE(CamxLogGroupChi, "Port %d in[0] stride %d, scanline %d Port %d in[1]: stride %d scanline %d",
                    hpInputHandle[0]->portId,
                    hpInputHandle[0]->format.formatParams.yuvFormat[0].planeStride,
                    hpInputHandle[0]->format.formatParams.yuvFormat[0].sliceHeight,
                    hpInputHandle[1]->portId,
                    hpInputHandle[1]->format.formatParams.yuvFormat[0].planeStride,
                    hpInputHandle[1]->format.formatParams.yuvFormat[0].sliceHeight);

        result = (*m_pSWRegistration)(&anchorImg, &refImg, &postProc, motionvectors, &algoconfidence);
    }

    if (CDKResultSuccess == result)
    {
        LOG_VERBOSE(CamxLogGroupChi, "sw registration motion vectors %f,%f,%f,%f,%f,%f,%f,%f,%f,and algoconfidence %d",
            motionvectors[0], motionvectors[1], motionvectors[2], motionvectors[3],
            motionvectors[4], motionvectors[5], motionvectors[6], motionvectors[7],
            motionvectors[8], algoconfidence);
        LOG_VERBOSE(CamxLogGroupChi, "sw registration algo executed successfully");
        UpdateMetaData(pProcessRequestInfo->frameNum, motionvectors, algoconfidence);
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "error in sw registration algo execution");
    }

    m_processedFrame++;

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSWRegistrationNode::ChiSWRegistrationNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiSWRegistrationNode::ChiSWRegistrationNode()
    : m_hChiSession(NULL)
    , m_nodeId(0)
    , m_nodeCaps(0)
    , m_processedFrame(0)
{
    memset(&m_format, 0, sizeof(CHINODEIMAGEFORMAT));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSWRegistrationNode::~ChiSWRegistrationNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiSWRegistrationNode::~ChiSWRegistrationNode()
{
    m_hChiSession = NULL;
    if (NULL != m_handle)
    {
        ChiNodeUtils::LibUnmap(m_handle);
        m_handle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSWRegistrationNode::GetICATransformType
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiSWRegistrationNode::GetICATransformType()
{
    UINT32           index                = 0;
    UINT32           transformType        = 0;
    CDKResult        result               = CDKResultSuccess;
    CHIMETADATAINFO  metadataInfo         = { 0 };
    const UINT32     tagSize              = sizeof(g_VendorTagSectionICACapabilities) /
                                                sizeof(g_VendorTagSectionICACapabilities[0]);
    CHITAGDATA       tagData[tagSize]     = { {0} };
    UINT32           tagList[tagSize]     = { 0 };

    metadataInfo.size                     = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession               = m_hChiSession;
    metadataInfo.tagNum                   = tagSize;
    metadataInfo.metadataType             = ChiMetadataStatic;
    metadataInfo.pTagList                 = &tagList[0];
    metadataInfo.pTagData                 = &tagData[0];

    // get the vendor tag offset
    CHIVENDORTAGBASEINFO vendorTagBase    = { 0 };
    vendorTagBase.size                    = sizeof(CHIVENDORTAGBASEINFO);
    vendorTagBase.pComponentName          = ICACapabilitiesSectionName;
    vendorTagBase.pTagName                = g_VendorTagSectionICACapabilities[index].pVendorTagName;
    result                                = g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);

    tagList[index]                        = vendorTagBase.vendorTagBase;
    tagData[index].size                   = sizeof(CHITAGDATA);
    tagData[index].pData                  = NULL;
    tagData[index].dataSize               = 0;
    result                                = g_ChiNodeInterface.pGetMetadata(&metadataInfo);

    memcpy(&transformType, tagData[index].pData, sizeof(transformType));

    return transformType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSWRegistrationNode::UpdateMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiSWRegistrationNode::UpdateMetaData(
    UINT64 requestId,
    FLOAT motionvectors[],
    UINT algoconfidence)
{
    CDKResult       result           = CDKResultSuccess;
    CHIMETADATAINFO metadataInfo     = { 0 };
    const UINT32    tagSize          = sizeof(g_VendorTagSectionSWRegistration) /sizeof(g_VendorTagSectionSWRegistration[0]);
    CHITAGDATA      tagData[tagSize] = { {0} };
    UINT32          tagList[tagSize];

    metadataInfo.size       = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession = m_hChiSession;
    metadataInfo.tagNum     = tagSize;
    metadataInfo.pTagList   = &tagList[0];
    metadataInfo.pTagData   = &tagData[0];

    UINT32 index = 0;
    // get the vendor tag offset
    CHIVENDORTAGBASEINFO vendorTagBase = { 0 };
    vendorTagBase.size                 = sizeof(CHIVENDORTAGBASEINFO);
    vendorTagBase.pComponentName       = SWRegistrationNodeSectionName;
    vendorTagBase.pTagName             = g_VendorTagSectionSWRegistration[index].pVendorTagName;

    if(static_cast<UINT32>(CamX::IPEICATransformType::ReferenceTransform) ==  GetICATransformType())
    {
        vendorTagBase.pTagName = ICARefPerspectiveTag;
    }

    result                             = g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);
    // CAMX_ASSERT(CDKResultSuccess == result);

    IPEICAPerspectiveTransform  perspectiveTransform;
    perspectiveTransform.perspectiveTransformEnable       = 1;
    perspectiveTransform.ReusePerspectiveTransform        = 0;
    perspectiveTransform.perspetiveGeometryNumColumns     = 1;
    perspectiveTransform.perspectiveGeometryNumRows       = 1;
    perspectiveTransform.perspectiveConfidence            = algoconfidence;
    perspectiveTransform.byPassAlignmentMatrixAdjustement = FALSE;
    perspectiveTransform.transformDefinedOnWidth          = m_format.width;
    perspectiveTransform.transformDefinedOnHeight         = m_format.height;
    memcpy(&perspectiveTransform.perspectiveTransformArray,
        motionvectors,
        (sizeof(FLOAT)*g_swregistrationNumOfMotionVectors));

    tagList[index]           = vendorTagBase.vendorTagBase;
    tagData[index].size      = sizeof(CHITAGDATA);
    tagData[index].requestId = requestId;
    tagData[index].pData     = &perspectiveTransform;
    tagData[index].dataSize  = g_VendorTagSectionSWRegistration[index].numUnits;

    g_ChiNodeInterface.pSetMetadata(&metadataInfo);

}
