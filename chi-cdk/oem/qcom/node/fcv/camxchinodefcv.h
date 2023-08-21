////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodefcv.h
/// @brief Chi node for FCV
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODEFCV_H
#define CAMXCHINODEFCV_H

#include "chinode.h"
#include "camxchinodeutil.h"
#include "fastcv/fastcv.h"
#include "fastcv/fastcvExt.h"
#include "camxosutils.h"


typedef VOID* OSLIBRARYHANDLE;

enum RotationAngle
{
    Rotate0Degrees,   ///< Rotation of 0 Degrees in the Clockwise Direction
    Rotate90Degrees,  ///< Rotation of 90 Degrees in the Clockwise Direction
    Rotate180Degrees, ///< Rotation of 180 Degress in the Clockwise Direction
    Rotate270Degrees  ///< Rotation of 270 Degrees in the Clockwise Direction
};

// NOWHINE FILE NC004c: Things outside the Camx namespace should be prefixed with Camx/CSL

typedef void (*PFNREMOTEREGISTERBUF)(
    void* buf,
    int size,
    int fd);

typedef void (*PFNFCVSCALEDOWNMNU8)(
    const uint8_t* __restrict src,
    uint32_t srcWidth,
    uint32_t srcHeight,
    uint32_t srcStride,
    uint8_t* __restrict dst,
    uint32_t dstWidth,
    uint32_t dstHeight,
    uint32_t dstStride);

typedef void (*PFNFCVSCALEDOWNMNINTERLEAVEU8)(
    const uint8_t* __restrict src,
    uint32_t srcWidth,
    uint32_t srcHeight,
    uint32_t srcStride,
    uint8_t* __restrict dst,
    uint32_t dstWidth,
    uint32_t dstHeight,
    uint32_t dstStride);

typedef fcvStatus (*PFNFCVROTATEIMAGEU8)(
    const uint8_t* __restrict src,
    uint32_t srcWidth,
    uint32_t srcHeight,
    uint32_t srcStride,
    uint8_t* __restrict dst,
    uint32_t dstStride,
    uint32_t  degree);

typedef fcvStatus (*PFNFCVROTATEIMAGEINTERLEAVEU8)(
    const uint8_t* __restrict src,
    uint32_t srcWidth,
    uint32_t srcHeight,
    uint32_t srcStride,
    uint8_t* __restrict dst,
    uint32_t dstStride,
    uint32_t  degree);

typedef int (*PFNFCVSETOPERATIONMODEEXT)(
    fcvOperationModeExt mode);

typedef void (*PFNFCVCLEANUP)(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Chi node structure for Chi interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFCVNode
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
    /// ChiFCVNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFCVNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFCVNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFCVNode();
private:
    ChiFCVNode(const ChiFCVNode&) = delete;               ///< Disallow the copy constructor
    ChiFCVNode& operator=(const ChiFCVNode&) = delete;    ///< Disallow assignment operator

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RotateImage
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RotateImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput,
        RotationAngle       targetRotation);

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
    /// UpdateOrientation
    ///
    /// @brief  Update the orientation data from metadata in the pipeline
    ///
    /// @param requestId    The request Id to update orientation metadata from
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateOrientation(
        UINT64 requestId);

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

    OSLIBRARYHANDLE                    m_hfcvLib;                              ///< handle for fast CV library

    PFNREMOTEREGISTERBUF               m_pfnRemoteRegisterBuf;                 ///< Function pointer for remote_register_buf
    PFNFCVSCALEDOWNMNU8                m_pfnfcvScaleDownMNu8;                  ///< Function pointer for fcvScaleDownMNu8
    PFNFCVSCALEDOWNMNINTERLEAVEU8      m_pfnfcvScaleDownMNInterleaveu8;        ///< Function pointer for fcvScaleDownMNInterleaveu8
    PFNFCVROTATEIMAGEU8                m_pfnfcvRotateImageu8;                  ///< Function pointer for fcvRotateImageu8
    PFNFCVROTATEIMAGEINTERLEAVEU8      m_pfnfcvRotateImageInterleavedu8;       ///< Function pointer for fcvRotateImageInterleavedu8
    PFNFCVSETOPERATIONMODEEXT          m_pfnfcvSetOperationModeExt;            ///< Function pointer for fcvSetOperationModeExt
    PFNFCVCLEANUP                      m_pfnfcvCleanUp;                        ///< Function pointer for fcvCleanUp

    CHIHANDLE           m_hChiSession;  ///< The Chi session handle
    UINT32              m_nodeId;       ///< The node's Id
    UINT32              m_nodeCaps;     ///< The selected node caps
    CHINODEIMAGEFORMAT  m_format;       ///< The selected format
    UINT32              m_fullOutputDimensions[2]; ///< The output width x height dimension
    RotationAngle       m_currentRotation;  ///< If fcv rotation, use this to pass as metadata to dependant nodes

    UINT64              m_processedFrame;   ///< The count for processed frame
};
#endif // CAMXCHINODEFCV_H
