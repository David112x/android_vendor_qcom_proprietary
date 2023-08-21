////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpacket.cpp
/// @brief Packet class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcmdbuffer.h"
#include "camxcslispdefs.h"
#include "camxformats.h"
#include "camxincs.h"
#include "camxhashmap.h"
#include "camximagebuffer.h"
#include "camximageformatutils.h"
#include "camxlist.h"
#include "camxmem.h"
#include "camxpacket.h"

CAMX_NAMESPACE_BEGIN

// When new command types are added, we need to keep Camx side definitions consistent with CSL's.
CAMX_STATIC_ASSERT(CSLCmdTypeInvalid        == static_cast<UINT32>(CmdType::Invalid));
CAMX_STATIC_ASSERT(CSLCmdTypeCDMDMI         == static_cast<UINT32>(CmdType::CDMDMI));
CAMX_STATIC_ASSERT(CSLCmdTypeCDMDMI16       == static_cast<UINT32>(CmdType::CDMDMI16));
CAMX_STATIC_ASSERT(CSLCmdTypeCDMDMI32       == static_cast<UINT32>(CmdType::CDMDMI32));
CAMX_STATIC_ASSERT(CSLCmdTypeCDMDMI64       == static_cast<UINT32>(CmdType::CDMDMI64));
CAMX_STATIC_ASSERT(CSLCmdTypeCDMDirect      == static_cast<UINT32>(CmdType::CDMDirect));
CAMX_STATIC_ASSERT(CSLCmdTypeCDMIndirect    == static_cast<UINT32>(CmdType::CDMIndirect));
CAMX_STATIC_ASSERT(CSLCmdTypeI2C            == static_cast<UINT32>(CmdType::I2C));
CAMX_STATIC_ASSERT(CSLCmdTypeFW             == static_cast<UINT32>(CmdType::FW));
CAMX_STATIC_ASSERT(CSLCmdTypeGeneric        == static_cast<UINT32>(CmdType::Generic));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Packet::Create(
    const PacketParams*     pParams,
    const CSLBufferInfo*    pBufferInfo,
    SIZE_T                  offset,
    SIZE_T                  size,
    Packet**                ppPacketOut)
{
    CamxResult  result  = CamxResultSuccess;
    Packet*     pPacket = NULL;

    if (NULL == ppPacketOut)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "ppPacketOut is NULL.");
        result = CamxResultEInvalidArg;
    }
    else
    {
        pPacket = CAMX_NEW Packet();
        if (NULL == pPacket)
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory; cannot create Packet");
            result = CamxResultENoMemory;
        }
        else
        {
            result = pPacket->Initialize(pParams, pBufferInfo, offset, size);
        }
    }

    if (CamxResultSuccess == result)
    {
        *ppPacketOut = pPacket;
    }
    else
    {
        if (NULL != pPacket)
        {
            CAMX_DELETE pPacket;
            pPacket = NULL;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Packet::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::Packet
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Packet::Packet()
    : m_committed(FALSE)
    , m_pPatchGraphVisitedMap(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::~Packet
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Packet::~Packet()
{
    if (NULL != m_pPatchGraphVisitedMap)
    {
        m_pPatchGraphVisitedMap->Destroy();
        m_pPatchGraphVisitedMap = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Packet::Initialize(
    const PacketParams*     pParams,
    const CSLBufferInfo*    pBufferInfo,
    SIZE_T                  offset,
    SIZE_T                  size)
{
    CamxResult result = PacketResource::Initialize(pBufferInfo, offset, size);

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(NULL != pParams);

        m_pPacketHeader     = static_cast<CSLPacketHeader*>(Utils::VoidPtrInc(pBufferInfo->pVirtualAddr, offset));
        CSLPacket* pPacket  = GetCSLPacket(m_pPacketHeader);

        CAMX_ASSERT(NULL != pPacket);

        Utils::Memset(pPacket, 0, size);

        m_maxNumCmdBufferDesc   = pParams->maxNumCmdBuffers;
        m_maxNumIOConfig        = pParams->maxNumIOConfigs;
        m_maxNumPatches         = pParams->maxNumPatches;
        m_patchingEnable        = (1 == pParams->enableAddrPatching) ? TRUE : FALSE;

        if (TRUE == m_patchingEnable)
        {
            CAMX_ASSERT(m_maxNumPatches > 0);

            HashmapParams mapParams = {0};
            mapParams.maxNumBuckets = m_maxNumPatches * 2; // To get a final load factor of .5
            m_pPatchGraphVisitedMap = Hashmap::Create(&mapParams);

            if (NULL == m_pPatchGraphVisitedMap)
            {
                result = CamxResultENoMemory;
                CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory");
            }
        }
        else
        {
            m_maxNumPatches = 0;
        }

        if (CamxResultSuccess == result)
        {
            m_dataSize                  = CalculateDataPayloadSize(pParams);
            pPacket->header.size        = static_cast<UINT32>(size);
            pPacket->cmdBuffersOffset   = CalculateCmdBufferDescsOffset(pParams);
            pPacket->ioConfigsOffset    = CalculateIOConfigsOffset(pParams);
            pPacket->patchsetOffset     = CalculatePatchsetOffset(pParams);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::CalculatePacketSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Packet::CalculatePacketSize(
    const PacketParams* pParams)
{
    CAMX_ASSERT(NULL != pParams);

    // We subtract the size of data[1] at the end of the CSLPacket since it's accounted for with the memory descriptors and
    // buffer IO configs.
    return (sizeof(CSLPacket) - sizeof(UINT64) + CalculateDataPayloadSize(pParams));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::CalculateDataPayloadSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Packet::CalculateDataPayloadSize(
    const PacketParams* pParams)
{
    CAMX_ASSERT(NULL != pParams);

    return (pParams->maxNumCmdBuffers   * sizeof(CSLCmdMemDesc))        +
           (pParams->maxNumIOConfigs    * sizeof(CSLBufferIOConfig))    +
           (pParams->maxNumPatches      * sizeof(CSLAddrPatch));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::CalculatePatchsetOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Packet::CalculatePatchsetOffset(
    const PacketParams* pParams)
{
    CAMX_ASSERT(NULL != pParams);

    return (pParams->maxNumCmdBuffers * sizeof(CSLCmdMemDesc)) + (pParams->maxNumIOConfigs * sizeof(CSLBufferIOConfig));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::CalculateIOConfigsOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Packet::CalculateIOConfigsOffset(
    const PacketParams* pParams)
{
    CAMX_ASSERT(NULL != pParams);

    return (pParams->maxNumCmdBuffers * sizeof(CSLCmdMemDesc));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::CalculateCmdBufferDescsOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Packet::CalculateCmdBufferDescsOffset(
    const PacketParams* pParams)
{
    CAMX_UNREFERENCED_PARAM(pParams);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::AddCmdBufferReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Packet::AddCmdBufferReference(
    CmdBuffer*  pCmdBuffer,
    UINT32*     pIndexOut)
{
    CamxResult result   = CamxResultSuccess;
    CSLPacket* pPacket  = GetCSLPacket(m_pPacketHeader);

    CAMX_ASSERT(NULL != pPacket);

    if (NULL == pPacket)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "pPacket is NULL");
    }
    else if ((NULL == pCmdBuffer) || (pPacket->numCmdBuffers >= m_maxNumCmdBufferDesc))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid command buffer arguments");
    }
    else if ((TRUE == m_patchingEnable) && ((pCmdBuffer->GetNumNestedAddrInfo() + pPacket->numPatches) > m_maxNumPatches))
    {
        result = CamxResultEOutOfBounds;
        CAMX_LOG_ERROR(CamxLogGroupUtils,
                       "No more patch room in packet: Nested buffers (%d), used patches (%d), max (%d)",
                       pCmdBuffer->GetNumNestedAddrInfo(),
                       pPacket->numPatches,
                       m_maxNumPatches);
    }
    else
    {
        CSLCmdMemDesc* pCommandBufferDescs = GetCommandBufferDescs(pPacket);

        CAMX_ASSERT(NULL != pCommandBufferDescs);

        pCmdBuffer->GetCmdBufferDesc(pCommandBufferDescs + pPacket->numCmdBuffers);

        if (NULL != pIndexOut)
        {
            *pIndexOut = pPacket->numCmdBuffers;
        }

        // If patching is enabled
        if (TRUE == m_patchingEnable)
        {
            CAMX_ASSERT(NULL != m_pPatchGraphVisitedMap);

            LDLLNode* pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));

            if (NULL != pNode)
            {
                pNode->pData = pCmdBuffer;
                m_patchList.InsertToTail(pNode);

                m_pPatchGraphVisitedMap->Clear();
                // Use the map as a set (the value is really the same as the key) to track "visited" status.
                result = m_pPatchGraphVisitedMap->Put(pCmdBuffer, pCmdBuffer);
                CAMX_ASSERT(CamxResultSuccess == result);
            }
            else
            {
                result = CamxResultENoMemory;
            }

            while ((CamxResultSuccess == result) && (0 < m_patchList.NumNodes()))
            {
                pNode = m_patchList.RemoveFromHead();

                CmdBuffer* pParentBuffer = NULL;
                if (NULL != pNode)
                {
                    pParentBuffer = reinterpret_cast<CmdBuffer*>(pNode->pData);
                    if (NULL != pParentBuffer)
                    {
                        NestedAddrInfo* pNestedCmdInfo = (NULL != pParentBuffer) ? pParentBuffer->GetNestedAddrInfo() : NULL;
                        if ((0 < pParentBuffer->GetNumNestedAddrInfo()) && (NULL != pNestedCmdInfo))
                        {
                            for (UINT32 i = 0; i < pParentBuffer->GetNumNestedAddrInfo(); i++)
                            {
                                CSLMemHandle    hSrcBuffer = CSLInvalidHandle;
                                UINT32          srcOffset  = pNestedCmdInfo[i].srcOffset;
                                VOID*           pVisited   = NULL;

                                if (1 == pNestedCmdInfo[i].isCmdBuffer)
                                {
                                    CmdBuffer* pChildBuffer = pNestedCmdInfo[i].pCmdBuffer;
                                    hSrcBuffer              = pChildBuffer->GetMemHandle();
                                    srcOffset               += static_cast<UINT32>(pChildBuffer->GetOffset());

                                    if (CamxResultENoSuch == m_pPatchGraphVisitedMap->Get(pChildBuffer, &pVisited))
                                    {
                                        // If no node to reuse (from m_patchList.RemoveFromHead())
                                        if (NULL == pNode)
                                        {
                                            pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));

                                            if (NULL == pNode)
                                            {
                                                result = CamxResultENoMemory;
                                            }
                                        }

                                        if (NULL != pNode)
                                        {
                                            pNode->pData = pChildBuffer;
                                            m_patchList.InsertToTail(pNode);
                                            pNode  = NULL;

                                            result = m_pPatchGraphVisitedMap->Put(pChildBuffer, pChildBuffer);
                                            CAMX_ASSERT(CamxResultSuccess == result);
                                        }
                                    }
                                    else
                                    {
                                        CAMX_ASSERT(pChildBuffer == pVisited);
                                    }
                                }
                                else
                                {
                                    hSrcBuffer = pNestedCmdInfo[i].hSrcBuffer;
                                }

                                if (CamxResultSuccess == result)
                                {
                                    result = AddAddrPatch(pParentBuffer->GetMemHandle(),
                                        static_cast<UINT32>(pParentBuffer->GetOffset() + pNestedCmdInfo[i].dstOffset),
                                        hSrcBuffer,
                                        srcOffset);
                                }

                                if (CamxResultSuccess != result)
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupUtils,
                                        "Failed to add address patch (hParent: %d, offs: %d, hChild: %d, offs: %d)",
                                        pParentBuffer->GetMemHandle(),
                                        static_cast<UINT32>(pParentBuffer->GetOffset() + pNestedCmdInfo[i].dstOffset),
                                        hSrcBuffer,
                                        srcOffset);
                                    // Empty entire list
                                    pNode = m_patchList.RemoveFromHead();
                                    while (NULL != pNode)
                                    {
                                        CAMX_FREE(pNode);
                                        pNode = m_patchList.RemoveFromHead();
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupUtils, "pParentBuffer is NULL pointer");
                        result = CamxResultEInvalidPointer;
                        break;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupUtils, "pNode is NULL Pointer");
                    result = CamxResultEInvalidPointer;
                    break;
                }

                // Node was not reused
                if (NULL != pNode)
                {
                    CAMX_FREE(pNode);
                    pNode = NULL;
                }
            }
            CAMX_ASSERT(0 == m_patchList.NumNodes());
        }

        if (CamxResultSuccess == result)
        {
            pPacket->numCmdBuffers++;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::SetKMDCmdBufferIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Packet::SetKMDCmdBufferIndex(
    UINT32  cmdBufferIndex,
    UINT32  offset)
{
    CamxResult result   = CamxResultSuccess;
    CSLPacket* pPacket  = GetCSLPacket(m_pPacketHeader);

    CAMX_ASSERT(NULL != pPacket);

    if (NULL == pPacket)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "pPacket is NULL");
    }
    else if (cmdBufferIndex >= pPacket->numCmdBuffers)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid cmdBufferIndex (%u >= %u)", cmdBufferIndex, pPacket->numCmdBuffers);
    }
    else
    {
        pPacket->kmdCmdBufferIndex = cmdBufferIndex;
        pPacket->kmdCmdBufferOffset = offset;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::ConfigUBWCPlane
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Packet::ConfigUBWCPlane(
    CSLPlane*          pCSLPlane,
    const YUVFormat*   pYUVPlane,
    const ImageFormat* pFormat,
    UINT               planeIndex)
{
    UBWCPartialTileInfo UBWCPartialTileParam;

    CAMX_UNREFERENCED_PARAM(planeIndex);
    pCSLPlane->width          = pYUVPlane->width;
    pCSLPlane->height         = pYUVPlane->height;
    pCSLPlane->planeStride    = pYUVPlane->planeStride;
    pCSLPlane->sliceHeight    = pYUVPlane->sliceHeight;
    pCSLPlane->metadataStride = pYUVPlane->metadataStride;
    pCSLPlane->metadataSize   = pYUVPlane->metadataSize;

    switch (pFormat->format)
    {
        case Format::UBWCTP10:
            pCSLPlane->packerConfig = 0xb;
            break;
        case Format::UBWCNV12:
        case Format::UBWCNV12FLEX:
            pCSLPlane->packerConfig = 0xe;
            break;
        case Format::UBWCNV124R:
            pCSLPlane->packerConfig = 0xe;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid UBWC format: %d", pFormat->format);
            break;
    }

    const struct UBWCTileInfo* pTileInfo = ImageFormatUtils::GetUBWCTileInfo(pFormat);

    if (NULL != pTileInfo)
    {
        // partial tile information calculated assuming this full frame processing
        // dual ife this will be overwritten by ife node.
        ImageFormatUtils::GetUBWCPartialTileInfo(
            pTileInfo,
            &UBWCPartialTileParam,
            0,
            pFormat->width);

        pCSLPlane->tileConfig  |= (UBWCPartialTileParam.partialTileBytesLeft & 0x3F)  << 16;
        pCSLPlane->tileConfig  |= (UBWCPartialTileParam.partialTileBytesRight & 0x3F) << 23;


        pCSLPlane->hInit           = UBWCPartialTileParam.horizontalInit;    // 0, as we are programming entire frame
        pCSLPlane->vInit           = UBWCPartialTileParam.verticalInit;      // 0, as we are programming entire frame
        pCSLPlane->metadataOffset  = 0;                                      // no need to program this as of now
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "pTileInfo is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::LookupUAPIFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Packet::LookupUAPIFormat(
    const ImageFormat* pFormat)
{
    UINT32 format = 0;

    switch (pFormat->format)
    {
        case Format::YUV420NV12:
            format = ISPFormatNV21;
            break;

        case Format::YUV420NV21:
            format = ISPFormatNV12;
            break;

        case Format::UBWCTP10:
            format = ISPFormatUBWCTP10;
            break;

        case Format::UBWCP010:
            format = ISPFormatUBWCP010;
            break;

        case Format::UBWCNV12:
        case Format::UBWCNV12FLEX:
            format = ISPFormatUBWCNV12;
            break;

        case Format::UBWCNV124R:
            format = ISPFormatUBWCNV124R;
            break;

        case Format::PD10:
            format = ISPFormatPD10;
            break;

        case Format::RawMIPI8:
            format = ISPFormatMIPIRaw8;
            break;
        case Format::P010:
            format = ISPFormatPlain1610;
            break;

        case Format::RawMIPI:
            switch (pFormat->formatParams.rawFormat.bitsPerPixel)
            {
                case 6:
                    format = ISPFormatMIPIRaw6;
                    break;

                case 8:
                    format = ISPFormatMIPIRaw8;
                    break;

                case 10:
                    format = ISPFormatMIPIRaw10;
                    break;

                case 12:
                    format = ISPFormatMIPIRaw12;
                    break;

                case 14:
                    format = ISPFormatMIPIRaw14;
                    break;

                case 16:
                    format = ISPFormatMIPIRaw16;
                    break;

                case 20:
                    format = ISPFormatMIPIRaw20;
                    break;

                default:
                    CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported bits per pixel format for RawMIPI %d",
                                               pFormat->formatParams.rawFormat.bitsPerPixel);
                    break;
            }
            break;

        case Format::RawPlain16:
            switch (pFormat->formatParams.rawFormat.bitsPerPixel)
            {
                case 8:
                    format = ISPFormatPlain168;
                    break;

                case 10:
                    format = ISPFormatPlain1610;
                    break;

                case 12:
                    format = ISPFormatPlain1612;
                    break;

                case 14:
                    format = ISPFormatPlain1614;
                    break;

                case 16:
                    format = ISPFormatPlain1616;
                    break;

                default:
                    CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported bits per pixel format for RawPlain16 %d",
                                               pFormat->formatParams.rawFormat.bitsPerPixel);
                    break;
            }
            break;

        case Format::RawPlain64:
            format = ISPFormatPlain64;
            break;

        case Format::YUV420NV12TP10:
        case Format::YUV420NV21TP10:
        case Format::YUV422NV16TP10:
            format = ISPFormatTP10;
            break;

        case Format::Blob:
            format = ISPFormatPlain128;
            break;
        case Format::Y8:
            format = ISPFormatY;
            break;
        case Format::YUV422NV16:
        case Format::Jpeg:
        case Format::Y16:
        case Format::RawYUV8BIT:
        case Format::RawPrivate:
        case Format::RawMeta8BIT:
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported CSL format %d", pFormat->format);
            break;
    }

    return format;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::AddIOConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NOWHINE NC008: complains about phFences
CamxResult Packet::AddIOConfig(
    ImageBuffer*          pImage,
    UINT32                portResourceId,
    CSLIODirection        direction,
    const CSLFence*       phFences,
    UINT32                numFences,
    UINT32*               pIndexOut,
    FrameSubsampleConfig* pSubsampleConfig,
    UINT                  batchFrameIndex)
{
    CamxResult  result       = CamxResultSuccess;
    CSLPacket*  pPacket      = GetCSLPacket(m_pPacketHeader);
    SIZE_T      offset       = 0;
    UINT32      format       = 0;
    SIZE_T      metadataSize = 0;

    CAMX_ASSERT(NULL != pPacket);
    CAMX_ASSERT((numFences == 0) || (NULL != phFences));

    if (NULL == pPacket)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "pPacket is NULL");
    }
    else if (NULL == pImage)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "pImage is NULL");
    }
    else if (pPacket->numBufferIOConfigs >= m_maxNumIOConfig)
    {
        result = CamxResultEOutOfBounds;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "pPacket->numBufferIOConfigs >= m_maxNumIOConfig");
    }
    else
    {
        CSLBufferIOConfig* pIOConfigs = GetIOConfigs(pPacket);
        CAMX_ASSERT(NULL != pIOConfigs);

        const ImageFormat* pFormat = pImage->GetFormat();
        CAMX_ASSERT(NULL != pFormat);

        if ((NULL == pIOConfigs) || (NULL == pFormat))
        {
            result = CamxResultEInvalidState;
        }
        else
        {
            /// @todo (CAMX-1262): Remove Lookup of UAPI header values
            format = LookupUAPIFormat(pFormat);

            if (0 == format)
            {
                result = CamxResultEUnsupported;
            }
            else
            {
                if (TRUE == ImageFormatUtils::IsRAW(pFormat))
                {
                    pIOConfigs[pPacket->numBufferIOConfigs].portResourceId = portResourceId;

                    pIOConfigs[pPacket->numBufferIOConfigs].planes[0].planeStride =
                        pFormat->formatParams.rawFormat.stride;

                    pIOConfigs[pPacket->numBufferIOConfigs].planes[0].sliceHeight =
                        pFormat->formatParams.rawFormat.sliceHeight;

                    pIOConfigs[pPacket->numBufferIOConfigs].planes[0].height = pFormat->height;
                    pIOConfigs[pPacket->numBufferIOConfigs].planes[0].width  = pFormat->width;

                    result = pImage->GetPlaneCSLMemHandle(batchFrameIndex,
                                                          0,
                                                          &pIOConfigs[pPacket->numBufferIOConfigs].hMems[0],
                                                          &offset,
                                                          &metadataSize);

                    pIOConfigs[pPacket->numBufferIOConfigs].offsets[0] = static_cast<UINT32>(offset);
                    pIOConfigs[pPacket->numBufferIOConfigs].bitsPerPixel = pFormat->formatParams.rawFormat.bitsPerPixel;
                    pIOConfigs[pPacket->numBufferIOConfigs].colorFilterPattern =
                        static_cast<CSLColorFilterPattern>(pFormat->formatParams.rawFormat.colorFilterPattern);
                }
                else
                {
                    CAMX_ASSERT(pImage->GetNumberOfPlanes() <= CSLMaxNumPlanes);

                    for (UINT i = 0; i < pImage->GetNumberOfPlanes(); i++)
                    {
                        if (i < CSLMaxNumPlanes)
                        {
                            pIOConfigs[pPacket->numBufferIOConfigs].portResourceId = portResourceId;

                            if (TRUE == ImageFormatUtils::IsUBWC(pFormat->format))
                            {
                                // If UBWC, directly program the IOConfig to the Bus I/O
                                ConfigUBWCPlane(&pIOConfigs[pPacket->numBufferIOConfigs].planes[i],
                                                &pFormat->formatParams.yuvFormat[i],
                                                pFormat,
                                                i);
                            }
                            else
                            {
                                pIOConfigs[pPacket->numBufferIOConfigs].planes[i].height =
                                    pFormat->formatParams.yuvFormat[i].height;

                                pIOConfigs[pPacket->numBufferIOConfigs].planes[i].width =
                                    pFormat->formatParams.yuvFormat[i].width;

                                pIOConfigs[pPacket->numBufferIOConfigs].planes[i].planeStride =
                                    pFormat->formatParams.yuvFormat[i].planeStride;
                                pIOConfigs[pPacket->numBufferIOConfigs].planes[i].sliceHeight =
                                    pFormat->formatParams.yuvFormat[i].sliceHeight;
                            }

                            result = pImage->GetPlaneCSLMemHandle(batchFrameIndex,
                                                                  i,
                                                                  &pIOConfigs[pPacket->numBufferIOConfigs].hMems[i],
                                                                  &offset,
                                                                  &metadataSize);

                            // Currently, packet's use 32bit for offset and we don't expect anything larger.
                            pIOConfigs[pPacket->numBufferIOConfigs].offsets[i] = static_cast<UINT32>(offset);
                            CAMX_LOG_VERBOSE(CamxLogGroupChi, "plane %d, hMems = %d", i,
                                pIOConfigs[pPacket->numBufferIOConfigs].hMems[i]);
                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupChi, "Failed to get Mem handle");
                                break;
                            }
                        }
                    }
                }

                if (CamxResultSuccess == result)
                {
                    pIOConfigs[pPacket->numBufferIOConfigs].colorSpace          =
                        static_cast<CSLColorSpace>(pFormat->colorSpace);

                    pIOConfigs[pPacket->numBufferIOConfigs].rotation            = static_cast<CSLRotation>(pFormat->rotation);
                    pIOConfigs[pPacket->numBufferIOConfigs].format              = format;
                    pIOConfigs[pPacket->numBufferIOConfigs].direction           = direction;

                    if (NULL != pSubsampleConfig)
                    {
                        pIOConfigs[pPacket->numBufferIOConfigs].framedropPattern = pSubsampleConfig->frameDropPattern;
                        pIOConfigs[pPacket->numBufferIOConfigs].framedropPeriod  = pSubsampleConfig->frameDropPeriod;
                        pIOConfigs[pPacket->numBufferIOConfigs].subsamplePattern = pSubsampleConfig->subsamplePattern;
                        pIOConfigs[pPacket->numBufferIOConfigs].subsamplePeriod  = pSubsampleConfig->subsamplePeriod;
                    }
                    else
                    {
                        pIOConfigs[pPacket->numBufferIOConfigs].framedropPattern = 1;
                        pIOConfigs[pPacket->numBufferIOConfigs].framedropPeriod  = 0;
                        pIOConfigs[pPacket->numBufferIOConfigs].subsamplePattern = 1;
                        pIOConfigs[pPacket->numBufferIOConfigs].subsamplePeriod  = 0;
                    }

                    if (numFences > 0)
                    {
                        pIOConfigs[pPacket->numBufferIOConfigs].hSync       = phFences[0];
                    }

                    if (numFences > 1)
                    {
                        pIOConfigs[pPacket->numBufferIOConfigs].hEarlySync  = phFences[1];
                    }

                    if (NULL != pIndexOut)
                    {
                        *pIndexOut = pPacket->numBufferIOConfigs;
                    }
                    pPacket->numBufferIOConfigs++;
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::AddAddrPatch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Packet::AddAddrPatch(
    CSLMemHandle    hDst,
    UINT32          dstOffset,
    CSLMemHandle    hSrc,
    UINT32          srcOffset)
{
    CamxResult result   = CamxResultSuccess;
    CSLPacket* pPacket  = GetCSLPacket(m_pPacketHeader);

    CAMX_ASSERT(NULL != pPacket);

    if (NULL == pPacket)
    {
        result = CamxResultEInvalidState;
        CAMX_ASSERT_ALWAYS_MESSAGE("pPacket is NULL");
    }
    else if (0 == m_maxNumPatches)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Packet was created without patching support.");
    }
    else if (pPacket->numPatches >= m_maxNumPatches)
    {
        result = CamxResultEOutOfBounds;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Reached max number of patches: %d", pPacket->numPatches);
    }
    else
    {
        CSLAddrPatch* pPatches = GetAddrPatchset(pPacket);

        CAMX_ASSERT(NULL != pPatches);

        pPatches                += pPacket->numPatches;
        pPatches->hDstBuffer    = hDst;
        pPatches->dstOffset     = dstOffset;
        pPatches->hSrcBuffer    = hSrc;
        pPatches->srcOffset     = srcOffset;
        pPacket->numPatches++;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::Reset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Packet::Reset()
{
    CSLPacket* pPacket  = GetCSLPacket(m_pPacketHeader);

    CAMX_ASSERT(NULL != pPacket);

    if (NULL == pPacket)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "pPacket is NULL");
    }
    else
    {
        Utils::Memset(pPacket->data, 0, m_dataSize);

        pPacket->numBufferIOConfigs = 0;
        pPacket->numCmdBuffers      = 0;
        pPacket->numPatches         = 0;
        pPacket->kmdCmdBufferIndex  = static_cast<UINT32>(-1);

        m_committed = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::CommitPacket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Packet::CommitPacket()
{
    CamxResult result   = CamxResultSuccess;
    CSLPacket* pPacket  = GetCSLPacket(m_pPacketHeader);

    CAMX_ASSERT(NULL != pPacket);

    if (NULL == pPacket)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupUtils, "pPacket is NULL");
    }
    else
    {
        pPacket->header.requestId = GetRequestId();
        m_committed = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Packet::GetCSLPacketHeader
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLPacketHeader* Packet::GetCSLPacketHeader()
{
    if (FALSE == m_committed)
    {
        CommitPacket();
    }
    return m_pPacketHeader;
}

CAMX_NAMESPACE_END
