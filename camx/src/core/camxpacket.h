////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpacket.h
/// @brief Packet class declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-275) Create equivalent Camx definitions for CSL entities used in Camx packet/command classes

#ifndef CAMXPACKET_H
#define CAMXPACKET_H

#include "camxformats.h"
#include "camxlist.h"
#include "camxpacketdefs.h"
#include "camxpacketresource.h"
#include "camxutils.h"

CAMX_NAMESPACE_BEGIN

class CmdBuffer;
class Hashmap;
class ImageBuffer;

///@ brief Parameters needed to specify a packet object. This is from a client's perspective (so should not contain
///        implementation-dependent parameters. Also, only immutable parameters that are only set once on construction.
struct PacketParams
{
    union
    {
        struct
        {
            UINT32  enableAddrPatching   : 1;
            UINT32  reserved             : 31;
        };
        UINT32 flags;
    };
    UINT32  maxNumCmdBuffers;   ///< Maximum number of command buffer references that will be added to this packet
    UINT32  maxNumIOConfigs;    ///< Maximum number of IO configurations that will be added to this packet
    UINT32  maxNumPatches;      ///< Maximum number of patches that will be added to this packet
};

///@ brief Define the framedrop pattern/period and subsample pattern/period
struct FrameSubsampleConfig
{
    UINT32 frameDropPattern;   ///< Frame Drop Pattern
    UINT32 frameDropPeriod;    ///< Frame Drop Period
    UINT32 subsamplePattern;   ///< Subsample Pattern
    UINT32 subsamplePeriod;    ///< Subsample Period
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the packet behavior and encapsulates the Packet structures. It is indented to provide the core
///        interface for composing packets. Special packet classes should extend this class.
///        This class wraps the underlying CSL packet structures. It is intended that the Packet class wrap a memory area
///        provided to it and allow clients to create and retrieve a valid CSLPacketHeader from it. The lifetime of the memory
///        (which may or may not reside on shared memory) is not necessarily tied to the lifetime of Packet object.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Packet final: public PacketResource
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create an instance of Packet.
    ///
    /// @param  pParams     Pointer to packet parameters
    /// @param  pBufferInfo Pointer to CSL-allocated memory
    /// @param  offset      Start offset in the region of the memory hMem represents
    /// @param  size        Size of the memory region this resource can assume
    /// @param  ppPacketOut Address where the create packet is returned
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        const PacketParams*     pParams,
        const CSLBufferInfo*    pBufferInfo,
        SIZE_T                  offset,
        SIZE_T                  size,
        Packet**                ppPacketOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroys this instance of Packet.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculatePacketSize
    ///
    /// @brief  Calculate the size of the CSL packet with the given parameters.
    ///
    /// @param  pParams Parameters needed to specify a packet object.
    ///
    /// @return Size of the memory for the packet
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 CalculatePacketSize(
        const PacketParams* pParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetOpcode
    ///
    /// @brief  Set the opcode of the packet.
    ///
    /// @param  device    Device Id this packet is for (more like a type)
    /// @param  opcode    Opcode of the packet
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetOpcode(
        CSLDeviceType   device,
        UINT32          opcode)
    {
        m_pPacketHeader->opcode = MakePacketOpcode(device, opcode);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CommitPacket
    ///
    /// @brief  Commits the state of the packet and prepares for submission
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CommitPacket();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLPacketHeader
    ///
    /// @brief  Returns a pointer to a CSLPacketHeader structure that can be submitted to CSL
    ///
    /// @return CSLPacketHeader Packet header
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CSLPacketHeader* GetCSLPacketHeader();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddCmdBufferReference
    ///
    /// @brief  Adds a command buffer object to the packet.
    ///
    /// @param  pCmdBuffer    Pointer to a command buffer object
    /// @param  pIndexOut     Pointer to a variable that will receive the index of the added command buffer
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddCmdBufferReference(
        CmdBuffer* pCmdBuffer,
        UINT32*    pIndexOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetKMDCmdBufferIndex
    ///
    /// @brief  Sets the index and offset of a command buffer that can be used by KMD for command composition.
    ///
    /// @param  cmdBufferIndex  Index of the command buffer in the packet
    /// @param  offset          Offset within the command buffer
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetKMDCmdBufferIndex(
        UINT32  cmdBufferIndex,
        UINT32  offset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddIOConfig
    ///
    /// @brief  Adds an IO configuration to the packet.
    ///
    /// @param  pImage           Pointer to image buffer
    /// @param  portResourceId   Port resource identifier
    /// @param  direction        CSLIODirectionInput or CSLIODirectionOutput
    /// @param  phFences         Array of fence handles
    /// @param  numFences        Number of fences in the fence array
    /// @param  pIndexOut        Pointer to a variable that will receive the index of the added IO configuration
    /// @param  pSubsampleConfig Pointer to Frame Subsample Configuration Setting
    /// @param  batchFrameIndex  index of frame in batch
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NOWHINE NC008: complains about phFences
    CamxResult AddIOConfig(
        ImageBuffer*          pImage,
        UINT32                portResourceId,
        CSLIODirection        direction,
        const CSLFence*       phFences,
        UINT32                numFences,
        UINT32*               pIndexOut,
        FrameSubsampleConfig* pSubsampleConfig,
        UINT                  batchFrameIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddAddrPatch
    ///
    /// @brief  Sets the command buffer index and offset where the physical address of the given IO config will be patched.
    ///
    /// @param  hDst        Handle to buffer that needs to be patched
    /// @param  dstOffset   Byte offset in the buffer that needs to be patched
    /// @param  hSrc        Handle to be used to calculate the address
    /// @param  srcOffset   Offset that needs to be added to the hSrc's base address to get the final address
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddAddrPatch(
        CSLMemHandle    hDst,
        UINT32          dstOffset,
        CSLMemHandle    hSrc,
        UINT32          srcOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Reset
    ///
    /// @brief  Reset clears the current state of the resource to prepare for reuse.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Reset();

protected:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Packet
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Packet();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Packet
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~Packet();

    CSLPacketHeader*    m_pPacketHeader         = NULL;     ///< Pointer to the beginning of CSL packet structure
    UINT32              m_maxNumCmdBufferDesc   = 0;        ///< Maximum number of command buffers object can hold
    UINT32              m_maxNumIOConfig        = 0;        ///< Maximum number of IO configurations this object can hold
    UINT32              m_maxNumPatches         = 0;        ///< Maximum number of patches this object can hold
    BOOL                m_patchingEnable        = FALSE;    ///< Indicates if patching is enabled
    UINT32              m_dataSize              = 0;        ///< The size of the data section in the packet

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Method to initialize Packet object.
    ///
    /// @param  pParams     Parameters needed to specify a packet object.
    /// @param  pBufferInfo Pointer to CSL-allocated memory
    /// @param  offset      Start offset in the region of the memory hMem represents
    /// @param  size        Size of the memory region this resource can assume
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        const PacketParams*     pParams,
        const CSLBufferInfo*    pBufferInfo,
        SIZE_T                  offset,
        SIZE_T                  size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLPacket
    ///
    /// @brief  Helper function to get a pointer to the underlying CSLPacket
    ///
    /// @param  pPacketHeader   CSL packet header pointer
    ///
    /// @return Pointer to the CSLPacket
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static CSLPacket* GetCSLPacket(
        CSLPacketHeader* pPacketHeader)
    {
        return reinterpret_cast<CSLPacket*>(pPacketHeader);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCommandBufferDescs
    ///
    /// @brief  Helper function to get a pointer to the beginning of the command descriptor list of a packet
    ///
    /// @param  pPacket CSL packet pointer
    ///
    /// @return Pointer to the command descriptor region of the packet
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static CSLCmdMemDesc* GetCommandBufferDescs(
        CSLPacket* pPacket)
    {
        return reinterpret_cast<CSLCmdMemDesc*>(Utils::VoidPtrInc(pPacket->data, pPacket->cmdBuffersOffset));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIOConfigs
    ///
    /// @brief  Helper function to get a pointer to the beginning of the IO configs of a packet
    ///
    /// @param  pPacket CSL packet pointer
    ///
    /// @return Pointer to the IO configuration region of the packet
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static CSLBufferIOConfig* GetIOConfigs(
        CSLPacket* pPacket)
    {
        return reinterpret_cast<CSLBufferIOConfig*>(Utils::VoidPtrInc(pPacket->data, pPacket->ioConfigsOffset));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAddrPatchset
    ///
    /// @brief  Helper function to get a pointer to the beginning of the address patch list
    ///
    /// @param  pPacket CSL packet pointer
    ///
    /// @return Pointer to the address patch region of the packet
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static CSLAddrPatch* GetAddrPatchset(
        CSLPacket* pPacket)
    {
        return reinterpret_cast<CSLAddrPatch*>(Utils::VoidPtrInc(pPacket->data, pPacket->patchsetOffset));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateDataPayloadSize
    ///
    /// @brief  Calculate the size of data section of the packet based on the packet parameters
    ///
    /// @param  pParams Parameters needed to specify a packet object.
    ///
    /// @return Size of the data section
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static UINT32 CalculateDataPayloadSize(
        const PacketParams* pParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculatePatchsetOffset
    ///
    /// @brief  Calculate the offset of the patchset data in the data section
    ///
    /// @param  pParams Parameters needed to specify a packet object.
    ///
    /// @return Size of the data section
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static UINT32 CalculatePatchsetOffset(
        const PacketParams* pParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateIOConfigsOffset
    ///
    /// @brief  Calculate the offset of the IO configuration data in the data section
    ///
    /// @param  pParams Parameters needed to specify a packet object.
    ///
    /// @return Size of the data section
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static UINT32 CalculateIOConfigsOffset(
        const PacketParams* pParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateCmdBufferDescsOffset
    ///
    /// @brief  Calculate the offset of the command buffer descriptors in the data section
    ///
    /// @param  pParams Parameters needed to specify a packet object.
    ///
    /// @return Size of the data section
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static UINT32 CalculateCmdBufferDescsOffset(
        const PacketParams* pParams);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigUBWCPlane
    ///
    /// @brief  Helper function to program a UBWC CSL plane
    ///
    /// @param  pCSLPlane       Pointer to the CSL plane being programmed
    /// @param  pYUVPlane       Inputs from the corresponding YUV plane
    /// @param  pFormat         Pointer to image format data structure
    /// @param  planeIndex      Y or UV plane index
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ConfigUBWCPlane(
        CSLPlane*          pCSLPlane,
        const YUVFormat*   pYUVPlane,
        const ImageFormat* pFormat,
        UINT               planeIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LookupUAPIFormat
    ///
    /// @brief  Helper function to look up UAPI interface values
    ///
    /// @param  pFormat Pointer to the image format
    ///
    /// @return UAPI interface format corresponding to UMD CSL format
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 LookupUAPIFormat(
        const ImageFormat* pFormat);

    Packet(const Packet&)               = delete;  // Disallow the copy constructor.
    Packet& operator=(const Packet&)    = delete;  // Disallow assignment operator.

    BOOL                        m_committed;             ///< Indicates if the packet's state is committed
    LightweightDoublyLinkedList m_patchList;             ///< This is used as a work list in patching
    Hashmap*                    m_pPatchGraphVisitedMap; ///< This is used to track the visited nodes in patch graph
};

CAMX_NAMESPACE_END

#endif // CAMXPACKET_H
