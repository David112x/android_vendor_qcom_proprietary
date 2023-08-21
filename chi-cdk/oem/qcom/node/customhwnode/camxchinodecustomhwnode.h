////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodecustomhwnode.h
/// @brief Chi node for memcpy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODECUSTOMHWNODE_H
#define CAMXCHINODECUSTOMHWNODE_H

#include "chinode.h"
#include "camxchinodeutil.h"

// Custom HW input resource type
static const UINT32 CustomHwInputPHY0      = 0x5001;
static const UINT32 CustomHwInputPHY1      = 0x5002;
static const UINT32 CustomHwInputPHY2      = 0x5003;
static const UINT32 CustomHwInputPHY3      = 0x5004;
static const UINT32 CustomHwFetchEngine    = 0x5005;

// IFE input resource Lane Type
static const UINT32 CustomLaneTypeDPHY     = 0;
static const UINT32 CustomLaneTypeCPHY     = 1;

// ISP Color Pattern
static const UINT32 CustomHwNodePatternBayerRGRGRG  = 0;
static const UINT32 CustomHwNodePatternBayerGRGRGR  = 1;
static const UINT32 CustomHwNodePatternBayerBGBGBG  = 2;
static const UINT32 CustomHwNodePatternBayerGBGBGB  = 3;
static const UINT32 CustomHwNodePatternYUVYCBYCR    = 4;
static const UINT32 CustomHwNodePatternYUVYCRYCB    = 5;
static const UINT32 CustomHwNodePatternYUVCBYCRY    = 6;

// ISP Input Format
static const UINT32 CustomHwNodeFormatMIPIRaw8      = 2;
static const UINT32 CustomHwNodeFormatMIPIRaw10     = 3;
static const UINT32 CustomHwNodeFormatPlain8        = 12;
static const UINT32 CustomHwNodeFormatPlain128      = 20;
static const UINT32 CustomHwNodeFormatYUV422        = 34;
static const UINT32 CustomHwNodeFormatYV12          = 44;

// Custom HW ports
static const UINT32 ChiNodeCustomHwInputPortSensor      = 0;
static const UINT32 ChiNodeCustomHwInputPortFetchEngine = 1;

static const UINT32 ChiNodeCustomHwOutputPortPixel      = 0;
static const UINT32 ChiNodeCustomHwOutputPortMeta       = 1;
static const UINT32 ChiNodeCustomHwOutputPortRDI        = 2;

// This is an example
static const UINT32 ChiNodeCustomOutputWMPixel      = 0x3000;
static const UINT32 ChiNodeCustomOutputWMRDI        = 0x3001;
static const UINT32 ChiNodeCustomOutputWMMeta       = 0x3002;

// NOWHINE FILE NC004c: Things outside the Camx namespace should be prefixed with Camx/CSL

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Chi node structure for Chi interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiCustomHwNode
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
    /// FlushRequest
    ///
    /// @brief  Clear up node's internal resources for a request
    ///
    /// @param  pFlushRequestInfo Pointer to a structure that defines the information required for flushing a request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FlushRequest(
        CHINODEFLUSHREQUESTINFO* pFlushRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ComputeResponseTime
    ///
    /// @brief  Compute flush response time
    ///
    /// @param  pInfo Pointer to a structure that defines the information required for calculating and storing response time.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ComputeResponseTime(
        CHINODERESPONSEINFO* pInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareCSLHwInfo
    ///
    /// @brief  Fill CSL Hw Information
    ///
    /// @param  pInfo      Pointer to a structure that defines the information required for CSL and Hw.
    /// @param  pPortInfo  Pointer to a structure that defines the input and output port information
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PrepareCSLHwInfo(
        CHICSLHWINFO*    pInfo,
        CHINODEPORTINFO* pPortInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillHWIOConfig
    ///
    /// @brief  Fill Hw IO Information
    ///
    /// @param  pProcessHWData  Pointer to a structure that defines the information required in HW for processing a request.
    /// @param  pInfo           Pointer to a structure that defines the information required for processing a request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FillHWIOConfig(
        CHINODEFILLHWDATA*         pProcessHWData,
        CHINODEPROCESSREQUESTINFO* pInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCommandBufferData
    ///
    /// @brief  Fill Command Buffer Data
    ///
    /// @param  pProcessHWData  Pointer to a structure that defines the information required in HW for processing a request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FillCommandBufferData(
        CHINODEFILLHWDATA* pProcessHWData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiCustomHwNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiCustomHwNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiCustomHwNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiCustomHwNode();
private:
    ChiCustomHwNode(const ChiCustomHwNode&) = delete;               ///< Disallow the copy constructor
    ChiCustomHwNode& operator=(const ChiCustomHwNode&) = delete;    ///< Disallow assignment operator

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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsRealTime
    ///
    /// @brief  Method to query if node is part of realtime pipeline
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsRealTime() const
    {
        return m_nodeFlags.isRealTime;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBypassableNode
    ///
    /// @brief  Method to query if node is bypassable
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsBypassableNode() const
    {
        return m_nodeFlags.isBypassable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslateColorFilterPatternToISPPattern
    ///
    /// @brief  Helper method to map sensor color filter arrangement value to ISP Bayer pattern
    ///
    /// @param  colorFilterArrangementValue the sensor provided color filter arrangement value
    ///
    /// @return ISP Bayer pattern value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 TranslateColorFilterPatternToCustomHwNodePattern(
        camera_metadata_enum_android_sensor_info_color_filter_arrangement colorFilterArrangementValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupChannelResource
    ///
    /// @brief  Setup Channel Resource
    ///
    /// @param  pInputResource Pointer to the input resource to be filled
    /// @param  pInputPortInfo Pointer to input portID
    ///
    /// @return CDKResultSuccess if success or appropriate error code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetupChannelResource(
        struct cam_custom_in_port_info* pInputResource,
        UINT32*                         pInputPortInfo);

    CHIHANDLE                      m_hChiSession;                           ///< The Chi session handle
    UINT32                         m_nodeId;                                ///< The node's Id
    UINT32                         m_nodeCaps;                              ///< The selected node caps
    CHINODEFLAGS                   m_nodeFlags;                             ///< Node flags
    CHINODEIMAGEFORMAT             m_outputFormat;                          ///< The selected output format
    UINT64                         m_processedFrame;                        ///< The count for processed frame
    UINT64                         m_pipelineDelay;                         ///< Pipeline delay
    FLOAT                          m_currentFPS;                            ///< Current FPS
    CHINODEPORTINFO                m_portInfo;                              ///< Port Info
    struct cam_custom_resource     m_customResource[ChiNodeMaxInputPorts];  ///< Info required for HW Acquire
    struct cam_custom_in_port_info m_inputResource;                         ///< Input Port info for HW Acquire
};

#endif // CAMXCHINODECUSTOMHWNODE_H
