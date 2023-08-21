////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodememcpy.h
/// @brief Chi node for memcpy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODEMEMCPY_H
#define CAMXCHINODEMEMCPY_H

#include "chinode.h"
#include "camxchinodeutil.h"

#define  NUMBER_OF_OUTPUT_PORTS 5

// NOWHINE FILE NC004c: Things outside the Camx namespace should be prefixed with Camx/CSL

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Chi node structure for Chi interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiMemCpyNode
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
    /// @param  pInfo Pointer to a structure that defines the information required for calculating response time.
    ///
    /// @return worst-case response time in ms.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ComputeResponseTime(
        CHINODERESPONSEINFO* pInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiMemCpyNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiMemCpyNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiMemCpyNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiMemCpyNode();
private:
    ChiMemCpyNode(const ChiMemCpyNode&) = delete;               ///< Disallow the copy constructor
    ChiMemCpyNode& operator=(const ChiMemCpyNode&) = delete;    ///< Disallow assignment operator

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

    CHIHANDLE           m_hChiSession;                            ///< The Chi session handle
    UINT32              m_nodeId;                                 ///< The node's Id
    UINT32              m_nodeCaps;                               ///< The selected node caps
    CHINODEFLAGS        m_nodeFlags;                              ///< Node flags
    CHINODEIMAGEFORMAT  m_format;                                 ///< The selected format
    BOOL                m_isSinkNoBuffer[NUMBER_OF_OUTPUT_PORTS]; ///< Whether respective output port is sinkNoBuffer

    UINT64              m_processedFrame;                         ///< The count for processed frame
    UINT                m_inducedSleepTime;                       ///< Induce extra sleep in milliseconds
};
#endif // CAMXCHINODEMEMCPY_H
