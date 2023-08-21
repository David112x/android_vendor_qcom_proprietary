////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodedummysat.h
/// @brief Chi node for dummysat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODEDUMMYSAT_H
#define CAMXCHINODEDUMMYSAT_H

#include "chinode.h"
#include "camxchinodeutil.h"
#include "chivendortag.h"


enum LPMMode
{
    WIDEONLY,
    TELEONLY,
    OVERLAP,
};

static const UINT ZONE_CHANGE_THRESHOLD    = 10;         ///<.This is threshold in frames for not changing the state
                                                         /// while previous camera session is going on
static const INT DUAL_TRANSITION_LOW      = 400;        ///<.Threshold for switching to DUAL ZONE
static const INT DUAL_TRANSITION_HIGH     = 1500;       ///<.Threshold for switching to TELE ZONE
static const INT DUAL_MASTER_SWITCH       = 1000;       ///< Threshold for switching master
static const INT TRIPLE_TRANSITION_LOW    = 1000;       ///<.Threshold for switching to DUAL ZONE
static const INT TRIPLE_TRANSITION_HIGH   = 1700;       ///<.Threshold for switching to TELE ZONE
static const INT TRIPLE_MASTER_SWITCH     = 1550;       ///< Threshold for switching master
static const UINT MAX_CONCURRENT_CAMERAS   = 2;          ///< max number of camera that can be active at a time
static const UINT32 INVALID_INDEX          = 0xFFFFFFFF; ///< Invalid Index

// NOWHINE FILE NC004c: Things outside the Camx namespace should be prefixed with Camx/CSL

/// @brief Metadata information for the framework request
typedef struct CameraInfo
{
    CameraConfiguration*    pCameraConfig;     ///< physical camera configuration
    FLOAT                   focalLength;       ///< focal lenth of the camera
    FLOAT                   fovRatio;          ///< FOV Ratio of the camera
    UINT32                  inputPortId;       ///< Buffer index of the full input port
} SatCameraInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Chi node structure for Chi interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiDummySATNode
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialization required to create a node
    ///
    /// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        CHINODECREATEINFO* pCreateInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryBufferInfo
    ///
    /// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
    ///
    /// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult QueryBufferInfo(
        CHINODEQUERYBUFFERINFO* pQueryBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataPublishList
    ///
    /// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
    ///
    /// @param  pMetadataPublishlist    Pointer to a structure to query the publish list
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult QueryMetadataPublishList(
        CHINODEMETADATALIST* pMetadataPublishlist);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBufferInfo
    ///
    /// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
    ///
    /// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetBufferInfo(
        CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequest
    ///
    /// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
    ///
    /// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessRequest(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ComputeResponseTime
    ///
    /// @brief  Compute flush response time
    ///
    /// @param  pInfo Pointer to a structure that defines the information required for calculating response time.
    ///
    /// @return worst-case response time in ms.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ComputeResponseTime(
        CHINODERESPONSEINFO* pInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiDummySATNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiDummySATNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiDummySATNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiDummySATNode();

private:
    ChiDummySATNode(const ChiDummySATNode&) = delete;               ///< Disallow the copy constructor
    ChiDummySATNode& operator=(const ChiDummySATNode&) = delete;    ///< Disallow assignment operator

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyImage
    ///
    /// @brief  Copy image from input to output, it's a simple memory copy
    ///
    /// @param  hOutput  The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput   The CHINODEBUFFERHANDLE to output image buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CopyImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMetaData
    ///
    /// @brief  Update the metadata in the pipeline
    ///
    /// @param  requestId    The request id for current request
    /// @param  pInputFormat Pointer to input format
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateMetaData(
        UINT64          requestId,
        CHIIMAGEFORMAT* pInputFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureCameraInfo
    ///
    /// @brief  Configure Physical camera data
    ///
    /// @param  None
    ///
    /// @return CDKResult  CDKResultSuccess if succesful. Error otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ConfigureCameraInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraIndexFromID
    ///
    /// @brief  Get Index from camera_info structure based on camera ID
    ///
    /// @param  camRole
    ///
    /// @return camera index  if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetCameraIndexFromID(
        UINT32 cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureInputBufferMapping
    ///
    /// @brief  Get Index from camera_info structure based on camera ID
    ///
    /// @param  pBufferInfo    Pointer to a structure to query the input buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ConfigureInputBufferMapping();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBypassableNode
    ///
    /// @brief  Get isBypassable node flag
    ///
    /// @param  None
    ///
    /// @return True/False whether isBypassable node flag is set
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsBypassableNode() const
    {
        return m_nodeFlags.isBypassable;
    }

    CHIHANDLE                 m_hChiSession;                   ///< The Chi session handle
    UINT32                    m_nodeId;                        ///< The node's Id
    UINT32                    m_nodeCaps;                      ///< The selected node caps
    CHINODEIMAGEFORMAT        m_format;                        ///< The selected format
    UINT64                    m_processedFrame;                ///< The count for processed frame
    OutputMetadataOpticalZoom m_resultDataOZ;                  ///< OZ result metadata
    BOOL                      m_LPMEnable;                     ///< LPM enable flag
    UINT32                    m_inStageFrames;                 ///< Frame counter for state transition
    LPMMode                   m_lpmMode;                       ///< LPM mode(wide, tele, dual)
    LPMMode                   m_recommendedLPMMode;            ///< LPM mode(wide, tele, dual)
    UINT8                     m_preRoleSwitchWaitFrames;       ///< Number of frames to wait for before doing the role switch in dual zone
    UINT32                    m_ozResultVendorTag;             ///< Vendor tag ID for optical zoom result
    SatCameraInfo             m_cameraInfo[MaxLinkedCameras];  ///< Per camera Info
    UINT32                    m_numOfLinkedCameras;            ///< Number of linked cameras in the current session
    UINT32                    m_primaryCameraId;               ///< Primary Camera Id
    UINT32                    m_primaryCameraIndex;            ///< Primary Camera Index
    UINT32                    m_nodeInstanceId;                ///< Node Instance Id
    CHINODEFLAGS              m_nodeFlags;                     ///< Node flags
    UINT32                    m_currentMasterCameraId;         ///< The inputMasterCameraId used per request
    UINT32                    m_camIdWide;                     ///<Wide camera id
    UINT32                    m_camIdTele;                     ///<Tele camera id
    UINT                      m_isSnapshotFusion;              ///< Snapshot Fusion case
    UINT32                    m_masterInputportId;             ///<Input portid of the master
    UINT32                    m_masterFromInputForPSM;         ///< master camera from input
    UINT                      m_inducedSleepTime;              ///< Induce extra sleep in milliseconds
};
#endif // CAMXCHINODEDUMMYSAT_H
