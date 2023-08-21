////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodedummysat.cpp
/// @brief Chi node for dummysat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <inttypes.h>
#include <system/camera_metadata.h>

#include "camxchinodedummysat.h"
#include "chiifedefs.h"
#include "chiipedefs.h"

// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads
// NOWHINE FILE NC008: Warning: - Var names should be lower camel case

#undef LOG_TAG
#define LOG_TAG "CHIDUMMYSAT"

#define PRINT_METADATA FALSE

// For test and backward compatibility
#define ENABLE_DYNAMIC_MASTER_SWITCH TRUE

ChiNodeInterface g_ChiNodeInterface;    ///< The instance save the CAMX Chi interface
UINT32           g_vendorTagBase = 0;   ///< Chi assigned runtime vendor tag base for the node

/// @todo (CAMX-1854) the major / minor version shall get from CHI
static const UINT32 ChiNodeMajorVersion = 0;    ///< The major version of CHI interface
static const UINT32 ChiNodeMinorVersion = 0;    ///< The minor version of CHI interface

static const UINT32 ChiNodeCapsDummySAT       = 1;                    ///< Simply copy the image
static const UINT32 ChiNodeCapsHalfScale    = 1 << 1;               ///< 1/2 downsize
static const UINT32 ChiNodeCapsQuarterScale = 1 << 2;               ///< 1/4 downsize

static const CHAR   DummySATNodeSectionName[]         = "com.qti.node.dummysat";   ///< The section name for node

static const UINT32 DummySATNodeTagBase               = 0;                          ///< Tag base
static const UINT32 DummySATNodeTagSupportedFeature   = DummySATNodeTagBase + 0;    ///< Tag for supported features
static const UINT32 DummySATNodeTagCurrentMode        = DummySATNodeTagBase + 1;    ///< Tag to indicated current operation mode
static const UINT32 DummySATNodeTagProcessedFrameNum  = DummySATNodeTagBase + 2;    ///< Tag to show processed frame's count
static const UINT32 DummySATNodeTagFrameDimension     = DummySATNodeTagBase + 3;    ///< Tag to show current's frame dimension
static const UINT32 RoleSwitchWaitFrameCount          = 7;                          ///< Number of frame to wait for role switch case

static const FLOAT perspArray[81] =
{
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

///< Supported vendor tag list, it shall align with the definition in g_VendorTagSectionDummySAT
static const UINT32 g_VendorTagList[] =
{
    DummySATNodeTagSupportedFeature,
    DummySATNodeTagCurrentMode,
    DummySATNodeTagProcessedFrameNum,
    DummySATNodeTagFrameDimension
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGDATA g_VendorTagSectionDummySAT[] =
{
    { "SupportedFeature",       TYPE_INT32, 1 },
    { "CurrentMode",            TYPE_BYTE,  1 },
    { "ProcessedFrameNumber",   TYPE_INT64, 1 },
    { "FrameDimension",         TYPE_INT32, 2 },
};

///< This is an array of all vendor tag section data
static CHIVENDORTAGSECTIONDATA g_VendorTagDummySATSection[] =
{
    {
        DummySATNodeSectionName,  0,
        sizeof(g_VendorTagSectionDummySAT) / sizeof(g_VendorTagSectionDummySAT[0]), g_VendorTagSectionDummySAT,
        CHITAGSECTIONVISIBILITY::ChiTagSectionVisibleToAll
    }
};

///< This is an array of all vendor tag section data
static ChiVendorTagInfo g_VendorTagInfoDummySAT[] =
{
    {
        &g_VendorTagDummySATSection[0],
        sizeof(g_VendorTagDummySATSection) / sizeof(g_VendorTagDummySATSection[0])
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DummySATNodeGetCaps
///
/// @brief  Implementation of PFNNODEGETCAPS defined in chinode.h
///
/// @param  pCapsInfo   Pointer to a structure that defines the capabilities supported by the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DummySATNodeGetCaps(
    CHINODECAPSINFO* pCapsInfo)
{
    CDKResult result = CDKResultSuccess;

    if (pCapsInfo == NULL)
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument: pCapsInfo is NULL");
        return result;
    }

    // dereference pCapsInfo only when it is not NULL
    if (CDKResultSuccess == result)
    {
        if (pCapsInfo->size >= sizeof(CHINODECAPSINFO))
        {
            pCapsInfo->nodeCapsMask = ChiNodeCapsDummySAT | ChiNodeCapsHalfScale | ChiNodeCapsQuarterScale;
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
/// DummySATNodeQueryVendorTag
///
/// @brief  Implementation of PFNCHIQUERYVENDORTAG defined in chinode.h
///
/// @param  pQueryVendorTag Pointer to a structure that returns the exported vendor tag
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DummySATNodeQueryVendorTag(
    CHIQUERYVENDORTAG* pQueryVendorTag)
{
    CDKResult result = CDKResultSuccess;
    result = ChiNodeUtils::QueryVendorTag(pQueryVendorTag, g_VendorTagInfoDummySAT);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DummySATNodeCreate
///
/// @brief  Implementation of PFNNODECREATE defined in chinode.h
///
/// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DummySATNodeCreate(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult       result  = CDKResultSuccess;
    ChiDummySATNode*  pNode   = NULL;

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
        pNode = new ChiDummySATNode;
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
/// DummySATNodeDestroy
///
/// @brief  Implementation of PFNNODEDESTROY defined in chinode.h
///
/// @param  pDestroyInfo    Pointer to a structure that defines the session destroy information for the node.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DummySATNodeDestroy(
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
            ChiDummySATNode* pNode = static_cast<ChiDummySATNode*>(pDestroyInfo->hNodeSession);
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
/// DummySATNodeQueryBufferInfo
///
/// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
///
/// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DummySATNodeQueryBufferInfo(
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
            ChiDummySATNode* pNode = static_cast<ChiDummySATNode*>(pQueryBufferInfo->hNodeSession);
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
/// DummySATNodeQueryMetadataPublishList
///
/// @brief  Implementation of PFNNODEQUERYMETADATAPUBLISHLIST defined in chinode.h
///
/// @param  pMetadataPublishlist    Pointer to a structure to query the metadata list
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DummySATNodeQueryMetadataPublishList(
    CHINODEMETADATALIST* pMetadataPublishlist)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pMetadataPublishlist) || (NULL == pMetadataPublishlist->hNodeSession))
    {
        result = CDKResultEInvalidPointer;
        LOG_ERROR(CamxLogGroupChi, "Invalid argument");
    }

    if (CDKResultSuccess == result)
    {
        if (pMetadataPublishlist->size == sizeof(CHINODEMETADATALIST))
        {
            ChiDummySATNode* pNode = static_cast<ChiDummySATNode*>(pMetadataPublishlist->hNodeSession);
            result                 = pNode->QueryMetadataPublishList(pMetadataPublishlist);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODEMETADATALIST is smaller than expected");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DummySATNodeSetBufferInfo
///
/// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
///
/// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DummySATNodeSetBufferInfo(
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
            ChiDummySATNode* pNode = static_cast<ChiDummySATNode*>(pSetBufferInfo->hNodeSession);
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
/// DummySATNodeProcRequest
///
/// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DummySATNodeProcRequest(
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
            ChiDummySATNode* pNode = static_cast<ChiDummySATNode*>(pProcessRequestInfo->hNodeSession);
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
/// DummySATNodeSetNodeInterface
///
/// @brief  Implementation of PFCHINODESETNODEINTERFACE defined in chinode.h
///
/// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID DummySATNodeSetNodeInterface(
    ChiNodeInterface* pNodeInterface)
{
    CDKResult result = CDKResultSuccess;
    result = ChiNodeUtils::SetNodeInterface(pNodeInterface, DummySATNodeSectionName,
        &g_ChiNodeInterface, &g_vendorTagBase);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DummySATNodeGetFlushResponseTime
///
/// @brief  Implementation of PFNNODEFLUSHRESPONSEINFO defined in chinode.h
///
/// @param  pInfo Pointer to a structure that defines the information required for processing a request.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult DummySATNodeGetFlushResponseTime(
    CHINODERESPONSEINFO* pInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pInfo) || (NULL == pInfo->hChiSession))
    {
        result = CDKResultEInvalidPointer;
        if (NULL == pInfo)
        {
            LOG_ERROR(CamxLogGroupChi, "pInfo is NULL");
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "pInfo %p pInfo->hChiSession is NULL", pInfo);
        }
    }
    else
    {
        LOG_VERBOSE(CamxLogGroupChi, "pInfo %p pInfo->hChiSession %p", pInfo, pInfo->hChiSession);
    }

    if (CDKResultSuccess == result)
    {
        if (pInfo->size == sizeof(CHINODERESPONSEINFO))
        {
            ChiDummySATNode* pNode = static_cast<ChiDummySATNode*>(pInfo->hChiSession);
            result = pNode->ComputeResponseTime(pInfo);
        }
        else
        {
            LOG_ERROR(CamxLogGroupChi, "CHINODERESPONSEINFO %u is smaller than expected %u", pInfo->size,
                static_cast<UINT>(sizeof(CHINODERESPONSEINFO)));
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
            pNodeCallbacks->majorVersion              = ChiNodeMajorVersion;
            pNodeCallbacks->minorVersion              = ChiNodeMinorVersion;
            pNodeCallbacks->pGetCapabilities          = DummySATNodeGetCaps;
            pNodeCallbacks->pQueryVendorTag           = DummySATNodeQueryVendorTag;
            pNodeCallbacks->pCreate                   = DummySATNodeCreate;
            pNodeCallbacks->pDestroy                  = DummySATNodeDestroy;
            pNodeCallbacks->pQueryBufferInfo          = DummySATNodeQueryBufferInfo;
            pNodeCallbacks->pSetBufferInfo            = DummySATNodeSetBufferInfo;
            pNodeCallbacks->pProcessRequest           = DummySATNodeProcRequest;
            pNodeCallbacks->pChiNodeSetNodeInterface  = DummySATNodeSetNodeInterface;
            pNodeCallbacks->pGetFlushResponse         = DummySATNodeGetFlushResponseTime;

            pNodeCallbacks->pQueryMetadataPublishList = DummySATNodeQueryMetadataPublishList;
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
/// ChiDummySATNode::ConfigureInputBufferMapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiDummySATNode::ConfigureInputBufferMapping()
{
    CDKResult                result             = CDKResultSuccess;
    ChiPhysicalCameraConfig* pCameraInputConfig = NULL;

    //Read the Input Port Mapping vendor tag
    CHIVENDORTAGBASEINFO vendorTagBase;
    vendorTagBase.size           = sizeof(CHIVENDORTAGBASEINFO);
    vendorTagBase.pComponentName = "com.qti.chi.cameraconfiguration";
    vendorTagBase.pTagName       = "PhysicalCameraInputConfig";

    g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);

    void* pData = ChiNodeUtils::GetMetaData(0, vendorTagBase.vendorTagBase | UsecaseMetadataSectionMask,
                                      ChiMetadataDynamic, &g_ChiNodeInterface, m_hChiSession);
    pCameraInputConfig = static_cast<ChiPhysicalCameraConfig*>(pData);

    if (NULL != pData)
    {
        pCameraInputConfig = static_cast<ChiPhysicalCameraConfig*>(pData);

        for (UINT32 cfgIdx = 0; cfgIdx < pCameraInputConfig->numConfigurations; cfgIdx++)
        {
            CHILINKNODEDESCRIPTOR *pNodeDescriptor = &pCameraInputConfig->configuration[cfgIdx].nodeDescriptor;

            if ((pNodeDescriptor->nodeId == m_nodeId) && (pNodeDescriptor->nodeInstanceId == m_nodeInstanceId))
            {
                UINT   cameraId = pCameraInputConfig->configuration[cfgIdx].physicalCameraId;
                UINT32 camIdx   = GetCameraIndexFromID(cameraId);
                if (INVALID_INDEX == camIdx)
                {
                    LOG_ERROR(CamxLogGroupChi, "No Camera Index found for camera id: %u", cameraId);
                    result = CDKResultEFailed;
                }
                else
                {
                    m_cameraInfo[camIdx].inputPortId = pNodeDescriptor->nodePortId;

                    LOG_VERBOSE(CamxLogGroupChi, "Camera id %d, Full Input buffer Idx %d",
                                pCameraInputConfig->configuration[cfgIdx].physicalCameraId,
                                m_cameraInfo[camIdx].inputPortId);

                }
            }
        }
    }
    else
    {
        LOG_ERROR(CamxLogGroupChi, "PhysicalCameraInputConfig metadata is NULL");
        result = CDKResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::ConfigureCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiDummySATNode::ConfigureCameraInfo()
{
    CameraConfigs*           pPhysicalCameraConfigs = NULL;
    CDKResult                result                 = CDKResultSuccess;

    /// Read the camera config vendor tag
    CHIVENDORTAGBASEINFO vendorTagBase;
    vendorTagBase.size = sizeof(CHIVENDORTAGBASEINFO);

    vendorTagBase.pComponentName = "com.qti.chi.cameraconfiguration";
    vendorTagBase.pTagName       = "PhysicalCameraConfigs";
    g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);

    void* pData = ChiNodeUtils::GetMetaData(0, vendorTagBase.vendorTagBase | UsecaseMetadataSectionMask,
                                            ChiMetadataDynamic, &g_ChiNodeInterface, m_hChiSession);
    if (NULL != pData)
    {
        pPhysicalCameraConfigs = static_cast<CameraConfigs*>(pData);
    }
    else
    {
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        m_numOfLinkedCameras = pPhysicalCameraConfigs->numPhysicalCameras;
        m_primaryCameraId    = pPhysicalCameraConfigs->primaryCameraId;

        if (MaxLinkedCameras < m_numOfLinkedCameras)
        {
            LOG_ERROR(CamxLogGroupChi, "Number of linked cameras %d, exceeds max supported linked cameras %d ",
                      m_numOfLinkedCameras, MaxLinkedCameras);
            result = CDKResultEFailed;
        }
        else
        {
            LOG_INFO(CamxLogGroupChi, "Number of linked cameras %d, primary camera id %d ",
                     m_numOfLinkedCameras, m_primaryCameraId);
            for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
            {
                m_cameraInfo[i].pCameraConfig = &pPhysicalCameraConfigs->cameraConfigs[i];

                if (m_primaryCameraId == m_cameraInfo[i].pCameraConfig->cameraId)
                {
                    m_primaryCameraIndex = i;
                }
            }

            result = ConfigureInputBufferMapping();
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiDummySATNode::Initialize(
    CHINODECREATEINFO* pCreateInfo)
{
    CDKResult            result         = CDKResultSuccess;
    UINT*                pSleepTime     = NULL;
    CHIVENDORTAGBASEINFO vendorTagBase  = { 0 };

    /// @todo (CAMX-1854) Check for Node Capabilities using NodeCapsMask
    m_hChiSession           = pCreateInfo->hChiSession;
    m_nodeId                = pCreateInfo->nodeId;
    m_nodeCaps              = pCreateInfo->nodeCaps.nodeCapsMask;
    m_nodeInstanceId        = pCreateInfo->nodeInstanceId;
    m_nodeFlags             = pCreateInfo->nodeFlags;
    m_processedFrame        = 0;
    m_isSnapshotFusion      = 0;
    m_masterInputportId     = 0;
    m_masterFromInputForPSM = -1;

    for (UINT i = 0; i < pCreateInfo->nodePropertyCount; i++)
    {
        if (NodePropertyProcessingType == pCreateInfo->pNodeProperties[i].id)
        {
            const CHAR* pValue  = reinterpret_cast<const CHAR*>(pCreateInfo->pNodeProperties[i].pValue);
            m_isSnapshotFusion  = static_cast<UINT>(atoi(pValue));
            break;
        }
    }

    m_LPMEnable                         = TRUE;
    m_inStageFrames                     = 0;
    m_lpmMode                           = WIDEONLY;
    m_recommendedLPMMode                = WIDEONLY;
    vendorTagBase.size                  = sizeof(CHIVENDORTAGBASEINFO);
    vendorTagBase.pComponentName        = "org.quic.camera.induceSleepInChiNode";
    vendorTagBase.pTagName              = "InduceSleep";
    g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);

    void* pData                         = ChiNodeUtils::GetMetaData(0,
                                                                    vendorTagBase.vendorTagBase | UsecaseMetadataSectionMask,
                                                                    ChiMetadataDynamic,
                                                                    &g_ChiNodeInterface,
                                                                    m_hChiSession);

    if (NULL != pData)
    {
        pSleepTime                      = static_cast<UINT*>(pData);
        m_inducedSleepTime              = *pSleepTime;
    }

    result = ConfigureCameraInfo();
    if (CDKResultSuccess == result)
    {
        /// Initialize result data with default values
        m_resultDataOZ.numLinkedCameras = m_numOfLinkedCameras;
        m_resultDataOZ.masterCameraId = m_cameraInfo[m_primaryCameraIndex].pCameraConfig->cameraId;
        m_resultDataOZ.recommendedMasterCameraId = m_resultDataOZ.masterCameraId;

        for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
        {
            m_resultDataOZ.lowPowerMode[i].cameraId = m_cameraInfo[i].pCameraConfig->cameraId;
            m_resultDataOZ.lowPowerMode[i].isEnabled = TRUE;
            // For dual and triple camera case
            if (m_numOfLinkedCameras >=2)
            {
                if (i == m_numOfLinkedCameras-1)
                {
                    m_camIdTele = m_cameraInfo[i].pCameraConfig->cameraId;
                }
                if (i == m_numOfLinkedCameras-2)
                {
                    m_camIdWide= m_cameraInfo[i].pCameraConfig->cameraId;
                }
            }
        }
        LOG_VERBOSE(CamxLogGroupChi, "wide id %d and tele id %d", m_camIdWide, m_camIdTele);

        if (TRUE == m_LPMEnable)
        {
            // Enable only the primary camera at start
            m_resultDataOZ.lowPowerMode[0].isEnabled = FALSE;
        }
        else
        {
            if (MAX_CONCURRENT_CAMERAS >= m_numOfLinkedCameras)
            {
                for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
                {
                    m_resultDataOZ.lowPowerMode[i].isEnabled = FALSE;
                }
            }
            else
            {
                LOG_ERROR(CamxLogGroupChi, "Fatal! Cannot enable %d cameras at a time. Enable LPM mode",
                          m_numOfLinkedCameras);
            }
        }
        m_preRoleSwitchWaitFrames = RoleSwitchWaitFrameCount;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::QueryBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiDummySATNode::QueryBufferInfo(
    CHINODEQUERYBUFFERINFO* pQueryBufferInfo)
{
    CDKResult result                     = CDKResultSuccess;

    // set the nodeCaps later
    switch (m_nodeCaps)
    {
        case ChiNodeCapsHalfScale:
        case ChiNodeCapsQuarterScale:
            result = CDKResultEUnsupported;
            break;
        case ChiNodeCapsDummySAT:
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
/// ChiDummySATNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiDummySATNode::QueryMetadataPublishList(
    CHINODEMETADATALIST* pMetadataPublishlist)
{
    UINT count = 0;

    if (FALSE == m_isSnapshotFusion)
    {
        UINT outputMetadataVendorTagBase = ChiNodeUtils::GetVendorTagBase("com.qti.chi.multicameraoutputmetadata",
                                                                          "OutputMetadataOpticalZoom",
                                                                          &g_ChiNodeInterface);

        LOG_INFO(CamxLogGroupChi, "output metadata Vendor Tag published by SAT node %x", outputMetadataVendorTagBase);
        pMetadataPublishlist->tagArray[count] = outputMetadataVendorTagBase;
        pMetadataPublishlist->tagCount        = ++count;
        pMetadataPublishlist->partialTagCount = 0;
    }

    UINT ICAInPerspectiveTransformTagBase = ChiNodeUtils::GetVendorTagBase("org.quic.camera2.ipeicaconfigs",
                                                                         "ICAInPerspectiveTransform",
                                                                         &g_ChiNodeInterface);

    LOG_INFO(CamxLogGroupChi, "ICA Vendor Tag published by SAT node %x", ICAInPerspectiveTransformTagBase);
    pMetadataPublishlist->tagArray[count] = ICAInPerspectiveTransformTagBase;
    pMetadataPublishlist->tagCount        = ++count;
    pMetadataPublishlist->partialTagCount = 0;


    UINT Satstreamcrop = ChiNodeUtils::GetVendorTagBase("com.qti.camera.streamCropInfo", "StreamCropInfo", &g_ChiNodeInterface);
    LOG_INFO(CamxLogGroupChi, "streamcrop Vendor Tag published by SAT node %x", Satstreamcrop);
    pMetadataPublishlist->tagArray[count] = Satstreamcrop;
    pMetadataPublishlist->tagCount        = ++count;
    pMetadataPublishlist->partialTagCount = 0;


    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::SetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiDummySATNode::SetBufferInfo(
    CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo)
{
    m_format.width  = pSetBufferInfo->pFormat->width;
    m_format.height = pSetBufferInfo->pFormat->height;

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiDummySATNode::ProcessRequest(
    CHINODEPROCESSREQUESTINFO* pProcessRequestInfo)
{
    CDKResult result                = CDKResultSuccess;
    UINT32    masterSrcPortIndex    = 0;
    UINT8     SATMode               = TRUE;

    m_resultDataOZ.outputShiftPreview.horizonalShift  = 0;
    m_resultDataOZ.outputShiftPreview.verticalShift   = 0;
    m_resultDataOZ.outputShiftSnapshot.horizonalShift = 0;
    m_resultDataOZ.outputShiftSnapshot.verticalShift  = 0;
    m_resultDataOZ.refResForOutputShift.width         = 1920;
    m_resultDataOZ.refResForOutputShift.height        = 1080;
    m_resultDataOZ.outputCrop.top                     = 0;
    m_resultDataOZ.outputCrop.left                    = 0;
    m_resultDataOZ.outputCrop.width                   = 1920;
    m_resultDataOZ.outputCrop.height                  = 1080;

    CHIVENDORTAGBASEINFO vendorTagBase = { 0 };
    vendorTagBase.size                 = sizeof(CHIVENDORTAGBASEINFO);
    vendorTagBase.pComponentName       = "com.qti.chi.multicamerainputmetadata";
    vendorTagBase.pTagName             = "InputMetadataOpticalZoom";
    g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);

    void* pData = ChiNodeUtils::GetMetaData(pProcessRequestInfo->frameNum,
        vendorTagBase.vendorTagBase | InputMetadataSectionMask, ChiMetadataDynamic, &g_ChiNodeInterface, m_hChiSession);

    InputMetadataOpticalZoom* pInputMetaOZ = static_cast<InputMetadataOpticalZoom*>(pData);

    if (NULL == pInputMetaOZ)
    {
        LOG_ERROR(CamxLogGroupChi, "pInputMetaOZ is null");
        result = CDKResultEInvalidPointer;
    }
    else if (0 == pInputMetaOZ->numInputs)
    {
        LOG_ERROR(CamxLogGroupChi, "Invalid number of inputs");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        // This is only for testing the scanario where chinode takes time. The time can be
        // induced by overridesettings. Time can be induced only for offline Pipeline/snapshot request.
        if ((0 != m_inducedSleepTime) && (TRUE == pInputMetaOZ->isSnapshot))
        {
            LOG_INFO(CamxLogGroupChi, "Induced extra sleep in ms %u", m_inducedSleepTime);
            ChiNodeUtils::SleepMicroseconds(m_inducedSleepTime * 1000);
        }

        for (UINT32 i = 0; i < pInputMetaOZ->numInputs; i++)
        {
            LOG_VERBOSE(CamxLogGroupChi, "SAT Num of Input %d, Camera Id %d: leftxtop %d x %d,"
                        "lpm mode - (%d - %d)",
                        pInputMetaOZ->numInputs, pInputMetaOZ->cameraMetadata[i].cameraId,
                        pInputMetaOZ->cameraMetadata[i].userCropRegion.left,
                        pInputMetaOZ->cameraMetadata[i].userCropRegion.top,
                        m_recommendedLPMMode, m_lpmMode);
        }

        // For four cameras , smoothzoom is disabled as of now therefore which camera needs to be
        // in LPM wiil be decided in multicamcontoller.
        LPMMode        localLPMMode     = m_lpmMode;
        UINT32         currentMasterId  = pInputMetaOZ->cameraMetadata[0].masterCameraId;
        UINT32         currentMasterIdx = GetCameraIndexFromID(currentMasterId);

        if (0 != m_isSnapshotFusion ||
            FALSE == m_cameraInfo[currentMasterIdx].pCameraConfig->enableSmoothTransition)
        {
            /// Fusion Snapshot mode
            SATMode = FALSE;
        }

        //Decide LPM
        if (TRUE == SATMode)
        {
            if (TRUE == m_LPMEnable)
            {
                //Todo: Update with FOV ratio.This is temp check
                if ((WIDEONLY == m_lpmMode) &&
                    (((m_numOfLinkedCameras == 2) &&
                    (pInputMetaOZ->cameraMetadata[0].userCropRegion.left > DUAL_TRANSITION_LOW)) ||
                    ((m_numOfLinkedCameras == 3) &&
                     (pInputMetaOZ->cameraMetadata[0].userCropRegion.left > TRIPLE_TRANSITION_LOW))))
                {
                    // Switch to Transition zone
                    localLPMMode = OVERLAP;
                    LOG_INFO(CamxLogGroupChi, "Dummy SAT node - Moving to OVERLAP zone");
                }
                else if ((TELEONLY == m_lpmMode) &&
                    (((m_numOfLinkedCameras == 2) &&
                      (pInputMetaOZ->cameraMetadata[0].userCropRegion.left <= DUAL_TRANSITION_HIGH)) ||
                      ((m_numOfLinkedCameras == 3) &&
                     (pInputMetaOZ->cameraMetadata[0].userCropRegion.left <= TRIPLE_TRANSITION_HIGH))))
                {
                    // Switch to Transition zone
                    localLPMMode = OVERLAP;
                    LOG_INFO(CamxLogGroupChi, "Dummy SAT node - Moving to OVERLAP zone");
                }
                else if (OVERLAP == m_lpmMode)
                {
                    // Switch to Wide Zone only if master is WIDE and Zoom < TRANSITION_LOW
                    for (UINT32 i = 0; i < pInputMetaOZ->numInputs; i++)
                    {
                        if (((m_numOfLinkedCameras == 2) &&
                            (m_camIdWide == pInputMetaOZ->cameraMetadata[i].masterCameraId) &&
                            (pInputMetaOZ->cameraMetadata[i].userCropRegion.left <= DUAL_TRANSITION_LOW)) ||
                             ((m_numOfLinkedCameras == 3) &&
                            (m_camIdWide == pInputMetaOZ->cameraMetadata[i].masterCameraId) &&
                            (pInputMetaOZ->cameraMetadata[i].userCropRegion.left <= TRIPLE_TRANSITION_LOW)))
                        {
                            localLPMMode = WIDEONLY;
                            LOG_INFO(CamxLogGroupChi, "Dummy SAT node - Moving to WIDEONLY zone");
                        }
                        // Switch to Tele Zone only if master is TELE and Zoom > TRANSITION_HIGH
                        else if (((m_numOfLinkedCameras == 2) &&
                            (m_camIdTele == pInputMetaOZ->cameraMetadata[i].masterCameraId) &&
                            (pInputMetaOZ->cameraMetadata[i].userCropRegion.left > DUAL_TRANSITION_HIGH)) ||
                            ((m_numOfLinkedCameras == 3) &&
                            (m_camIdTele == pInputMetaOZ->cameraMetadata[i].masterCameraId) &&
                            (pInputMetaOZ->cameraMetadata[i].userCropRegion.left > TRIPLE_TRANSITION_HIGH)))
                        {
                            localLPMMode = TELEONLY;
                            LOG_INFO(CamxLogGroupChi, "Dummy SAT node - Moving to TELEONLY zone");
                        }
                    }
                }

                if (localLPMMode != m_recommendedLPMMode)
                {
                    m_inStageFrames = 0;
                }
                else
                {
                    m_inStageFrames++;
                }
                m_recommendedLPMMode = localLPMMode;

                if ((m_recommendedLPMMode != m_lpmMode) &&
                    ((m_inStageFrames >= ZONE_CHANGE_THRESHOLD) || (OVERLAP == m_recommendedLPMMode)))
                {
                    LOG_INFO(CamxLogGroupChi, "masterId:%d, result masterId %d",
                             currentMasterId, m_resultDataOZ.masterCameraId);

                    if ((WIDEONLY == localLPMMode) || (TELEONLY == localLPMMode))
                    {
                        for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
                        {
                            if (m_resultDataOZ.lowPowerMode[i].cameraId == currentMasterId)
                            {
                                m_resultDataOZ.lowPowerMode[i].isEnabled = FALSE;
                            }
                            else
                            {
                                m_resultDataOZ.lowPowerMode[i].isEnabled = TRUE;
                            }
                        }
                    }
                    //Todo: Cache overlap cameras and update this section for multiple overlap regions
                    else if (OVERLAP == localLPMMode)
                    {
                        UINT32 cameraIdx = 0;
                        for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
                        {
                            cameraIdx = GetCameraIndexFromID(m_resultDataOZ.lowPowerMode[i].cameraId);
                            /// Always have non smooth zoom enabled camera in LPM
                            if (FALSE == m_cameraInfo[cameraIdx].pCameraConfig->enableSmoothTransition)
                            {
                                m_resultDataOZ.lowPowerMode[i].isEnabled = TRUE;
                            }
                            else
                            {
                                m_resultDataOZ.lowPowerMode[i].isEnabled = FALSE;
                            }
                        }
                    }
                    LOG_INFO(CamxLogGroupChi, "MasterId:%d, result masterId:%d",
                             currentMasterId, m_resultDataOZ.masterCameraId);
                    LOG_ERROR(CamxLogGroupChi, "SAT: Trigger LPM %d => %d",
                              m_lpmMode, m_recommendedLPMMode);

                    m_lpmMode = m_recommendedLPMMode;
                    m_inStageFrames = 0;
                }
                else if (m_recommendedLPMMode == m_lpmMode)
                {
                    m_inStageFrames = 0;
                }
            }
            else
            {
                m_lpmMode = OVERLAP;
            }


            //Switch master
            if (OVERLAP == m_lpmMode)
            {
                if (m_preRoleSwitchWaitFrames > 0)
                {
                    m_preRoleSwitchWaitFrames--;
                }
                if (0 == m_preRoleSwitchWaitFrames)
                {
                    // Set the master camera depending on crop region
                    if (((m_numOfLinkedCameras == 2) &&
                         pInputMetaOZ->cameraMetadata[0].userCropRegion.left > DUAL_MASTER_SWITCH) ||
                        ((m_numOfLinkedCameras == 3) &&
                         pInputMetaOZ->cameraMetadata[0].userCropRegion.left > TRIPLE_MASTER_SWITCH))
                    {
                        currentMasterId = m_camIdTele;
                    }
                    else
                    {
                        currentMasterId = m_camIdWide;
                    }
                    LOG_INFO(CamxLogGroupChi, "MasterId:%d, result masterId:%d",
                             currentMasterId, m_resultDataOZ.masterCameraId);
                }
                else
                {
                    LOG_INFO(CamxLogGroupChi, "wait for role switch m_preRoleSwitchWaitFrames is %d ",
                             m_preRoleSwitchWaitFrames);
                }

                if (pInputMetaOZ->cameraMetadata[0].masterCameraId != currentMasterId)
                {
                    LOG_ERROR(CamxLogGroupChi, "SAT: Switch Master: %d => %d",
                              pInputMetaOZ->cameraMetadata[0].masterCameraId, currentMasterId);
                }
            }
            else
            {
                if (0 == m_preRoleSwitchWaitFrames)
                {
                    m_preRoleSwitchWaitFrames = RoleSwitchWaitFrameCount;
                }
            }
            m_resultDataOZ.masterCameraId            = currentMasterId;
            m_resultDataOZ.recommendedMasterCameraId = currentMasterId;
            LOG_INFO(CamxLogGroupChi, "MasterId:%d, result masterId:%d"
                     "pInputMetaOZ->numInputs:%d",
                     currentMasterId,
                     m_resultDataOZ.masterCameraId,
                     pInputMetaOZ->numInputs);
        }
        else
        {
            m_resultDataOZ.masterCameraId            = pInputMetaOZ->cameraMetadata[0].masterCameraId;
            m_resultDataOZ.recommendedMasterCameraId = pInputMetaOZ->cameraMetadata[0].masterCameraId;
            LOG_INFO(CamxLogGroupChi, "Else MasterId:%d, result masterId:%d",
                     currentMasterId, m_resultDataOZ.masterCameraId);
        }

        UINT32 cameraIdx = 0;
        for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
        {
            cameraIdx = GetCameraIndexFromID(m_resultDataOZ.lowPowerMode[i].cameraId);
            /// Always have hard switch camera in LPM, MCC will take care of enabling/disabling
            if (FALSE == m_cameraInfo[cameraIdx].pCameraConfig->enableSmoothTransition)
            {
                m_resultDataOZ.lowPowerMode[i].isEnabled = TRUE;
            }
        }

        // Copy current master buffer and update metadata correctly,
        // if in dual zone,so other nodes can consume it
        if ((1 < pInputMetaOZ->numInputs) &&
            (TRUE == ENABLE_DYNAMIC_MASTER_SWITCH))
        {
            currentMasterId = m_resultDataOZ.masterCameraId;

            LOG_INFO(CamxLogGroupChi, "SAT Result Switching Dynamically MasterId=%d",
                    currentMasterId);
        }
        else
        {
            currentMasterId               = pInputMetaOZ->cameraMetadata[0].masterCameraId;
            m_resultDataOZ.masterCameraId = currentMasterId;
        }

        BOOL           willBypass = IsBypassableNode(); // In a real SAT Node, IsBypassableNode is a prereq to bypass
                                                        // but we may not bypass even if we can
        m_resultDataOZ.hasProcessingOccured = FALSE;

        // Copy the input from master
        UINT32 masterCameraIdx      = GetCameraIndexFromID(currentMasterId);
        m_currentMasterCameraId     = currentMasterId;

        for (UINT32 i = 0; i < pInputMetaOZ->numInputs; i++)
        {
            if (pProcessRequestInfo->phInputBuffer[i]->portId == m_cameraInfo[masterCameraIdx].inputPortId)
            {
                masterSrcPortIndex = i;
                LOG_INFO(CamxLogGroupChi, "Dummy SAT node - Master is %d, Master Port Id %d, Input Port Id %d",
                    pInputMetaOZ->cameraMetadata[i].masterCameraId, m_cameraInfo[masterCameraIdx].inputPortId,
                    pProcessRequestInfo->phInputBuffer[i]->portId);
                break;
            }
        }

        m_resultDataOZ.masterCameraId = currentMasterId;

        // Bypass or CopyImage
        if (TRUE == willBypass)
        {
            // Bypass (Assuming always bypass)
            pProcessRequestInfo->pBypassData[0].isBypassNeeded = TRUE;
            pProcessRequestInfo->pBypassData[0].selectedInputPortIndex = masterSrcPortIndex;
        }
        else
        {
            // CopyImage
            // Cache invalidate on input buffer
            g_ChiNodeInterface.pCacheOps(pProcessRequestInfo->phInputBuffer[masterSrcPortIndex], TRUE, FALSE);
            CopyImage(pProcessRequestInfo->phOutputBuffer[0], pProcessRequestInfo->phInputBuffer[masterSrcPortIndex]);
            g_ChiNodeInterface.pCacheOps(pProcessRequestInfo->phOutputBuffer[0], FALSE, TRUE);
        }

        if (pInputMetaOZ->cameraMetadata[masterSrcPortIndex].masterCameraId != m_masterFromInputForPSM)
        {
            m_masterFromInputForPSM = pInputMetaOZ->cameraMetadata[masterSrcPortIndex].masterCameraId;
            m_masterInputportId     = pProcessRequestInfo->phInputBuffer[masterSrcPortIndex]->portId;
            LOG_INFO(CamxLogGroupChi, "Updating Master inputport id for PSM");
        }

        LOG_INFO(CamxLogGroupChi, "Request: %" PRIi64 " %sing input port %u @ idx %u to output port 0 ",
            pProcessRequestInfo->frameNum,
            willBypass ? "Bypass" : "Memcpy",
            pProcessRequestInfo->phInputBuffer[masterSrcPortIndex]->portId,
            masterSrcPortIndex);

    }

    for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
    {
        LOG_ERROR(CamxLogGroupChi, "SAT Result: cameraid %d, lpm enable %d,"
                 " Master Camera Id: %u ",
                 m_resultDataOZ.lowPowerMode[i].cameraId,
                 m_resultDataOZ.lowPowerMode[i].isEnabled,
                 m_resultDataOZ.masterCameraId);
    }

    m_processedFrame++;
    UpdateMetaData(pProcessRequestInfo->frameNum, &pProcessRequestInfo->phInputBuffer[masterSrcPortIndex]->format);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::ChiDummySATNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiDummySATNode::ChiDummySATNode()
    : m_hChiSession(NULL)
    , m_nodeId(0)
    , m_nodeCaps(0)
    , m_processedFrame(0)
    , m_inducedSleepTime(0)
{
    memset(&m_format, 0, sizeof(CHINODEIMAGEFORMAT));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::~ChiDummySATNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiDummySATNode::~ChiDummySATNode()
{
    m_hChiSession = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::CopyImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiDummySATNode::CopyImage(
    CHINODEBUFFERHANDLE hOutput,
    CHINODEBUFFERHANDLE hInput)
{
    UINT32 outPlaneStride  = hOutput->format.formatParams.yuvFormat[0].planeStride;
    UINT32 outHeight       = hOutput->format.formatParams.yuvFormat[0].height;
    UINT32 outSliceHeight  = hOutput->format.formatParams.yuvFormat[0].sliceHeight;
    UINT32 inPlaneStride   = hInput->format.formatParams.yuvFormat[0].planeStride;
    UINT32 inHeight        = hInput->format.formatParams.yuvFormat[0].height;
    UINT32 inSliceHeight   = hInput->format.formatParams.yuvFormat[0].sliceHeight;
    UINT32 copyPlaneStride = outPlaneStride > inPlaneStride ? inPlaneStride : outPlaneStride;
    UINT32 copyHeight      = outHeight > inHeight ? inHeight : outHeight;

    memcpy(&hOutput->format, &hInput->format, sizeof(CHIIMAGEFORMAT));
    memcpy(&hOutput->planeSize[0], &hInput->planeSize[0], sizeof(hInput->planeSize));
    memcpy(&hOutput->metadataSize[0], &hInput->metadataSize[0], sizeof(hInput->metadataSize));

    hOutput->imageCount     = hInput->imageCount;
    hOutput->numberOfPlanes = hInput->numberOfPlanes;

    for (UINT i = 0; i < hOutput->imageCount; i++)
    {
        if (((outPlaneStride != inPlaneStride) || (outSliceHeight != inSliceHeight)) &&
            ((YUV420NV12 == hOutput->format.format) || (YUV420NV21 == hOutput->format.format) ||
            (RawPlain16 == hOutput->format.format)))
        {
            hOutput->format.formatParams.yuvFormat[0].planeStride = outPlaneStride;
            hOutput->format.formatParams.yuvFormat[0].sliceHeight = outSliceHeight;
            hOutput->format.formatParams.yuvFormat[0].height      = outHeight;

            for (UINT m = 0; m < copyHeight; m++)
            {
                memcpy((hOutput->pImageList[i].pAddr[0] + outPlaneStride * m),
                       (hInput->pImageList[i].pAddr[0] + inPlaneStride * m),
                       copyPlaneStride);
            }

            if (1 < hOutput->numberOfPlanes)
            {
                hOutput->format.formatParams.yuvFormat[1].planeStride = outPlaneStride >> 1;
                hOutput->format.formatParams.yuvFormat[1].sliceHeight = outSliceHeight >> 1;
                hOutput->format.formatParams.yuvFormat[1].height      = outHeight >> 1;

                for (UINT m = 0; m < copyHeight >> 1; m++)
                {
                    memcpy((hOutput->pImageList[i].pAddr[1] + outPlaneStride * m),
                           (hInput->pImageList[i].pAddr[1] + inPlaneStride * m),
                           copyPlaneStride);
                }
            }
        }
        else
        {
            for (UINT j = 0; j < hOutput->numberOfPlanes; j++)
            {
                memcpy(hOutput->pImageList[i].pAddr[j], hInput->pImageList[i].pAddr[j], hInput->planeSize[j]);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::UpdateMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiDummySATNode::UpdateMetaData(
    UINT64          requestId,
    CHIIMAGEFORMAT* pInputFormat)
{
    CDKResult              result       = CDKResultSuccess;
    CHIMETADATAINFO        metadataInfo = {0};
    // This is an example for the SAT node to publish the result metadata.
    const UINT32           tagSize      = sizeof(g_VendorTagSectionDummySAT) / sizeof(g_VendorTagSectionDummySAT[0]) + 4;
    CHITAGDATA             tagData[tagSize];
    UINT32                 tagList[tagSize];

    metadataInfo.size       = sizeof(CHIMETADATAINFO);
    metadataInfo.chiSession = m_hChiSession;
    metadataInfo.pTagList   = &tagList[0];
    metadataInfo.pTagData   = &tagData[0];

    UINT32 index = 0;

    UINT32  supportedFeature    = ChiNodeCapsDummySAT;
    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &supportedFeature;
    tagData[index].dataSize     = g_VendorTagSectionDummySAT[index].numUnits;
    index++;

    UINT32  currentMode         = m_nodeCaps;
    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &currentMode;
    tagData[index].dataSize     = g_VendorTagSectionDummySAT[index].numUnits;
    index++;

    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &m_processedFrame;
    tagData[index].dataSize     = g_VendorTagSectionDummySAT[index].numUnits;
    index++;

    UINT32 dimension[2];
    dimension[0]                = m_format.width;
    dimension[1]                = m_format.height;
    tagList[index]              = g_VendorTagList[index] + g_vendorTagBase;
    tagData[index].size         = sizeof(CHITAGDATA);
    tagData[index].requestId    = requestId;
    tagData[index].pData        = &dimension[0];
    tagData[index].dataSize     = g_VendorTagSectionDummySAT[index].numUnits;

    // This is an example for the SAT node to publish the result metadata.
    CHIVENDORTAGBASEINFO vendorTagBase = { 0 };
    vendorTagBase.size                 = sizeof(CHIVENDORTAGBASEINFO);
    vendorTagBase.pComponentName       = "com.qti.chi.multicameraoutputmetadata";
    vendorTagBase.pTagName             = "OutputMetadataOpticalZoom";
    g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);

    if (0 != vendorTagBase.vendorTagBase)
    {
        index++;
        tagList[index] = vendorTagBase.vendorTagBase;
        tagData[index].size = sizeof(CHITAGDATA);
        tagData[index].requestId = requestId;
        tagData[index].pData = &m_resultDataOZ;
        tagData[index].dataSize = sizeof(OutputMetadataOpticalZoom);
    }


    // This is an example for the SAT node to residual crop info
    VOID*        pData           = NULL;
    CHIRECT      residualCrop    = { 0, 0, 1000, 1000 };

    vendorTagBase                = { 0 };
    vendorTagBase.size           = sizeof(CHIVENDORTAGBASEINFO);
    vendorTagBase.pComponentName = "com.qti.cropregions";
    vendorTagBase.pTagName       = "ChiNodeResidualCrop";
    g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);

    CHIVENDORTAGBASEINFO ifeCropTagBase = { 0 };
    ifeCropTagBase.size                 = sizeof(CHIVENDORTAGBASEINFO);
    ifeCropTagBase.pComponentName       = "org.quic.camera.ifecropinfo";
    ifeCropTagBase.pTagName             = "ResidualCrop";
    g_ChiNodeInterface.pGetVendorTagBase(&ifeCropTagBase);

    if (0 != ifeCropTagBase.vendorTagBase)
    {
        pData = ChiNodeUtils::GetMetaData(requestId, ifeCropTagBase.vendorTagBase,
        ChiMetadataDynamic, &g_ChiNodeInterface, m_hChiSession);

        if (NULL == pData)
        {
            pData = ChiNodeUtils::GetMulticamDynamicMetaByCamId(requestId, ifeCropTagBase.vendorTagBase | InputMetadataSectionMask,
                ChiMetadataDynamic, &g_ChiNodeInterface, m_hChiSession, m_currentMasterCameraId);
        }
    }

    if ((NULL != pData) && (0 != vendorTagBase.vendorTagBase))
    {
        IFECropInfo* ifeResidualCrop = static_cast<IFECropInfo *>(pData);

        residualCrop.left   = ifeResidualCrop->fullPath.left;
        residualCrop.top    = ifeResidualCrop->fullPath.top;
        residualCrop.width  = ifeResidualCrop->fullPath.width;
        residualCrop.height = ifeResidualCrop->fullPath.height;
    }

    if (0 != vendorTagBase.vendorTagBase)
    {
        index++;
        tagList[index] = vendorTagBase.vendorTagBase;
        tagData[index].size = sizeof(CHITAGDATA);
        tagData[index].requestId = requestId;
        tagData[index].pData = &residualCrop;
        tagData[index].dataSize = sizeof(CHIRECT);
    }

    if (FALSE == m_resultDataOZ.hasProcessingOccured)
    {
        index++;
        tagList[index]           = ChiNodeUtils::GetVendorTagBase("com.qti.node.gpu", "BypassCameraId", &g_ChiNodeInterface);
        tagData[index].requestId = requestId;
        tagData[index].pData     = &m_currentMasterCameraId;
        tagData[index].dataSize  = sizeof(UINT32);
    }

    metadataInfo.tagNum = index + 1;
    g_ChiNodeInterface.pSetMetadata(&metadataInfo);

    // SAT ICA In perspective transform
    IPEICAPerspectiveTransform perspectiveTrans;
    memset(&perspectiveTrans, 0, sizeof(IPEICAPerspectiveTransform));

    perspectiveTrans.perspectiveTransformEnable       = TRUE;
    perspectiveTrans.perspectiveConfidence            = 1;
    perspectiveTrans.byPassAlignmentMatrixAdjustement = TRUE;
    perspectiveTrans.perspetiveGeometryNumColumns     = 1;
    perspectiveTrans.perspectiveGeometryNumRows       = 1;
    perspectiveTrans.transformDefinedOnWidth          = pInputFormat->width;
    perspectiveTrans.transformDefinedOnHeight         = pInputFormat->height;
    perspectiveTrans.ReusePerspectiveTransform        = 0;
    memcpy(&perspectiveTrans.perspectiveTransformArray, &perspArray, sizeof(FLOAT) * ICAParametersPerPerspectiveTransform);

    result = ChiNodeUtils::GetVendorTagBase("org.quic.camera2.ipeicaconfigs",
                                            "ICAInPerspectiveTransform",
                                            &g_ChiNodeInterface,
                                            &vendorTagBase);

   if (CDKResultSuccess == result)
   {
        UINT32 ICAInPerspectiveTransformTagId = vendorTagBase.vendorTagBase;
        LOG_INFO(CamxLogGroupChi,"SAT_PSM currentmastercamera id %d input port %d", m_masterFromInputForPSM,
            m_masterInputportId);

        UINT Satstreamcrop = ChiNodeUtils::GetVendorTagBase("com.qti.camera.streamCropInfo", "StreamCropInfo", &g_ChiNodeInterface);

        UINT32 psTagList[] =
        {
            ICAInPerspectiveTransformTagId,
            Satstreamcrop,
        };

        const UINT32     psTagSize            = sizeof(psTagList) / sizeof(psTagList[0]);
        CHITAGDATA       psTagData[psTagSize] = { { 0 } };

        index = 0;
        psTagData[index].size      = sizeof(CHITAGDATA);
        psTagData[index].requestId = requestId;
        psTagData[index].pData     = &perspectiveTrans;
        psTagData[index].dataSize  = sizeof(IPEICAPerspectiveTransform);
        index++;

        void* pstreamData = ChiNodeUtils::GetPSMetaData(requestId, Satstreamcrop, m_masterInputportId, &g_ChiNodeInterface, m_hChiSession);
        StreamCropInfo* parentCropInfo  = static_cast<StreamCropInfo*>(pstreamData);
        LOG_INFO(CamxLogGroupChi, "SAT_PSM FOV wrt input frame w.r.t Active array [(%d %d), %d,%d]"
                 "residual crop [(%d %d) %d %d  frame dim (%d %d)",
                 parentCropInfo->fov.left, parentCropInfo->fov.top,
                 parentCropInfo->fov.width, parentCropInfo->fov.height,
                 parentCropInfo->crop.left, parentCropInfo->crop.top,
                 parentCropInfo->crop.width, parentCropInfo->crop.height,
                 parentCropInfo->frameDimension.width, parentCropInfo->frameDimension.height);

        psTagData[index].size      = sizeof(CHITAGDATA);
        psTagData[index].requestId = requestId;
        psTagData[index].pData     = parentCropInfo;
        psTagData[index].dataSize  = sizeof(StreamCropInfo);
        index++;

        CHIPSMETADATA psmetadataInfo;
        memset(&psmetadataInfo, 0x0, sizeof(CHIPSMETADATA));

        psmetadataInfo.size       = sizeof(CHIPSMETADATA);
        psmetadataInfo.chiSession = m_hChiSession;
        psmetadataInfo.tagCount   = psTagSize;
        psmetadataInfo.pTagList   = &psTagList[0];
        psmetadataInfo.pTagData   = &psTagData[0];
        psmetadataInfo.portId     = 0; // ChiNodeOutputPort full

        CHIPSMETADATABYPASSINFO psbypassinfo;
        memset(&psbypassinfo, 0x0, sizeof(CHIPSMETADATABYPASSINFO));

        psbypassinfo.size       = sizeof(CHIPSMETADATABYPASSINFO);
        psbypassinfo.chiSession = m_hChiSession;

        g_ChiNodeInterface.pSetPSMetadata(&psmetadataInfo);
        for (UINT32 i = 0; i < psTagSize; i++)
        {
            g_ChiNodeInterface.pPublishPSMetadata(psTagList[i], &psbypassinfo);
        }

   }
#if PRINT_METADATA
    CHIVENDORTAGBASEINFO vendorTagBase  = {0};
    vendorTagBase.size                  = sizeof(CHIVENDORTAGBASEINFO);
    vendorTagBase.pComponentName        = DummySATNodeSectionName;
    vendorTagBase.pTagName              = g_VendorTagSectionDummySAT[index].pVendorTagName;
    g_ChiNodeInterface.pGetVendorTagBase(&vendorTagBase);
    LOG_VERBOSE(CamxLogGroupChi, "Vendor Tags value shall be same %x %x",
            g_VendorTagList[index] + g_vendorTagBase, vendorTagBase.vendorTagBase);

    // get the updated metadata from CamX and print them out
    VOID* pData = NULL;
    pData = ChiNodeUtils::GetMetaData(requestId, DummySATNodeTagSupportedFeature + g_vendorTagBase, ChiMetadataDynamic,
        &g_ChiNodeInterface, m_hChiSession);
    if (NULL != pData)
    {
        LOG_VERBOSE(CamxLogGroupChi, "supported feature %d supported feature %d",
            *static_cast<UINT32*>(pData), supportedFeature);
    }

    pData = ChiNodeUtils::GetMetaData(requestId, DummySATNodeTagCurrentMode + g_vendorTagBase, ChiMetadataDynamic,
        &g_ChiNodeInterface, m_hChiSession);
    if (NULL != pData)
    {
        LOG_VERBOSE(CamxLogGroupChi, "current mode %d current mode %d",
            *static_cast<UINT32*>(pData), currentMode);
    }

    pData = ChiNodeUtils::GetMetaData(requestId, DummySATNodeTagProcessedFrameNum + g_vendorTagBase, ChiMetadataDynamic,
        &g_ChiNodeInterface, m_hChiSession);
    if (NULL != pData)
    {
        LOG_VERBOSE(CamxLogGroupChi, "processed frameNum %" PRIu64 " processed frame %" PRIu64,
            *static_cast<UINT64*>(pData), m_processedFrame);
    }

    pData = ChiNodeUtils::GetMetaData(requestId, DummySATNodeTagFrameDimension + g_vendorTagBase, ChiMetadataDynamic,
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::GetCameraIndexFromID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiDummySATNode::GetCameraIndexFromID(UINT32 cameraId)
{
    UINT32 camIndex = INVALID_INDEX;

    for (UINT32 i = 0; i < m_numOfLinkedCameras; i++)
    {
        if (m_cameraInfo[i].pCameraConfig->cameraId == cameraId)
        {
            camIndex = i;
            break;
        }
    }
    return camIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDummySATNode::ComputeResponseTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiDummySATNode::ComputeResponseTime(
    CHINODERESPONSEINFO* pInfo)
{
    CHI_UNREFERENCED_PARAM(pInfo);

    // Each node needs to publish a response time (milliseconds) for how long it takes to
    // stop processing requests after flush call.
    // During flush, session will consider this reponse time and will wait for nodes to flush gracefully.
    // This response time can be computed dynamically based on the type of the node,
    // otherwise default reponse time can be used.
    pInfo->responseTimeInMillisec = DEFAULT_FLUSH_RESPONSE_TIME + m_inducedSleepTime;

    return CDKResultSuccess;
}

// CAMX_NAMESPACE_END
