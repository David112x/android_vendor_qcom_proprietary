////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcmdbuffer.h
/// @brief CmdBuffer class declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCMDBUFFER_H
#define CAMXCMDBUFFER_H

#include "camxpacketresource.h"

CAMX_NAMESPACE_BEGIN

/// @brief Command type constants reflecting CSL command types
enum class CmdType: UINT32
{
    Invalid = 0,    ///< Invalid command type
    CDMDMI,         ///< DMI buffer (new format used in Titan)
    CDMDMI16,       ///< DMI buffer for 16-bit elements
    CDMDMI32,       ///< DMI buffer for 32-bit elements
    CDMDMI64,       ///< DMI buffer for 64-bit elements
    CDMDirect,      ///< Direct command buffer
    CDMIndirect,    ///< Indirect command buffer
    I2C,            ///< I2C command buffer
    FW,             ///< Firmware command buffer
    Generic,        ///< Generic command buffer
    Legacy          ///< Legacy blob
};

///@ brief Parameters needed for command objects. This is from a client's perspective (so should not contain
///        implementation-dependent parameters. Also, only immutable parameters that are only set once on construction.
struct CmdParams
{
    CmdType type;               ///< Command type constants reflecting CSL command types
    union
    {
        struct
        {
            UINT32  enableAddrPatching          : 1;  ///< Setting this bit indicates that indirect buffers should be tracked.
            UINT32  mustInlineIndirectBuffers   : 1;  ///< Setting this bit will cause inlining of all indirect buffers
            UINT32  reserved                    : 30;
        };
        UINT32 flags;
    };
    UINT32  maxNumNestedAddrs;  ///< Maximum number of nested addresses that will be added to this command buffer
};

class CmdBuffer;

/// @brief  Nested address info
struct NestedAddrInfo
{
    UINT32          dstOffset;      ///< Byte offset within destination command buffer where the physical address is written
    BOOL            isCmdBuffer;    ///< TRUE if the source address is a command buffer, FALSE if it's a handle/offset
    UINT32          srcOffset;      ///< Offset into the source buffer
    union
    {
        CmdBuffer*      pCmdBuffer; ///< Memory handle of the embedded buffer
        CSLMemHandle    hSrcBuffer; ///< Memory handle to source buffer
    };
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief
///     Class that implements the CAMX command buffer. This is designed to accommodate various kinds of command buffers without
///     needing to extend (through subclassing). The assumption is that There is a small set of command buffer "formats" that
///     should remain backward-compatible and for which composition logic is captured in here. This implies new command buffer
///     formats require adding new methods to this class; that may or may not be achieved through subclassing.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CmdBuffer final: public PacketResource
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create an instance of CmdBuffer.
    ///
    /// @param  pParams         Pointer to command parameters
    /// @param  pBufferInfo     Pointer to CSL-allocated memory
    /// @param  offset          Start offset in the region of the memory hMem represents
    /// @param  size            Size of the memory region this resource can assume
    /// @param  ppCmdBufferOut  Address where the create packet is returned
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        const CmdParams*        pParams,
        const CSLBufferInfo*    pBufferInfo,
        SIZE_T                  offset,
        SIZE_T                  size,
        CmdBuffer**             ppCmdBufferOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroys this instance of CmdBuffer.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Reset
    ///
    /// @brief  Reset clears the current state of the buffer and prepares for new command composition. The internal resource is
    ///         NOT released.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Reset();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentWriteAddr
    ///
    /// @brief  Get a pointer to the current command in the buffer (that will be written next).
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID* GetCurrentWriteAddr()
    {
        return static_cast<UINT32*>(GetHostAddr()) + m_resourceUsedDwords;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BeginCommands
    ///
    /// @brief  Reserve numDwords in the command buffer and return the pointer to the beginning. The effects of this function
    ///         will be overwritten if called again before a Commit.
    ///
    /// @param  numDwordsToReserve  Number of dwords
    ///
    /// @return Pointer to the beginning of the reserved area or NULL if not available
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* BeginCommands(
        UINT32 numDwordsToReserve);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CommitCommands
    ///
    /// @brief  Will commit the last reservation made in BeginCommands and move the current pointer to point to the available
    ///         space following.
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CommitCommands();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CancelCommands
    ///
    /// @brief  Will cancel the previous reservation made in BeginCommands
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CancelCommands();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetType
    ///
    /// @brief  Get the type of this command buffer.
    ///
    /// @return CmdType The type of the command buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CmdType GetType()
    {
        return m_type;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetType
    ///
    /// @brief  Set the type of this command buffer ()
    ///
    /// @param  type A value from CmdType enum to specify what the buffer is intended for
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetType(
        CmdType type)
    {
        m_type = type;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetResourceUsedDwords
    ///
    /// @brief  Get the current length of valid commands in the buffer.
    ///
    /// @return UINT32 number of dwords written so far
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT32 GetResourceUsedDwords()
    {
        return m_resourceUsedDwords;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumDwordsAvailable
    ///
    /// @brief  Get the remaining capacity of the buffer in dwords.
    ///
    /// @return UINT32 number of dwords that can be written before the buffer is full
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetNumDwordsAvailable()
    {
        return static_cast<UINT32>(GetMaxLength() / sizeof(UINT32)) - GetResourceUsedDwords();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCmdBufferDesc
    ///
    /// @brief  Return the command buffer descriptor that can be encoded in the packet to be submitted to CSL.
    ///
    /// @param  pCmdDescOut Pointer to the command buffer that can be encoded in the packet to be submitted to CSL.
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCmdBufferDesc(
        CSLCmdMemDesc* pCmdDescOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMetadata
    ///
    /// @brief  Set command buffer metadata
    ///
    /// @param  metadata    Packet-specific metadata between node and command processor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetMetadata(
        UINT32 metadata)
    {
        m_metadata = metadata;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadata
    ///
    /// @brief  Get metadata
    ///
    /// @return metadata
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetMetadata()
    {
        return m_metadata;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddNestedCmdBufferInfo
    ///
    /// @brief  Add info for an embedded buffer
    ///
    /// @param  dstOffset   Offset within the destination command buffer
    /// @param  pCmdBuffer  Source command buffer whose address is patched
    /// @param  srcOffset   Offset within the source command buffer
    ///
    /// @return result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddNestedCmdBufferInfo(
        UINT32      dstOffset,
        CmdBuffer*  pCmdBuffer,
        UINT32      srcOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddNestedBufferInfo
    ///
    /// @brief  Add info for an embedded buffer
    ///
    /// @param  dstOffset   Offset within destination command buffer
    /// @param  hMem        Source buffer's memory handle
    /// @param  srcOffset   Offset into the source buffer
    ///
    /// @return result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddNestedBufferInfo(
        UINT32          dstOffset,
        CSLMemHandle    hMem,
        UINT32          srcOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNestedAddrInfo
    ///
    /// @brief  Get embedded buffer info
    ///
    /// @return Nested buffer info
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE NestedAddrInfo* GetNestedAddrInfo()
    {
        return m_pNestedBuffersInfo;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumNestedAddrInfo
    ///
    /// @brief  Get number of an embedded buffer info
    ///
    /// @return Number of nested buffer info
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetNumNestedAddrInfo()
    {
        return m_numNestedBuffers;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPatchingEnabled
    ///
    /// @brief  Specifies if patching of indirect addresses is enabled.
    ///
    /// @return TRUE if patching is enabled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPatchingEnabled()
    {
        return m_params.enableAddrPatching;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MustInlineIndirectBuffers
    ///
    /// @brief  Indirect buffers need to be inlined.
    ///
    /// @return TRUE if indirect buffers must be inlined, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL MustInlineIndirectBuffers()
    {
        return m_params.mustInlineIndirectBuffers;
    }

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CmdBuffer
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CmdBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CmdBuffer
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~CmdBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Method to initialize CmdBuffer object.
    ///
    /// @param  pParams     Parameters needed to specify a CmdBuffer object.
    /// @param  pBufferInfo Pointer to CSL-allocated memory
    /// @param  offset      Start offset in the region of the memory hMem represents
    /// @param  size        Size of the memory region this resource can assume
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        const CmdParams*        pParams,
        const CSLBufferInfo*    pBufferInfo,
        SIZE_T                  offset,
        SIZE_T                  size);

    UINT32                  m_resourceUsedDwords;   ///< The (valid) length of the commands in DWORDs
    CmdType                 m_type;                 ///< Type of the packet
    UINT32                  m_pendingDwords;        ///< Number of dwords that will be be recorded on commit
    UINT32                  m_metadata;             ///< Metadata
    CmdParams               m_params;               ///< Command buffer params
    UINT32                  m_numNestedBuffers;     ///< Number of immediately-nested command buffers
    NestedAddrInfo*         m_pNestedBuffersInfo;   ///< Array of immediately-nested command buffers

private:
    CmdBuffer(const CmdBuffer&)              = delete;    // Disallow the copy constructor.
    CmdBuffer& operator=(const CmdBuffer&)   = delete;    // Disallow assignment operator.
};

CAMX_NAMESPACE_END

#endif // CAMXCMDBUFFER_H
