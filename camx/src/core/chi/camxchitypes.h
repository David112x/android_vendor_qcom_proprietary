////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchitypes.h
/// @brief Declarations of wrapped CHI types
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHITYPES_H
#define CAMXCHITYPES_H

#include "camxcommontypes.h"
#include "camxdefs.h"
#include "camxtypes.h"
#include "camxcdktypes.h"

CAMX_NAMESPACE_BEGIN

/// @todo (CAMX-1512) Static asserts for all wrappers

/// @brief Callbacks into the driver provided to the CHI app
struct HALCallbacks
{
    VOID(*ProcessCaptureResult)(
        const Camera3Device*,
        const Camera3CaptureResult*);

    VOID(*NotifyResult)(
        const Camera3Device*,
        const Camera3NotifyMessage*);
};

/// @brief Callback functions made by the driver into the application framework. This is a wrapper for
///        "struct chi_hal_callback_ops_t" defined in chioverride.h
struct CHIAppCallbacks
{
    /// @brief Called by the driver to get number of cameras
    VOID (*CHIGetNumCameras)(
        UINT32* pNumFwCameras,
        UINT32* pNumLogicalCameras);

    /// @brief Called by the driver to get the camera info for the camera id
    CamxResult (*CHIGetCameraInfo)(
        UINT32      cameraId,
        CameraInfo* pCameraInfo);

    /// @brief HAL/CHI Apps can use this to query information from the CHI implementation
    CamxResult(*CHIGetInfo)(
        CDKGetInfoCmd       infoCmd,
        VOID*               pInputParams,
        VOID*               pOutputParams);

    /// @brief Defines the prototype for the device status change callback method from to the framework. Please refer to
    ///        the camera_device_status_change documentation in hardware/camera_common.h.
    CamxResult (*CHIInitializeOverrideSession)(
        UINT32               cameraId,
        const Camera3Device* pCamera3Device,
        const HALCallbacks*  pHALCallbacks,
        Camera3StreamConfig* pStreamConfig,
        BOOL*                isOverrideEnabled,
        VOID**               ppPrivate);

    /// @brief Defines the prototype for the torch mode status change callback method from to the framework. Please refer to
    ///        the torch_mode_status_change documentation in hardware/camera_common.h.
    CamxResult (*CHIFinalizeOverrideSession)(
        const Camera3Device* pCamera3Device,
        UINT64*              pSession,
        VOID**               ppPrivate);

    /// @brief Called by the driver to inform about session closing
    VOID (*CHITeardownOverrideSession)(
        const Camera3Device* pCamera3Device,
        UINT64*              pSession,
        VOID*                pPrivate);

    /// @brief Called by the driver to pass on capture request call to CHI
    INT (*CHIOverrideProcessRequest)(
        const Camera3Device*    pCamera3Device,
        Camera3CaptureRequest*  pCaptureRequest,
        VOID*                   pPrivate);

    /// @brief Called by the driver to allow for additional override processing during open()
    VOID (*CHIExtendOpen)(
        UINT32  cameraId,
        VOID*   pPrivateData);

    /// @brief Called by the driver to allow for additional override processing during close()
    VOID (*CHIExtendClose)(
        UINT32  cameraId,
        VOID*   pPrivateData);

    /// @brief Called by the driver to allow override to remap special camera IDs into logical camera IDs
    UINT32(*CHIRemapCameraId)(
        UINT32              frameworkCameraId,
        CameraIdRemapMode   mode);

    /// @brief Interface to allow various override-specific settings to be toggled.
    VOID (*CHIModifySettings)(
        VOID*   pPrivateData);

    /// @brief Get any vendor tag specific request settings the override wants to get added to the default settings
    VOID (*CHIGetDefaultRequestSettings)(
        UINT32           frameworkCameraId,
        INT              requestTemplate,
        const Metadata** pAdditionalMetadata);

    /// @brief Called by the driver to allow for flush()
    INT(*CHIOverrideFlush)(
        const Camera3Device*    pCamera3Device);

};

/// @brief Chi port descriptor - Wrapper for "struct chi_pipeline_request"
struct CHIPipelineRequest
{
    UINT32                          size;                   ///< Size of this structure
    const Camera3CaptureRequest*    pCaptureRequest;        ///< Details of the request
};

/// @brief Chi port descriptor - Wrapper for "struct chi_output_port_descriptor"
struct CHIOutputPortDescriptor
{
    UINT32 portId;                                          ///< Input/Output port id
    UINT32 isSinkPort;                                      ///< Is this a sink port
    UINT32 isOutputStreamBuffer;                            ///< Is the output a stream buffer
    UINT32 portSourceTypeId;                                ///< Optional port source type id
};

/// @brief Chi port descriptor - Wrapper for "struct chi_input_port_descriptor"
struct CHIInputPortDescriptor
{
    UINT32 portId;                                          ///< Input/Output port id
    UINT32 isInputStreamBuffer;                             ///< Is the input a stream buffer
    UINT32 portSourceTypeId;                                ///< Optional port source type id
};

/// @brief Chi node ports info - Wrapper for "struct chi_ports"
struct CHIPorts
{
    UINT32                   numInputPorts;                 ///< Num input ports
    CHIInputPortDescriptor*  pInputPortDescriptors;         ///< Pointer to input ports
    UINT32                   numOutputPorts;                ///< Num output ports
    CHIOutputPortDescriptor* pOutputPortDescriptors;        ///< Pointer to output ports
};

/// @brief Chi node info - Wrapper for "struct chi_node"
struct CHINode
{
    VOID*    pNodeProperties;                               ///< Properties associacted with the node
    UINT32   nodeId;                                        ///< Node identifier
    UINT32   nodeInstanceId;                                ///< Node instance identifier
    CHIPorts nodeAllPorts;                                  ///< Information about all ports
};

/// @brief Chi link node - Wrapper for "struct chi_link_node_descriptor"
struct CHILinkNodeDescriptor
{
    UINT32 nodeId;                                          ///< Node identifier
    UINT32 nodeInstanceId;                                  ///< Node instance id
    UINT32 nodePortId;                                      ///< Node port id
};

/// @brief Chi link buffer properties - Wrapper for "struct chi_link_buffer_properties"
struct CHILinkBufferProperties
{
    UINT32 format;                  ///< Format
    UINT32 size;                    ///< Size of buffer
    UINT32 bufferQueueDepth;        ///< Buffer queue depth
    UINT32 heap;                    ///< Heap
    UINT32 flags;                   ///< Flags
};

/// @brief Chi node-port linkages - Wrapper for "struct chi_link_properties"
struct CHILinkProperties
{
    UINT32 isBatchedMode;                                    ///< Batched mode indicator
    UINT32 flags;                                            ///< Flags indicating link specific properties
};

/// @brief Chi node-port linkages - Wrapper for "struct chi_node_linkages"
struct CHINodeLinkages
{
    CHILinkNodeDescriptor    srcNode;                       ///< Src node in a link
    UINT32                   numDestNodes;                  ///< Dest nodes in a link that the src node can be connected to
    CHILinkNodeDescriptor*   pDestNodes;                    ///< Pointer to all the dest nodes connected to the src node
    CHILinkBufferProperties  linkBufferProperties;          ///< Properties of the buffer on the link
    CHILinkProperties        linkProperties;                ///< Properties associated with the link
};

/// @brief Chi node port links info - Wrapper for "struct chi_pipeline_create_descriptor"
struct CHIPipelineCreateDescriptor
{
    UINT32           size;                                  ///< Size of this structure
    UINT32           numPipelineNodes;                      ///< Number of pipeline nodes
    CHINode*         pPipelineNodes;                        ///< Pipeline nodes
    UINT32           numNodeLinkages;                       ///< Number of links in the pipeline
    CHINodeLinkages* pNodeLinkages;                         ///< Description of all the links
    BOOL             isRealTime;                            ///< Is this a real time pipeline
};

/// @brief Details of the target dimensions - Wrapper for "struct chi_buffer_dimension"
struct CHIBufferDimension
{
    UINT32 width;             ///< Width
    UINT32 height;            ///< Height
};

/// @brief Details of the target dimensions - Wrapper for "struct chi_input_buffer_requirements"
struct CHIInputBufferRequirements
{
    UINT32             size;                ///< Size of this structure
    CHIBufferDimension minDimension;        ///< Min Dimension
    CHIBufferDimension maxDimension;        ///< Max Dimension
    CHIBufferDimension optimalDimension;    ///< Optimal Dimension
};

/// @brief Details of the target dimensions - Wrapper for "struct chi_input_buffer_descriptor"
struct CHIInputBufferDescriptor
{
    UINT32                size;             ///< Size of this structure
    CHILinkNodeDescriptor nodePort;         ///< Description of a node + port that will output the app buffer
    Camera3Stream*        pStream;          ///< Stream associated with this node
};

/// @brief Details of the target dimensions - Wrapper for "struct chi_output_buffer_descriptor"
struct CHIOutputBufferDescriptor
{
    UINT32                size;             ///< Size of this structure
    CHILinkNodeDescriptor nodePort;         ///< Description of a node + port that will output the app buffer
    Camera3Stream*        pStream;          ///< Stream associated with this node
};

CAMX_NAMESPACE_END

#endif // CAMXCHITYPES_H
