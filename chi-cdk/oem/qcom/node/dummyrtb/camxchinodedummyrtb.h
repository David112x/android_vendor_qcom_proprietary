////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodedummyrtb.h
/// @brief Chi node for dummyrtb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODEDUMMYRTB_H
#define CAMXCHINODEDUMMYRTB_H

#include "chinode.h"
#include "camxchinodeutil.h"
#include "chivendortag.h"

// NOWHINE FILE NC004c: Things outside the Camx namespace should be prefixed with Camx/CSL

/// @brief Metadata information for the framework request

static const UINT   TransitionThreshold      = 8000;      ///< threshold for switch
static const UINT32 DualCamCount             = 2;
static const UINT32 TripleCamCount           = 3;
static const UINT32 INVALID_INDEX          = 0xFFFFFFFF; ///< Invalid Index


typedef struct CameraInfo
{
    CameraConfiguration*    pCameraConfig;           ///< physical camera configuration
    FLOAT                   focalLength;             ///< focal lenth of the camera
    FLOAT                   fovRatio;                ///< FOV Ratio of the camera
    UINT32                  inputPortId;             ///< Buffer index of the full input port
} RTBCameraInfo;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Chi node structure for Chi interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiDummyRTBNode
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
    /// ChiDummyRTBNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiDummyRTBNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiDummyRTBNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiDummyRTBNode();
private:
    ChiDummyRTBNode(const ChiDummyRTBNode&) = delete;               ///< Disallow the copy constructor
    ChiDummyRTBNode& operator=(const ChiDummyRTBNode&) = delete;    ///< Disallow assignment operator

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
    /// @param  camRole   Camera Id
    ///
    /// @return camera index  if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetCameraIndexFromID(
        UINT32 cameraId);

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
    /// @param  requestId   The request id for current request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateMetaData(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckDependency
    ///
    /// @brief  Check if the dependencies are satisfied. Set the dependency if the dependency is not met.
    ///
    /// @param  pProcessRequestInfo   Pointer to a structure that defines the information required for processing a request.
    ///
    /// @return TRUE if dependency is satisfied, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckDependency(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo);

    CHIHANDLE                 m_hChiSession;                   ///< The Chi session handle
    UINT32                    m_nodeId;                        ///< The node's Id
    UINT32                    m_nodeCaps;                      ///< The selected node caps
    CHINODEIMAGEFORMAT        m_format;                        ///< The selected format
    UINT64                    m_processedFrame;                ///< The count for processed frame
    OutputMetadataBokeh       m_resultDataRTB;                 ///< RTB result metadata
    RTBCameraInfo             m_cameraInfo[MaxLinkedCameras];  ///< Per camera Info
    UINT32                    m_numOfLinkedCameras;            ///< Number of linked cameras in the current session
    UINT32                    m_primaryCameraId;               ///< Primary Camera Id
    UINT32                    m_primaryCameraIndex;            ///< Primary Camera Index
    UINT32                    m_nodeInstanceId;                ///< Node Instance Id
    UINT                      m_inducedSleepTime;              ///< Induce extra sleep in milliseconds
};
#endif // CAMXCHINODEDUMMYRTB_H
