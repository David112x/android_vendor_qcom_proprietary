////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsnode.cpp
/// @brief BPS Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcdmdefs.h"
#include "camxcslicpdefs.h"
#include "camxcslispdefs.h"
#include "camxcslresourcedefs.h"
#include "camximagebuffer.h"
#include "camximageformatutils.h"
#include "camxhal3metadatautil.h"
#include "camxhwcontext.h"
#include "camxpipeline.h"
#include "camxtrace.h"
#include "camxvendortags.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxtitan17xcontext.h"
#include "camxtitan17xdefs.h"
#include "camxispiqmodule.h"
#include "camxbpsnode.h"
#include "camxbpspipelinetitan150.h"
#include "camxbpspipelinetitan160.h"
#include "camxbpspipelinetitan170.h"
#include "camxbpspipelinetitan175.h"
#include "camxbpspipelinetitan480.h"
#include "camxiqinterface.h"
#include "camxtranslator.h"
#include "chituningmodeparam.h"
#include "camxpacketdefs.h"

#include "camxipeicatestdata.h"
#include "GeoLibUtils.h"
#include "mf_1_1_0.h"


/// BPS GeoLib metadata tag name array indexed by GeoLibMultiFrameMode
static const CHAR* BPSGeolibMetaTagNameList[GEOLIB_MF_MODE_NUM] =
{
    "GeoLibStillFrameConfigPrefilter",
    "GeoLibStillFrameConfigBlending",
    "GeoLibStillFrameConfigPostFiler"
};

/// @brief  Function to transalate HAl formats to BPS firmware image format.
/// This function needs to be outside camx namespace due to ImageFormat data structure
/// @todo (CAMX-1015) To avoid this translation if possible
ImageFormat TranslateBPSFormatToFirmwareImageFormat(
   const CamX::ImageFormat* pImageFormat,
   const BPS_IO_IMAGES      firmwarePortId)
{
    ImageFormat firmwareFormat = IMAGE_FORMAT_INVALID;

    /// @todo (CAMX-1375) Update the Plain16- YUV420 10b / P010-YUV420 10b / Plain16-Y 10b /
    /// P010-Y 10b / PD10. Update raw formats based bpp.

    switch (pImageFormat->format)
    {
        case CamX::Format::YUV420NV12TP10:
        case CamX::Format::YUV420NV21TP10:
            firmwareFormat = IMAGE_FORMAT_LINEAR_TP_10;
            break;
        case CamX::Format::UBWCTP10:
            firmwareFormat = IMAGE_FORMAT_UBWC_TP_10;
            break;
        case CamX::Format::RawMIPI:
            switch (pImageFormat->formatParams.rawFormat.bitsPerPixel)
            {
                case 8:
                    firmwareFormat = IMAGE_FORMAT_MIPI_8;
                    break;
                case 10:
                    firmwareFormat = IMAGE_FORMAT_MIPI_10;
                    break;
                case 12:
                    firmwareFormat = IMAGE_FORMAT_MIPI_12;
                    break;
                case 14:
                    firmwareFormat = IMAGE_FORMAT_MIPI_14;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Unsupported MIPI format");
                    break;
            }
            break;
        case CamX::Format::RawPlain16:
            switch (pImageFormat->formatParams.rawFormat.bitsPerPixel)
            {
                case 8:
                    firmwareFormat = IMAGE_FORMAT_BAYER_8;
                    break;
                case 10:
                    firmwareFormat = IMAGE_FORMAT_BAYER_10;
                    break;
                case 12:
                    firmwareFormat = IMAGE_FORMAT_BAYER_12;
                    break;
                case 14:
                    firmwareFormat = IMAGE_FORMAT_BAYER_14;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Unsupported Bayer format");
                    break;
            }
            break;
        case CamX::Format::YUV420NV12:
        case CamX::Format::YUV420NV21:
            firmwareFormat = IMAGE_FORMAT_LINEAR_NV12;
            break;
        case CamX::Format::PD10:
            firmwareFormat = IMAGE_FORMAT_PD_10;
            break;
        case CamX::Format::Blob:
            if (BPS_OUTPUT_IMAGE_STATS_BG == firmwarePortId)
            {
                firmwareFormat = IMAGE_FORMAT_STATISTICS_BAYER_GRID;
            }
            else if (BPS_OUTPUT_IMAGE_STATS_BHIST == firmwarePortId)
            {
                firmwareFormat = IMAGE_FORMAT_STATISTICS_BAYER_HISTOGRAM;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Unsupported BPS Port %d", firmwarePortId);
            }
            break;
        case CamX::Format::P010:
            firmwareFormat = IMAGE_FORMAT_LINEAR_P010;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unsupported BPS format %d", pImageFormat->format);
            break;
    }
    return firmwareFormat;
}

/// @brief  Function to transalate UMD  formats to BPS firmware Bayer pixel order
BayerPixelOrder TranslateBPSFormatToFirmwareBayerOrder(const CamX::PixelFormat format)
{
    BayerPixelOrder pixelOrder = FIRST_PIXEL_MAX;

    switch (format)
    {
        case CamX::PixelFormat::BayerBGGR:
            pixelOrder = FIRST_PIXEL_B;
            break;
        case CamX::PixelFormat::BayerGBRG:
            pixelOrder = FIRST_PIXEL_GB;
            break;
        case CamX::PixelFormat::BayerGRBG:
            pixelOrder = FIRST_PIXEL_GR;
            break;
        case CamX::PixelFormat::BayerRGGB:
            pixelOrder = FIRST_PIXEL_R;
            break;
        case CamX::PixelFormat::YUVFormatY:
            pixelOrder = FIRST_PIXEL_R;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Wrong BPS Pixel Format");
            break;
    }

    return pixelOrder;
}
CAMX_NAMESPACE_BEGIN

/// @todo (CAMX-1117) Calculate the max number of patches in BPS packet / DMI and update.
static const UINT BPSMaxPatchAddress    = 128;                  //< Max number of patch address that a BPS packet can have
static const UINT BPSMaxDMIPatchAddress = 64;                   //< Number of Max address patching for DMI headers
static const UINT BPSMaxCDMPatchAddress = 64;                   //< Number of Max address patching for CDM headers

static const UINT BPSMaxFWCmdBufferManagers = 10;  ///< Maximum FW command buffer Managers

// Private Static member holding fixed values of Frame buffer offsets within BpsFrameProcess struct, for ease of patching
FrameBuffers BPSNode::s_frameBufferOffset[BPSMaxSupportedBatchSize][BPS_IO_IMAGES_MAX];

static const UINT BPSCmdBufferFrameProcessSizeBytes =
    sizeof(BpsFrameProcess) +
    (sizeof(CDMProgramArray)) +
    (static_cast<UINT>(BPSCDMProgramOrder::BPSProgramIndexMax) * sizeof(CdmProgram));

static const UINT CmdBufferGenericBlobSizeInBytes = (
    CSLGenericBlobHeaderSizeInDwords * sizeof(UINT32)            +
    sizeof(CSLICPClockBandwidthRequest)                          +
    /* BPS has one path for read and one for write so adding one additional sizeof(CSLAXIperPathBWVote) */
    (sizeof(CSLICPClockBandwidthRequestV2)                       +
    (sizeof(CSLAXIperPathBWVote))) +
    CSLGenericBlobHeaderSizeInDwords * sizeof(UINT32)            +
    sizeof(ConfigIORequest)                                      +
    CSLGenericBlobHeaderSizeInDwords * sizeof(UINT32)            +
    sizeof(CSLICPMemoryMapUpdate));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::BPSNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSNode::BPSNode()
    : m_pIQPacketManager(NULL)
    , m_OEMIQSettingEnable(FALSE)
    , m_OEMStatsSettingEnable(FALSE)
{
    m_pNodeName                 = "BPS";
    m_OEMStatsSettingEnable     = GetStaticSettings()->IsOEMStatSettingEnable;
    m_BPSHangDumpEnable         = GetStaticSettings()->enableBPSHangDump;
    m_BPSStripeDumpEnable       = GetStaticSettings()->enableBPSStripeDump;
    m_pPreTuningDataManager     = NULL;
    m_hStripingLib              = NULL;
    m_camIdChanged              = TRUE;
    m_deviceResourceRequest     = { 0 };
    m_configIOMem               = { 0 };
    m_curIntermediateDimension  = { 0, 0, 1.0f };
    m_prevIntermediateDimension = { 0, 0, 1.0f };

    // LDC related
    m_ICAGridOut2InEnabled = FALSE;
    m_ICAGridIn2OutEnabled = FALSE;
    m_publishLDCGridData   = FALSE;
    m_ICAGridGeometry      = 0;
    m_fdDataOffset         = 0;

    for (UINT i = 0; i < GridMaxType; i++)
    {
        m_pWarpGridData[i] = NULL;
    }

    for (UINT batchIndex = 0; batchIndex < BPSMaxSupportedBatchSize; batchIndex++)
    {
        for (UINT port = 0; port < BPS_IO_IMAGES_MAX; port++)
        {
            for (UINT plane = 0; plane < MAX_NUM_OF_IMAGE_PLANES; plane++)
            {
                // Precompute the frame buffer offset for all ports
                s_frameBufferOffset[batchIndex][port].bufferPtr[plane] =
                    static_cast<UINT32>(offsetof(BpsFrameProcessData, frameSets[0]) + sizeof(FrameSetBps) * batchIndex) +
                    static_cast<UINT32>(offsetof(FrameSetBps, frameBuffers[0]) + sizeof(FrameBuffers) * port) +
                    static_cast<UINT32>(offsetof(FrameBuffers, bufferPtr[0]) + (sizeof(FrameBufferPtr) * plane));

                s_frameBufferOffset[batchIndex][port].metadataBufferPtr[plane] =
                    static_cast<UINT32>(offsetof(BpsFrameProcessData, frameSets[0]) + sizeof(FrameSetBps) * batchIndex) +
                    static_cast<UINT32>(offsetof(FrameSetBps, frameBuffers[0]) + sizeof(FrameBuffers) * port) +
                    static_cast<UINT32>(offsetof(FrameBuffers, metadataBufferPtr[0]) + (sizeof(FrameBufferPtr) * plane));
            }
        }
    }

    Utils::Memset(m_maxCmdBufferSizeBytes, 0x0, sizeof(m_maxCmdBufferSizeBytes));

    m_cropWindowCurrent  = {0, 0, 0, 0};
    m_cropWindowPrevious = {0, 0, 0, 0};
    m_refCropWindow      = {0, 0};
    m_ldFullOutSize      = {0, 0};
    m_dumpGeolibResult   = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::~BPSNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSNode::~BPSNode()
{
    Cleanup();

    if (TRUE == IsDeviceAcquired())
    {
        if (CSLInvalidHandle != m_configIOMem.hHandle)
        {
            CSLReleaseBuffer(m_configIOMem.hHandle);
        }

        if (NULL != m_deviceResourceRequest.pDeviceResourceParam)
        {
            CAMX_FREE(m_deviceResourceRequest.pDeviceResourceParam);
            m_deviceResourceRequest.pDeviceResourceParam = NULL;
        }

        IQInterface::IQSettingModuleUninitialize(&m_libInitialData);

        ReleaseDevice();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::TranslateToFirmwarePortId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE VOID BPSNode::TranslateToFirmwarePortId(
    UINT32          portId,
    BPS_IO_IMAGES*  pFirmwarePortId)
{
    CAMX_ASSERT(portId < static_cast<UINT32>(BPS_IO_IMAGES::BPS_IO_IMAGES_MAX));

    *pFirmwarePortId = static_cast<BPS_IO_IMAGES>(portId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSNode* BPSNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    if ((NULL != pCreateInputData) && (NULL != pCreateInputData->pNodeInfo))
    {
        UINT32           propertyCount   = pCreateInputData->pNodeInfo->nodePropertyCount;
        PerNodeProperty* pNodeProperties = pCreateInputData->pNodeInfo->pNodeProperties;

        BPSNode* pNodeObj = CAMX_NEW BPSNode;

        if (NULL != pNodeObj)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "nodePropertyCount %d", propertyCount);

            // There can be multiple BPS instances in a pipeline, each instance can have differnt IQ modules enabled
            for (UINT32 count = 0; count < propertyCount; count++)
            {
                UINT32 nodePropertyId     = pNodeProperties[count].id;
                VOID*  pNodePropertyValue = pNodeProperties[count].pValue;

                switch (nodePropertyId)
                {
                    case NodePropertyProfileId:
                        pNodeObj->m_instanceProperty.profileId = static_cast<BPSProfileId>(
                            atoi(static_cast<const CHAR*>(pNodePropertyValue)));
                        break;
                    case NodePropertyProcessingType: // If MFNR/MFSR is enabled, BPS instance needs to know.
                        pNodeObj->m_instanceProperty.processingType = static_cast<BPSProcessingType>(
                            atoi(static_cast<const CHAR*>(pNodePropertyValue)));
                        break;
                    default:
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unhandled node property Id %d", nodePropertyId);
                        break;
                }
            }

            CAMX_LOG_INFO(CamxLogGroupBPS, "BPS Instance profileId: %d processing: %d",
                          pNodeObj->m_instanceProperty.profileId,
                          pNodeObj->m_instanceProperty.processingType);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to create BPSNode, no memory");
        }

        return pNodeObj;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null input pointer");
        return NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);

    CamxResult        result                   = CamxResultSuccess;
    INT32             deviceIndex              = -1;
    UINT              indicesLengthRequired    = 0;
    HwEnvironment*    pHwEnvironment           = HwEnvironment::GetInstance();

    CAMX_ASSERT(BPS == Type());
    CAMX_ASSERT(NULL != pCreateOutputData);

    Titan17xContext* pContext = static_cast<Titan17xContext*>(GetHwContext());
    m_OEMIQSettingEnable      = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->IsOEMIQSettingEnable;

    if ((NULL == pCreateOutputData) || (NULL == pCreateInputData))
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Output Data.");
    }
    if (CamxResultSuccess == result)
    {
        // Configure BPS Capability
        result = ConfigureBPSCapability();
    }
    if (CamxResultSuccess == result)
    {
        m_pChiContext = pCreateInputData->pChiContext;

        pCreateOutputData->maxOutputPorts = BPSMaxOutput;
        pCreateOutputData->maxInputPorts  = BPSMaxInput;

        UINT numOutputPorts = 0;
        UINT outputPortId[MaxBufferComposite];

        GetAllOutputPortIds(&numOutputPorts, &outputPortId[0]);

        CAMX_ASSERT(MaxBufferComposite >= numOutputPorts);
        UINT32 groupID = ISPOutputGroupIdMAX;
        for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
        {
            switch (outputPortId[outputPortIndex])
            {
                case BPSOutputPortFull:
                case BPSOutputPortDS4:
                case BPSOutputPortDS16:
                case BPSOutputPortDS64:
                    pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                                                                                   ISPOutputGroupId0;
                    break;

                case BPSOutputPortReg1:
                    pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] = ISPOutputGroupId1;
                    break;

                case BPSOutputPortReg2:
                    pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] = ISPOutputGroupId2;
                    break;

                case BPSOutputPortStatsBG:
                case BPSOutputPortStatsHDRBHist:
                    pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                        ISPOutputGroupId3;
                    break;

                default:
                    pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                        groupID++;
                    break;
            }

            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Index %d, Port ID %d is maped group %d",
                           outputPortIndex,
                           outputPortId[outputPortIndex],
                           pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]]);
        }

        pCreateOutputData->bufferComposite.hasCompositeMask = TRUE;

        // Add device indices
        result = pHwEnvironment->GetDeviceIndices(CSLDeviceTypeICP, &deviceIndex, 1, &indicesLengthRequired);
    }
    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(indicesLengthRequired == 1);
        result = AddDeviceIndex(deviceIndex);
        m_deviceIndex = deviceIndex;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::PostPipelineCreate()
{
    CamxResult      result = CamxResultSuccess;
    ResourceParams  params = { 0 };

    // If BPS tuning-data enable, initialize debug-data writer
    if ((TRUE   == GetStaticSettings()->enableTuningMetadata)   &&
        (0      != GetStaticSettings()->tuningDumpDataSizeBPS))
    {
        // We would disable dual BPS when supporting tuning data
        m_pTuningMetadata = static_cast<BPSTuningMetadata*>(CAMX_CALLOC(sizeof(BPSTuningMetadata)));
        if (NULL == m_pTuningMetadata)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to allocate Tuning metadata.");
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            m_pDebugDataWriter = CAMX_NEW TDDebugDataWriter();
            if (NULL == m_pDebugDataWriter)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to allocate Tuning metadata.");
                result = CamxResultENoMemory;
            }
        }
    }

    // Assemble BPS IQ Modules
    if (CamxResultSuccess == result)
    {
        result = CreateBPSIQModules();

        m_tuningData.noOfSelectionParameter = 1;
        m_tuningData.TuningMode[0].mode     = ChiModeType::Default;
    }

    if (CamxResultSuccess == result)
    {
        UpdateCommandBufferSize();
    }

    if (CamxResultSuccess == result)
    {
        m_BPSCmdBlobCount = GetPipeline()->GetRequestQueueDepth();
        result = InitializeCmdBufferManagerList(BPSCmdBufferMaxIds);
    }

    if (CamxResultSuccess == result)
    {
        // Command buffer for IQ packet
        params.usageFlags.packet               = 1;
        params.packetParams.maxNumCmdBuffers   = 2;
        // 1 Input and 5 Outputs
        params.packetParams.maxNumIOConfigs    = BPSMaxInput + BPSMaxOutput;
        params.packetParams.enableAddrPatching = 1;
        params.packetParams.maxNumPatches      = BPSMaxPatchAddress;
        params.resourceSize                    = Packet::CalculatePacketSize(&params.packetParams);
        params.memFlags                        = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
        // Same number as cmd buffers
        params.poolSize                        = m_BPSCmdBlobCount * params.resourceSize;
        params.alignment                       = CamxPacketAlignmentInBytes;
        params.pDeviceIndices                  = NULL;
        params.numDevices                      = 0;

        result = CreateCmdBufferManager("IQPacketManager", &params, &m_pIQPacketManager);
        if (CamxResultSuccess == result)
        {
            // command buffer for BPS clock and BW
            params                              = { 0 };
            params.resourceSize                 = CmdBufferGenericBlobSizeInBytes;
            params.poolSize                     = m_BPSCmdBlobCount * params.resourceSize;
            params.usageFlags.cmdBuffer         = 1;
            params.cmdParams.type               = CmdType::Generic;
            params.alignment                    = CamxCommandBufferAlignmentInBytes;
            params.cmdParams.enableAddrPatching = 0;
            params.cmdParams.maxNumNestedAddrs  = 0;
            params.memFlags                     = CSLMemFlagUMDAccess | CSLMemFlagKMDAccess;
            params.pDeviceIndices               = DeviceIndices();
            params.numDevices                   = 1;

            result = CreateCmdBufferManager("CmdBufferGenericBlob", &params,
                                            &m_pBPSCmdBufferManager[BPSCmdBufferGenericBlob]);

            if (CamxResultSuccess == result)
            {
                ResourceParams               resourceParams[BPSMaxFWCmdBufferManagers];
                CHAR                         bufferManagerName[BPSMaxFWCmdBufferManagers][MaxStringLength256];
                struct CmdBufferManagerParam createParam[BPSMaxFWCmdBufferManagers];
                UINT32                       numberOfBufferManagers = 0;
                CamxResult                   result                 = CamxResultSuccess;

                if (m_maxCmdBufferSizeBytes[BPSCmdBufferCDMProgram] > 0)
                {
                    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                                        m_maxCmdBufferSizeBytes[BPSCmdBufferCDMProgram],
                                        CmdType::CDMDirect,
                                        CSLMemFlagUMDAccess,
                                        BPSMaxCDMPatchAddress,
                                        DeviceIndices(),
                                        m_BPSCmdBlobCount);
                    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                                      sizeof(CHAR) * MaxStringLength256,
                                      "CBM_%s_%s",
                                      NodeIdentifierString(),
                                      "CmdBufferCDMProgram");
                    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
                    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
                    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pBPSCmdBufferManager[BPSCmdBufferCDMProgram];

                    numberOfBufferManagers++;
                }

                if (m_maxCmdBufferSizeBytes[BPSCmdBufferDMIHeader] > 0)
                {
                    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                                        m_maxCmdBufferSizeBytes[BPSCmdBufferDMIHeader],
                                        CmdType::CDMDMI,
                                        CSLMemFlagUMDAccess,
                                        BPSMaxDMIPatchAddress,
                                        DeviceIndices(),
                                        m_BPSCmdBlobCount);
                    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                                      sizeof(CHAR) * MaxStringLength256,
                                      "CBM_%s_%s",
                                      NodeIdentifierString(),
                                      "BPSCmdBufferDMIHeader");
                    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
                    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
                    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pBPSCmdBufferManager[BPSCmdBufferDMIHeader];

                    numberOfBufferManagers++;
                }

                if (0 != numberOfBufferManagers)
                {
                    result = CreateMultiCmdBufferManager(createParam, numberOfBufferManagers);
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Multi Command Buffer Creation Failed");
                }
            }

        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to Creat Cmd Buffer Manager");
        }
    }

    if (result == CamxResultSuccess)
    {
        result = OpenStripingLibrary();
    }

    if (CamxResultSuccess != result)
    {
        Cleanup();
    }

    if (CamxResultSuccess == result)
    {
        result = AcquireDevice();
    }

    if (CamxResultSuccess == result)
    {
        // Save required static metadata
        GetStaticMetadata(GetPipeline()->GetCameraId());
        InitializeDefaultHALTags();
    }

    if (CamxResultSuccess == result)
    {
        IQInterface::IQSettingModuleInitialize(&m_libInitialData);
    }

    if (CamxResultSuccess == result)
    {
        UINT           numberOfMappings = 0;
        CSLBufferInfo  bufferInfo = { 0 };
        CSLBufferInfo* pBufferInfo[CSLICPMaxMemoryMapRegions];

        if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferFrameProcess])
        {
            if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferFrameProcess]->GetMergedCSLBufferInfo())
            {
                Utils::Memcpy(&bufferInfo,
                              m_pBPSCmdBufferManager[BPSCmdBufferFrameProcess]->GetMergedCSLBufferInfo(),
                              sizeof(CSLBufferInfo));
                pBufferInfo[numberOfMappings] = &bufferInfo;
                numberOfMappings++;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to get the Merged Buffer info");
                result = CamxResultEFailed;
            }
        }

        if (0 != numberOfMappings)
        {
            result = SendFWCmdRegionInfo(CSLICPGenericBlobCmdBufferMapFWMemRegion,
                                         pBufferInfo,
                                         numberOfMappings);
        }
    }

    m_dumpGeolibResult = GetStaticSettings()->dumpGeolibResult;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::HardcodeSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BPSNode::HardcodeSettings(
    ISPInputData* pInputData)
{
    AECFrameControl*  pAECInput       = pInputData->pAECUpdateData;
    AWBFrameControl*  pAWBInput       = pInputData->pAWBUpdateData;
    BGBEConfig*       pBGConfig       = &pInputData->pAWBStatsUpdateData->statsConfig.BGConfig;
    HDRBHistConfig*   pHDRBHistConfig = &pInputData->pAECStatsUpdateData->statsConfig.HDRBHistConfig;
    UINT32            inputWidth      = pInputData->pipelineBPSData.width;
    UINT32            inputHeight     = pInputData->pipelineBPSData.height;

    if (NULL != pAECInput)
    {
        pAECInput->luxIndex                     = 220.0f;
        pAECInput->predictiveGain               = 1.0;
        pAECInput->exposureInfo[0].exposureTime = 1;
        pAECInput->exposureInfo[0].linearGain   = 1.0f;
        pAECInput->exposureInfo[0].sensitivity  = 0.0f;

        for ( UINT i = 0 ; i < ExposureIndexCount ; i++ )
        {
            pAECInput->exposureInfo[i].exposureTime = 1;
            pAECInput->exposureInfo[i].linearGain   = 1.0f;
            pAECInput->exposureInfo[i].sensitivity  = 1.0f;
        }
    }

    if (NULL != pAWBInput)
    {
        pAWBInput->colorTemperature = 0;
        pAWBInput->AWBGains.gGain   = 1.0f;
        pAWBInput->AWBGains.bGain   = 2.0f;
        pAWBInput->AWBGains.rGain   = 1.796875f;
    }

    if (NULL != pBGConfig)
    {
        pBGConfig->channelGainThreshold[ChannelIndexR]  = (1 << BPSPipelineBitWidth) - 1;
        pBGConfig->channelGainThreshold[ChannelIndexGR] = (1 << BPSPipelineBitWidth) - 1;
        pBGConfig->channelGainThreshold[ChannelIndexB]  = (1 << BPSPipelineBitWidth) - 1;
        pBGConfig->channelGainThreshold[ChannelIndexGB] = (1 << BPSPipelineBitWidth) - 1;
        pBGConfig->horizontalNum                        = 64;
        pBGConfig->verticalNum                          = 48;
        pBGConfig->ROI.left                             = 0;
        pBGConfig->ROI.top                              = 0;
        pBGConfig->ROI.width                            = inputWidth - (inputWidth / 10);
        pBGConfig->ROI.height                           = inputHeight - (inputHeight / 10);
        pBGConfig->outputBitDepth                       = 0;
        pBGConfig->outputMode                           = BGBERegular;
        pBGConfig->YStatsWeights[0]                     = static_cast<FLOAT>(0.2f);
        pBGConfig->YStatsWeights[0]                     = static_cast<FLOAT>(0.3);
        pBGConfig->YStatsWeights[0]                     = static_cast<FLOAT>(0.4);
        pBGConfig->greenType                            = Gr;
    }

    // HDR Bhist config
    if (NULL != pHDRBHistConfig)
    {
        pHDRBHistConfig->ROI.top           = 0;
        pHDRBHistConfig->ROI.left          = 0;
        pHDRBHistConfig->ROI.width         = inputWidth;
        pHDRBHistConfig->ROI.height        = inputHeight;
        pHDRBHistConfig->greenChannelInput = HDRBHistSelectGB;
        pHDRBHistConfig->inputFieldSelect  = HDRBHistInputAll;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSNode::Set3AInputData()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::Set3AInputData(
    ISPInputData* pInputData)
{
    VOID**           ppPropertyData3A   = NULL;
    UINT             length             = 0;
    ISPStripeConfig* pFrameConfig       = pInputData->pStripeConfig;
    BOOL             updateStats        = TRUE;

    CAMX_ASSERT(NULL != pFrameConfig);

    // maximum stats item constant
    static const UINT32 Max3AItems       = 11;
    VOID*  pProperties3AData[Max3AItems] = { 0 };

    ppPropertyData3A = pProperties3AData;

    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "IsRealTime() %d", IsRealTime());

    auto DumpStatsToFile = [this] (const CHAR* pStatsFileNameExt,
                                   VOID*       pStats,
                                   UINT        size,
                                   UINT        tagID) -> VOID
    {
        const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();
        if (TRUE == (pSettings->enableFeature2Dump))
        {
            UINT32            tagLocation        = 0;
            CDKResult         result             =
                VendorTagManager::QueryVendorTagLocation("org.quic.camera.debugDumpConfig", "DebugDumpConfig", &tagLocation);
            tagLocation                         |= InputMetadataSectionMask;
            UINT              properties[]       = { tagLocation };
            static const UINT Length             = CAMX_ARRAY_SIZE(properties);
            VOID*             pData[Length]      = { 0 };
            UINT64            offsets[Length]    = { 0 };
            UINT              dumpStatsIndex     = 0;
            DumpFileName*     pDumpFileName      = NULL;
            GetDataList(properties, pData, offsets, Length);

            if (NULL != pData[0])
            {
                pDumpFileName = static_cast<DumpFileName*>(pData[0]);
                if (0 != OsUtils::StrCmp(pDumpFileName->dumpFileName[m_cameraId], g_dumpFileName.dumpFileName[m_cameraId]))
                {
                    Utils::Memcpy(g_dumpFileName.dumpFileName[m_cameraId], pDumpFileName->dumpFileName[m_cameraId], 128);
                    g_dumpTintStatsIndex[m_cameraId]  = 0;
                    g_dumpBhistStatsIndex[m_cameraId] = 0;
                    g_dumpAwbbgStatsIndex[m_cameraId] = 0;
                }

                if (0 == OsUtils::StrCmp(".tint", pStatsFileNameExt))
                {
                    dumpStatsIndex = g_dumpTintStatsIndex[m_cameraId];
                    g_dumpTintStatsIndex[m_cameraId]++;
                }
                else if (0 == OsUtils::StrCmp(".bhist", pStatsFileNameExt))
                {
                    dumpStatsIndex = g_dumpBhistStatsIndex[m_cameraId];
                    g_dumpBhistStatsIndex[m_cameraId]++;
                }
                else if (0 == OsUtils::StrCmp(".awbbg", pStatsFileNameExt))
                {
                    dumpStatsIndex = g_dumpAwbbgStatsIndex[m_cameraId];
                    g_dumpAwbbgStatsIndex[m_cameraId]++;
                }

                CHAR   dumpFilename[256];
                OsUtils::SNPrintF(dumpFilename,
                                  sizeof(dumpFilename),
                                  "%s/%s_%d%s",
                                  ConfigFileDirectory,
                                  g_dumpFileName.dumpFileName[m_cameraId],
                                  dumpStatsIndex,
                                  pStatsFileNameExt);

                FILE* pFile = OsUtils::FOpen(dumpFilename, "wb");
                if (NULL != pFile)
                {
                    SIZE_T len   = 0;
                    UINT   tagId = tagID;

                    len += OsUtils::FWrite(&tagId, 1, sizeof(tagId), pFile);
                    len += OsUtils::FWrite(pStats, 1, size, pFile);

                    CAMX_LOG_INFO(CamxLogGroupBPS, "Dump stats %d bytes to %s", len, dumpFilename);
                    OsUtils::FClose(pFile);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupBPS,
                        "Cannot dump Stats file name %s", dumpFilename);
                }
            }
        }
    };

    if (TRUE == IsRealTime())
    {
        static const UINT Properties3A[] =
        {
            PropertyIDAECFrameControl,
            PropertyIDAWBFrameControl,
            PropertyIDAECStatsControl,
            PropertyIDAWBStatsControl,
            PropertyIDISPTintlessBGConfig,
            PropertyIDParsedTintlessBGStatsOutput,
            PropertyIDISPBHistConfig,
            PropertyIDParsedBHistStatsOutput,
            PropertyIDSensorNumberOfLEDs,
            PropertyIDISPAWBBGConfig,
            PropertyIDParsedAWBBGStatsOutput,
        };
        static const UINT PropertiesSize = CAMX_ARRAY_SIZE(Properties3A);

        if (Max3AItems != PropertiesSize)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Can ont get 3A data due to mismatch in property size!");
            return;
        }

        UINT64 propertyData3AOffset[PropertiesSize]    = { 0 };


        propertyData3AOffset[4] = GetMaximumPipelineDelay();
        propertyData3AOffset[6] = GetMaximumPipelineDelay();
        propertyData3AOffset[9] = GetMaximumPipelineDelay();

        length          = PropertiesSize;

        if (FALSE == GetPipeline()->HasStatsNode())
        {
            updateStats = FALSE;
        }
        else
        {
            GetDataList(Properties3A, ppPropertyData3A, propertyData3AOffset, length);
        }
    }
    else
    {
        static const UINT Properties3A[] =
        {
            PropertyIDAECFrameControl             | InputMetadataSectionMask,
            PropertyIDAWBFrameControl             | InputMetadataSectionMask,
            PropertyIDAECStatsControl             | InputMetadataSectionMask,
            PropertyIDAWBStatsControl             | InputMetadataSectionMask,
            PropertyIDISPTintlessBGConfig         | InputMetadataSectionMask,
            PropertyIDParsedTintlessBGStatsOutput | InputMetadataSectionMask,
            PropertyIDISPBHistConfig              | InputMetadataSectionMask,
            PropertyIDParsedBHistStatsOutput      | InputMetadataSectionMask,
            PropertyIDSensorNumberOfLEDs          | InputMetadataSectionMask,
            PropertyIDISPAWBBGConfig              | InputMetadataSectionMask,
            PropertyIDParsedAWBBGStatsOutput      | InputMetadataSectionMask,
        };
        static const UINT PropertiesSize = CAMX_ARRAY_SIZE(Properties3A);
        if (Max3AItems != PropertiesSize)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Can ont get 3A data due to mismatch in property size!");
            return;
        }

        UINT64 propertyData3AOffset[PropertiesSize]    = { 0 };

        length          = PropertiesSize;

        GetDataList(Properties3A, ppPropertyData3A, propertyData3AOffset, length);
    }

    if (TRUE == updateStats)
    {
        // PropertyIDAECFrameControl
        if (NULL != ppPropertyData3A[0])
        {
            Utils::Memcpy(pInputData->pAECUpdateData, ppPropertyData3A[0], sizeof(AECFrameControl));
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "No AEC frame control");
        }

        // PropertyIDAWBFrameControl
        if (NULL != ppPropertyData3A[1])
        {
            Utils::Memcpy(pInputData->pAWBUpdateData, ppPropertyData3A[1], sizeof(AWBFrameControl));
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "No AWB frame control");
        }

        // PropertyIDAECStatsControl
        if ((NULL != ppPropertyData3A[2]) && (TRUE == IsTagPresentInPublishList(PropertyIDAECStatsControl)))
        {
            Utils::Memcpy(pInputData->pAECStatsUpdateData, ppPropertyData3A[2], sizeof(AECStatsControl));
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "No AEC Stats control");
        }

        // PropertyIDAWBStatsControl
        if ((NULL != ppPropertyData3A[3]) && (TRUE == IsTagPresentInPublishList(PropertyIDAWBStatsControl)))
        {
            Utils::Memcpy(pInputData->pAWBStatsUpdateData, ppPropertyData3A[3], sizeof(AWBStatsControl));
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "No AWB Stats control");
        }


        CAMX_LOG_VERBOSE(CamxLogGroupBPS,
                         "AWB Frame Control: Gain(R:%f, G: %f, B: %f) CCT(%u)",
                         pInputData->pAWBUpdateData->AWBGains.rGain,
                         pInputData->pAWBUpdateData->AWBGains.gGain,
                         pInputData->pAWBUpdateData->AWBGains.bGain,
                         pInputData->pAWBUpdateData->colorTemperature,
                         pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].linearGain,
                         pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort].exposureTime);
        CAMX_LOG_VERBOSE(CamxLogGroupBPS,
                         "BPS: Processing for ReqId(frameNum) = %llu. AEC Gain = %f, ExposureTime = %llu, luxIndex = %f",
                         pInputData->frameNum,
                         pInputData->pAECUpdateData->exposureInfo[0].linearGain,
                         pInputData->pAECUpdateData->exposureInfo[0].exposureTime,
                         pInputData->pAECUpdateData->luxIndex);


        UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(pInputData->frameNum);

        if ((GetMaximumPipelineDelay() < requestIdOffsetFromLastFlush) || (FALSE == IsRealTime()))
        {
            // PropertyIDISPTintlessBGConfig
            if (NULL != ppPropertyData3A[4])
            {
                Utils::Memcpy(&(pFrameConfig->statsDataForISP.tintlessBGConfig),
                    &(reinterpret_cast<PropertyISPTintlessBG*>(ppPropertyData3A[4])->statsConfig),
                    sizeof(pFrameConfig->statsDataForISP.tintlessBGConfig));
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupBPS, "No Tintless BG configuration");
            }

            // PropertyIDParsedTintlessBGStatsOutput
            if (NULL != ppPropertyData3A[5])
            {
                pFrameConfig->statsDataForISP.pParsedTintlessBGStats =
                    *(reinterpret_cast<ParsedTintlessBGStatsOutput**>(ppPropertyData3A[5]));
                if (NULL != pFrameConfig->statsDataForISP.pParsedTintlessBGStats)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupBPS,
                        "TLBG Config %d %d | %d | statsBitWidth %d | Threashold %d %d %d %d | m_flags %d | RSum %d BSum %d",
                        pFrameConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.ROI.height,
                        pFrameConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.ROI.width,
                        pFrameConfig->statsDataForISP.pParsedTintlessBGStats->m_numOfRegions,
                        pFrameConfig->statsDataForISP.pParsedTintlessBGStats->GetStatsBitWidth(),
                        pFrameConfig->statsDataForISP.pParsedTintlessBGStats->GetChannelGainThreshold(0),
                        pFrameConfig->statsDataForISP.pParsedTintlessBGStats->GetChannelGainThreshold(1),
                        pFrameConfig->statsDataForISP.pParsedTintlessBGStats->GetChannelGainThreshold(2),
                        pFrameConfig->statsDataForISP.pParsedTintlessBGStats->GetChannelGainThreshold(3),
                        pFrameConfig->statsDataForISP.pParsedTintlessBGStats->m_flags,
                        pFrameConfig->statsDataForISP.pParsedTintlessBGStats->GetTintlessBGStatsInfo(0)->RSum,
                        pFrameConfig->statsDataForISP.pParsedTintlessBGStats->GetTintlessBGStatsInfo(0)->BSum);
                    DumpStatsToFile(".tint",
                                    pFrameConfig->statsDataForISP.pParsedTintlessBGStats,
                                    sizeof(ParsedTintlessBGStatsOutput),
                                    PropertyIDParsedTintlessBGStatsOutput | InputMetadataSectionMask);
                }
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupBPS, "No Parsed Tintless BG Stats");
            }

            // PropertyIDISPBHistConfig
            if (NULL != ppPropertyData3A[6])
            {
                Utils::Memcpy(&(pFrameConfig->statsDataForISP.BHistConfig),
                    &(reinterpret_cast<PropertyISPBHistStats*>(ppPropertyData3A[6])->statsConfig),
                    sizeof(pFrameConfig->statsDataForISP.BHistConfig));
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupBPS, "No BHist Configuration");
            }

            // PropertyIDParsedBHistStatsOutput
            if (NULL != ppPropertyData3A[7])
            {
                pFrameConfig->statsDataForISP.pParsedBHISTStats =
                    *(reinterpret_cast<ParsedBHistStatsOutput**>(ppPropertyData3A[7]));
                if (NULL != pFrameConfig->statsDataForISP.pParsedBHISTStats)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "BHIST Config %d %d %d | %d %d %d BHIST stats %x %x %x ",
                                     pFrameConfig->statsDataForISP.BHistConfig.BHistConfig.channel,
                                     pFrameConfig->statsDataForISP.BHistConfig.BHistConfig.uniform,
                                     pFrameConfig->statsDataForISP.BHistConfig.numBins,
                                     pFrameConfig->statsDataForISP.pParsedBHISTStats->channelType,
                                     pFrameConfig->statsDataForISP.pParsedBHISTStats->uniform,
                                     pFrameConfig->statsDataForISP.pParsedBHISTStats->numBins,
                                     pFrameConfig->statsDataForISP.pParsedBHISTStats->BHistogramStats[0],
                                     pFrameConfig->statsDataForISP.pParsedBHISTStats->BHistogramStats[1],
                                     pFrameConfig->statsDataForISP.pParsedBHISTStats->BHistogramStats[2]);
                    DumpStatsToFile(".bhist",
                                    pFrameConfig->statsDataForISP.pParsedBHISTStats,
                                    sizeof(ParsedBHistStatsOutput),
                                    PropertyIDParsedBHistStatsOutput | InputMetadataSectionMask);
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupBPS, "Parsed BHist stats is NULL");
                }
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupBPS, "No Parsed BHist stats");
            }

            // PropertyIDParsedAWBBGStatsOutput
            if (NULL != ppPropertyData3A[10])
            {
                pFrameConfig->statsDataForISP.pParsedAWBBGStats =
                    *(reinterpret_cast<ParsedAWBBGStatsOutput**>(ppPropertyData3A[10]));
                if (NULL != pFrameConfig->statsDataForISP.pParsedAWBBGStats)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "AWBBG stats %x %x",
                                     pFrameConfig->statsDataForISP.pParsedAWBBGStats->flags.usesY,
                                     pFrameConfig->statsDataForISP.pParsedAWBBGStats->flags.hasSatInfo);
                    DumpStatsToFile(".awbbg",
                                    pFrameConfig->statsDataForISP.pParsedAWBBGStats,
                                    sizeof(ParsedAWBBGStatsOutput),
                                    PropertyIDParsedAWBBGStatsOutput | InputMetadataSectionMask);
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupBPS, "AWB BG Stats is NULL");
                }
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupBPS, "No Parsed AWB BG Stats");
            }
        }

        // PropertyIDSensorNumberOfLEDs
        if (NULL != ppPropertyData3A[8])
        {
            pInputData->numberOfLED = *reinterpret_cast<UINT16*>(ppPropertyData3A[8]);
            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Number of led %d", pInputData->numberOfLED);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "No number of LEDs");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetSensorModeData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const SensorMode* BPSNode::GetSensorModeData(
    BOOL sensorConnected)
{
    CamxResult        result      = CamxResultSuccess;
    const SensorMode* pSensorMode = NULL;

    if (FALSE == sensorConnected)
    {
        UINT       metaTag   = 0;
        UINT       modeIndex = 0;

        result = VendorTagManager::QueryVendorTagLocation("com.qti.sensorbps", "mode_index", &metaTag);

        metaTag |= InputMetadataSectionMask;

        /// @todo (CAMX-1015) Can optimize by keeping an array of SensorMode* per requestId
        UINT              propertiesBPS[] = { PropertyIDSensorCurrentMode, PropertyIDUsecaseSensorModes, metaTag };
        static const UINT Length          = CAMX_ARRAY_SIZE(propertiesBPS);
        VOID*             pData[Length]   = { 0 };
        UINT64            offsets[Length] = { 0, 0, 0 };

        GetDataList(propertiesBPS, pData, offsets, Length);

        if (NULL != pData[2])
        {
            // Sensor not connected so pull from the input metadata
            modeIndex = *reinterpret_cast<UINT*>(pData[2]);
            CAMX_LOG_INFO(CamxLogGroupBPS, "BPS using vendor tag com.qti.sensorbps mode index %d", modeIndex);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "Sensor mode index vendor tag not provided! Using default 0!");
        }

        CAMX_ASSERT_MESSAGE(NULL != pData[1], "Usecase pool did not contain sensor modes. Going to fault");
        if (NULL != pData[1])
        {
            pSensorMode = &(reinterpret_cast<UsecaseSensorModes*>(pData[1])->allModes[modeIndex]);
        }
    }
    else
    {
        Node::GetSensorModeData(&pSensorMode);
        CAMX_LOG_INFO(CamxLogGroupBPS, "BPS using pipeline mode index");
    }

    return pSensorMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetPDAFInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::GetPDAFInformation(
    SensorPDAFInfo* pSensorPDAFInfo)
{
    UINT   sensorPDAFInfoTag[1] = { 0 };
    VOID*  pDataOutput[]        = { 0 };
    UINT64 PDAFdataOffset[1]    = { 0 };

    if (TRUE == IsRealTime())
    {
        sensorPDAFInfoTag[0] = { PropertyIDSensorPDAFInfo };
    }
    else
    {
        sensorPDAFInfoTag[0] = { PropertyIDSensorPDAFInfo | InputMetadataSectionMask };
    }

    GetDataList(sensorPDAFInfoTag, pDataOutput, PDAFdataOffset, 1);
    if (NULL != pDataOutput[0])
    {
        Utils::Memcpy(pSensorPDAFInfo, pDataOutput[0], sizeof(SensorPDAFInfo));
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "PDAF not enabled");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::IsSensorModeFormatBayer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSNode::IsSensorModeFormatBayer(
    PixelFormat format)
{
    BOOL isBayer = FALSE;

    if ((PixelFormat::BayerBGGR == format) ||
        (PixelFormat::BayerGBRG == format) ||
        (PixelFormat::BayerGRBG == format) ||
        (PixelFormat::BayerRGGB == format))
    {
        isBayer = TRUE;
    }

    return isBayer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::IsSensorModeFormatMono
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSNode::IsSensorModeFormatMono(
    PixelFormat format)
{
    BOOL isMono = FALSE;

    if (PixelFormat::Monochrome == format ||
        PixelFormat::YUVFormatY == format)
    {
        isMono = TRUE;
    }

    return isMono;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ReadSensorDigitalGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT BPSNode::ReadSensorDigitalGain(
    BOOL sensorConnected)
{
    FLOAT      dGain   = 1.0;
    CamxResult result  = CamxResultSuccess;
    UINT       metaTag = 0;

    result   = VendorTagManager::QueryVendorTagLocation("com.qti.sensorbps", "gain", &metaTag);
    metaTag |= InputMetadataSectionMask;

    UINT              propertiesBPS[] = { PropertyIDPostSensorGainId, metaTag };
    static const UINT Length          = CAMX_ARRAY_SIZE(propertiesBPS);
    VOID*             pData[Length]   = { 0 };
    UINT64            offsets[Length] = { 0, 0 };

    GetDataList(propertiesBPS, pData, offsets, Length);

    if ((NULL != pData[1]) && ((FALSE == sensorConnected) ||(GetPipeline()->HasStatsNode() == FALSE)))
    {
        dGain = *reinterpret_cast<FLOAT*>(pData[1]);
        CAMX_LOG_INFO(CamxLogGroupBPS, "BPS using vendor tag com.qti.sensorbps gain");
    }
    else if ( NULL != pData[0])
    {
        dGain = *reinterpret_cast<FLOAT*>(pData[0]);
        CAMX_LOG_INFO(CamxLogGroupBPS, "BPS using pipeline gain");
    }

    return dGain;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::SendFWCmdRegionInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::SendFWCmdRegionInfo(
    UINT32          opcode,
    CSLBufferInfo** ppBufferInfo,
    UINT32          numberOfMappings)
{
    CamxResult            result          = CamxResultSuccess;
    Packet*               pPacket         = NULL;
    CmdBuffer*            pCmdBuffer      = NULL;
    CSLICPMemoryMapUpdate memoryMapUpdate = { 0 };
    CSLBufferInfo*        pBufferInfo     = NULL;

    pPacket = GetPacket(m_pIQPacketManager);

    if ((NULL != pPacket) && (NULL != ppBufferInfo) && (numberOfMappings <= CSLICPMaxMemoryMapRegions))
    {
        pPacket->SetOpcode(CSLDeviceType::CSLDeviceTypeICP, CSLPacketOpcodesBPSMemoryMapUpdate);
        pCmdBuffer = GetCmdBuffer(m_pBPSCmdBufferManager[BPSCmdBufferGenericBlob]);
        if (NULL != pCmdBuffer)
        {
            memoryMapUpdate.version               = CSLICPMemoryMapVersion0;
            memoryMapUpdate.numberOfMappings      = numberOfMappings;
            for (UINT index = 0; index < numberOfMappings; index++)
            {
                pBufferInfo = ppBufferInfo[index];
                if (NULL != pBufferInfo)
                {
                    memoryMapUpdate.regionInfo[index].hHandle = pBufferInfo->hHandle;
                    memoryMapUpdate.regionInfo[index].flags   = pBufferInfo->flags;
                    memoryMapUpdate.regionInfo[index].offset  = pBufferInfo->offset;
                    memoryMapUpdate.regionInfo[index].size    = pBufferInfo->size;
                }
                else
                {
                    result = CamxResultEInvalidArg;
                }
            }
            if (CamxResultSuccess == result)
            {
                PacketBuilder::WriteGenericBlobData(pCmdBuffer,
                                                    opcode,
                                                    sizeof(CSLICPMemoryMapUpdate),
                                                    reinterpret_cast<BYTE*>(&memoryMapUpdate));
                pPacket->CommitPacket();
                pCmdBuffer->SetMetadata(static_cast<UINT32>(CSLICPCmdBufferIdGenericBlob));
                result = pPacket->AddCmdBufferReference(pCmdBuffer, NULL);
                result = GetHwContext()->Submit(GetCSLSession(), m_hDevice, pPacket);
            }
        }
    }

    if (NULL != pCmdBuffer)
    {
        m_pBPSCmdBufferManager[BPSCmdBufferGenericBlob]->Recycle(reinterpret_cast<PacketResource*>(pCmdBuffer));
    }
    if (NULL != pPacket)
    {
        m_pIQPacketManager->Recycle(reinterpret_cast<PacketResource*>(pPacket));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSNode::InitializeFrameProcessData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::InitializeFrameProcessData(
    BpsFrameProcessData*    pFrameProcessData,
    BpsIQSettings*          pBPSIQsettings)
{
    Utils::Memset(&pFrameProcessData->frameSets[0], 0x0, sizeof(pFrameProcessData->frameSets));

    pFrameProcessData->numFrameSetsInBatch    = 1;
    pFrameProcessData->ubwcStatsBufferAddress = 0;
    pFrameProcessData->cdmProgramArrayAddr    = 0;
    pFrameProcessData->iqSettingsAddr         = 0;
    pFrameProcessData->stripingLibOutAddr     = 0;
    pFrameProcessData->cdmBufferAddress       = 0;

    InitializeCDMProgramArray(pFrameProcessData);
    InitializeBPSIQSettings(pBPSIQsettings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CamxResult                  result                            = CamxResultSuccess;
    Packet*                     pIQPacket                         = NULL;
    CmdBuffer*                  pBPSCmdBuffer[BPSCmdBufferMaxIds] = { NULL };
    NodeProcessRequestData*     pNodeRequestData                  = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*      pPerRequestPorts                  = pExecuteProcessRequestData->pEnabledPortsInfo;
    UINT64                      requestId                         = pNodeRequestData->pCaptureRequest->requestId;
    ISPInputData                moduleInput                       = { 0 };
    ISPInternalData             calculatedData                    = { 0 };
    ISPCalculatedMetadata       calculatedMetadata                = {};
    BpsFrameProcess*            pFrameProcess                     = NULL;
    BpsFrameProcessData*        pFrameProcessData                 = NULL;
    const ImageFormat*          pImageFormat                      = NULL;
    const SensorMode*           pSensorModeData                   = NULL; ///< Sensor related data for the current mode
    BpsIQSettings*              pBPSIQsettings                    = NULL;
    VOID*                       pOEMSetting                       = NULL;
    UINT32                      metaTag                           = 0;
    BOOL                        sensorConnected                   = FALSE;
    BOOL                        statsConnected                    = FALSE;
    BOOL                        hasDependency                     = FALSE;
    INT                         sequenceNumber                    = pNodeRequestData->processSequenceId;
    UINT32                      cameraId                          = GetPipeline()->GetCameraId();
    BOOL                        isMasterCamera                    = TRUE;
    BPSTuningMetadata*          pTuningMetadata                   = NULL;
    TuningDataManager*          pCurrentTuningDataManager         = NULL;
    AECFrameControl             AECUpdateData                     = {};
    AWBFrameControl             AWBUpdateData                     = {};
    AWBStatsControl             AWBStatsUpdateData                = {};
    AECStatsControl             AECStatsUpdateData                = {};
    AFStatsControl              AFStatsUpdateData                 = {};
    ISPStripeConfig             frameConfig                       = {};
    BPSClockAndBandwidth        clockAndBandwidth                 = { 0 };
    // FD is connected in pipeline with sensor. if the node is part of realtime  then read from offset 1 (from request n-1)
    // otherwise from input data which is always n.
    m_fdDataOffset                                                = IsRealTime() ? 1 : 0;


    // Cannot have HFR request with BPS node
    CAMX_ASSERT(1 == pNodeRequestData->pCaptureRequest->numBatchedFrames);

    // Need to determine if sensor node is hooked up to set the appropriate dependencies
    if (TRUE == IsRealTime())
    {
        sensorConnected = TRUE;
    }
    if (TRUE == GetPipeline()->HasStatsNode())
    {
        statsConnected = TRUE;
    }

    if (0 == sequenceNumber)
    {
        SetDependencies(pExecuteProcessRequestData, sensorConnected, statsConnected);
    }

    if (pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask)
    {
        hasDependency = TRUE;
    }

    // If the sequenceRequest is 1, it means that we were called from the DRQ w/
    // all deps satisified.
    if ((CamxResultSuccess == result) &&
        ((1 == sequenceNumber) || ((0 == sequenceNumber) && (sensorConnected == FALSE) && (FALSE == hasDependency))))
    {
        UINT32 numberOfCamerasRunning;
        UINT32 currentCameraId;
        BOOL   isMultiCameraUsecase;

        GetMultiCameraInfo(&isMultiCameraUsecase, &numberOfCamerasRunning, &currentCameraId, &isMasterCamera);
        if (TRUE == isMultiCameraUsecase)
        {
            UINT32 cameraIDmetaTag = 0;
            result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.metadataOwnerInfo",
                     "MetadataOwner",
                     &cameraIDmetaTag);

            if (CamxResultSuccess == result)
            {
                UINT              propertiesIPE[]   = { cameraIDmetaTag | InputMetadataSectionMask };
                static const UINT Length            = CAMX_ARRAY_SIZE(propertiesIPE);
                VOID*             pData[Length]     = { 0 };
                UINT64            offsets[Length]   = { 0 };

                result = GetDataList(propertiesIPE, pData, offsets, Length);

                if (CamxResultSuccess == result)
                {
                    if (NULL != pData[0])
                    {
                        cameraId = *reinterpret_cast<UINT*>(pData[0]);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "BPS:%u Can't get Current camera ID!, pData is NULL", InstanceID());
                    }
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "BPS:%u Can't query vendor tag: MetadataOwner", InstanceID());
            }
            CAMX_LOG_VERBOSE(CamxLogGroupBPS,
                "node: %s,req ID %llu numberOfCamerasRunning = %d, cameraId = %d isMasterCamera = %d",
                NodeIdentifierString(), requestId, numberOfCamerasRunning, cameraId, isMasterCamera);

            GetStaticMetadata(cameraId);
        }
        pTuningMetadata = (TRUE == isMasterCamera) ? m_pTuningMetadata : NULL;

        pCurrentTuningDataManager = GetTuningDataManagerWithCameraId(cameraId);

        if (cameraId == m_cameraId)
        {
            m_camIdChanged = FALSE;
        }
        else
        {
            m_cameraId     = cameraId;
            m_camIdChanged = TRUE;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "instance=%p, cameraId = %d,currentCameraId=%d, isMasterCamera = %d",
                         this, cameraId, currentCameraId, isMasterCamera);

        if (TRUE == m_OEMIQSettingEnable)
        {
            result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.iqsettings", "OEMBPSIQSetting", &metaTag);
            CAMX_ASSERT(CamxResultSuccess == result);
            const UINT OEMProperty[1] = { metaTag | InputMetadataSectionMask };
            VOID* pOEMData[1] = { 0 };
            UINT64 OEMDataOffset[1] = { 0 };
            GetDataList(OEMProperty, pOEMData, OEMDataOffset, 1);
            pOEMSetting = pOEMData[0];
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "BPS continuing with Non-OEM settings");
        }

        // Sensor begins with publishing the selected mode into the "FirstValidRequestId" slot in the perFrame metadata pool
        pSensorModeData = GetSensorModeData(sensorConnected);
        CAMX_ASSERT(pSensorModeData != NULL);
        if (NULL == pSensorModeData)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Sensor Mode Data is NULL.");
        }

        CAMX_ASSERT(m_pOTPData != NULL);
        moduleInput.pOTPData       = m_pOTPData;

        // Get CmdBuffer for request
        pIQPacket = GetPacketForRequest(requestId, m_pIQPacketManager);

        if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferGenericBlob])
        {
            pBPSCmdBuffer[BPSCmdBufferGenericBlob] =
                GetCmdBufferForRequest(requestId, m_pBPSCmdBufferManager[BPSCmdBufferGenericBlob]);
        }

        if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferFrameProcess])
        {
            pBPSCmdBuffer[BPSCmdBufferFrameProcess] =
                GetCmdBufferForRequest(requestId, m_pBPSCmdBufferManager[BPSCmdBufferFrameProcess]);
        }

        if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferIQSettings])
        {
            pBPSCmdBuffer[BPSCmdBufferIQSettings] =
                GetCmdBufferForRequest(requestId, m_pBPSCmdBufferManager[BPSCmdBufferIQSettings]);
        }

        if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferCDMProgram])
        {
            pBPSCmdBuffer[BPSCmdBufferCDMProgram] =
                GetCmdBufferForRequest(requestId, m_pBPSCmdBufferManager[BPSCmdBufferCDMProgram]);
        }

        if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferDMIHeader])
        {
            pBPSCmdBuffer[BPSCmdBufferDMIHeader] =
                GetCmdBufferForRequest(requestId, m_pBPSCmdBufferManager[BPSCmdBufferDMIHeader]);
        }

        if ((NULL == pIQPacket)                                                         ||
            (NULL == pBPSCmdBuffer[BPSCmdBufferFrameProcess])                           ||
            ((m_maxLUT > 0) && (NULL == pBPSCmdBuffer[BPSCmdBufferDMIHeader]))          ||
            (NULL == pBPSCmdBuffer[BPSCmdBufferCDMProgram])                             ||
            (NULL == pBPSCmdBuffer[BPSCmdBufferIQSettings]))
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "Null IQPacket or CmdBuffer %x, %x, %x, %x, %x",
                          pIQPacket,
                          pBPSCmdBuffer[BPSCmdBufferFrameProcess],
                          pBPSCmdBuffer[BPSCmdBufferDMIHeader],
                          pBPSCmdBuffer[BPSCmdBufferCDMProgram],
                          pBPSCmdBuffer[BPSCmdBufferIQSettings]);
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            pFrameProcess = reinterpret_cast<BpsFrameProcess*>(
                                pBPSCmdBuffer[BPSCmdBufferFrameProcess]->BeginCommands(
                                    m_maxCmdBufferSizeBytes[BPSCmdBufferFrameProcess] / sizeof(UINT32)));

            CAMX_ASSERT(NULL != pFrameProcess);
            if (NULL == pFrameProcess)
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Frame Process Data.");
            }
        }
        if (CamxResultSuccess == result)
        {
            pFrameProcess->userArg          = m_hDevice;
            pFrameProcessData               = &pFrameProcess->cmdData;
            pFrameProcessData->requestId    = static_cast<UINT32>(requestId);
            pBPSIQsettings = reinterpret_cast<BpsIQSettings*>(
                                 pBPSCmdBuffer[BPSCmdBufferIQSettings]->BeginCommands(
                                     m_maxCmdBufferSizeBytes[BPSCmdBufferIQSettings] / sizeof(UINT32)));

            InitializeFrameProcessData(pFrameProcessData, pBPSIQsettings);

            // BPS has only one input port so GetInputPortImageFormat is called with zero.
            pImageFormat = GetInputPortImageFormat(0);

            if (NULL != pImageFormat)
            {
                const SensorMode* pSensorModeRes0 = NULL;

                // Setup the Input data for IQ Parameter
                moduleInput.pOEMIQSetting                        = pOEMSetting;
                moduleInput.titanVersion                         = m_titanVersion;
                moduleInput.frameNum                             = requestId;
                moduleInput.pipelineBPSData.pIQSettings          = pBPSIQsettings;
                moduleInput.pipelineBPSData.ppBPSCmdBuffer       = pBPSCmdBuffer;
                moduleInput.pipelineBPSData.width                = pImageFormat->width;
                moduleInput.pipelineBPSData.height               = pImageFormat->height;
                moduleInput.pipelineBPSData.pBPSPathEnabled      = &m_BPSPathEnabled[0];
                moduleInput.pHwContext                           = GetHwContext();
                moduleInput.pTuningDataManager                   = GetTuningDataManagerWithCameraId(cameraId);
                moduleInput.pHALTagsData                         = &m_HALTagsData;
                moduleInput.pCmdBuffer                           = pBPSCmdBuffer[BPSCmdBufferCDMProgram];
                moduleInput.pDMICmdBuffer                        = pBPSCmdBuffer[BPSCmdBufferDMIHeader];
                moduleInput.pCalculatedData                      = &calculatedData;
                moduleInput.pBPSTuningMetadata                   = pTuningMetadata;
                moduleInput.pAECUpdateData                       = &AECUpdateData;
                moduleInput.pAWBUpdateData                       = &AWBUpdateData;
                moduleInput.pAWBStatsUpdateData                  = &AWBStatsUpdateData;
                moduleInput.pAECStatsUpdateData                  = &AECStatsUpdateData;
                moduleInput.pAFStatsUpdateData                   = &AFStatsUpdateData;
                moduleInput.pStripeConfig                        = &frameConfig;
                moduleInput.registerBETEn                        = FALSE;
                moduleInput.maximumPipelineDelay                 = GetMaximumPipelineDelay();

                // Set the sensor data information for ISP input
                moduleInput.sensorData.CAMIFCrop.firstPixel      = pSensorModeData->cropInfo.firstPixel;
                moduleInput.sensorData.CAMIFCrop.firstLine       = pSensorModeData->cropInfo.firstLine;
                moduleInput.sensorData.CAMIFCrop.lastPixel       = pSensorModeData->cropInfo.lastPixel;
                moduleInput.sensorData.CAMIFCrop.lastLine        = pSensorModeData->cropInfo.lastLine;
                moduleInput.sensorData.format                    = pSensorModeData->format;
                moduleInput.sensorData.isBayer                   = IsSensorModeFormatBayer(moduleInput.sensorData.format);
                moduleInput.sensorData.isMono                    = IsSensorModeFormatMono(moduleInput.sensorData.format);
                moduleInput.sensorData.sensorOut.width           = pSensorModeData->resolution.outputWidth;
                moduleInput.sensorData.sensorOut.height          = pSensorModeData->resolution.outputHeight;
                moduleInput.sensorData.sensorScalingFactor       = pSensorModeData->downScaleFactor;
                moduleInput.sensorData.sensorBinningFactor       = static_cast<FLOAT>(pSensorModeData->binningTypeH);
                moduleInput.sensorData.sensorOut.offsetX         = pSensorModeData->offset.xStart;
                moduleInput.sensorData.sensorOut.offsetY         = pSensorModeData->offset.yStart;
                moduleInput.sensorData.dGain                     = ReadSensorDigitalGain(sensorConnected);
                moduleInput.sensorID                             = cameraId;
                moduleInput.sensorData.ZZHDRColorPattern         = pSensorModeData->ZZHDRColorPattern;
                moduleInput.sensorData.ZZHDRFirstExposure        = pSensorModeData->ZZHDRFirstExposure;
                moduleInput.useHardcodedRegisterValues           = CheckToUseHardcodedRegValues(moduleInput.pHwContext);
                moduleInput.sensorBitWidth                       = pSensorModeData->streamConfig[0].bitWidth;
                moduleInput.pCalculatedMetadata                  = &calculatedMetadata;

                HardcodeSettings(&moduleInput);

                GetInSensorSeamlessControltState(&moduleInput);

                if ((InSensorHDR3ExpStart   == moduleInput.seamlessInSensorState) ||
                    (InSensorHDR3ExpEnabled == moduleInput.seamlessInSensorState))
                {
                    moduleInput.sensorData.isIHDR = TRUE;
                    CAMX_LOG_INFO(CamxLogGroupBPS, "BPS: %s Seamless In-sensor HDR 3 Exposure is set, control state = %u",
                                  NodeIdentifierString(),
                                  moduleInput.seamlessInSensorState);
                }
                else
                {
                    for (UINT32 i = 0; i < pSensorModeData->capabilityCount; i++)
                    {
                        if (pSensorModeData->capability[i] == SensorCapability::IHDR)
                        {
                            moduleInput.sensorData.isIHDR = TRUE;
                            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "sensor I-HDR mode");
                            break;
                        }
                    }
                }

                GetSensorModeRes0Data(&pSensorModeRes0);
                if (NULL != pSensorModeRes0)
                {
                    moduleInput.sensorData.fullResolutionWidth   = pSensorModeRes0->resolution.outputWidth;
                    moduleInput.sensorData.fullResolutionHeight  = pSensorModeRes0->resolution.outputHeight;
                }
                else
                {
                    moduleInput.sensorData.fullResolutionWidth   = pSensorModeData->resolution.outputWidth;
                    moduleInput.sensorData.fullResolutionHeight  = pSensorModeData->resolution.outputHeight;
                }
                // Get Sensor PDAF information
                GetPDAFInformation(&moduleInput.sensorData.sensorPDAFInfo);

                // Update data from 3A
                Set3AInputData(&moduleInput);

                if (TRUE == m_OEMStatsSettingEnable)
                {
                    GetOEMStatsConfig(&moduleInput);
                }

                if (TRUE == m_libInitialData.isSucceed)
                {
                    moduleInput.pLibInitialData = m_libInitialData.pLibData;
                }

                if ((TRUE == GetStaticSettings()->offlineImageDumpOnly) && (TRUE != IsRealTime()))
                {
                    CHAR  dumpFilename[256];
                    FILE* pFile = NULL;
                    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/BPS_regdump.txt", ConfigFileDirectory);
                    pFile = CamX::OsUtils::FOpen(dumpFilename, "w");
                    CamX::OsUtils::FPrintF(pFile, "******** BPS REGISTER DUMP FOR BET ********************* \n");
                    CamX::OsUtils::FClose(pFile);
                    moduleInput.dumpRegConfig                        = 0x7FFFFF;
                    moduleInput.regOffsetIndex                       = OffsetOfBPSIQModuleIndex;
                }
                else
                {
                    moduleInput.dumpRegConfig                        = static_cast<Titan17xContext*>(GetHwContext())->
                                                                       GetTitan17xSettingsManager()->
                                                                       GetTitan17xStaticSettings()->
                                                                       dumpBPSRegConfigMask;
                    moduleInput.regOffsetIndex                       = OffsetOfBPSIQModuleIndex;
                }

                if (m_pPreTuningDataManager == pCurrentTuningDataManager)
                {
                    moduleInput.tuningModeChanged = ISPIQModule::IsTuningModeDataChanged(
                        pExecuteProcessRequestData->pTuningModeData,
                        &m_tuningData);
                }
                else
                {
                    moduleInput.tuningModeChanged = TRUE;
                    m_pPreTuningDataManager = pCurrentTuningDataManager;
                    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "TuningDataManager pointer is updated");
                }

                // if camera ID changed, it should set tuningModeChanged TRUE to trigger all IQ
                // module to update tuning parameters
                if (TRUE == m_camIdChanged)
                {
                    moduleInput.tuningModeChanged = TRUE;
                }

                // Needed to have different tuning data for different instances of a node within same pipeline
                //
                // Also, cache tuning mode selector data for comparison for next frame, to help
                // optimize tuning data (tree) search in the IQ modules
                if (TRUE == moduleInput.tuningModeChanged)
                {
                    Utils::Memcpy(&m_tuningData, pExecuteProcessRequestData->pTuningModeData, sizeof(ChiTuningModeParameter));

                    if (((BPSProfileId::BPSProfileIdBlendPost  == m_instanceProperty.profileId)        &&
                         (BPSProcessingType::BPSProcessingMFNR == m_instanceProperty.processingType))  ||
                        ((BPSProfileId::BPSProfileIdBlendPost  == m_instanceProperty.profileId)        &&
                         (BPSProcessingType::BPSProcessingMFSR == m_instanceProperty.processingType)))
                    {
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2 =
                            ChiModeFeature2SubModeType::MFNRBlend;
                    }
                    UpdateICAChromatixGridData(cameraId, pExecuteProcessRequestData->pTuningModeData);
                }

                // Now refer to the updated tuning mode selector data
                moduleInput.pTuningData = &m_tuningData;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Null ImageFormat for Input");
                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess == result)
            {
                // Update batch count from node request data
                if (1 != pNodeRequestData->pCaptureRequest->numBatchedFrames)
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "%s: Invalid batch count : batch frames %d - batch mode not supported",
                                   NodeIdentifierString(), pNodeRequestData->pCaptureRequest->numBatchedFrames);
                    result = CamxResultEInvalidArg;
                }
                else
                {
                    pFrameProcessData->numFrameSetsInBatch = pNodeRequestData->pCaptureRequest->numBatchedFrames;
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            result = GetMetadataTags(&moduleInput);
        }

        if ((NULL != pTuningMetadata) && (TRUE == isMasterCamera) && (CamxResultSuccess == result))
        {
            // Only use debug data on the master camera
            PrepareTuningMetadataDump(&moduleInput);
        }
        else if (NULL != m_pTuningMetadata)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupDebugData,
                             "Tuning-metadata:BPS: SKIP: reqID: %llu isMaster: %u RT: %u InstanceID: %u, "
                             "profId: %u, procType: %u mfNum: %u",
                             moduleInput.frameNum,
                             isMasterCamera,
                             IsRealTime(),
                             InstanceID(),
                             m_instanceProperty.profileId,
                             m_instanceProperty.processingType,
                             moduleInput.mfFrameNum);
        }

        if (CamxResultSuccess == result)
        {
            result = GetFaceROI(&moduleInput);
        }

        if (CamxResultSuccess == result)
        {
            // Update the BPS IQ settings. these are settings for blocks that do not have a module implementation.
            result = FillIQSetting(&moduleInput, pBPSIQsettings);
        }

        if (CamxResultSuccess == result)
        {
            result = SetScaleRatios(&moduleInput);
        }

        if (CamxResultSuccess == result)
        {
            // API invoking the IQ modules to update the register/ DMI settings
            result = ProgramIQConfig(&moduleInput, pBPSCmdBuffer[BPSCmdBufferGenericBlob]);
            UpdateLUTData(pBPSIQsettings);
        }

        if (CamxResultSuccess == result)
        {
            pIQPacket->SetRequestId(GetCSLSyncId(requestId));
            pIQPacket->SetOpcode(CSLDeviceType::CSLDeviceTypeICP, CSLPacketOpcodesBPSUpdate);

            PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[0];

            if ((NULL != pInputPort) && (NULL != pInputPort->pImageBuffer))
            {
                UINT32   numFences   = (NULL == pInputPort->phFence) ? 0 : 1;
                CSLFence hInputFence = 0;

                ImageBuffer* pImageBuffer = pInputPort->pImageBuffer;

                CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Input port %d: cslh=0x%x, ft=0x%x, buffer size %d,"
                    "w=%d, h=%d,(w=%d, h=%d, s=%d, sh=%d)",
                    pInputPort->portId,
                    pImageBuffer->GetCSLBufferInfo()->hHandle,
                    pImageBuffer->GetFormat()->format,
                    ImageFormatUtils::GetTotalSize(pImageBuffer->GetFormat()),
                    pImageBuffer->GetFormat()->width,
                    pImageBuffer->GetFormat()->height,
                    pImageBuffer->GetFormat()->formatParams.yuvFormat[0].width,
                    pImageBuffer->GetFormat()->formatParams.yuvFormat[0].height,
                    pImageBuffer->GetFormat()->formatParams.yuvFormat[0].planeStride,
                    pImageBuffer->GetFormat()->formatParams.yuvFormat[0].sliceHeight);

                // IO config for Input
                result = pIQPacket->AddIOConfig(pInputPort->pImageBuffer,
                                                pInputPort->portId,
                                                CSLIODirection::CSLIODirectionInput,
                                                pInputPort->phFence,
                                                numFences,
                                                NULL,
                                                NULL,
                                                0);

                hInputFence = ((NULL != pInputPort->phFence) ? *(pInputPort->phFence) : CSLInvalidHandle);

                if (0 == numFences)
                {
                    CAMX_LOG_INFO(CamxLogGroupBPS,
                                  "node %s reporting Input config, portId=%d, request=%llu",
                                  NodeIdentifierString(),
                                  pInputPort->portId,
                                  pNodeRequestData->pCaptureRequest->requestId);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupBPS,
                                  "node %s, reporting Input config, portId=%d, hFence=%d, request=%llu",
                                  NodeIdentifierString(),
                                  pInputPort->portId,
                                  hInputFence,
                                  pNodeRequestData->pCaptureRequest->requestId);
                }

                if (CamxResultSuccess == result)
                {
                    // Update the frame buffer data for input buffer
                    result = FillFrameBufferData(pBPSCmdBuffer[BPSCmdBufferFrameProcess],
                                                 pInputPort->pImageBuffer,
                                                 pFrameProcessData->numFrameSetsInBatch,
                                                 pInputPort->portId);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Add input IO config failed");

                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Input Port/Image Buffer is Null ");
                result = CamxResultEInvalidArg;
            }
        }

        if (CamxResultSuccess == result)
        {
            for (UINT i = 0; i < pPerRequestPorts->numOutputPorts; i++)
            {
                PerRequestOutputPortInfo* pOutputPort = &pPerRequestPorts->pOutputPorts[i];

                // Cannot have HFR with a BPS request
                CAMX_ASSERT((NULL != pOutputPort) && (1 == pOutputPort->numOutputBuffers));

                // Cannot have HFR with a BPS request so only the first ImageBuffer (index 0) will ever be valid
                ImageBuffer* pImageBuffer = pOutputPort->ppImageBuffer[0];

                if (NULL != pImageBuffer)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupBPS,
                        "Output port %d: cslh=0x%x, ft=0x%x, buffer size %d, w=%d, h=%d,(w=%d, h=%d, s=%d, sh=%d)",
                        pOutputPort->portId,
                        pImageBuffer->GetCSLBufferInfo()->hHandle,
                        pImageBuffer->GetFormat()->format,
                        ImageFormatUtils::GetTotalSize(pImageBuffer->GetFormat()),
                        pImageBuffer->GetFormat()->width,
                        pImageBuffer->GetFormat()->height,
                        pImageBuffer->GetFormat()->formatParams.yuvFormat[0].width,
                        pImageBuffer->GetFormat()->formatParams.yuvFormat[0].height,
                        pImageBuffer->GetFormat()->formatParams.yuvFormat[0].planeStride,
                        pImageBuffer->GetFormat()->formatParams.yuvFormat[0].sliceHeight);

                    // Add IO config for output buffer
                    result = pIQPacket->AddIOConfig(pImageBuffer,
                                                    pOutputPort->portId,
                                                    CSLIODirection::CSLIODirectionOutput,
                                                    pOutputPort->phFence,
                                                    1,
                                                    NULL,
                                                    NULL,
                                                    0);

                    CAMX_ASSERT(NULL != pOutputPort->phFence);

                    CAMX_LOG_INFO(CamxLogGroupBPS,
                                  "node %s, reporting I/O config, portId=%d, imgBuf=0x%x, hFence=%d, request=%llu",
                                  NodeIdentifierString(),
                                  pOutputPort->portId,
                                  pImageBuffer,
                                  *(pOutputPort->phFence),
                                  pNodeRequestData->pCaptureRequest->requestId);

                    if (CamxResultSuccess == result)
                    {
                        // Fill frame  buffer data for output buffer
                        result = FillFrameBufferData(pBPSCmdBuffer[BPSCmdBufferFrameProcess],
                                                     pImageBuffer,
                                                     pFrameProcessData->numFrameSetsInBatch,
                                                     pOutputPort->portId);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "Add output IO config failed");

                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Output Port/Image is Null ");
                    result = CamxResultEInvalidArg;
                }
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Output Port: Add IO config failed");
                    break;
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            result = FillFrameCDMArray(pBPSCmdBuffer, pFrameProcessData);
        }

        // Following code can only be called after SetScaleRatios(...)
        if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferStriping])
        {
            pBPSCmdBuffer[BPSCmdBufferStriping] =
                GetCmdBufferForRequest(requestId, m_pBPSCmdBufferManager[BPSCmdBufferStriping]);
        }

        if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferBLMemory])
        {
            pBPSCmdBuffer[BPSCmdBufferBLMemory] =
                GetCmdBufferForRequest(requestId, m_pBPSCmdBufferManager[BPSCmdBufferBLMemory]);
        }

        if ((CamxResultSuccess == result) && (TRUE == m_swStriping))
        {
            result = FillStripingParams(pFrameProcessData, pBPSIQsettings, pBPSCmdBuffer, &clockAndBandwidth);
        }
        if (CamxResultSuccess == result)
        {
            result = PatchBLMemoryBuffer(pFrameProcessData, pBPSCmdBuffer);
        }
        if (CamxResultSuccess == result)
        {
            CheckAndUpdateClockBW(pBPSCmdBuffer[BPSCmdBufferGenericBlob], pExecuteProcessRequestData,
                                  &clockAndBandwidth);
        }
        if (CamxResultSuccess == result)
        {
            result = CommitAllCommandBuffers(pBPSCmdBuffer);
        }
        if (CamxResultSuccess == result)
        {
            result = pIQPacket->CommitPacket();
        }
        if (CamxResultSuccess == result)
        {
            result = pIQPacket->AddCmdBufferReference(pBPSCmdBuffer[BPSCmdBufferFrameProcess], NULL);
        }
        if (CamxResultSuccess == result)
        {
            if (pBPSCmdBuffer[BPSCmdBufferGenericBlob]->GetResourceUsedDwords() > 0)
            {
                pBPSCmdBuffer[BPSCmdBufferGenericBlob]->SetMetadata(static_cast<UINT32>(CSLICPCmdBufferIdGenericBlob));
                result = pIQPacket->AddCmdBufferReference(pBPSCmdBuffer[BPSCmdBufferGenericBlob], NULL);
            }
        }
        if ((CamxResultSuccess == result) && (BPSProfileId::BPSProfileIdHNR != m_instanceProperty.profileId))
        {
            result = PostMetadata(&moduleInput);
        }

        if ((NULL != pTuningMetadata) && (TRUE == isMasterCamera) && (CamxResultSuccess == result))
        {
            // Only use debug data on the master camera
            DumpTuningMetadata(&moduleInput);
        }

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Submit packets for BPS:%d request %llu",
                             InstanceID(),
                             requestId);

            result = GetHwContext()->Submit(GetCSLSession(), m_hDevice, pIQPacket);

            if (CamxResultSuccess != result)
            {
                if (CamxResultECancelledRequest == result)
                {
                    CAMX_LOG_INFO(CamxLogGroupBPS, "%s Submit packets with requestId = %llu failed %d"
                        " due to Ongoing Flush",
                        NodeIdentifierString(),
                        requestId,
                        result);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "%s Submit packets with requestId = %llu failed %d"
                        " due to Ongoing Flush",
                        NodeIdentifierString(),
                        requestId,
                        result);
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "BPS:%d Submitted packet(s) with requestId = %llu successfully",
                          InstanceID(),
                          requestId);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetFaceROI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::GetFaceROI(
    ISPInputData* pInputData)
{
    CamxResult   result                    = CamxResultSuccess;
    UINT32       metaTagFDRoi              = 0;
    BOOL         isFDPostingResultsEnabled = FALSE;

    GetFDPerFrameMetaDataSettings(m_fdDataOffset, &isFDPostingResultsEnabled, NULL);
    if (TRUE == isFDPostingResultsEnabled)
    {
        result = VendorTagManager::QueryVendorTagLocation(VendorTagSectionOEMFDResults,
                                                          VendorTagNameOEMFDResults, &metaTagFDRoi);

        if (CamxResultSuccess == result)
        {
            UINT              GetProps[]              = { metaTagFDRoi };
            static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
            VOID*             pData[GetPropsLength]   = { 0 };
            UINT64            offsets[GetPropsLength] = { 0 };

            if (FALSE == IsRealTime())
            {
                GetProps[0] |= InputMetadataSectionMask;
            }
            offsets[0] = m_fdDataOffset;

            GetDataList(GetProps, pData, offsets, GetPropsLength);

            if (NULL != pData[0])
            {
                FaceROIInformation faceRoiData = {};
                RectangleCoordinate* pRoiRect  = NULL;

                Utils::Memcpy(&faceRoiData, pData[0], sizeof(FaceROIInformation));
                pInputData->fDData.numberOfFace = static_cast<UINT16>(
                    (faceRoiData.ROICount > MAX_FACE_NUM) ? MAX_FACE_NUM : faceRoiData.ROICount);

                for (UINT32 i = 0; i < pInputData->fDData.numberOfFace; i++)
                {
                    pRoiRect = &faceRoiData.unstabilizedROI[i].faceRect;

                    // FD is already wrt camif, no need to translate

                    pInputData->fDData.faceCenterX[i] = static_cast<INT16>(pRoiRect->left + (pRoiRect->width / 2));
                    pInputData->fDData.faceCenterY[i] = static_cast<INT16>(pRoiRect->top + (pRoiRect->height / 2));
                    pInputData->fDData.faceRadius[i]  = static_cast<INT16>(Utils::MinUINT32(pRoiRect->width, pRoiRect->height));

                    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "center x:%d y:%d  r:%d", pInputData->fDData.faceCenterX[i],
                                     pInputData->fDData.faceCenterY[i], pInputData->fDData.faceRadius[i]);
                }

                result = CamxResultSuccess;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Face ROI is not published");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "query Face ROI failed result %d", result);
        }
    }

    return CamxResultSuccess;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::SetupDeviceResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::SetupDeviceResource(
    CSLBufferInfo*     pConfigIOMem,
    CSLDeviceResource* pResource)
{
    CamxResult                  result                      = CamxResultSuccess;
    CSLICPAcquireDeviceInfo*    pICPResource                = NULL;
    CSLICPResourceInfo*         pICPOutResource             = NULL;
    SIZE_T                      resourceSize                = 0;
    UINT                        numInputPort                = 0;
    UINT                        numOutputPort               = 0;
    UINT                        outputPortIndex             = 0;
    BpsConfigIo*                pConfigIO                   = NULL;
    BpsConfigIoData*            pConfigIOData               = NULL;
    UINT                        inputPortId[BPSMaxInput]    = { 0 };
    UINT                        outputPortId[BPSMaxOutput]  = { 0 };
    UINT                        FPS                         = 30;
    const ImageFormat*          pImageFormat                = NULL;
    BPS_IO_IMAGES               firmwarePortId;
    BOOL                        validFormat;
    UINT32                      BPSPathIndex;
    UINT32                      resourceType;

    // Get Input Port List
    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    // Get Output Port List
    GetAllOutputPortIds(&numOutputPort, &outputPortId[0]);

    if ((numInputPort <= 0)             ||
        (numOutputPort <= 0)            ||
        (numOutputPort > BPSMaxOutput)  ||
        (numInputPort > BPSMaxInput))
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "invalid input/output port");
        result = CamxResultEUnsupported;
    }

    if (CamxResultSuccess == result)
    {
        GetBPSResrouceType(&resourceType);

        /// @todo (CAMX-2142) Need correct (non-real-time) fps from HAL
        m_FPS = 30;
        static const UINT PropertiesBPS[] =
        {
            PropertyIDUsecaseFPS
        };
        static const UINT Length = CAMX_ARRAY_SIZE(PropertiesBPS);
        VOID* pData[Length] = { 0 };
        UINT64 propertyDataBPSOffset[Length] = { 0 };

        GetDataList(PropertiesBPS, pData, propertyDataBPSOffset, Length);
        if (NULL != pData[0])
        {
            m_FPS = *reinterpret_cast<UINT*>(pData[0]);
        }

        UINT clockBWResSize = Utils::MaxUINT32(sizeof(CSLICPClockBandwidthRequest),
            (sizeof(CSLICPClockBandwidthRequestV2) +
            (sizeof(CSLAXIperPathBWVote))));
        m_pGenericClockAndBandwidthData = static_cast<BYTE*>(CAMX_CALLOC(clockBWResSize));
        if (NULL == m_pGenericClockAndBandwidthData)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Out of memory  - allocating GenericClockAndBandwidthData");
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            resourceSize = sizeof(CSLICPAcquireDeviceInfo) + (sizeof(CSLICPResourceInfo) * (numOutputPort - 1));
            pICPResource = static_cast<CSLICPAcquireDeviceInfo*>(CAMX_CALLOC(resourceSize));
            if (NULL == pICPResource)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Out of memory  - allocating Acquire device info");
                result = CamxResultENoMemory;
            }
        }

        if (CamxResultSuccess == result)
        {
            pICPResource->resourceType = resourceType;
            pICPOutResource            = &pICPResource->outputResource[0];
            pConfigIO                  = reinterpret_cast<BpsConfigIo*>(pConfigIOMem->pVirtualAddr);
            pConfigIO->userArg         = 0;
            pConfigIOData              = &pConfigIO->cmdData;

            CamX::Utils::Memset(pConfigIOData, 0, sizeof(*pConfigIOData));

            CAMX_ASSERT(numInputPort == 1);

            // BPS has only one input port so GetInputPortImageFormat is called with zero.

            pImageFormat = GetInputPortImageFormat(0);
            if (NULL != pImageFormat)
            {
                // Once CHI Supports end to end BPS IDEAL RAW that is 14b bayer output we need to revisit
                // below check as by default it is "IMAGE_FORMAT_BAYER_10"
                pConfigIOData->images[BPS_INPUT_IMAGE].info.format                 =
                    TranslateBPSFormatToFirmwareImageFormat(pImageFormat, BPS_INPUT_IMAGE);
                pConfigIOData->images[BPS_INPUT_IMAGE].info.dimensions.widthPixels = pImageFormat->width;
                pConfigIOData->images[BPS_INPUT_IMAGE].info.dimensions.heightLines = pImageFormat->height;
                pConfigIOData->images[BPS_INPUT_IMAGE].info.pixelPackingAlignment  = PIXEL_LSB_ALIGNED;
                pConfigIOData->images[BPS_INPUT_IMAGE].info.enableByteSwap         = 0;

                CAMX_LOG_CONFIG(CamxLogGroupBPS, "node %s Input : firmwarePortId %d format %d, width %d, height %d",
                              NodeIdentifierString(),
                              BPS_INPUT_IMAGE, pImageFormat->format,
                              pImageFormat->width, pImageFormat->height);

                if (TRUE == ImageFormatUtils::IsRAW(pImageFormat))
                {
                    const SensorMode* pSensorMode = GetSensorModeData(IsRealTime());
                    CAMX_ASSERT(NULL != pSensorMode);
                    if (NULL != pSensorMode)
                    {
                        pConfigIOData->images[BPS_INPUT_IMAGE].info.bayerOrder =
                            TranslateBPSFormatToFirmwareBayerOrder(pSensorMode->format);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to get Sensor format.. BPS Output may be Wrong");
                        pConfigIOData->images[BPS_INPUT_IMAGE].info.bayerOrder =
                            FIRST_PIXEL_R;
                    }
                    pConfigIOData->images[BPS_INPUT_IMAGE].bufferLayout[0].bufferStride =
                        pImageFormat->formatParams.rawFormat.stride;
                    pConfigIOData->images[BPS_INPUT_IMAGE].bufferLayout[0].bufferHeight =
                        pImageFormat->formatParams.rawFormat.sliceHeight;
                    CAMX_LOG_INFO(CamxLogGroupBPS, "Raw output pattern %d, bpp %d, stride %d, scanline %d",
                                     pImageFormat->formatParams.rawFormat.colorFilterPattern,
                                     pImageFormat->formatParams.rawFormat.bitsPerPixel,
                                     pImageFormat->formatParams.rawFormat.stride,
                                     pImageFormat->formatParams.rawFormat.sliceHeight);
                }
                else
                {
                    validFormat = ((TRUE == ImageFormatUtils::IsYUV(pImageFormat)) ||
                        (TRUE == ImageFormatUtils::IsUBWC(pImageFormat->format)));

                    CAMX_ASSERT(TRUE == validFormat);

                    pConfigIOData->images[BPS_INPUT_IMAGE].info.yuv422Order = PIXEL_ORDER_Y_U_Y_V;
                    for (UINT plane = 0; plane < ImageFormatUtils::GetNumberOfPlanes(pImageFormat); plane++)
                    {
                        CAMX_ASSERT(plane <= MAX_NUM_OF_IMAGE_PLANES);
                        pConfigIOData->images[BPS_INPUT_IMAGE].bufferLayout[plane].bufferStride         =
                            pImageFormat->formatParams.yuvFormat[plane].planeStride;
                        pConfigIOData->images[BPS_INPUT_IMAGE].bufferLayout[plane].bufferHeight         =
                            pImageFormat->formatParams.yuvFormat[plane].sliceHeight;
                        pConfigIOData->images[BPS_INPUT_IMAGE].metadataBufferLayout[plane].bufferStride =
                            pImageFormat->formatParams.yuvFormat[plane].metadataStride;
                        pConfigIOData->images[BPS_INPUT_IMAGE].metadataBufferLayout[plane].bufferHeight =
                            pImageFormat->formatParams.yuvFormat[plane].metadataHeight;
                        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Input plane %d, stride %d, scanline %d, metastride %d,"
                                         "metascanline %d",
                                         plane,
                                         pImageFormat->formatParams.yuvFormat[plane].planeStride,
                                         pImageFormat->formatParams.yuvFormat[plane].sliceHeight,
                                         pImageFormat->formatParams.yuvFormat[plane].metadataStride,
                                         pImageFormat->formatParams.yuvFormat[plane].metadataHeight);
                    }
                }

                pICPResource->inputResource.format = static_cast <UINT32>(pImageFormat->format);
                pICPResource->inputResource.width  = pImageFormat->width;
                pICPResource->inputResource.height = pImageFormat->height;
                pICPResource->inputResource.FPS    = m_FPS;
                pICPResource->numOutputResource    = numOutputPort;
                m_fullInputWidth                   = pImageFormat->width;
                m_fullInputHeight                  = pImageFormat->height;
                /// @todo (CAMX-1375) verify BPS ports with corresponding formats
                for (outputPortIndex = 0; outputPortIndex < numOutputPort; outputPortIndex++)
                {
                    // Enable the path for BPS, required for dynamically enable/disable output paths based on use case
                    BPSPathIndex                   = outputPortId[outputPortIndex];
                    m_BPSPathEnabled[BPSPathIndex] = TRUE;

                    TranslateToFirmwarePortId(outputPortId[outputPortIndex], &firmwarePortId);

                    /// @todo (CAMX-1375) update raw format output. Also update byte swap
                    pImageFormat =
                        GetOutputPortImageFormat(OutputPortIndex(outputPortId[outputPortIndex]));

                    if (NULL == pImageFormat)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null ImageFormat for output port: %d",
                                       outputPortId[outputPortIndex]);

                        result = CamxResultEFailed;
                        break;
                    }

                    if (BPS_OUTPUT_IMAGE_FULL == firmwarePortId)
                    {
                        validFormat = ((CamX::Format::YUV420NV21TP10 == pImageFormat->format) ||
                                       (CamX::Format::YUV420NV12TP10 == pImageFormat->format) ||
                                       (CamX::Format::UBWCTP10 == pImageFormat->format)       ||
                                       (CamX::Format::P010 == pImageFormat->format));

                        if (FALSE == validFormat)
                        {
                            CAMX_LOG_WARN(CamxLogGroupBPS, "BPS output format for output port %d,"
                                          "firmwarePortId %d, pImageFormat->format %d",
                                          outputPortId[outputPortIndex],
                                          firmwarePortId,
                                          pImageFormat->format);
                        }
                    }

                    // Configure Image Info:
                    // Once CHI Supports end to end BPS IDEAL RAW that is 14b bayer output we need to revisit
                    // below check as by default it is "IMAGE_FORMAT_BAYER_10"
                    pConfigIOData->images[firmwarePortId].info.format =
                        TranslateBPSFormatToFirmwareImageFormat(pImageFormat, firmwarePortId);

                    if ((Format::Blob == pImageFormat->format) && (BPS_IO_IMAGES::BPS_OUTPUT_IMAGE_STATS_BG == firmwarePortId))
                    {
                        // Special case where we have width in pixels stored in imageformat, as different blob buffers could
                        // have different strides. So for actual width read HW capability
                        pConfigIOData->images[firmwarePortId].info.dimensions.widthPixels = AWBBGStatsMaxHorizontalRegions - 1;
                        pConfigIOData->images[firmwarePortId].info.dimensions.heightLines = AWBBGStatsMaxVerticalRegions - 1;
                    }
                    else
                    {
                        pConfigIOData->images[firmwarePortId].info.dimensions.widthPixels =
                            pImageFormat->width;
                        pConfigIOData->images[firmwarePortId].info.dimensions.heightLines =
                            pImageFormat->height;
                    }

                    pConfigIOData->images[firmwarePortId].info.enableByteSwap =
                        ((pImageFormat->format == CamX::Format::YUV420NV21) ||
                        (pImageFormat->format == CamX::Format::YUV420NV21TP10)) ? 1 : 0;

                    if (TRUE == ImageFormatUtils::IsUBWC(pImageFormat->format))
                    {
                        pConfigIOData->images[firmwarePortId].info.ubwcVersion =
                            static_cast <UbwcVersion>(pImageFormat->ubwcVerInfo.version);
                        if (UBWC_VERSION_3 <= static_cast <UbwcVersion>(pImageFormat->ubwcVerInfo.version))
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Setting ubwc version %d lossy %d",
                                             pImageFormat->ubwcVerInfo.version,
                                             pImageFormat->ubwcVerInfo.lossy);
                            pConfigIOData->images[firmwarePortId].info.ubwcLossyMode =
                                static_cast <UbwcLossyMode>(pImageFormat->ubwcVerInfo.lossy);
                            pConfigIOData->images[firmwarePortId].info.ubwcBwLimit   =
                                (static_cast<Titan17xContext *>(GetHwContext()))->GetUBWCBandwidthLimit(
                                    pImageFormat->ubwcVerInfo.version,
                                    LossyPathBPS,
                                    0,
                                    pImageFormat);
                            pConfigIOData->images[firmwarePortId].info.ubwcThreshLossy1 = 0x0;
                            pConfigIOData->images[firmwarePortId].info.ubwcThreshLossy0 = 0x0;
                            pConfigIOData->images[firmwarePortId].info.argbAlpha        = 0x0;
                        }
                    }

                    CAMX_LOG_CONFIG(CamxLogGroupBPS, "node %s O/P: firmwarePortId %d format %d,"
                                  "FW format %d width %d, height %d",
                                  NodeIdentifierString(),
                                  firmwarePortId,
                                  pImageFormat->format,
                                  pConfigIOData->images[firmwarePortId].info.format,
                                  pImageFormat->width,
                                  pImageFormat->height);

                    if (BPS_OUTPUT_IMAGE_REG1 == firmwarePortId)
                    {
                        m_regOutSize.widthPixels = pImageFormat->width;
                        m_regOutSize.heightLines = pImageFormat->height;
                        CAMX_LOG_INFO(CamxLogGroupBPS, "Set registration output w=%d, h=%d",
                            m_regOutSize.widthPixels, m_regOutSize.heightLines);
                    }

                    // Configure Buffer Layout
                    if (TRUE == ImageFormatUtils::IsRAW(pImageFormat))
                    {
                        pConfigIOData->images[firmwarePortId].bufferLayout[0].bufferStride =
                            pImageFormat->formatParams.rawFormat.stride;
                        pConfigIOData->images[firmwarePortId].bufferLayout[1].bufferHeight =
                            pImageFormat->formatParams.rawFormat.sliceHeight;
                    }
                    else if ((Format::Blob == pImageFormat->format) &&
                             (BPS_IO_IMAGES::BPS_OUTPUT_IMAGE_STATS_BG == firmwarePortId))
                    {
                        pConfigIOData->images[firmwarePortId].bufferLayout[0].bufferStride =
                            AWBBGStatsMaxHorizontalRegions * AWBBGStatsOutputSizePerRegion;
                        pConfigIOData->images[firmwarePortId].bufferLayout[0].bufferHeight =
                            AWBBGStatsMaxVerticalRegions;

                        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "AWB BG stats buffer layout: stride=%d, height=%d",
                                         pConfigIOData->images[firmwarePortId].bufferLayout[0].bufferStride,
                                         pConfigIOData->images[firmwarePortId].bufferLayout[0].bufferHeight);
                    }
                    else
                    {
                        validFormat = ((TRUE == ImageFormatUtils::IsYUV(pImageFormat))          ||
                                       (TRUE == ImageFormatUtils::IsUBWC(pImageFormat->format)) ||
                                       (Format::Blob == pImageFormat->format));

                        CAMX_ASSERT(TRUE == validFormat);

                        for (UINT plane = 0; plane < ImageFormatUtils::GetNumberOfPlanes(pImageFormat); plane++)
                        {
                            CAMX_ASSERT(plane <= MAX_NUM_OF_IMAGE_PLANES);
                            pConfigIOData->images[firmwarePortId].bufferLayout[plane].bufferStride         =
                                pImageFormat->formatParams.yuvFormat[plane].planeStride;
                            pConfigIOData->images[firmwarePortId].bufferLayout[plane].bufferHeight         =
                                pImageFormat->formatParams.yuvFormat[plane].sliceHeight;
                            pConfigIOData->images[firmwarePortId].metadataBufferLayout[plane].bufferStride =
                                pImageFormat->formatParams.yuvFormat[plane].metadataStride;
                            pConfigIOData->images[firmwarePortId].metadataBufferLayout[plane].bufferHeight =
                                pImageFormat->formatParams.yuvFormat[plane].metadataHeight;

                            CAMX_LOG_INFO(CamxLogGroupBPS, "Ouptut plane %d, stride %d, scanline %d,"
                                          "metastride %d, metascanline %d",
                                          plane,
                                          pImageFormat->formatParams.yuvFormat[plane].planeStride,
                                          pImageFormat->formatParams.yuvFormat[plane].sliceHeight,
                                          pImageFormat->formatParams.yuvFormat[plane].metadataStride,
                                          pImageFormat->formatParams.yuvFormat[plane].metadataHeight);
                        }
                    }

                    // Special case where we have width in pixels stored in imageformat, as different blob buffers could
                    // have different strides. So for actual width read HW capability
                    if ((Format::Blob == pImageFormat->format) && (BPS_IO_IMAGES::BPS_OUTPUT_IMAGE_STATS_BG == firmwarePortId))
                    {
                        pICPOutResource->width  = AWBBGStatsMaxHorizontalRegions - 1;
                        pICPOutResource->height = AWBBGStatsMaxVerticalRegions - 1;
                    }
                    else
                    {
                        pICPOutResource->width  = pImageFormat->width;
                        pICPOutResource->height = pImageFormat->height;
                    }
                    pICPOutResource->format = static_cast <UINT32>(pImageFormat->format);
                    pICPOutResource->FPS    = m_FPS;
                    pICPOutResource++;
                }

                if (CamxResultSuccess == result)
                {
                    pICPResource->hIOConfigCmd = pConfigIOMem->hHandle;
                    pICPResource->IOConfigLen  = sizeof(BpsConfigIo);

                    // Add to the resource list
                    pResource->resourceID              = resourceType;
                    pResource->pDeviceResourceParam    = static_cast<VOID*>(pICPResource);
                    pResource->deviceResourceParamSize = resourceSize;

                    CAMX_LOG_INFO(CamxLogGroupBPS, "resourceSize = %d, IOConfigLen = %d",
                                  resourceSize, pICPResource->IOConfigLen);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Null ImageFormat for Input");
                result = CamxResultENoMemory;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        result = InitializeStripingParams(pConfigIOData);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Initialize Striping params failed %d", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::AcquireDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::AcquireDevice()
{
    CamxResult           result                   = CamxResultSuccess;

    if (CSLInvalidHandle == m_configIOMem.hHandle)
    {
        result = CSLAlloc(NodeIdentifierString(),
                      &m_configIOMem,
                      GetFWBufferAlignedSize(sizeof(BpsConfigIo)),
                      1,
                      (CSLMemFlagUMDAccess | CSLMemFlagSharedAccess | CSLMemFlagHw | CSLMemFlagKMDAccess),
                      &DeviceIndices()[0],
                      1);
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "CSLAlloc returned configIOMem.fd=%d", m_configIOMem.fd);
        CAMX_ASSERT(CSLInvalidHandle != m_configIOMem.hHandle);
        CAMX_ASSERT(NULL != m_configIOMem.pVirtualAddr);

        if ((NULL != m_configIOMem.pVirtualAddr) && (CSLInvalidHandle != m_configIOMem.hHandle))
        {
            result = SetupDeviceResource(&m_configIOMem, &m_deviceResourceRequest);
        }

        if (CamxResultSuccess == result)
        {
            result = CSLAcquireDevice(GetCSLSession(),
                                      &m_hDevice,
                                      DeviceIndices()[0],
                                      &m_deviceResourceRequest,
                                      1,
                                      NULL,
                                      0,
                                      NodeIdentifierString());

            if (CamxResultSuccess == result)
            {
                SetDeviceAcquired(TRUE);
                AddCSLDeviceHandle(m_hDevice);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Acquire BPS Device Failed");
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Out of memory");
    }



    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ReleaseDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::ReleaseDevice()
{
    CamxResult result = CamxResultSuccess;

    DeInitializeStripingParams();

    if (NULL != m_hHandle)
    {
        CloseStripingLibrary();
    }

    if (NULL != GetHwContext())
    {
        result = CSLReleaseDevice(GetCSLSession(), m_hDevice);

        if (CamxResultSuccess == result)
        {
            SetDeviceAcquired(FALSE);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to release device");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ConfigureBPSCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::ConfigureBPSCapability()
{
    CamxResult result = CamxResultSuccess;

    m_titanVersion = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();

    switch (m_titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
            m_pPipeline = CAMX_NEW BPSPipelineTitan150;
            break;

        case CSLCameraTitanVersion::CSLTitan160:
            m_pPipeline = CAMX_NEW BPSPipelineTitan160;
            break;

        case CSLCameraTitanVersion::CSLTitan170:
            m_pPipeline = CAMX_NEW BPSPipelineTitan170;
            break;

        case CSLCameraTitanVersion::CSLTitan175:
            m_pPipeline = CAMX_NEW BPSPipelineTitan175;
            break;

        case CSLCameraTitanVersion::CSLTitan480:
            m_pPipeline = CAMX_NEW BPSPipelineTitan480;
            break;

        default:
            result = CamxResultEUnsupported;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unsupported Version Number");
            break;
    }

    if (NULL != m_pPipeline)
    {
        m_pPipeline->GetCapability(&m_capability);
        m_pPipeline->GetModuleList(&m_IQModuleList);
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory");
    }


    // Striping library in firmware will be removed in future. Remove this setting once striping in FW is removed.
    m_swStriping = static_cast<Titan17xContext*>(
        GetHwContext())->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->BPSSwStriping;

    OverrideDefaultIQModuleEnablement();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::OverrideDefaultIQModuleEnablement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::OverrideDefaultIQModuleEnablement()
{
    const Titan17xStaticSettings* pSettings =
        static_cast<Titan17xContext*>(GetHwContext())->GetTitan17xSettingsManager()->GetTitan17xStaticSettings();

    for (UINT i = 0; i < m_IQModuleList.numBPSIQModules; i++)
    {
        // If there is a create function for the IQ module, check to see if it is being forced on/off
        if (NULL != m_IQModuleList.pBPSIQModule[i]->IQCreate)
        {
            switch (m_IQModuleList.pBPSIQModule[i]->moduleType)
            {
                case ISPIQModuleType::BPSPedestalCorrection:
                    if (IQModuleForceDefault != pSettings->forceBPSPedestalCorrection)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSPedestalCorrection;
                    }
                    break;
                case ISPIQModuleType::BPSLinearization:
                    if (IQModuleForceDefault != pSettings->forceBPSLinearization)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSLinearization;
                    }
                    break;
                case ISPIQModuleType::BPSBPCPDPC:
                    if (IQModuleForceDefault != pSettings->forceBPSBPCPDPC)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSBPCPDPC;
                    }
                    break;
                case ISPIQModuleType::BPSDemux:
                    if (IQModuleForceDefault != pSettings->forceBPSDemux)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSDemux;
                    }
                    break;
                case ISPIQModuleType::BPSHDR:
                    if (IQModuleForceDefault != pSettings->forceBPSHDR)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSHDR;
                    }
                    break;
                case ISPIQModuleType::BPSABF:
                    if (IQModuleForceDefault != pSettings->forceBPSABF)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSABF;
                    }
                    break;
                case ISPIQModuleType::BPSGIC:
                    if (IQModuleForceDefault != pSettings->forceBPSGIC)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSGIC;
                    }
                    break;
                case ISPIQModuleType::BPSLSC:
                    if (IQModuleForceDefault != pSettings->forceBPSLSC)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSLSC;
                    }
                    break;
                case ISPIQModuleType::BPSWB:
                    if (IQModuleForceDefault != pSettings->forceBPSWB)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSWB;
                    }
                    break;
                case ISPIQModuleType::BPSDemosaic:
                    if (IQModuleForceDefault != pSettings->forceBPSDemosaic)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSDemosaic;
                    }
                    break;
                case ISPIQModuleType::BPSCC:
                    if (IQModuleForceDefault != pSettings->forceBPSCC)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSCC;
                    }
                    break;
                case ISPIQModuleType::BPSGTM:
                    if (IQModuleForceDefault != pSettings->forceBPSGTM)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSGTM;
                    }
                    break;
                case ISPIQModuleType::BPSGamma:
                    if (IQModuleForceDefault != pSettings->forceBPSGamma)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSGamma;
                    }
                    break;
                case ISPIQModuleType::BPSCST:
                    if (IQModuleForceDefault != pSettings->forceBPSCST)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSCST;
                    }
                    break;
                case ISPIQModuleType::BPSChromaSubSample:
                    if (IQModuleForceDefault != pSettings->forceBPSChromaSubSample)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSChromaSubSample;
                    }
                    break;
                case ISPIQModuleType::BPSHNR:
                    if (IQModuleForceDefault != pSettings->forceBPSHNR)
                    {
                        m_IQModuleList.pBPSIQModule[i]->isEnabled = pSettings->forceBPSHNR;
                    }
                    break;
                default:
                    CAMX_LOG_VERBOSE(CamxLogGroupBPS,
                                   "IQ module exists without an appropriate settings override.  Must add override");
                    break;

            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetProcessingSectionForBPSProfile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSProcessingSection GetProcessingSectionForBPSProfile(
    BPSProfileId propertyValue)
{
    BPSProcessingSection type = BPSProcessingSection::BPSAll;
    switch (propertyValue)
    {
        case BPSProfileId::BPSProfileIdHNR:
            type = BPSProcessingSection::BPSHNR;
            break;
        // For ideal (LSC) raw input image, disable all the block pre-LSC and LSC blocks
        case BPSProfileId::BPSProfileIdIdealRawInput:
            type = BPSProcessingSection::BPSPostLSC;
            break;
        // For ideal (LSC) raw output image, disable all the block Post LSC
        case BPSProfileId::BPSProfileIdIdealRawOutput:
            type = BPSProcessingSection::BPSLSCOut;
            break;
        case BPSProfileId::BPSProfileIdDefault:
        case BPSProfileId::BPSProfileIdPrefilter:
        case BPSProfileId::BPSProfileIdBlend:
        case BPSProfileId::BPSProfileIdBlendPost:
            type = BPSProcessingSection::BPSAll;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unsupported IQ module type");
            break;
    }
    return type;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::CreateBPSIQModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::CreateBPSIQModules()
{
    CamxResult             result                   = CamxResultSuccess;
    UINT                   count                    = 0;
    BPSModuleCreateData    moduleInputData          = { 0 };
    BPSProcessingSection   moduleSection            = BPSProcessingSection::BPSAll;
    CmdBufferManagerParam* pBufferManagerParam      = NULL;
    IQModuleCmdBufferParam bufferManagerCreateParam = { 0 };
    BOOL                   moduleDependeciesEnabled = TRUE;

    moduleInputData.initializationData.pipelineBPSData.pDeviceIndex = &m_deviceIndex;
    moduleInputData.initializationData.requestQueueDepth            = GetPipeline()->GetRequestQueueDepth();
    moduleInputData.pNodeIdentifier                                 = NodeIdentifierString();
    moduleInputData.titanVersion                                    = m_titanVersion;

    // Based on profileId get modules from which processing section shall be created.
    m_instanceProcessingSection = GetProcessingSectionForBPSProfile(m_instanceProperty.profileId);

    pBufferManagerParam =
        static_cast<CmdBufferManagerParam*>(CAMX_CALLOC(sizeof(CmdBufferManagerParam) * m_IQModuleList.numBPSIQModules));
    if (NULL != pBufferManagerParam)
    {
        bufferManagerCreateParam.pCmdBufManagerParam    = pBufferManagerParam;
        bufferManagerCreateParam.numberOfCmdBufManagers = 0;
        for (count = 0; count < m_IQModuleList.numBPSIQModules; count++)
        {
            moduleDependeciesEnabled = TRUE;
            if (m_IQModuleList.pBPSIQModule[count]->moduleType == ISPIQModuleType::BPSHNR)
            {
                moduleSection = BPSProcessingSection::BPSHNR;
            }
            // check if module belongs to section which profile has asked to enable.
            if ((m_instanceProcessingSection != moduleSection) && (m_instanceProcessingSection != BPSProcessingSection::BPSAll))
            {
                moduleDependeciesEnabled = FALSE;
            }

            // Should be taken care by instance adding safe check
            if ((BPSProfileId::BPSProfileIdBlendPost == m_instanceProperty.profileId) &&
                    (m_IQModuleList.pBPSIQModule[count]->moduleType == ISPIQModuleType::BPSHNR))
            {
                moduleDependeciesEnabled = FALSE;
            }

            if ((BPSProcessingSection::BPSPostLSC == m_instanceProcessingSection) &&
                (BPSProfileId::BPSProfileIdIdealRawInput == m_instanceProperty.profileId))
            {
                // Disable IQ modules till LSC and Enable Post LSC modules
                moduleDependeciesEnabled = IsPostLSCModule(m_IQModuleList.pBPSIQModule[count]->moduleType);
                CAMX_LOG_INFO(CamxLogGroupBPS,
                    "BPS module: PostLSC: count = %u, m_IQModuleList.numBPSIQModules = %u, moduleDependeciesEnabled= %u",
                    count,
                    m_IQModuleList.numBPSIQModules,
                    moduleDependeciesEnabled);
            }

            if ((BPSProcessingSection::BPSLSCOut == m_instanceProcessingSection) &&
                (BPSProfileId::BPSProfileIdIdealRawOutput == m_instanceProperty.profileId))
            {
                // Enable IQ modules till LSC and Disable Post LSC modules
                moduleDependeciesEnabled = !IsPostLSCModule(m_IQModuleList.pBPSIQModule[count]->moduleType);
                CAMX_LOG_INFO(CamxLogGroupBPS,
                    "BPS module: PreLSC: count = %u, m_IQModuleList.numBPSIQModules = %u, moduleDependeciesEnabled= %u",
                    count,
                    m_IQModuleList.numBPSIQModules,
                    moduleDependeciesEnabled);
            }

            if ((TRUE == m_IQModuleList.pBPSIQModule[count]->isEnabled) && (TRUE == moduleDependeciesEnabled))
            {
                result = m_IQModuleList.pBPSIQModule[count]->IQCreate(&moduleInputData);

                if (CamxResultSuccess == result)
                {
                    moduleInputData.pModule->FillCmdBufferManagerParams(&moduleInputData.initializationData,
                                                                        &bufferManagerCreateParam);
                    m_pBPSIQModules[m_numIQMOdules] = moduleInputData.pModule;
                    m_numIQMOdules++;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to Create IQ Module, count = %d", count);
                    break;
                }
            }
        }

        if ((CamxResultSuccess == result) && (0 != bufferManagerCreateParam.numberOfCmdBufManagers))
        {
            // Create Cmd Buffer Managers for IQ Modules
            result = CmdBufferManager::CreateMultiManager(FALSE,
                                                          pBufferManagerParam,
                                                          bufferManagerCreateParam.numberOfCmdBufManagers);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "IQ MOdules Cmd Buffer Manager Creation failed");
            }
        }

        // Free up the memory allocated by IQ Blocks
        for (UINT index = 0; index < bufferManagerCreateParam.numberOfCmdBufManagers; index++)
        {
            if (NULL != pBufferManagerParam[index].pBufferManagerName)
            {
                CAMX_FREE(pBufferManagerParam[index].pBufferManagerName);
            }

            if (NULL != pBufferManagerParam[index].pParams)
            {
                CAMX_FREE(pBufferManagerParam[index].pParams);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Out Of Memory");
        result = CamxResultENoMemory;
    }

    if (NULL != pBufferManagerParam)
    {
        CAMX_FREE(pBufferManagerParam);
    }

    // The clean-up for the error case happens outside this function
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetBPSSWTMCModuleInstanceVersion()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SWTMCVersion BPSNode::GetBPSSWTMCModuleInstanceVersion()
{
    // If SWTMC IQ module is not installed, version is 1.0
    SWTMCVersion version = SWTMCVersion::TMC10;

    if ((BPSProfileId::BPSProfileIdHNR == m_instanceProperty.profileId) &&
        (BPSProcessingType::BPSProcessingMFNR == m_instanceProperty.processingType))
    {
        version = m_capability.tmcversion;
    }
    else
    {
        for (UINT i = 0; i < m_numIQMOdules; i++)
        {
            if (ISPIQModuleType::SWTMC == m_pBPSIQModules[i]->GetIQType())
            {
                version = static_cast<SWTMCVersion>(m_pBPSIQModules[i]->GetVersion());
                break;
            }
        }
    }

    return version;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::Cleanup()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::Cleanup()
{
    UINT           count            = 0;
    CamxResult     result           = CamxResultSuccess;
    UINT           numberOfMappings = 0;
    CSLBufferInfo  bufferInfo       = { 0 };
    CSLBufferInfo* pBufferInfo[CSLICPMaxMemoryMapRegions];

    if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferFrameProcess])
    {
        if (NULL != m_pBPSCmdBufferManager[BPSCmdBufferFrameProcess]->GetMergedCSLBufferInfo())
        {
            Utils::Memcpy(&bufferInfo,
                          m_pBPSCmdBufferManager[BPSCmdBufferFrameProcess]->GetMergedCSLBufferInfo(),
                          sizeof(CSLBufferInfo));
            pBufferInfo[numberOfMappings] = &bufferInfo;
            numberOfMappings++;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to get the Merged Buffer info");
            result = CamxResultEFailed;
        }
    }

    if (0 != numberOfMappings)
    {
        result = SendFWCmdRegionInfo(CSLICPGenericBlobCmdBufferUnMapFWMemRegion,
                                     pBufferInfo,
                                     numberOfMappings);
    }

    // De-allocate all of the IQ modules
    for (count = 0; count < m_numIQMOdules; count++)
    {
        if (NULL != m_pBPSIQModules[count])
        {
            m_pBPSIQModules[count]->Destroy();
            m_pBPSIQModules[count] = NULL;
        }
    }

    // Check if striping in umd is enabled before destroying striping library context
    if (NULL != m_hStripingLib)
    {
        result              = m_funcPtrBPSDestroy(&m_hStripingLib);
        m_hStripingLib      = NULL;
        if (result != 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to destroy striping library");
            result = CamxResultEFailed;
        }
    }

    if (NULL != m_pTuningMetadata)
    {
        CAMX_FREE(m_pTuningMetadata);
        m_pTuningMetadata = NULL;
    }

    if (NULL != m_pDebugDataWriter)
    {
        CAMX_DELETE m_pDebugDataWriter;
        m_pDebugDataWriter = NULL;
    }

    if (NULL != m_pPipeline)
    {
        CAMX_DELETE m_pPipeline;
        m_pPipeline = NULL;
    }

    if (NULL != m_pGenericClockAndBandwidthData)
    {
        CAMX_FREE(m_pGenericClockAndBandwidthData);
        m_pGenericClockAndBandwidthData = NULL;
    }

    DeallocateICAGridData();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::PopulateGeneralTuningMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::PopulateGeneralTuningMetadata(
    ISPInputData* pInputData)
{
    BpsIQSettings*  pBPSIQsettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    // Populate Module Config data
    CAMX_STATIC_ASSERT(sizeof(BpsIQSettings) == sizeof(pInputData->pBPSTuningMetadata->BPSFWHeader.BPSFirmwareSetting));
    // If these are not the same size, BpsIQSettings has new modules. camxtuningdump.h needs to be updated.
    // Update BPSFirmwareAPIVersion with the current Firmware Version
    // #include "icpdefs.h" "FW Version: %x", FW_API_VERSION
    // Update BPSModulesConfigFields with the correct field size
    // "Array Size: %d", sizeof(BpsIQSettings) / sizeof(UINT32)

    pInputData->pBPSTuningMetadata->BPSFWHeader.BPSFWAPIVersion = BPSFirmwareAPIVersion;

    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSFWHeader.BPSFirmwareSetting,
                  pBPSIQsettings,
                  sizeof(pInputData->pBPSTuningMetadata->BPSFWHeader.BPSFirmwareSetting));

    // Populate trigger data
    BPSTuningTriggerData* pBPSTuningTriggers        = &pInputData->pBPSTuningMetadata->BPSTuningTriggers;

    pBPSTuningTriggers->AECexposureTime             = pInputData->triggerData.AECexposureTime;
    pBPSTuningTriggers->AECSensitivity              = pInputData->triggerData.AECSensitivity;
    pBPSTuningTriggers->AECGain                     = pInputData->triggerData.AECGain;
    pBPSTuningTriggers->AECLuxIndex                 = pInputData->triggerData.AECLuxIndex;
    pBPSTuningTriggers->AWBleftGGainWB              = pInputData->triggerData.AWBleftGGainWB;
    pBPSTuningTriggers->AWBleftBGainWB              = pInputData->triggerData.AWBleftBGainWB;
    pBPSTuningTriggers->AWBleftRGainWB              = pInputData->triggerData.AWBleftRGainWB;
    pBPSTuningTriggers->AWBColorTemperature         = pInputData->triggerData.AWBColorTemperature;
    pBPSTuningTriggers->DRCGain                     = pInputData->triggerData.DRCGain;
    pBPSTuningTriggers->DRCGainDark                 = pInputData->triggerData.DRCGainDark;
    pBPSTuningTriggers->lensPosition                = pInputData->triggerData.lensPosition;
    pBPSTuningTriggers->lensZoom                    = pInputData->triggerData.lensZoom;
    pBPSTuningTriggers->postScaleRatio              = pInputData->triggerData.postScaleRatio;
    pBPSTuningTriggers->preScaleRatio               = pInputData->triggerData.preScaleRatio;
    pBPSTuningTriggers->sensorImageWidth            = pInputData->triggerData.sensorImageWidth;
    pBPSTuningTriggers->sensorImageHeight           = pInputData->triggerData.sensorImageHeight;
    pBPSTuningTriggers->CAMIFWidth                  = pInputData->triggerData.CAMIFWidth;
    pBPSTuningTriggers->CAMIFHeight                 = pInputData->triggerData.CAMIFHeight;
    pBPSTuningTriggers->numberOfLED                 = pInputData->triggerData.numberOfLED;
    pBPSTuningTriggers->LEDSensitivity              = static_cast<INT32>(pInputData->triggerData.LEDSensitivity);
    pBPSTuningTriggers->bayerPattern                = pInputData->triggerData.bayerPattern;
    pBPSTuningTriggers->sensorOffsetX               = pInputData->triggerData.sensorOffsetX;
    pBPSTuningTriggers->sensorOffsetY               = pInputData->triggerData.sensorOffsetY;
    pBPSTuningTriggers->blackLevelOffset            = pInputData->triggerData.blackLevelOffset;
    // Parameters added later (if this trigger data gets update it again, integrate with main BPSTuningTriggerData struct)
    pInputData->pBPSTuningMetadata->BPSExposureGainRatio = pInputData->triggerData.AECexposureGainRatio;

    // Populate Sensor configuration data
    IFEBPSSensorConfigData* pBPSSensorConfig    = &pInputData->pBPSTuningMetadata->BPSSensorConfig;
    pBPSSensorConfig->isBayer                   = pInputData->sensorData.isBayer;
    pBPSSensorConfig->format                    = static_cast<UINT32>(pInputData->sensorData.format);
    pBPSSensorConfig->digitalGain               = pInputData->sensorData.dGain;
    pBPSSensorConfig->ZZHDRColorPattern         = pInputData->sensorData.ZZHDRColorPattern;
    pBPSSensorConfig->ZZHDRFirstExposure        = pInputData->sensorData.ZZHDRFirstExposure;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::PrepareTuningMetadataDump()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::PrepareTuningMetadataDump(
    ISPInputData* pInputData)
{
    CamxResult      result                              = CamxResultSuccess;
    DebugData*      pDebugDataBase                      = NULL;
    DebugData       debugData                           = { 0 };
    UINT            PropertiesTuning[]                  = { 0 };
    static UINT     metaTagDebugDataAll                 = 0;
    const UINT      length                              = CAMX_ARRAY_SIZE(PropertiesTuning);
    VOID*           pData[length]                       = { 0 };
    UINT64          propertyDataTuningOffset[length]    = { 0 };
    UINT32          debugDataPartition                  = 0;

    if (TRUE == IsRealTime())
    {
        PropertiesTuning[0] = PropertyIDTuningDataBPS;
    }
    else
    {
        VendorTagManager::QueryVendorTagLocation("org.quic.camera.debugdata", "DebugDataAll", &metaTagDebugDataAll);
        PropertiesTuning[0] = metaTagDebugDataAll | InputMetadataSectionMask;
    }

    GetDataList(PropertiesTuning, pData, propertyDataTuningOffset, length);
    pDebugDataBase = reinterpret_cast<DebugData*>(pData[0]);

    // Check if debug data buffer is available
    if (NULL == pDebugDataBase || NULL == pDebugDataBase->pData)
    {
        // Debug-data buffer not available is valid use case
        // Normal execution will continue without debug data
        CAMX_LOG_VERBOSE(CamxLogGroupDebugData, "SKIP no debug-data available/needed "
                         "RT: %u, frameNum: %llu mf: %u, InstanceID: %u, profId: %u, procType: %u",
                         IsRealTime(),
                         pInputData->frameNum,
                         pInputData->mfFrameNum,
                         InstanceID(),
                         m_instanceProperty.profileId,
                         m_instanceProperty.processingType);
    }
    else
    {
        // Same partition shared between real time and offline
        if (TRUE == IsRealTime())
        {
            debugData.size  = pDebugDataBase->size / DebugDataPartitionsBPS;
            debugData.pData = pDebugDataBase->pData;
        }
        else
        {
            if ((BPSProcessingMFNR == m_instanceProperty.processingType) &&
                (BPSProfileIdHNR   == m_instanceProperty.profileId))
            {
                debugDataPartition = 1;
            }

            if (DebugDataPartitionsBPS > debugDataPartition)
            {
                SIZE_T instanceOffset = 0;
                debugData.size  = HAL3MetadataUtil::DebugDataSize(DebugDataType::BPSTuning) / DebugDataPartitionsBPS;
                instanceOffset = debugDataPartition * debugData.size;
                debugData.pData = Utils::VoidPtrInc(
                    pDebugDataBase->pData,
                    (HAL3MetadataUtil::DebugDataOffset(DebugDataType::BPSTuning) + instanceOffset));
            }
            else
            {
                result = CamxResultENoMemory;
                CAMX_LOG_WARN(CamxLogGroupDebugData,
                              "Partition: %u: Not enough partitions available to save bps data frameNum: %llu",
                              debugDataPartition,
                              pInputData->frameNum);
            }
        }

        CAMX_LOG_VERBOSE(CamxLogGroupDebugData, "Tuning-metadata:BPS:RT: %u, frameNum: %llu mf: %u, InstanceID: %u, base: %p"
                         ", start: %p, size: %u, profId: %u, procType: %u, debugDataPartition: %u",
                         IsRealTime(),
                         pInputData->frameNum,
                         pInputData->mfFrameNum,
                         InstanceID(),
                         pDebugDataBase->pData,
                         debugData.pData,
                         debugData.size,
                         m_instanceProperty.profileId,
                         m_instanceProperty.processingType,
                         debugDataPartition);

        if (CamxResultSuccess == result)
        {
            // Set the buffer pointer
            m_pDebugDataWriter->SetBufferPointer(static_cast<BYTE*>(debugData.pData), debugData.size);

            // Populate node information
            pInputData->pBPSTuningMetadata->BPSNodeInformation.instanceId       = InstanceID();
            pInputData->pBPSTuningMetadata->BPSNodeInformation.requestId        = pInputData->frameNum;
            pInputData->pBPSTuningMetadata->BPSNodeInformation.isRealTime       = IsRealTime();
            pInputData->pBPSTuningMetadata->BPSNodeInformation.profileId        = m_instanceProperty.profileId;
            pInputData->pBPSTuningMetadata->BPSNodeInformation.processingType   = m_instanceProperty.processingType;

            result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningBPSNodeInfo,
                                                    DebugDataTagType::TuningIQNodeInfo,
                                                    1,
                                                    &pInputData->pBPSTuningMetadata->BPSNodeInformation,
                                                    sizeof(pInputData->pBPSTuningMetadata->BPSNodeInformation));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "Add Data Tag failed with error: %d.", result);
                result = CamxResultSuccess; // Non-fatal error
            }
        }
    }
    // Pass the writer down
    pInputData->pipelineBPSData.pDebugDataWriter = m_pDebugDataWriter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::DumpTuningMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::DumpTuningMetadata(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    // Populate any metadata obtained direclty from base BPS node
    PopulateGeneralTuningMetadata(pInputData);

    // Add BPS Chipset Version
    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningBPSChipsetVersion,
                                            DebugDataTagType::UInt32,
                                            1,
                                            &pInputData->titanVersion,
                                            sizeof(pInputData->titanVersion));

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "Add Data Tag failed with error: %d.", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    // Add BPS tuning metadata tags
    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningBPSFirmwareHeader,
                                            DebugDataTagType::TuningBPSFirmwareHeader,
                                            1,
                                            &pInputData->pBPSTuningMetadata->BPSFWHeader,
                                            sizeof(pInputData->pBPSTuningMetadata->BPSFWHeader));

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "Add Data Tag failed with error: %d.", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningBPSTriggerModulesData,
                                            DebugDataTagType::TuningBPSTriggerData,
                                            1,
                                            &pInputData->pBPSTuningMetadata->BPSTuningTriggers,
                                            sizeof(pInputData->pBPSTuningMetadata->BPSTuningTriggers));

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "Add Data Tag failed with error: %d.", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningBPSensorConfigData,
                                            DebugDataTagType::TuningBPSSensorConfig,
                                            1,
                                            &pInputData->pBPSTuningMetadata->BPSSensorConfig,
                                            sizeof(pInputData->pBPSTuningMetadata->BPSSensorConfig));

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "Add Data Tag failed with error: %d.", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningBPSExposureGainRatio,
                                            DebugDataTagType::Float,
                                            1,
                                            &pInputData->pBPSTuningMetadata->BPSExposureGainRatio,
                                            sizeof(pInputData->pBPSTuningMetadata->BPSExposureGainRatio));

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "Add Data Tag failed with error: %d.", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningBPSOEMTuningData,
                                            DebugDataTagType::UInt32,
                                            CAMX_ARRAY_SIZE(pInputData->pBPSTuningMetadata->oemTuningData.IQOEMTuningData),
                                            &pInputData->pBPSTuningMetadata->oemTuningData.IQOEMTuningData,
                                            sizeof(pInputData->pBPSTuningMetadata->oemTuningData.IQOEMTuningData));

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "Add Data Tag failed with error: %d.", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    if (TRUE == IsRealTime())
    {
        // Make a copy in main metadata pool
        static const UINT   PropertiesDebugData[]                  = { PropertyIDDebugDataAll };
        VOID*               pSrcData[1]                            = { 0 };
        const UINT          lengthAll                              = CAMX_ARRAY_SIZE(PropertiesDebugData);
        UINT64              propertyDataTuningAllOffset[lengthAll] = { 0 };
        static UINT         metaTagDebugDataAll = 0;
        GetDataList(PropertiesDebugData, pSrcData, propertyDataTuningAllOffset, lengthAll);
        VendorTagManager::QueryVendorTagLocation("org.quic.camera.debugdata", "DebugDataAll", &metaTagDebugDataAll);

        const UINT  TuningVendorTag[]   = { metaTagDebugDataAll };
        const VOID* pDstData[1]         = { pSrcData[0] };
        UINT        pDataCount[1]       = { sizeof(DebugData) };
        WriteDataList(TuningVendorTag, pDstData, pDataCount, 1);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ProgramIQModuleEnableConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::ProgramIQModuleEnableConfig(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInputData);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ProgramIQConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::ProgramIQConfig(
    ISPInputData* pInputData,
    CmdBuffer*    pCmdBufferBlob)
{
    CamxResult              result              = CamxResultSuccess;
    UINT                    count               = 0;
    BOOL                    adrcEnabled         = FALSE;
    FLOAT                   percentageOfGTM     = 0.0f;
    ISPIQTuningDataBuffer   IQOEMTriggerData;

    pInputData->IFEDynamicEnableMask = GetStaticSettings()->IFEDynamicEnableMask;

    if (NULL != pInputData->pBPSTuningMetadata)
    {
        IQOEMTriggerData.pBuffer    = pInputData->pBPSTuningMetadata->oemTuningData.IQOEMTuningData;
        IQOEMTriggerData.size       = sizeof(pInputData->pBPSTuningMetadata->oemTuningData.IQOEMTuningData);
    }
    else
    {
        IQOEMTriggerData.pBuffer    = NULL;
        IQOEMTriggerData.size       = 0;
    }

    // Call IQInterface to Set up the Trigger data
    Node* pBaseNode = this;

    IQInterface::IQSetupTriggerData(pInputData, pBaseNode, IsRealTime(), &IQOEMTriggerData);

    pInputData->triggerData.fullInputWidth  = pInputData->pipelineBPSData.width;
    pInputData->triggerData.fullInputHeight = pInputData->pipelineBPSData.height;

    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Sensor image: w=%d, h=%d, ox=%d, oy=%d. camif image: w=%d, h=%d",
        pInputData->triggerData.sensorImageWidth,
        pInputData->triggerData.sensorImageHeight,
        pInputData->triggerData.sensorOffsetX,
        pInputData->triggerData.sensorOffsetY,
        pInputData->triggerData.CAMIFWidth,
        pInputData->triggerData.CAMIFHeight);

    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "FullInput: w=%d, h=%d",
        pInputData->triggerData.fullInputWidth,
        pInputData->triggerData.fullInputHeight);


    // Invoke GeoLib
    if (CamxResultSuccess == result)
    {
        result = ProcessGeoLib(pInputData, pCmdBufferBlob);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invoking GeoLIb error");
    }

    if (CamxResultSuccess == result)
    {
        // Get the default ADRC status.
        IQInterface::GetADRCParams(pInputData, &adrcEnabled, &percentageOfGTM, GetBPSSWTMCModuleInstanceVersion());
        for (count = 0; count < m_numIQMOdules; count++)
        {
            if (TRUE == adrcEnabled)
            {
                // Update AEC Gain values for ADRC use cases, before GTM(includes) will be triggered by shortGain,
                // betweem GTM & LTM(includes) will be by shortGain*power(DRCGain, gtm_perc) and post LTM will be
                // by shortGain*DRCGain
                IQInterface::UpdateAECGain(m_pBPSIQModules[count]->GetIQType(), pInputData, percentageOfGTM);
            }
            if (m_pBPSIQModules[count]->GetIQType() == ISPIQModuleType::BPSWB)
            {
                pInputData->pAECUpdateData->exposureInfo[0].sensitivity
                    = pInputData->pAECUpdateData->exposureInfo[0].sensitivity * pInputData->pAECUpdateData->predictiveGain;
                pInputData->pAECUpdateData->exposureInfo[0].linearGain
                    = pInputData->pAECUpdateData->exposureInfo[0].linearGain * pInputData->pAECUpdateData->predictiveGain;
                IQInterface::IQSetupTriggerData(pInputData, pBaseNode, 0, &IQOEMTriggerData);
            }
            result = m_pBPSIQModules[count]->Execute(pInputData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to Run IQ Config, count %d , IQ type %d",
                               count, m_pBPSIQModules[count]->GetIQType());
            }
            if (TRUE == adrcEnabled &&
                    ISPIQModuleType::BPSGTM == m_pBPSIQModules[count]->GetIQType())
            {
                percentageOfGTM = pInputData->pCalculatedData->percentageOfGTM;
                CAMX_LOG_VERBOSE(CamxLogGroupBPS, "update the percentageOfGTM: %f", percentageOfGTM);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::UpdateCommandBufferSize()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::UpdateCommandBufferSize()
{
    UINT32 numLUT           = 0;

    for (UINT count = 0; count < m_numIQMOdules; count++)
    {
        numLUT    = m_pBPSIQModules[count]->GetNumLUT();
        m_maxLUT += numLUT;
        SetIQModuleNumLUT(m_pBPSIQModules[count]->GetIQType(), numLUT);

        m_maxCmdBufferSizeBytes[BPSCmdBufferCDMProgram] += m_pBPSIQModules[count]->GetIQCmdLength();
    }

    // Not accounting for stats, as there is no register writes

    m_maxCmdBufferSizeBytes[BPSCmdBufferDMIHeader]    =
        PacketBuilder::RequiredWriteDMISizeInDwords() * m_maxLUT * sizeof(UINT32);
    m_maxCmdBufferSizeBytes[BPSCmdBufferIQSettings]   = sizeof(BpsIQSettings);
    // Update the command buffer size to be aligned to multiple of 4
    m_maxCmdBufferSizeBytes[BPSCmdBufferIQSettings]  += Utils::ByteAlign32(m_maxCmdBufferSizeBytes[BPSCmdBufferIQSettings], 4);
    m_maxCmdBufferSizeBytes[BPSCmdBufferFrameProcess] = Utils::ByteAlign32(BPSCmdBufferFrameProcessSizeBytes, 4);
    // Convert BPSCmdBufferCDMProgram size from dwords to bytes. API from IQ modules returns the size in words
    m_maxCmdBufferSizeBytes[BPSCmdBufferCDMProgram]  *= sizeof(UINT32);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::SetDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    BOOL                        sensorConnected,
    BOOL                        statsConnected)
{
    NodeProcessRequestData* pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT32                  count               = 0;

    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(pNodeRequestData->pCaptureRequest->requestId);
    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "requestIdOffsetFromLastFlush %d count %d", requestIdOffsetFromLastFlush, count);
    if (FirstValidRequestId < requestIdOffsetFromLastFlush)
    {
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] = GetNodeCompleteProperty();
        // Always point to the previous request. Should NOT be tied to the pipeline delay!
        pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[count] = 1;
        count++;
    }
    // Set dependencies only when sensor is connected
    /// @todo (CAMX-1211) Need to add 3A dependency on AEC, AWB, AF Frame Control
    if (TRUE == sensorConnected && (TRUE == IsTagPresentInPublishList(PropertyIDSensorCurrentMode)))
    {
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count]    = PropertyIDSensorCurrentMode;
        count++;

        if (TRUE == statsConnected)
        {
            if (TRUE == IsTagPresentInPublishList(PropertyIDPostSensorGainId))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] = PropertyIDPostSensorGainId;
                count++;
            }

            /// @todo (CAMX-3080) As we do not support YUV ZSL our dependencies applied to BPS should reflect current
            ///                frame. At switch point reset of previous data otherwise would cause failures.
            ///                We will debate more for future YUV support in BPS. (Hence real-time or not BPS acts
            ///                like offline. Only delta is source of data read.
            ///                As a result; offsets for read meta is left as 0.
            if (TRUE == IsTagPresentInPublishList(PropertyIDAECFrameControl))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] = PropertyIDAECFrameControl;
                count++;
            }
            if (TRUE == IsTagPresentInPublishList(PropertyIDAWBFrameControl))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] = PropertyIDAWBFrameControl;
                count++;
            }
            if (TRUE == IsTagPresentInPublishList(PropertyIDAECStatsControl))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] = PropertyIDAECStatsControl;
                count++;
            }
            if (TRUE == IsTagPresentInPublishList(PropertyIDAWBStatsControl))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] = PropertyIDAWBStatsControl;
                count++;
            }
            if (TRUE == IsTagPresentInPublishList(PropertyIDISPTintlessBGConfig))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] = PropertyIDISPTintlessBGConfig;
                count++;
            }
            if (TRUE == IsTagPresentInPublishList(PropertyIDParsedTintlessBGStatsOutput))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] =
                    PropertyIDParsedTintlessBGStatsOutput;
                count++;
            }
            if (TRUE == IsTagPresentInPublishList(PropertyIDISPBHistConfig))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] = PropertyIDISPBHistConfig;
                count++;
            }
            if (TRUE == IsTagPresentInPublishList(PropertyIDParsedBHistStatsOutput))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] = PropertyIDParsedBHistStatsOutput;
                count++;
            }
        }

        if (BPSProfileId::BPSProfileIdHNR == m_instanceProperty.profileId)
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] = PropertyIDIntermediateDimension;
        }
    }

    if (count > 0)
    {
        pNodeRequestData->dependencyInfo[0].propertyDependency.count = count;
    }

    if (0 < pNodeRequestData->dependencyInfo[0].propertyDependency.count)
    {
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
    }

    if (TRUE  == GetStaticSettings()->enableImageBufferLateBinding)
    {
        // If latebinding is enabled, we want to delay packet preparation as late as possible, in other terms, we want to
        // prepare and submit to hw when it can really start processing. This is once all the input fences (+ property)
        // dependencies are satisfied. So, lets set input fence dependencies.
        // But we could optimize further to prepare packet with IQ configuration early once Property dependencies are
        // satisfied and IOConfiguration after input fence dependencies are satisfied.

        /// @todo (CAMX-12345678) Prepare IQConfig, IOConfig in 2 steps and set needBuffersOnDependencyMet in last sequenceId
        //  IQ Config : Prepare once the required (property) dependencies for IQ configuration are satisfied (sequenceId 1).
        //  IO Config : If LateBinding is enabled, Prepare after all dependencies(property + fence) are satisfied (sequendeId 2)
        //              If not enabled, prepare IO Config once property dependencies are satisfied (sequenceId 1)

        if (FALSE == AnyPortHasLateBindingDisabled(pExecuteProcessRequestData->pEnabledPortsInfo))
        {
            SetInputBuffersReadyDependency(pExecuteProcessRequestData, 0);
        }
    }

    if ((TRUE == GetStaticSettings()->enableImageBufferLateBinding) ||
        (TRUE == pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency))
    {
        pNodeRequestData->dependencyInfo[0].processSequenceId                                 = 1;
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
        pNodeRequestData->numDependencyLists                                                  = 1;
    }
    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s,processSequenceId %d,hasIOBufferAvailabilityDependency %d,"
                     "hasPropertyDependency %d,count %d,numDependencyLists %d",
                     NodeIdentifierString(),
                     pNodeRequestData->dependencyInfo[0].processSequenceId,
                     pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency,
                     pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency,
                     pNodeRequestData->dependencyInfo[0].propertyDependency.count,
                     pNodeRequestData->numDependencyLists);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::FillIQSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::FillIQSetting(
    ISPInputData*  pInputData,
    BpsIQSettings* pBPSIQsettings)
{

    /// @todo (CAMX-775) Implement BPS IQ modules
    if (NULL != pBPSIQsettings)
    {
        CamX::Utils::Memset(pBPSIQsettings, 0, sizeof(BpsIQSettings));

        // BPS IQ quater scaling parameters
        TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
        CAMX_ASSERT(NULL != pTuningManager);

        ds4to1_1_0_0::chromatix_ds4to1v10Type* pChromatix1 = NULL;
        ds4to1_1_0_0::chromatix_ds4to1v10Type* pChromatix2 = NULL;
        if (TRUE == pTuningManager->IsValidChromatix())
        {
            pChromatix1 = pTuningManager->GetChromatix()->GetModule_ds4to1v10_bps_full_dc4(
                reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                pInputData->pTuningData->noOfSelectionParameter);
            pChromatix2 = pTuningManager->GetChromatix()->GetModule_ds4to1v10_bps_dc4_dc16(
                reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                pInputData->pTuningData->noOfSelectionParameter);

            CAMX_ASSERT(NULL != pChromatix1 && NULL != pChromatix2);
            if (NULL != pChromatix1 && NULL != pChromatix2)
            {
                INT pass_dc64 = static_cast<INT>(ispglobalelements::trigger_pass::PASS_DC64);
                INT pass_dc16 = static_cast<INT>(ispglobalelements::trigger_pass::PASS_DC16);
                INT pass_dc4 = static_cast<INT>(ispglobalelements::trigger_pass::PASS_DC4);

                ds4to1_1_0_0::chromatix_ds4to1v10_reserveType*  pReserveData1 = &pChromatix1->chromatix_ds4to1v10_reserve;
                ds4to1_1_0_0::chromatix_ds4to1v10_reserveType*  pReserveData2 = &pChromatix2->chromatix_ds4to1v10_reserve;
                pBPSIQsettings->sixtyFourthScalingParameters.coefficient7 =
                    pReserveData2->mod_ds4to1v10_pass_reserve_data[pass_dc64].pass_data.coeff_07;
                pBPSIQsettings->sixtyFourthScalingParameters.coefficient16 =
                    pReserveData2->mod_ds4to1v10_pass_reserve_data[pass_dc64].pass_data.coeff_16;
                pBPSIQsettings->sixtyFourthScalingParameters.coefficient25 =
                    pReserveData2->mod_ds4to1v10_pass_reserve_data[pass_dc64].pass_data.coeff_25;;

                pBPSIQsettings->sixteenthScalingParameters.coefficient7 =
                    pReserveData2->mod_ds4to1v10_pass_reserve_data[pass_dc16].pass_data.coeff_07;
                pBPSIQsettings->sixteenthScalingParameters.coefficient16 =
                    pReserveData2->mod_ds4to1v10_pass_reserve_data[pass_dc16].pass_data.coeff_16;
                pBPSIQsettings->sixteenthScalingParameters.coefficient25 =
                    pReserveData2->mod_ds4to1v10_pass_reserve_data[pass_dc16].pass_data.coeff_25;

                pBPSIQsettings->quarterScalingParameters.coefficient7 =
                    pReserveData1->mod_ds4to1v10_pass_reserve_data[pass_dc4].pass_data.coeff_07;
                pBPSIQsettings->quarterScalingParameters.coefficient16 =
                    pReserveData1->mod_ds4to1v10_pass_reserve_data[pass_dc4].pass_data.coeff_16;
                pBPSIQsettings->quarterScalingParameters.coefficient25 =
                    pReserveData1->mod_ds4to1v10_pass_reserve_data[pass_dc4].pass_data.coeff_25;
            }
        }

        if (NULL == pChromatix1 || NULL == pChromatix2)
        {
            pBPSIQsettings->quarterScalingParameters.coefficient7 = 125;
            pBPSIQsettings->quarterScalingParameters.coefficient16 = 91;
            pBPSIQsettings->quarterScalingParameters.coefficient25 = 144;

            pBPSIQsettings->sixteenthScalingParameters.coefficient7 = 125;
            pBPSIQsettings->sixteenthScalingParameters.coefficient16 = 91;
            pBPSIQsettings->sixteenthScalingParameters.coefficient25 = 144;

            pBPSIQsettings->sixtyFourthScalingParameters.coefficient7 = 125;
            pBPSIQsettings->sixtyFourthScalingParameters.coefficient16 = 91;
            pBPSIQsettings->sixtyFourthScalingParameters.coefficient25 = 144;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupBPS,
                         "DC64: Coeff07 = %d, Coeff16 = %d, Coeff25 = %d",
                         pBPSIQsettings->sixtyFourthScalingParameters.coefficient7,
                         pBPSIQsettings->sixtyFourthScalingParameters.coefficient16,
                         pBPSIQsettings->sixtyFourthScalingParameters.coefficient25);

        CAMX_LOG_VERBOSE(CamxLogGroupBPS,
                         "DC16: Coeff07 = %d, Coeff16 = %d, Coeff25 = %d",
                         pBPSIQsettings->sixteenthScalingParameters.coefficient7,
                         pBPSIQsettings->sixteenthScalingParameters.coefficient16,
                         pBPSIQsettings->sixteenthScalingParameters.coefficient25);

        CAMX_LOG_VERBOSE(CamxLogGroupBPS,
                         "DC4: Coeff07 = %d, Coeff16 = %d, Coeff25 = %d",
                         pBPSIQsettings->quarterScalingParameters.coefficient7,
                         pBPSIQsettings->quarterScalingParameters.coefficient16,
                         pBPSIQsettings->quarterScalingParameters.coefficient25);

        // Chroma Subsample Parameters
        pBPSIQsettings->chromaSubsampleParameters.horizontalRoundingOption = 1;
        pBPSIQsettings->chromaSubsampleParameters.verticalRoundingOption   = 2;

        // BPS IQ full output Round and Clamp parameters
        const ImageFormat* pImageFormat = GetOutputPortImageFormat(OutputPortIndex(CSLBPSOutputPortIdPDIImageFull));
        uint32_t           clampMax     = Max8BitValue;
        if ((NULL != pImageFormat) && (TRUE == ImageFormatUtils::Is10BitFormat(pImageFormat->format)))
        {
            clampMax     = Max10BitValue;
        }
        pBPSIQsettings->fullRoundAndClampParameters.lumaClampParameters.clampMin                      = 0;
        pBPSIQsettings->fullRoundAndClampParameters.lumaClampParameters.clampMax                      = clampMax;
        pBPSIQsettings->fullRoundAndClampParameters.chromaClampParameters.clampMin                    = 0;
        pBPSIQsettings->fullRoundAndClampParameters.chromaClampParameters.clampMax                    = clampMax;
        pBPSIQsettings->fullRoundAndClampParameters.lumaRoundParameters.roundingPattern               = 0;
        pBPSIQsettings->fullRoundAndClampParameters.chromaRoundParameters.roundingPattern             = 0;

        // BPS IQ DS4 output Round and Clamp parameters
        pBPSIQsettings->quarterRoundAndClampParameters.lumaClampParameters.clampMin                   = 0;
        pBPSIQsettings->quarterRoundAndClampParameters.lumaClampParameters.clampMax                   = Max10BitValue;
        pBPSIQsettings->quarterRoundAndClampParameters.chromaClampParameters.clampMin                 = 0;
        pBPSIQsettings->quarterRoundAndClampParameters.chromaClampParameters.clampMax                 = Max10BitValue;
        pBPSIQsettings->quarterRoundAndClampParameters.lumaRoundParameters.roundingPattern            = 0;
        pBPSIQsettings->quarterRoundAndClampParameters.chromaRoundParameters.roundingPattern          = 0;

        // BPS IQ DS16 output Round and Clamp parameters
        pBPSIQsettings->sixteenthRoundAndClampParameters.lumaClampParameters.clampMin                 = 0;
        pBPSIQsettings->sixteenthRoundAndClampParameters.lumaClampParameters.clampMax                 = Max10BitValue;
        pBPSIQsettings->sixteenthRoundAndClampParameters.chromaClampParameters.clampMin               = 0;
        pBPSIQsettings->sixteenthRoundAndClampParameters.chromaClampParameters.clampMax               = Max10BitValue;
        pBPSIQsettings->sixteenthRoundAndClampParameters.lumaRoundParameters.roundingPattern          = 0;
        pBPSIQsettings->sixteenthRoundAndClampParameters.chromaRoundParameters.roundingPattern        = 0;

        // BPS IQ DS64 output Round and Clamp parameters
        pBPSIQsettings->sixtyFourthRoundAndClampParameters.lumaClampParameters.clampMin               = 0;
        pBPSIQsettings->sixtyFourthRoundAndClampParameters.lumaClampParameters.clampMax               = Max10BitValue;
        pBPSIQsettings->sixtyFourthRoundAndClampParameters.chromaClampParameters.clampMin             = 0;
        pBPSIQsettings->sixtyFourthRoundAndClampParameters.chromaClampParameters.clampMax             = Max10BitValue;
        pBPSIQsettings->sixtyFourthRoundAndClampParameters.lumaRoundParameters.roundingPattern        = 0;
        pBPSIQsettings->sixtyFourthRoundAndClampParameters.chromaRoundParameters.roundingPattern      = 0;

        // BPS IQ registration1 output Round and Clamp parameters
        pBPSIQsettings->registration1RoundAndClampParameters.lumaClampParameters.clampMin             = 0;
        pBPSIQsettings->registration1RoundAndClampParameters.lumaClampParameters.clampMax             = Max8BitValue;
        pBPSIQsettings->registration1RoundAndClampParameters.chromaClampParameters.clampMin           = 0;
        pBPSIQsettings->registration1RoundAndClampParameters.chromaClampParameters.clampMax           = Max8BitValue;
        pBPSIQsettings->registration1RoundAndClampParameters.lumaRoundParameters.roundingPattern      = 0;
        pBPSIQsettings->registration1RoundAndClampParameters.chromaRoundParameters.roundingPattern    = 0;

        // BPS IQ registration2 output Round and Clamp parameters
        pBPSIQsettings->registration2RoundAndClampParameters.lumaClampParameters.clampMin             = 0;
        pBPSIQsettings->registration2RoundAndClampParameters.lumaClampParameters.clampMax             = Max8BitValue;
        pBPSIQsettings->registration2RoundAndClampParameters.chromaClampParameters.clampMin           = 0;
        pBPSIQsettings->registration2RoundAndClampParameters.chromaClampParameters.clampMax           = Max8BitValue;
        pBPSIQsettings->registration2RoundAndClampParameters.lumaRoundParameters.roundingPattern      = 0;
        pBPSIQsettings->registration2RoundAndClampParameters.chromaRoundParameters.roundingPattern    = 0;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "BpsIQSettings: failed to add patch");
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::FillFrameBufferData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::FillFrameBufferData(
    CmdBuffer*      pFrameProcessCmdBuffer,
    ImageBuffer*    pImageBuffer,
    UINT32          numBatchFramesInBuffer,
    UINT32          portId)
{
    CamxResult      result         = CamxResultSuccess;
    SIZE_T          planeOffset    = 0;
    SIZE_T          metadataSize   = 0;
    CSLMemHandle    hMem;

    if (BPSMaxSupportedBatchSize > numBatchFramesInBuffer)
    {
        for (UINT32 batchedFrameIndex = 0; batchedFrameIndex < numBatchFramesInBuffer; batchedFrameIndex++)
        {
            // Prepare Patching struct for SMMU addresses
            for (UINT32 plane = 0; (plane < MAX_NUM_OF_IMAGE_PLANES) && (plane < pImageBuffer->GetNumberOfPlanes()); plane++)
            {
                result = pImageBuffer->GetPlaneCSLMemHandle(batchedFrameIndex, plane, &hMem, &planeOffset, &metadataSize);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid args pImageBuffer=%p, hMem=%x plane=%d, planeOffset %d"
                                   "metadataSize=%d batchInd %d",
                                   pImageBuffer, hMem, plane, planeOffset, metadataSize, batchedFrameIndex);
                }
                const ImageFormat* pFormat = pImageBuffer->GetFormat();
                if (NULL == pFormat)
                {
                    result = CamxResultEInvalidPointer;
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "pFormat is NULL");
                    break;
                }

                if (TRUE == ImageFormatUtils::IsUBWC(pFormat->format))
                {
                    result = pFrameProcessCmdBuffer->AddNestedBufferInfo(
                        s_frameBufferOffset[batchedFrameIndex][portId].metadataBufferPtr[plane],
                        hMem,
                        static_cast <UINT32>(planeOffset));
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "Error in patching address portId %d plane %d, batchedFrameIndex %d",
                                       portId, plane, batchedFrameIndex);
                        break;
                    }
                }

                result = pFrameProcessCmdBuffer->AddNestedBufferInfo(
                    s_frameBufferOffset[batchedFrameIndex][portId].bufferPtr[plane],
                    hMem,
                    (static_cast <UINT32>(planeOffset) +
                    static_cast <UINT32>(metadataSize)));

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Error in patching address portId %d plane %d, batchedFrameIndex %d",
                                   portId, plane, batchedFrameIndex);
                    break;
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Batch frame index %d is greater than BPSMaxSupportedBatchSize %d",
                       numBatchFramesInBuffer, BPSMaxSupportedBatchSize);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetCDMProgramOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT BPSNode::GetCDMProgramOffset(
    UINT CDMProgramIndex)
{
    UINT CDMProgramOffset = sizeof(BpsFrameProcess)                 +
                            sizeof(CDMProgramArray)                 +
                            offsetof(CdmProgram, cdmBaseAndLength)  +
                            offsetof(CDM_BASE_LENGHT, bitfields);

    return ((sizeof(CdmProgram) * CDMProgramIndex) + CDMProgramOffset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::FillCDMProgram
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::FillCDMProgram(
    CmdBuffer**             ppBPSCmdBuffer,
    CDMProgramArray*        pCDMProgramArray,
    CdmProgram*             pCDMProgram,
    UINT32                  programType,
    UINT32                  programIndex)
{
    CamxResult result           = CamxResultSuccess;
    UINT32     singleHeaderSize = (cdm_get_cmd_header_size(CDMCmdDMI) * RegisterWidthInBytes);

    if ((m_LUTCnt[programIndex] > 0) && (TRUE == m_moduleChromatixEnable[programIndex]))
    {

        UINT numPrograms                                 = pCDMProgramArray->numPrograms;
        pCDMProgram                                      = &pCDMProgramArray->programs[numPrograms];
        pCDMProgram->hasSingleReg                        = 0;
        pCDMProgram->programType                         = programType;
        pCDMProgram->uID                                 = 0;
        pCDMProgram->cdmBaseAndLength.bitfields.LEN      = (m_LUTCnt[programIndex] * singleHeaderSize) - 1;
        pCDMProgram->cdmBaseAndLength.bitfields.RESERVED = 0;
        pCDMProgram->bufferAllocatedInternally           = 0;

        /// @todo (CAMX-1033) Change below numPrograms to ProgramIndex once firmware support of program skip is available.
        INT dstOffset = GetCDMProgramOffset(numPrograms - 1);
        CAMX_ASSERT(dstOffset >= 0);

        result = ppBPSCmdBuffer[BPSCmdBufferFrameProcess]->AddNestedCmdBufferInfo(dstOffset,
                                                                                  ppBPSCmdBuffer[BPSCmdBufferDMIHeader],
                                                                                  m_LUTOffset[programIndex]);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "%s: Patching failed for DMI header result=%d, programIndex %d",
                NodeIdentifierString(), result, programIndex);
        }
        (pCDMProgramArray->numPrograms)++;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::FillFrameCDMArray
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::FillFrameCDMArray(
    CmdBuffer**          ppBPSCmdBuffer,
    BpsFrameProcessData* pFrameProcessData)
{
    CamxResult          result           = CamxResultSuccess;
    UINT                offset           = 0;
    CDMProgramArray*    pCDMProgramArray = NULL;
    CdmProgram*         pCDMProgram      = NULL;
    UINT32*             pCDMPayload      = reinterpret_cast<UINT32*>(pFrameProcessData);
    UINT32              dstoffset        = 0;
    UINT32              srcOffset        = 0;
    UINT                numPrograms      = 0;
    UINT32              programIndex     = BPSProgramIndexPEDESTAL;
    UINT32              programType      = BPS_PEDESTAL_LUT_PROGRAM;

    CAMX_ASSERT(NULL != pFrameProcessData);
    CAMX_ASSERT(NULL != ppBPSCmdBuffer);

    if ((NULL == pFrameProcessData) || (NULL == ppBPSCmdBuffer))
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Cmd Buffer.");
    }
    if (CamxResultSuccess == result)
    {
        // Patch IQSettings buffer in BpsFrameProcessData
        offset = static_cast<UINT32>(offsetof(BpsFrameProcess, cmdData)) +
                 static_cast<UINT32>(offsetof(BpsFrameProcessData, iqSettingsAddr));
        result = ppBPSCmdBuffer[BPSCmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset,
                                                                                  ppBPSCmdBuffer[BPSCmdBufferIQSettings],
                                                                                  0);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "%s: Patching failed for IQ setting result=%d",
                           NodeIdentifierString(), result);
        }
    }

    if (CamxResultSuccess == result)
    {
        dstoffset = static_cast <UINT32>(offsetof(BpsFrameProcess, cmdData) +
                    offsetof(BpsFrameProcessData, cdmProgramArrayAddr));
        srcOffset = sizeof(BpsFrameProcess);
        result    = ppBPSCmdBuffer[BPSCmdBufferFrameProcess]->AddNestedCmdBufferInfo(dstoffset,
                                                                                     ppBPSCmdBuffer[BPSCmdBufferFrameProcess],
                                                                                     srcOffset);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "%s: Patching failed for frame process result=%d",
                NodeIdentifierString(), result);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Patching failed for IQSettings / CDM program");
    }

    if (CamxResultSuccess == result)
    {
        // Patch CDM Cmd Buffer(BPS module registers)
        pCDMProgramArray              =
            reinterpret_cast<CDMProgramArray*>(pCDMPayload + (sizeof(BpsFrameProcess) / RegisterWidthInBytes));
        pCDMProgramArray->allocator   = 0;
        // One for CDMProgramArray and one each for LUT
        pCDMProgramArray->numPrograms = 0;

        numPrograms                                      = pCDMProgramArray->numPrograms;
        pCDMProgram                                      = &pCDMProgramArray->programs[numPrograms];
        pCDMProgram->hasSingleReg                        = 0;
        pCDMProgram->programType                         = PROGRAM_TYPE_GENERIC;
        pCDMProgram->uID                                 = 0;
        pCDMProgram->cdmBaseAndLength.bitfields.LEN =
            (ppBPSCmdBuffer[BPSCmdBufferCDMProgram]->GetResourceUsedDwords() != 0) ?
            ((ppBPSCmdBuffer[BPSCmdBufferCDMProgram]->GetResourceUsedDwords() * RegisterWidthInBytes) - 1) : 0;
        pCDMProgram->cdmBaseAndLength.bitfields.RESERVED = 0;
        pCDMProgram->bufferAllocatedInternally           = 0;

        // fetch the BASE offset in CdmProgram to patch with pCDMCmdBuffer
        dstoffset =
            sizeof(BpsFrameProcess) +
            offsetof(CDMProgramArray, programs) +
            offsetof(CdmProgram, cdmBaseAndLength) +
            offsetof(CDM_BASE_LENGHT, bitfields);

        // Patch cdmProgramArrayAddr with CDMProgramArray CdmProgram base
        result = ppBPSCmdBuffer[BPSCmdBufferFrameProcess]->AddNestedCmdBufferInfo(dstoffset,
                                                                                  ppBPSCmdBuffer[BPSCmdBufferCDMProgram],
                                                                                  0);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "%s: Patching failed for CDM program result=%d",
                NodeIdentifierString(), result);
        }
        (pCDMProgramArray->numPrograms)++;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Patching failed for CDM Program Array");
    }

    if (CamxResultSuccess == result)
    {
        for (programType = BPS_PEDESTAL_LUT_PROGRAM; programType <= BPS_HNR_LUT_PROGRAM; programType++, programIndex++)
        {
            result = FillCDMProgram(ppBPSCmdBuffer,
                                    pCDMProgramArray,
                                    pCDMProgram,
                                    programType,
                                    programIndex);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed CDMProgram for DMI %u", programType);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Patching failed for Generic IQ Settings");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::CreateFWCommandBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::CreateFWCommandBufferManagers()
{
    ResourceParams               resourceParams[BPSMaxFWCmdBufferManagers];
    CHAR                         bufferManagerName[BPSMaxFWCmdBufferManagers][MaxStringLength256];
    struct CmdBufferManagerParam createParam[BPSMaxFWCmdBufferManagers];
    UINT32                       numberOfBufferManagers = 0;
    CamxResult                   result                 = CamxResultSuccess;

    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                        m_maxCmdBufferSizeBytes[BPSCmdBufferStriping],
                        CmdType::FW,
                        CSLMemFlagUMDAccess,
                        0,
                        DeviceIndices(),
                        m_BPSCmdBlobCount);
    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                      sizeof(CHAR) * MaxStringLength256,
                      "CBM_%s_%s",
                      NodeIdentifierString(),
                      "CmdBufferStriping");
    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pBPSCmdBufferManager[BPSCmdBufferStriping];

    numberOfBufferManagers++;

    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                        m_maxCmdBufferSizeBytes[BPSCmdBufferBLMemory],
                        CmdType::FW,
                        CSLMemFlagUMDAccess,
                        0,
                        DeviceIndices(),
                        m_BPSCmdBlobCount);
    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                      sizeof(CHAR) * MaxStringLength256,
                      "CBM_%s_%s",
                      NodeIdentifierString(),
                      "CmdBufferBLMemory");
    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pBPSCmdBufferManager[BPSCmdBufferBLMemory];

    numberOfBufferManagers++;

    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                        m_maxCmdBufferSizeBytes[BPSCmdBufferFrameProcess],
                        CmdType::FW,
                        CSLMemFlagUMDAccess,
                        BPSMaxPatchAddress,
                        DeviceIndices(),
                        m_BPSCmdBlobCount);
    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                      sizeof(CHAR) * MaxStringLength256,
                      "CBM_%s_%s",
                      NodeIdentifierString(),
                      "CmdBufferFrameProcess");
    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pBPSCmdBufferManager[BPSCmdBufferFrameProcess];

    numberOfBufferManagers++;

    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                        m_maxCmdBufferSizeBytes[BPSCmdBufferIQSettings],
                        CmdType::FW,
                        CSLMemFlagUMDAccess,
                        0,
                        DeviceIndices(),
                        m_BPSCmdBlobCount);
    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                      sizeof(CHAR) * MaxStringLength256,
                      "CBM_%s_%s",
                      NodeIdentifierString(),
                      "CmdBufferIQSettings");
    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pBPSCmdBufferManager[BPSCmdBufferIQSettings];

    numberOfBufferManagers++;

    if (numberOfBufferManagers > BPSMaxFWCmdBufferManagers)
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Maximum FW Commadn Buffers recahed");
        result = CamxResultEFailed;
    }
    if ((CamxResultSuccess == result) &&  (0 != numberOfBufferManagers))
    {
        result = CreateMultiCmdBufferManager(createParam, numberOfBufferManagers);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "FW Command Buffer creation Failed %d", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::OpenStripingLibrary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::OpenStripingLibrary()
{
    CamxResult     result   = CamxResultSuccess;
    m_funcPtrBPSCreate      = NULL;
    m_funcPtrBPSDestroy     = NULL;
    m_funcPtrBPSExecute     = NULL;

#if defined(_LP64)
    const CHAR libFilename[] = "/vendor/lib64/libipebpsstriping.so";
#else // _LP64
    const CHAR libFilename[] = "/vendor/lib/libipebpsstriping.so";
#endif // _LP64
    m_hHandle = CamX::OsUtils::LibMap(libFilename);

    if (NULL != m_hHandle)
    {
        m_funcPtrBPSCreate = reinterpret_cast<BpsStripingLibraryContextCreate_t>
            (CamX::OsUtils::LibGetAddr(m_hHandle, "BPSStripingLibraryContextCreate"));
        if (NULL == m_funcPtrBPSCreate)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "function pointer is NULL for BPSStripingLibraryContextCreate");
        }
        if (result == CamxResultSuccess)
        {
            m_funcPtrBPSExecute = reinterpret_cast<BpsStripingLibraryExecute_t>
                (CamX::OsUtils::LibGetAddr(m_hHandle, "BPSStripingLibraryExecute"));
        }
        if (NULL == m_funcPtrBPSExecute)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "function pointer is NULL for BPSStripingLibraryExecute");
        }
        if (result == CamxResultSuccess)
        {
            m_funcPtrBPSDestroy = reinterpret_cast<BpsStripingLibraryContextDestroy_t>
                (CamX::OsUtils::LibGetAddr(m_hHandle, "BPSStripingLibraryContextDestroy"));
        }
        if (NULL == m_funcPtrBPSDestroy)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "function pointer is NULL for BPSStripingLibraryContextDestroy");
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Handle is NULL for Striping context create");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::CloseStripingLibrary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BPSNode::CloseStripingLibrary()
{
    CamX::OsUtils::LibUnmap(m_hHandle);
    m_hHandle = NULL;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::InitializeStripingParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::InitializeStripingParams(
    BpsConfigIoData* pConfigIOData)
{
    CamxResult     result          = CamxResultSuccess;
    ResourceParams params          = { 0 };
    UINT32         titanVersion    = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();
    UINT32         hwVersion       = static_cast<Titan17xContext *>(GetHwContext())->GetHwVersion();

    UINT32         cmdBufferStriping;
    UINT32         cmdBufferBLMemory;

    CAMX_ASSERT(NULL != pConfigIOData);

    result = m_funcPtrBPSCreate(pConfigIOData,
                                NULL,
                                titanVersion,
                                hwVersion,
                                &m_hStripingLib,
                                &cmdBufferStriping,
                                &cmdBufferBLMemory);

    CAMX_LOG_INFO(CamxLogGroupBPS, "Create BPS striping Library Context successfly with titanVersion 0x%x, hwVersion 0x%x",
        titanVersion, hwVersion);

    if (CamxResultSuccess == result )
    {
        if (0 == m_maxCmdBufferSizeBytes[BPSCmdBufferBLMemory])
        {
            m_maxCmdBufferSizeBytes[BPSCmdBufferStriping] = cmdBufferStriping;
            m_maxCmdBufferSizeBytes[BPSCmdBufferBLMemory] = cmdBufferBLMemory;

            CAMX_LOG_INFO(CamxLogGroupBPS, "Create FW command buffer manager: striping=%d, BL=%d",
                cmdBufferStriping, cmdBufferBLMemory);
            result =  CreateFWCommandBufferManagers();
        }
        else
        {
            // the buffer created initially should be good for the later zoom case. So there is no
            // need to recreate the buffer pools. we will only do basic checking here.
            if ((cmdBufferStriping > m_maxCmdBufferSizeBytes[BPSCmdBufferStriping]) ||
                (cmdBufferBLMemory > m_maxCmdBufferSizeBytes[BPSCmdBufferBLMemory]))
            {
                CAMX_LOG_INFO(CamxLogGroupBPS, "BPSCmdBufferStriping memory size is increased from %d to %d",
                    m_maxCmdBufferSizeBytes[BPSCmdBufferStriping], cmdBufferStriping);
                CAMX_LOG_INFO(CamxLogGroupBPS, "BPSCmdBufferBLMemory memory size is increated fromm %d to %d",
                    m_maxCmdBufferSizeBytes[BPSCmdBufferBLMemory], cmdBufferBLMemory);
            }
            CAMX_LOG_INFO(CamxLogGroupBPS, "Skip FW command buffer manager creation");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "StripingLib ctxt create failed %d configIO %p titanVersion 0x%x hwVersion 0x%x",
                           result, pConfigIOData, titanVersion, hwVersion);
        result = CamxResultEFailed;
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::DeInitializeStripingParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::DeInitializeStripingParams()
{
    CamxResult  result = CamxResultSuccess;

    if (NULL != m_hStripingLib)
    {
        result          = m_funcPtrBPSDestroy(&m_hStripingLib);
        m_hStripingLib  = NULL;
        if (result != 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "BPS:%d Cannot destroy Striping Library with result=%d", InstanceID(), result);
            result = CamxResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::FillStripingParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::FillStripingParams(
    BpsFrameProcessData*         pFrameProcessData,
    BpsIQSettings*               pBPSIQsettings,
    CmdBuffer**                  ppBPSCmdBuffer,
    BPSClockAndBandwidth*        pClockAndBandwidth)
{
    CamxResult                     result                       = CamxResultSuccess;
    UINT32*                        pStripeMem                   = NULL;
    UINT32                         offset                       = 0;
    CmdBuffer*                     pBPSStripingCmdBuffer        = ppBPSCmdBuffer[BPSCmdBufferStriping];
    CmdBuffer*                     pBPSFrameProcessCmdBuffer    = ppBPSCmdBuffer[BPSCmdBufferFrameProcess];
    BPSStripingLibExecuteParams    stripeParams                 = { 0 };
    BPSStripingLibExecuteMetaData  metaDataBuffer               = { 0 };
    userBPSArgs                    userBPSArgs                  = { 0 };
    CAMX_ASSERT(NULL != pFrameProcessData);
    if (NULL == pFrameProcessData)
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "BPS Frame Process Data is NULL.");
    }
    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(NULL != pBPSIQsettings);
        CAMX_ASSERT(NULL != ppBPSCmdBuffer);
        CAMX_ASSERT(NULL != pBPSStripingCmdBuffer);

        CAMX_LOG_VERBOSE(CamxLogGroupBPS, " node %s , profile %d,processingtype %d,pedestal %d,"
                      "linear %d,pdpc %d, hdr 1.0 %d, hdr 3.0 %d,bc %d,hdrmac %d,gic %d"
                      "abf %d, lens %d, demosaic %d, bayergrid %d, bayerhisto %d colorC %d, gtm %d, glut %d"
                      "colorXf %d, hnr %d",
                      NodeIdentifierString(),
                      m_instanceProperty.profileId, m_instanceProperty.processingType,
                      pBPSIQsettings->pedestalParameters.moduleCfg.EN,
                      pBPSIQsettings->linearizationParameters.moduleCfg.EN,
                      pBPSIQsettings->pdpcParameters.moduleCfg.EN,
                      pBPSIQsettings->hdrReconParameters.moduleCfg.EN,
                      pBPSIQsettings->hdrBinCorrectParameters_480.hdrEnable,
                      pBPSIQsettings->hdrBinCorrectParameters_480.hdrEnable,
                      pBPSIQsettings->hdrMacParameters.moduleCfg.EN,
                      pBPSIQsettings->gicParameters.moduleCfg.EN,
                      pBPSIQsettings->abfParameters.moduleCfg.EN,
                      pBPSIQsettings->lensRollOffParameters.moduleCfg.EN,
                      pBPSIQsettings->demosaicParameters.moduleCfg.EN,
                      pBPSIQsettings->bayerGridParameters.moduleCfg.EN,
                      pBPSIQsettings->bayerHistogramParameters.moduleCfg.EN,
                      pBPSIQsettings->colorCorrectParameters.moduleCfg.EN,
                      pBPSIQsettings->gtmParameters.moduleCfg.EN,
                      pBPSIQsettings->glutParameters.moduleCfg.EN,
                      pBPSIQsettings->colorXformParameters.moduleCfg.EN,
                      pBPSIQsettings->hnrParameters.moduleCfg.EN);

        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Zoom window: en=%d, left=%d, top=%d, width=%d, height=%d",
            pBPSIQsettings->fetchCropEN,
            pBPSIQsettings->fetchCropWindow.windowLeft,
            pBPSIQsettings->fetchCropWindow.windowTop,
            pBPSIQsettings->fetchCropWindow.windowWidth,
            pBPSIQsettings->fetchCropWindow.windowHeight);


        pFrameProcessData->targetCompletionTimeInNs = 0;

        pStripeMem = reinterpret_cast<UINT32*>(pBPSStripingCmdBuffer->BeginCommands(
            m_maxCmdBufferSizeBytes[BPSCmdBufferStriping] / sizeof(UINT32)));
    }
#if DEBUG
    DumpConfigIOData();
#endif  // DumpConfigIOData

    if ((NULL != pStripeMem) && (CamxResultSuccess == result))
    {
        stripeParams.iq                 = pBPSIQsettings;
        stripeParams.maxNumOfCoresToUse = pFrameProcessData->maxNumOfCoresToUse;

        userBPSArgs.frameNumber         = pFrameProcessData->requestId;
        userBPSArgs.instance            = InstanceID();
        userBPSArgs.processingType      = m_instanceProperty.processingType;
        userBPSArgs.profileID           = m_instanceProperty.profileId;
        userBPSArgs.realTime            = IsRealTime();
        userBPSArgs.FileDumpPath        = FileDumpPath;
        userBPSArgs.dumpEnable          = m_BPSStripeDumpEnable;

        if (NULL != m_hStripingLib)
        {
            result = m_funcPtrBPSExecute(m_hStripingLib, &stripeParams, pStripeMem,
                                         &metaDataBuffer, &userBPSArgs);
        }

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "Striping Library execution completed");
            offset =
                static_cast<UINT32>(offsetof(BpsFrameProcess, cmdData)) +
                static_cast<UINT32>(offsetof(BpsFrameProcessData, stripingLibOutAddr));
            result = pBPSFrameProcessCmdBuffer->AddNestedCmdBufferInfo(offset, pBPSStripingCmdBuffer, 0);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "%s: Patching failed for striping result=%d",
                    NodeIdentifierString(), result);
            }
            pClockAndBandwidth->frameCycles = metaDataBuffer.pixelCount;
            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Num pixels = %d", metaDataBuffer.pixelCount);
        }
        else
        {
            /// @todo (CAMX-1732) To add transalation for firmware errors
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Striping Library execution failed %d", result);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Striping memory");
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::InitializeBPSIQSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE VOID BPSNode::InitializeBPSIQSettings(
    BpsIQSettings*               pBPSIQsettings)
{
    pBPSIQsettings->pedestalParameters.moduleCfg.EN             = 0;
    pBPSIQsettings->linearizationParameters.moduleCfg.EN        = 0;
    pBPSIQsettings->pdpcParameters.moduleCfg.EN                 = 0;
    pBPSIQsettings->hdrReconParameters.moduleCfg.EN             = 0;
    pBPSIQsettings->hdrMacParameters.moduleCfg.EN               = 0;
    pBPSIQsettings->gicParameters.moduleCfg.EN                  = 0;
    pBPSIQsettings->abfParameters.moduleCfg.EN                  = 0;
    pBPSIQsettings->lensRollOffParameters.moduleCfg.EN          = 0;
    pBPSIQsettings->demosaicParameters.moduleCfg.EN             = 0;
    pBPSIQsettings->bayerGridParameters.moduleCfg.EN            = 0;
    pBPSIQsettings->bayerHistogramParameters.moduleCfg.EN       = 0;
    pBPSIQsettings->colorCorrectParameters.moduleCfg.EN         = 0;
    pBPSIQsettings->glutParameters.moduleCfg.EN                 = 0;
    pBPSIQsettings->colorXformParameters.moduleCfg.EN           = 0;
    pBPSIQsettings->hnrParameters.moduleCfg.EN                  = 0;

    // New modules for Titan 480
    pBPSIQsettings->pdpcParameters_480.moduleCfg.EN                 = 0;
    pBPSIQsettings->pdpcParameters_480.moduleCfg.PDAF_PDPC_EN       = 0;
    pBPSIQsettings->pdpcParameters_480.moduleCfg.BPC_EN             = 0;
    pBPSIQsettings->pdpcParameters_480.moduleCfg.FLAT_DETECTION_EN  = 0;
    pBPSIQsettings->pdpcParameters_480.moduleCfg.DBPC_EN            = 0;
    pBPSIQsettings->pdpcParameters_480.moduleCfg.CHANNEL_BALANCE_EN = 0;
    pBPSIQsettings->hdrBinCorrectParameters_480.moduleCfg.EN        = 0;
    pBPSIQsettings->lensRollOffParameters_480.moduleCfg.EN          = 0;
    pBPSIQsettings->lensRollOffParameters_480.moduleCfg.ALSC_EN     = 0;
    pBPSIQsettings->hdrBinCorrectParameters_480.binCorrEn           = 0;
    pBPSIQsettings->hdrBinCorrectParameters_480.moduleCfg.EN        = 0;
    pBPSIQsettings->fetchCropEN                                     = 0;

    // New module for future targets
    pBPSIQsettings->bayerLtmParameters.moduleCfg.EN             = 0;
    pBPSIQsettings->bayerGtmParameters.moduleCfg.EN             = 0;
    pBPSIQsettings->uvGammaParameters.moduleCfg.EN              = 0;
    pBPSIQsettings->compdecompParameters.moduleCfg.EN           = 0;
    pBPSIQsettings->wbParameters.moduleCfg.EN                   = 0;
    pBPSIQsettings->lcacParameters.moduleCfg.EN                 = 0;
    pBPSIQsettings->binCorrectParameters.moduleCfg.EN           = 0;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::PatchBLMemoryBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::PatchBLMemoryBuffer(
    BpsFrameProcessData* pFrameProcessData,
    CmdBuffer**          ppBPSCmdBuffer)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != ppBPSCmdBuffer[BPSCmdBufferBLMemory]) && (0 != m_maxCmdBufferSizeBytes[BPSCmdBufferBLMemory]))
    {
        pFrameProcessData->cdmBufferSize = m_maxCmdBufferSizeBytes[BPSCmdBufferBLMemory];

        UINT  offset =
            static_cast<UINT32>(offsetof(BpsFrameProcess, cmdData)) +
            static_cast<UINT32>(offsetof(BpsFrameProcessData, cdmBufferAddress));

        result = ppBPSCmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset,
                                                                               ppBPSCmdBuffer[BPSCmdBufferBLMemory],
                                                                               0);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "%s: Patching failed for BL mem result=%d",
                NodeIdentifierString(), result);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::CommitAllCommandBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::CommitAllCommandBuffers(
    CmdBuffer** ppBPSCmdBuffer)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != ppBPSCmdBuffer[BPSCmdBufferStriping])
    {
        result = ppBPSCmdBuffer[BPSCmdBufferStriping]->CommitCommands();
    }

    if (CamxResultSuccess == result)
    {
        result = ppBPSCmdBuffer[BPSCmdBufferIQSettings]->CommitCommands();
    }

    if (CamxResultSuccess == result)
    {
        ppBPSCmdBuffer[BPSCmdBufferFrameProcess]->CommitCommands();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::SetIQModuleNumLUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BPSNode::SetIQModuleNumLUT(
    ISPIQModuleType type,
    UINT            numLUTs)
{
    switch (type)
    {
        case ISPIQModuleType::BPSPedestalCorrection:
            m_LUTCnt[BPSProgramIndexPEDESTAL] = numLUTs;
            break;
        case ISPIQModuleType::BPSLinearization:
            m_LUTCnt[BPSProgramIndexLINEARIZATION] = numLUTs;
            break;
        case ISPIQModuleType::BPSBPCPDPC:
            m_LUTCnt[BPSProgramIndexBPCPDPC] = numLUTs;
            break;
        case ISPIQModuleType::BPSABF:
            m_LUTCnt[BPSProgramIndexABF] = numLUTs;
            break;
        case ISPIQModuleType::BPSLSC:
            m_LUTCnt[BPSProgramIndexRolloff] = numLUTs;
            break;
        case ISPIQModuleType::BPSGIC:
            m_LUTCnt[BPSProgramIndexGIC] = numLUTs;
            break;
        case ISPIQModuleType::BPSGTM:
            m_LUTCnt[BPSProgramIndexGTM] = numLUTs;
            break;
        case ISPIQModuleType::BPSGamma:
            m_LUTCnt[BPSProgramIndexGLUT] = numLUTs;
            break;
        case ISPIQModuleType::BPSHNR:
            m_LUTCnt[BPSProgramIndexHNR] = numLUTs;
            break;
        case ISPIQModuleType::BPSDemux:
        case ISPIQModuleType::BPSHDR:
        case ISPIQModuleType::BPSDemosaic:
        case ISPIQModuleType::BPSCC:
        case ISPIQModuleType::BPSCST:
        case ISPIQModuleType::BPSChromaSubSample:
        case ISPIQModuleType::SWTMC:
        case ISPIQModuleType::BPSAWBBG:
        case ISPIQModuleType::BPSHDRBHist:
        case ISPIQModuleType::BPSWB:
            // These IQ Modules does not have LUTs to program
            break;
        default:
            CAMX_LOG_WARN(CamxLogGroupBPS, "Unsupported BPS IQ Module type: %u", type);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::SetIQModuleLUTOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BPSNode::SetIQModuleLUTOffset(
    ISPIQModuleType type,
    BpsIQSettings*  pBPSIQSettings,
    UINT            LUTOffset,
    UINT32*         pLUTCount)
{
    switch (type)
    {
        case ISPIQModuleType::BPSPedestalCorrection:
            m_moduleChromatixEnable[BPSProgramIndexPEDESTAL] = pBPSIQSettings->pedestalParameters.moduleCfg.EN;
            if (TRUE == m_moduleChromatixEnable[BPSProgramIndexPEDESTAL])
            {
                m_LUTOffset[BPSProgramIndexPEDESTAL] = LUTOffset;
                *pLUTCount                           = m_LUTCnt[BPSProgramIndexPEDESTAL];
            }
            break;
        case ISPIQModuleType::BPSLinearization:
            m_moduleChromatixEnable[BPSProgramIndexLINEARIZATION] = pBPSIQSettings->linearizationParameters.moduleCfg.EN;
            if (TRUE == m_moduleChromatixEnable[BPSProgramIndexLINEARIZATION])
            {
                m_LUTOffset[BPSProgramIndexLINEARIZATION] = LUTOffset;
                *pLUTCount                                = m_LUTCnt[BPSProgramIndexLINEARIZATION];
            }
            break;
        case ISPIQModuleType::BPSBPCPDPC:
            m_moduleChromatixEnable[BPSProgramIndexBPCPDPC] =
                (pBPSIQSettings->pdpcParameters.moduleCfg.PDAF_PDPC_EN ||
                 pBPSIQSettings->pdpcParameters_480.moduleCfg.PDAF_PDPC_EN);

            if (TRUE == m_moduleChromatixEnable[BPSProgramIndexBPCPDPC])
            {
                m_LUTOffset[BPSProgramIndexBPCPDPC] = LUTOffset;
                *pLUTCount                          = m_LUTCnt[BPSProgramIndexBPCPDPC];
            }
            break;
        case ISPIQModuleType::BPSABF:
            m_moduleChromatixEnable[BPSProgramIndexABF] = pBPSIQSettings->abfParameters.moduleCfg.EN;
            if (TRUE == m_moduleChromatixEnable[BPSProgramIndexABF])
            {
                m_LUTOffset[BPSProgramIndexABF] = LUTOffset;
                *pLUTCount                      = m_LUTCnt[BPSProgramIndexABF];
            }
            break;
        case ISPIQModuleType::BPSLSC:
            m_moduleChromatixEnable[BPSProgramIndexRolloff] =
                (pBPSIQSettings->lensRollOffParameters.moduleCfg.EN || pBPSIQSettings->lensRollOffParameters_480.moduleCfg.EN);
            if (TRUE == m_moduleChromatixEnable[BPSProgramIndexRolloff])
            {
                m_LUTOffset[BPSProgramIndexRolloff] = LUTOffset;
                *pLUTCount                          = m_LUTCnt[BPSProgramIndexRolloff];
            }
            break;
        case ISPIQModuleType::BPSGIC:
            m_moduleChromatixEnable[BPSProgramIndexGIC] = pBPSIQSettings->gicParameters.moduleCfg.EN;
            if (TRUE == m_moduleChromatixEnable[BPSProgramIndexGIC])
            {
                m_LUTOffset[BPSProgramIndexGIC] = LUTOffset;
                *pLUTCount                      = m_LUTCnt[BPSProgramIndexGIC];
            }
            break;
        case ISPIQModuleType::BPSGTM:
            m_moduleChromatixEnable[BPSProgramIndexGTM] = pBPSIQSettings->gtmParameters.moduleCfg.EN;
            if (TRUE == m_moduleChromatixEnable[BPSProgramIndexGTM])
            {
                m_LUTOffset[BPSProgramIndexGTM] = LUTOffset;
                *pLUTCount                      = m_LUTCnt[BPSProgramIndexGTM];
            }
            break;
        case ISPIQModuleType::BPSGamma:
            m_moduleChromatixEnable[BPSProgramIndexGLUT] = pBPSIQSettings->glutParameters.moduleCfg.EN;
            if (TRUE == m_moduleChromatixEnable[BPSProgramIndexGLUT])
            {
                m_LUTOffset[BPSProgramIndexGLUT] = LUTOffset;
                *pLUTCount                       = m_LUTCnt[BPSProgramIndexGLUT];
            }
            break;
        case ISPIQModuleType::BPSHNR:
            m_moduleChromatixEnable[BPSProgramIndexHNR] = pBPSIQSettings->hnrParameters.moduleCfg.EN;
            if (TRUE == m_moduleChromatixEnable[BPSProgramIndexHNR])
            {
                m_LUTOffset[BPSProgramIndexHNR] = LUTOffset;
                *pLUTCount                      = m_LUTCnt[BPSProgramIndexHNR];
            }
            break;
        case ISPIQModuleType::BPSDemux:
        case ISPIQModuleType::BPSHDR:
        case ISPIQModuleType::BPSDemosaic:
        case ISPIQModuleType::BPSCC:
        case ISPIQModuleType::BPSCST:
        case ISPIQModuleType::BPSChromaSubSample:
        case ISPIQModuleType::BPSWB:
        case ISPIQModuleType::SWTMC:
        case ISPIQModuleType::BPSAWBBG:
        case ISPIQModuleType::BPSHDRBHist:
            // These IQ Modules does not have LUTs to program
            break;
        default:
            CAMX_LOG_WARN(CamxLogGroupBPS, "Unsupported BPS IQ Module type: %u", type);
            break;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::UpdateLUTData()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::UpdateLUTData(
    BpsIQSettings* pBPSIQSettings)
{
    UINT32 numLUT           = 0;
    UINT32 singleHeaderSize = cdm_get_cmd_header_size(CDMCmdDMI) * sizeof(UINT32);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    for (UINT count = 0; count < m_numIQMOdules; count++)
    {
        UINT32 LUTCount = 0;
        SetIQModuleLUTOffset(m_pBPSIQModules[count]->GetIQType(), pBPSIQSettings, (numLUT * singleHeaderSize), &LUTCount);
        numLUT += LUTCount;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CAMX_ASSERT(NULL != pBufferNegotiationData);

    CamxResult                 result                          = CamxResultSuccess;
    UINT32                     optimalInputWidth               = 0;
    UINT32                     optimalInputHeight              = 0;
    UINT32                     minInputWidth                   = BPSMinInputWidth;
    UINT32                     minInputHeight                  = BPSMinInputHeight;
    UINT32                     maxInputWidth                   = BPSMaxInputWidth;
    UINT32                     maxInputHeight                  = BPSMaxInputHeight;
    UINT32                     perOutputPortOptimalWidth       = 0;
    UINT32                     perOutputPortOptimalHeight      = 0;
    UINT32                     perOutputPortMinWidth           = 0;
    UINT32                     perOutputPortMinHeight          = 0;
    UINT32                     perOutputPortMaxWidth           = 0;
    UINT32                     perOutputPortMaxHeight          = 0;
    BufferRequirement*         pOutputBufferRequirementOptions = NULL;
    BufferRequirement*         pInputPortRequirement           = NULL;
    OutputPortNegotiationData* pOutputPortNegotiationData      = NULL;
    UINT                       outputPortIndex                 = 0;
    UINT                       outputPortId                    = 0;
    AlignmentInfo              alignmentLCM[FormatsMaxPlanes]  = { {0} };

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    // The input buffer requirement will be the super resolution after looping
    // through all the output ports.
    for (UINT index = 0; (NULL != pBufferNegotiationData) && (index < pBufferNegotiationData->numOutputPortsNotified); index++)
    {
        pOutputPortNegotiationData      = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        pOutputBufferRequirementOptions = &pOutputPortNegotiationData->outputBufferRequirementOptions;
        outputPortIndex                 = pOutputPortNegotiationData->outputPortIndex;
        outputPortId                    = GetOutputPortId(outputPortIndex);

        if ((TRUE == IsDownscaleOutputPort(outputPortId))   ||
            (TRUE == IsStatsPort(outputPortId)))
        {
            optimalInputWidth  = Utils::MaxUINT32(optimalInputWidth, minInputWidth);
            optimalInputHeight = Utils::MaxUINT32(optimalInputHeight, minInputHeight);
            continue;
        }

        Utils::Memset(pOutputBufferRequirementOptions, 0, sizeof(BufferRequirement));

        perOutputPortOptimalWidth  = 0;
        perOutputPortOptimalHeight = 0;
        perOutputPortMinWidth      = 0;
        perOutputPortMinHeight     = 0;
        perOutputPortMaxWidth      = 0;
        perOutputPortMaxHeight     = 0;

        // Go through the requirements of the input ports connected to the output port
        for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
        {
            pInputPortRequirement = &pOutputPortNegotiationData->inputPortRequirement[inputIndex];

            // Max dimension should be smallest from all the ports connected.
            perOutputPortMaxWidth = (0 == perOutputPortMaxWidth) ? pInputPortRequirement->maxWidth :
                Utils::MinUINT32(perOutputPortMaxWidth, pInputPortRequirement->maxWidth);
            perOutputPortMaxHeight = (0 == perOutputPortMaxHeight) ? pInputPortRequirement->maxHeight :
                Utils::MinUINT32(perOutputPortMaxHeight, pInputPortRequirement->maxHeight);

            // Optimal dimension should be largest optimal value from all the ports connected.
            perOutputPortOptimalWidth = Utils::MaxUINT32(perOutputPortOptimalWidth, pInputPortRequirement->optimalWidth);
            perOutputPortOptimalHeight = Utils::MaxUINT32(perOutputPortOptimalHeight, pInputPortRequirement->optimalHeight);

            // Min dimension should be largest minimum from all the ports connected.
            perOutputPortMinWidth = Utils::MaxUINT32(perOutputPortMinWidth, pInputPortRequirement->minWidth);
            perOutputPortMinHeight = Utils::MaxUINT32(perOutputPortMinHeight, pInputPortRequirement->minHeight);

            CAMX_LOG_INFO(CamxLogGroupBPS, "input ports: index %d, inputIndex %d, width((opt, min, max) %d , %d,  %d,"
                "height((opt, min, max) %d, %d, %d",
                index, inputIndex, perOutputPortOptimalWidth, perOutputPortMinWidth, perOutputPortMaxWidth,
                perOutputPortOptimalHeight, perOutputPortMinHeight, perOutputPortMaxHeight);

            for (UINT planeIdx = 0; planeIdx < FormatsMaxPlanes; planeIdx++)
            {
                alignmentLCM[planeIdx].strideAlignment   =
                Utils::CalculateLCM(
                                    static_cast<INT32>(alignmentLCM[planeIdx].strideAlignment),
                                    static_cast<INT32>(pInputPortRequirement->planeAlignment[planeIdx].strideAlignment));
                alignmentLCM[planeIdx].scanlineAlignment =
                Utils::CalculateLCM(
                                    static_cast<INT32>(alignmentLCM[planeIdx].scanlineAlignment),
                                    static_cast<INT32>(pInputPortRequirement->planeAlignment[planeIdx].scanlineAlignment));
            }
        }

        // Optimal dimension should lie between the min and max, ensure the same.
        // There is a chance of the Optimal dimension going over the max dimension.
        perOutputPortOptimalWidth =
            Utils::ClampUINT32(perOutputPortOptimalWidth, perOutputPortMinWidth, perOutputPortMaxWidth);
        perOutputPortOptimalHeight =
            Utils::ClampUINT32(perOutputPortOptimalHeight, perOutputPortMinHeight, perOutputPortMaxHeight);

        // Store the buffer requirements for this output port which will be reused to set, during forward walk.
        // The values stored here could be final output dimensions unless it is overridden by forward walk.
        pOutputBufferRequirementOptions->optimalWidth      = perOutputPortOptimalWidth;
        pOutputBufferRequirementOptions->optimalHeight     = perOutputPortOptimalHeight;

        pOutputBufferRequirementOptions->minWidth          = perOutputPortMinWidth;
        pOutputBufferRequirementOptions->maxWidth          = perOutputPortMaxWidth;
        pOutputBufferRequirementOptions->minHeight         = perOutputPortMinHeight;
        pOutputBufferRequirementOptions->maxHeight         = perOutputPortMaxHeight;

        Utils::Memcpy(&pOutputBufferRequirementOptions->planeAlignment[0],
                      &alignmentLCM[0],
                      sizeof(AlignmentInfo) * FormatsMaxPlanes);

        // Maximum input requirements can be adjusted for RegOut ports as they have an
        // internal downscaler.
        if ((outputPortId != BPSOutputPortReg1) && (outputPortId != BPSOutputPortReg2))
        {
            maxInputWidth = (0 == maxInputWidth) ? perOutputPortMaxWidth :
                Utils::MinUINT32(maxInputWidth, perOutputPortMaxWidth);
            maxInputHeight = (0 == maxInputHeight) ? perOutputPortMaxHeight :
                Utils::MinUINT32(maxInputHeight, perOutputPortMaxHeight);
        }
        else
        {
            maxInputWidth = (0 == maxInputWidth) ? BPSMaxInputWidth :
                Utils::MinUINT32(maxInputWidth, BPSMaxInputWidth);
            maxInputHeight = (0 == maxInputHeight) ? BPSMaxInputHeight :
                Utils::MinUINT32(maxInputHeight, BPSMaxInputHeight);
        }

        minInputWidth      = Utils::MaxUINT32(minInputWidth, perOutputPortMinWidth);
        minInputHeight     = Utils::MaxUINT32(minInputHeight, perOutputPortMinHeight);

        optimalInputWidth   = Utils::MaxUINT32(optimalInputWidth, perOutputPortOptimalWidth);
        optimalInputHeight  = Utils::MaxUINT32(optimalInputHeight, perOutputPortOptimalHeight);

        CAMX_LOG_INFO(CamxLogGroupBPS, "output port: index %d, width(opt, min, max) %d , %d,  %d,"
                         "height (opt, min, max) %d, %d, %d",
                         index, optimalInputWidth, minInputWidth, maxInputWidth,
                         optimalInputHeight, minInputHeight, maxInputHeight);
    }

    if ((optimalInputWidth == 0) || (optimalInputHeight == 0))
    {
        result = CamxResultEFailed;

        CAMX_LOG_ERROR(CamxLogGroupBPS,
                       "Buffer Negotiation Failed, W:%d x H:%d!\n",
                       optimalInputWidth,
                       optimalInputHeight);
    }
    else
    {
        // Ensure optimal dimension is within min and max dimension,
        // There are chances that the optimal dimension is more than max dimension.
        optimalInputWidth  =
            Utils::ClampUINT32(optimalInputWidth, minInputWidth, maxInputWidth);
        optimalInputHeight =
            Utils::ClampUINT32(optimalInputHeight, minInputHeight, maxInputHeight);

        UINT32 numInputPorts = 0;
        UINT32 inputPortId[BPSMaxInput];

        // Get Input Port List
        GetAllInputPortIds(&numInputPorts, &inputPortId[0]);
        if (NULL == pBufferNegotiationData)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Buffer Negotiation Data.");
        }

        if (CamxResultSuccess == result)
        {
            pBufferNegotiationData->numInputPorts = numInputPorts;

            for (UINT input = 0; input < numInputPorts; input++)
            {
                pBufferNegotiationData->inputBufferOptions[input].nodeId     = Type();
                pBufferNegotiationData->inputBufferOptions[input].instanceId = InstanceID();
                pBufferNegotiationData->inputBufferOptions[input].portId     = inputPortId[input];

                BufferRequirement* pInputBufferRequirement =
                    &pBufferNegotiationData->inputBufferOptions[input].bufferRequirement;

                pInputBufferRequirement->optimalWidth  = optimalInputWidth;
                pInputBufferRequirement->optimalHeight = optimalInputHeight;
                pInputBufferRequirement->minWidth      = minInputWidth;
                pInputBufferRequirement->maxWidth      = maxInputWidth;
                pInputBufferRequirement->minHeight     = minInputHeight;
                pInputBufferRequirement->maxHeight     = maxInputHeight;

                CAMX_LOG_INFO(CamxLogGroupBPS, "Port %d width(opt, min, max) %d , %d,  %d,"
                              "height((opt, min, max)) %d, %d, %d",
                              inputPortId[input],
                              optimalInputWidth,
                              minInputWidth,
                              maxInputWidth,
                              optimalInputHeight,
                              minInputHeight,
                              maxInputHeight);
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        result = SetupICAGrid();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    CAMX_ASSERT(NULL != pBufferNegotiationData);

    for (UINT index = 0; (NULL != pBufferNegotiationData) && (index < pBufferNegotiationData->numOutputPortsNotified); index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData   = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        InputPortNegotiationData*  pInputPortNegotiationData    = &pBufferNegotiationData->pInputPortNegotiationData[0];
        BufferProperties*          pFinalOutputBufferProperties = pOutputPortNegotiationData->pFinalOutputBufferProperties;

        if ((FALSE == IsSinkPortWithBuffer(pOutputPortNegotiationData->outputPortIndex)) &&
            (FALSE == IsNonSinkHALBufferOutput(pOutputPortNegotiationData->outputPortIndex)))
        {
            UINT outputPortId = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);

            switch (outputPortId)
            {
                case BPSOutputPortFull:
                    pFinalOutputBufferProperties->imageFormat.width  =
                        pInputPortNegotiationData->pImageFormat->width;
                    pFinalOutputBufferProperties->imageFormat.height =
                        pInputPortNegotiationData->pImageFormat->height;
                    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "full width %d, height %d",
                        pFinalOutputBufferProperties->imageFormat.width,
                        pFinalOutputBufferProperties->imageFormat.height);
                    break;
                case BPSOutputPortDS4:
                    if (BPSProfileId::BPSProfileIdIdealRawOutput == m_instanceProperty.profileId)
                    {
                        pFinalOutputBufferProperties->imageFormat.width  =
                            pInputPortNegotiationData->pImageFormat->width;
                        pFinalOutputBufferProperties->imageFormat.height =
                            pInputPortNegotiationData->pImageFormat->height;
                    }
                    else
                    {
                        pFinalOutputBufferProperties->imageFormat.width =
                            Utils::EvenCeilingUINT32(
                                Utils::AlignGeneric32(pInputPortNegotiationData->pImageFormat->width, 4) / 4);
                        pFinalOutputBufferProperties->imageFormat.height =
                            Utils::EvenCeilingUINT32(
                                Utils::AlignGeneric32(pInputPortNegotiationData->pImageFormat->height, 4) / 4);
                    }
                    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "DS4 width %d, height %d",
                        pFinalOutputBufferProperties->imageFormat.width,
                        pFinalOutputBufferProperties->imageFormat.height);
                    break;
                case BPSOutputPortDS16:
                    pFinalOutputBufferProperties->imageFormat.width =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(pInputPortNegotiationData->pImageFormat->width, 16) / 16);
                    pFinalOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(pInputPortNegotiationData->pImageFormat->height, 16) / 16);
                    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "DS16 width %d, height %d",
                        pFinalOutputBufferProperties->imageFormat.width,
                        pFinalOutputBufferProperties->imageFormat.height);
                    break;
                case BPSOutputPortDS64:
                    pFinalOutputBufferProperties->imageFormat.width =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(pInputPortNegotiationData->pImageFormat->width, 64) / 64);
                    pFinalOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(pInputPortNegotiationData->pImageFormat->height, 64) / 64);
                    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "DS64 width %d, height %d",
                        pFinalOutputBufferProperties->imageFormat.width,
                        pFinalOutputBufferProperties->imageFormat.height);
                    break;
                case BPSOutputPortReg1:
                case BPSOutputPortReg2:
                    if ((BPSProcessingMFNR == m_instanceProperty.processingType) ||
                        (BPSProcessingMFSR == m_instanceProperty.processingType))
                    {
                        CalculateRegistrationOutputSize(pInputPortNegotiationData->pImageFormat,
                                                        &pFinalOutputBufferProperties->imageFormat);

                        CAMX_LOG_INFO(CamxLogGroupBPS,
                                      "MFNR Reg width %d, height %d,"
                                      "planeStride=%d, sliceHeight=%d,"
                                      "planceStride2=%d, sliceHeight2=%d",
                                      pFinalOutputBufferProperties->imageFormat.width,
                                      pFinalOutputBufferProperties->imageFormat.height,
                                      pFinalOutputBufferProperties->imageFormat.formatParams.yuvFormat[0].planeStride,
                                      pFinalOutputBufferProperties->imageFormat.formatParams.yuvFormat[0].sliceHeight,
                                      pFinalOutputBufferProperties->imageFormat.formatParams.yuvFormat[1].planeStride,
                                      pFinalOutputBufferProperties->imageFormat.formatParams.yuvFormat[1].sliceHeight);
                    }
                    else
                    {
                        pFinalOutputBufferProperties->imageFormat.width =
                            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth;
                        pFinalOutputBufferProperties->imageFormat.height =
                            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight;
                        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Reg width %d, height %d",
                            pFinalOutputBufferProperties->imageFormat.width,
                            pFinalOutputBufferProperties->imageFormat.height);
                    }
                    break;

                case BPSOutputPortStatsBG:
                    pFinalOutputBufferProperties->imageFormat.width  = BPSAWBBGStatsMaxWidth;
                    pFinalOutputBufferProperties->imageFormat.height = BPSAWBBGStatsMaxHeight;
                    break;

                case BPSOutputPortStatsHDRBHist:
                    pFinalOutputBufferProperties->imageFormat.width  = HDRBHistStatsMaxWidth;
                    pFinalOutputBufferProperties->imageFormat.height = HDRBHistStatsMaxHeight;
                    break;

                default:
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Unsupported port %d", outputPortId);
                    break;
            }

            Utils::Memcpy(&pFinalOutputBufferProperties->imageFormat.planeAlignment[0],
                          &pOutputPortNegotiationData->outputBufferRequirementOptions.planeAlignment[0],
                          sizeof(AlignmentInfo) * FormatsMaxPlanes);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetStaticMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::GetStaticMetadata(
    UINT32 cameraId)
{
    CamxResult   result = CamxResultSuccess;
    HwCameraInfo cameraInfo;

    result = HwEnvironment::GetInstance()->GetCameraInfo(cameraId, &cameraInfo);
    if (CamxResultSuccess == result)
    {
        m_HALTagsData.controlPostRawSensitivityBoost    = cameraInfo.pPlatformCaps->minPostRawSensitivityBoost;
        // Retrieve the static capabilities for this camera
        m_pOTPData = &(cameraInfo.pSensorCaps->OTPData);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to obtain camera info, result: %d", result);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::InitializeDefaultHALTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::InitializeDefaultHALTags()
{
    // Initialize default metadata
    m_HALTagsData.blackLevelLock                = BlackLevelLockOff;
    m_HALTagsData.colorCorrectionMode           = ColorCorrectionModeFast;
    m_HALTagsData.controlAEMode                 = ControlAEModeOn;
    m_HALTagsData.controlAWBMode                = ControlAWBModeAuto;
    m_HALTagsData.controlMode                   = ControlModeAuto;
    m_HALTagsData.noiseReductionMode            = NoiseReductionModeFast;
    m_HALTagsData.shadingMode                   = ShadingModeFast;
    m_HALTagsData.statisticsHotPixelMapMode     = StatisticsHotPixelMapModeOff;
    m_HALTagsData.statisticsLensShadingMapMode  = StatisticsLensShadingMapModeOff;
    m_HALTagsData.tonemapCurves.tonemapMode     = TonemapModeFast;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetMetadataTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::GetMetadataTags(
    ISPInputData* pModuleInput)
{
    VOID*      pTagsData[NumBPSMetadataTags] = {0};
    UINT       dataIndex                     = 0;
    UINT       metaTag                       = 0;
    CamxResult result                        = CamxResultSuccess;

    GetDataList(BPSMetadataTags, pTagsData, BPSMetadataTagReqOffset, NumBPSMetadataTags);

    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->blackLevelLock = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }

    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->colorCorrectionGains = *(static_cast<ColorCorrectionGain*>(pTagsData[dataIndex++]));
    }

    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->colorCorrectionMode = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }

    if (NULL != pTagsData[dataIndex])
    {
        Utils::Memcpy(&pModuleInput->pHALTagsData->colorCorrectionTransform,
            pTagsData[dataIndex++], sizeof(pModuleInput->pHALTagsData->colorCorrectionTransform));
    }

    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->controlAEMode = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->controlAWBMode = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->controlMode = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->controlPostRawSensitivityBoost = *(static_cast<INT32*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->hotPixelMode = *(static_cast<HotPixelModeValues*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->noiseReductionMode = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->shadingMode = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->statisticsHotPixelMapMode = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->statisticsLensShadingMapMode = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->tonemapCurves.tonemapMode = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }

    if ((BPSProfileId::BPSProfileIdHNR         == m_instanceProperty.profileId) &&
        ((BPSProcessingType::BPSProcessingMFSR == m_instanceProperty.processingType)))
    {
        IntermediateDimensions* pIntermediateDimension = NULL;
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.intermediateDimension",
                                                          "IntermediateDimension",
                                                          &metaTag);
        if (CamxResultSuccess == result)
        {
            static const UINT PropertiesIPE[]     = { metaTag | InputMetadataSectionMask };
            const UINT        length              = CAMX_ARRAY_SIZE(PropertiesIPE);
            VOID*             pData[length]       = { 0 };
            UINT64            pDataOffset[length] = { 0 };

            GetDataList(PropertiesIPE, pData, pDataOffset, length);
            if (NULL != pData[0])
            {
                pIntermediateDimension = reinterpret_cast<IntermediateDimensions*>(pData[0]);
            }
        }

        if (NULL != pIntermediateDimension)
        {
            m_curIntermediateDimension.width  = pIntermediateDimension->width;
            m_curIntermediateDimension.height = pIntermediateDimension->height;
            m_curIntermediateDimension.ratio  = pIntermediateDimension->ratio;

            CAMX_LOG_INFO(CamxLogGroupBPS, "BPS:%d intermediate width=%d height=%d ratio=%f",
                           InstanceID(),
                           m_curIntermediateDimension.width,
                           m_curIntermediateDimension.height,
                           m_curIntermediateDimension.ratio);
        }
        else
        {
            result = CamxResultEResource;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "BPS:%d Error in getting intermediate dimension slot", InstanceID());
        }
    }

    GetMetadataContrastLevel(pModuleInput->pHALTagsData);
    GetMetadataTonemapCurve(pModuleInput->pHALTagsData);

    if (TRUE == IsTMC12Enalbed())
    {
        GetMetadataTMC12(pModuleInput);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::PostMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::PostMetadata(
    const ISPInputData* pInputData)
{
    CamxResult  result                               = CamxResultSuccess;
    const VOID* ppData[NumBPSMetadataOutputTags]     = { 0 };
    UINT        pDataCount[NumBPSMetadataOutputTags] = { 0 };

    const VOID* ppPostLSCData[NumBPSMetadataPostLSCOutputTags]    = { 0 };
    UINT        pPostLSCDataCount[NumBPSMetadataPostLSCOutputTags] = { 0 };

    UINT        index                                = 0;
    UINT32      metaTag                              = 0;
    FLOAT       dynamicBlackLevel[ISPChannelMax];

    if (TRUE == m_BPSPathEnabled[BPSOutputPortStatsBG])
    {
        PropertyISPAWBBGStats  AWBBGStatsProperty = {};
        Utils::Memcpy(&AWBBGStatsProperty.statsConfig,
                      &pInputData->pCalculatedData->metadata.AWBBGStatsConfig,
                      sizeof(ISPAWBBGStatsConfig));

        // HDRBE config should only be published in the BPS realtime scenario
        // as there is no IFE support. Config published is the one provided
        // by the AWB.
        static const UINT ISPAWBBGConfigProps[] = { PropertyIDISPAWBBGConfig, PropertyIDISPHDRBEConfig};
        const VOID*       pISPAWBBGConfigOutputData[CAMX_ARRAY_SIZE(ISPAWBBGConfigProps)] =
        {
            &AWBBGStatsProperty,
            &AWBBGStatsProperty
        };
        UINT              pISPAWBBGConfigDataCount[CAMX_ARRAY_SIZE(ISPAWBBGConfigProps)]  =
        {
            sizeof(AWBBGStatsProperty),
            sizeof(AWBBGStatsProperty)
        };

        // Publish HDRBE config in BPS realtime camera scenario only.
        // Care should be taken if additional properties are added so
        // that HDRBE is always the last property published in this list
        // to preserve its conditional enable logic.
        UINT ISPAWBBGConfigPropsSize = (TRUE == IsCameraRunningOnBPS())
            ? CAMX_ARRAY_SIZE(ISPAWBBGConfigProps) : CAMX_ARRAY_SIZE(ISPAWBBGConfigProps) - 1;

        WriteDataList(ISPAWBBGConfigProps, pISPAWBBGConfigOutputData,
                      pISPAWBBGConfigDataCount, ISPAWBBGConfigPropsSize);
    }

    if (TRUE == m_BPSPathEnabled[BPSOutputPortStatsHDRBHist])
    {
        // HDR BHist stats config data
        PropertyISPHDRBHistStats HDRBHistStatsProperty = {};
        Utils::Memcpy(&HDRBHistStatsProperty.statsConfig,
                      &pInputData->pCalculatedData->metadata.HDRBHistStatsConfig,
                      sizeof(ISPHDRBHistStatsConfig));

        static const UINT HDRBHistStatsConfigProps[] =
        {
            PropertyIDISPHDRBHistConfig
        };

        const VOID* pHDRBHistStatsConfigData[CAMX_ARRAY_SIZE(HDRBHistStatsConfigProps)] =
        {
            &HDRBHistStatsProperty
        };

        UINT pHDRBHistStatsConfigDataCount[CAMX_ARRAY_SIZE(HDRBHistStatsConfigProps)] =
        {
            sizeof(HDRBHistStatsProperty)
        };

        WriteDataList(HDRBHistStatsConfigProps, pHDRBHistStatsConfigData,
                      pHDRBHistStatsConfigDataCount, CAMX_ARRAY_SIZE(HDRBHistStatsConfigProps));
    }

    /**
     * Post ADRC/Gamma Metadata
     * During the post filter second stage, the BPS will run HNR only.
     * the Gamma/ADRC structure will not be filed. So, We skip the Gamma/ADRC info publishing.
     * and IPE of post filter second stage will use the ADRC/Gamma info published by
     * the Post filter first stage.
     */
    if ((BPSProfileId::BPSProfileIdHNR == m_instanceProperty.profileId) &&
        ((BPSProcessingType::BPSProcessingMFNR == m_instanceProperty.processingType) ||
         (BPSProcessingType::BPSProcessingMFSR == m_instanceProperty.processingType)))
    {
        CAMX_LOG_INFO(CamxLogGroupBPS, "Disable the BPS ADRC/Gamma Publish for MFNR/MFSR post filter second stage");
    }
    else
    {
        // Post PerFrame metadata tags
        /* Post Metadata Updated before LSC modules*/
        if ((BPSProcessingSection::BPSAll == m_instanceProcessingSection) ||
            ((BPSProcessingSection::BPSLSCOut == m_instanceProcessingSection) &&
            (BPSProfileId::BPSProfileIdIdealRawOutput == m_instanceProperty.profileId)))
        {
            pDataCount[index] = 1;
            ppData[index]     = &pInputData->pCalculatedData->blackLevelLock;
            index++;

            pDataCount[index] = 1;
            ppData[index]     = &pInputData->pCalculatedData->controlPostRawSensitivityBoost;
            index++;

            pDataCount[index] = 1;
            ppData[index]     = &pInputData->pCalculatedData->noiseReductionMode;
            index++;

            pDataCount[index] = 1;
            ppData[index]     = &pInputData->pCalculatedData->hotPixelMode;
            index++;

            pDataCount[index] = 1;
            ppData[index]     = &pInputData->pCalculatedData->lensShadingInfo.shadingMode;
            index++;

            pDataCount[index] = 1;
            ppData[index]     = &pInputData->pCalculatedData->lensShadingInfo.lensShadingMapMode;
            index++;

            pDataCount[index] = 2;
            ppData[index]     = &pInputData->pCalculatedData->lensShadingInfo.lensShadingMapSize;
            index++;

            if (StatisticsLensShadingMapModeOn == pInputData->pCalculatedData->lensShadingInfo.lensShadingMapMode)
            {
                pDataCount[index] = 4 * MESH_ROLLOFF_SIZE;
                ppData[index]     = pInputData->pCalculatedData->lensShadingInfo.lensShadingMap;
                index++;
            }
            else
            {
                pDataCount[index] = 1;
                ppData[index]     = NULL;
                index++;
            }


            for (UINT32 channel = 0; channel < ISPChannelMax; channel++)
            {
                dynamicBlackLevel[channel] =
                    static_cast<FLOAT>(((pInputData->pCalculatedMetadata->BLSblackLevelOffset +
                        pInputData->pCalculatedMetadata->linearizationAppliedBlackLevel[channel]) >>
                        (IFEPipelineBitWidth - pInputData->sensorBitWidth)));
            }

            pDataCount[index] = 4;
            ppData[index]     = dynamicBlackLevel;
            index++;

            pDataCount[index] = 1;
            ppData[index]     = &pInputData->pHALTagsData->statisticsHotPixelMapMode;
            index++;

            WriteDataList(BPSMetadataOutputTags, ppData, pDataCount, NumBPSMetadataOutputTags);
        }

        // Post PerFrame metadata tags
        /* Post Metadata Updated After LSC modules*/
        if ((BPSProcessingSection::BPSAll == m_instanceProcessingSection) ||
            ((BPSProcessingSection::BPSPostLSC == m_instanceProcessingSection) &&
            (BPSProfileId::BPSProfileIdIdealRawInput == m_instanceProperty.profileId)))
        {
            index = 0;

            pPostLSCDataCount[index] = 4;
            ppPostLSCData[index]     = &pInputData->pCalculatedData->colorCorrectionGains;
            index++;

            pPostLSCDataCount[index] = 1;
            ppPostLSCData[index]     = &pInputData->pHALTagsData->tonemapCurves.tonemapMode;
            index++;

            pPostLSCDataCount[index] = 1;
            ppPostLSCData[index]     = &pInputData->pCalculatedData->IPEGamma15PreCalculationOutput;
            index++;

            WriteDataList(BPSMetadataPostLSCOutputTags, ppPostLSCData, pPostLSCDataCount, NumBPSMetadataPostLSCOutputTags);
            // publish the metadata: PropertyIDBPSADRCInfoOutput
            PropertyISPADRCInfo adrcInfo;

            if (NULL != pInputData->triggerData.pADRCData)
            {
                adrcInfo.enable    = pInputData->triggerData.pADRCData->enable;
                adrcInfo.version   = pInputData->triggerData.pADRCData->version;
                adrcInfo.gtmEnable = pInputData->triggerData.pADRCData->gtmEnable;
                adrcInfo.ltmEnable = pInputData->triggerData.pADRCData->ltmEnable;

                if (SWTMCVersion::TMC10 == adrcInfo.version)
                {
                    Utils::Memcpy(&adrcInfo.kneePoints.KneePointsTMC10.kneeX,
                        pInputData->triggerData.pADRCData->kneePoints.KneePointsTMC10.kneeX,
                        sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC10);
                    Utils::Memcpy(&adrcInfo.kneePoints.KneePointsTMC10.kneeY,
                        pInputData->triggerData.pADRCData->kneePoints.KneePointsTMC10.kneeY,
                        sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC10);
                }
                else if (SWTMCVersion::TMC11 == adrcInfo.version)
                {
                    Utils::Memcpy(&adrcInfo.kneePoints.KneePointsTMC11.kneeX,
                        pInputData->triggerData.pADRCData->kneePoints.KneePointsTMC11.kneeX,
                        sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC11);
                    Utils::Memcpy(&adrcInfo.kneePoints.KneePointsTMC11.kneeY,
                        pInputData->triggerData.pADRCData->kneePoints.KneePointsTMC11.kneeY,
                        sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC11);
                }
                else if (SWTMCVersion::TMC12 == adrcInfo.version)
                {
                    Utils::Memcpy(&adrcInfo.kneePoints.KneePointsTMC12.kneeX,
                        pInputData->triggerData.pADRCData->kneePoints.KneePointsTMC12.kneeX,
                        sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC12);
                    Utils::Memcpy(&adrcInfo.kneePoints.KneePointsTMC12.kneeY,
                        pInputData->triggerData.pADRCData->kneePoints.KneePointsTMC12.kneeY,
                        sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC12);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "bad adrcInfo version: %d", adrcInfo.version);
                    result = CamxResultEInvalidArg;
                }

                adrcInfo.drcGainDark   = pInputData->triggerData.pADRCData->drcGainDark;
                adrcInfo.ltmPercentage = pInputData->triggerData.pADRCData->ltmPercentage;
                adrcInfo.gtmPercentage = pInputData->triggerData.pADRCData->gtmPercentage;

                Utils::Memcpy(&adrcInfo.coefficient,
                    pInputData->triggerData.pADRCData->coefficient,
                    sizeof(FLOAT) * MAX_ADRC_LUT_COEF_SIZE);

                Utils::Memcpy(&adrcInfo.pchipCoeffficient,
                    pInputData->triggerData.pADRCData->pchipCoeffficient,
                    sizeof(FLOAT) * MAX_ADRC_LUT_PCHIP_COEF_SIZE);
                Utils::Memcpy(&adrcInfo.contrastEnhanceCurve,
                    pInputData->triggerData.pADRCData->contrastEnhanceCurve,
                    sizeof(FLOAT) * MAX_ADRC_CONTRAST_CURVE);

                adrcInfo.curveModel       = pInputData->triggerData.pADRCData->curveModel;
                adrcInfo.contrastHEBright = pInputData->triggerData.pADRCData->contrastHEBright;
                adrcInfo.contrastHEDark   = pInputData->triggerData.pADRCData->contrastHEDark;
            }
            else
            {
                Utils::Memset(&adrcInfo, 0, sizeof(adrcInfo));
            }

            if (CamxResultSuccess == result)
            {
                static const UINT tADRCProps[]                                = { PropertyIDBPSADRCInfoOutput };
                const VOID*       pADRCData[CAMX_ARRAY_SIZE(tADRCProps)]      = { &adrcInfo };
                UINT              pADRCDataCount[CAMX_ARRAY_SIZE(tADRCProps)] = { sizeof(adrcInfo) };

                result = WriteDataList(tADRCProps, pADRCData, pADRCDataCount, CAMX_ARRAY_SIZE(tADRCProps));
            }

            // publish the metadata: PropertyIDBPSGammaOutput
            GammaInfo greenGamma = { 0 };
            Utils::Memcpy(&greenGamma.gammaG,
                pInputData->pCalculatedData->gammaOutput.gammaG,
                NumberOfGammaEntriesPerLUT * sizeof(UINT32));
            greenGamma.isGammaValid = pInputData->pCalculatedData->gammaOutput.isGammaValid;

            if (CamxResultSuccess == result)
            {
                static const UINT gammaValidProps[]                                      = { PropertyIDBPSGammaOutput };
                const VOID*       pGammaValidData[CAMX_ARRAY_SIZE(gammaValidProps)]      = { &greenGamma };
                UINT              pGammaValidDataCount[CAMX_ARRAY_SIZE(gammaValidProps)] = { sizeof(greenGamma) };

                result = WriteDataList(gammaValidProps,
                                       pGammaValidData,
                                       pGammaValidDataCount,
                                       CAMX_ARRAY_SIZE(gammaValidProps));
            }

            result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.gammainfo",
                "GammaInfo",
                &metaTag);
            if (CamxResultSuccess == result)
            {
                static const UINT gammaInfoTag[]        = { metaTag };
                const VOID*       pGammaInfoData[1]     = { &greenGamma };
                UINT              length                = CAMX_ARRAY_SIZE(gammaInfoTag);
                UINT              pGammaInfoDataCount[] = { sizeof(greenGamma) };

                WriteDataList(gammaInfoTag, pGammaInfoData, pGammaInfoDataCount, length);
            }
        }

    }

    // Read and post dual camera metadata if available
    UINT32 tag = 0;
    if (CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainfo", "MultiCameraIds", &tag))
    {
        UINT              tagReadInput[]                              = { tag | InputMetadataSectionMask };
        static const UINT TagLength                                   = CAMX_ARRAY_SIZE(tagReadInput);
        VOID*             pMultiCamIdGetData[TagLength]               = { 0 };
        UINT64            multiCamIdGetDataOffset[TagLength]          = { 0 };
        if (CamxResultSuccess == GetDataList(tagReadInput, pMultiCamIdGetData, multiCamIdGetDataOffset, 1))
        {
            const UINT pMultiCamIDDataCount[TagLength] = { sizeof(MultiCameraIds) };
            const VOID* pMultiCamIDData[TagLength]     = { pMultiCamIdGetData[0] };
            WriteDataList(&tag, pMultiCamIDData, pMultiCamIDDataCount, 1);
        }
    }

    if ((CamxResultSuccess == result) &&
        (TRUE == GetStaticSettings()->enableStreamCropZoom))
    {
        // Loop over all ports and publish their crop information.
        // As the BPS does not apply crop by itself, this is
        // direct copy of the input crop, with residual (fov) crop
        // updated to match the actual output format dimensions.
        StreamCropInfo parentCropInfo;
        UINT           tagId;

        result = VendorTagManager::QueryVendorTagLocation("com.qti.camera.streamCropInfo",
                                                          "StreamCropInfo", &tagId);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get port vendor tag location");
        }

        if (CamxResultSuccess == result)
        {
            static const UINT DataTypeId[]                       = { tagId };
            VOID*       pOutputData[CAMX_ARRAY_SIZE(DataTypeId)] = { &parentCropInfo };
            UINT64      pDataOffset[CAMX_ARRAY_SIZE(DataTypeId)] = { 0 };

            result = GetPSDataList(BPSInputPort, DataTypeId, pOutputData, pDataOffset, CAMX_ARRAY_SIZE(DataTypeId));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to read port metadata information.");
            }
        }

        if (CamxResultSuccess == result)
        {
            for (UINT i = BPSMaxInput; i < BPSMaxOutput + BPSMaxInput; i++)
            {
                // Publish port specific metadata for all non-stats output ports
                if (TRUE == m_BPSPathEnabled[i] && (BPSOutputPortStatsBG != i) && (BPSOutputPortStatsHDRBHist != i))
                {
                    StreamCropInfo portCropInfo = {};

                    static const UINT DataTypeId[]                             = { tagId };
                    const VOID*       pOutputData[CAMX_ARRAY_SIZE(DataTypeId)] = { &portCropInfo };
                    UINT              pDataSize[CAMX_ARRAY_SIZE(DataTypeId)]   = { sizeof(StreamCropInfo) };

                    // BPS does not apply any crop of the input image so just pass the input crop
                    // information to consumers.
                    portCropInfo.fov.left   = parentCropInfo.fov.left;
                    portCropInfo.fov.top    = parentCropInfo.fov.top;
                    portCropInfo.fov.width  = parentCropInfo.fov.width;
                    portCropInfo.fov.height = parentCropInfo.fov.height;

                    const ImageFormat* pImageFormat = GetOutputPortImageFormat(OutputPortIndex(i));

                    // No residual crop is expected when using the BPS
                    if (NULL != pImageFormat)
                    {
                        portCropInfo.crop.left   = 0;
                        portCropInfo.crop.top    = 0;
                        portCropInfo.crop.width  = pImageFormat->width;
                        portCropInfo.crop.height = pImageFormat->height;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to retrieve Image Format for port %d", i);
                    }

                    result = WritePSDataList(i, DataTypeId, pOutputData, pDataSize,
                                             CAMX_ARRAY_SIZE(DataTypeId));
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "WritePortDataList API failed for port %d, port idx %d",
                                i,
                                OutputPortIndex(i));
                    }
                }
            }
        }

        PublishPSData(tagId, NULL);
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == m_publishLDCGridData)
        {
            PublishICAGridTransform(m_pWarpGridData);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetOEMStatsConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::GetOEMStatsConfig(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    UINT32 metadataAECFrameControl = 0;
    UINT32 metadataAWBFrameControl = 0;
    UINT32 metadataAECStatsControl = 0;
    UINT32 metadataAWBStatsControl = 0;
    UINT32 metadataAFStatsControl  = 0;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AECFrameControl",
        &metadataAECFrameControl);
    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: AECFrameControl");

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBFrameControl",
        &metadataAWBFrameControl);
    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: AWBFrameControl");

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AECStatsControl",
        &metadataAECStatsControl);
    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: AECStatsControl");

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBStatsControl",
        &metadataAWBStatsControl);
    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: AWBStatsControl");

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AFStatsControl",
        &metadataAFStatsControl);
    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: AFStatsControl");

    static const UINT vendorTagsControl3A[] =
    {
        metadataAECStatsControl | InputMetadataSectionMask,
        metadataAWBStatsControl | InputMetadataSectionMask,
        metadataAFStatsControl  | InputMetadataSectionMask,
        metadataAECFrameControl | InputMetadataSectionMask,
        metadataAWBFrameControl | InputMetadataSectionMask,
    };

    const SIZE_T numTags                            = CAMX_ARRAY_SIZE(vendorTagsControl3A);
    VOID*        pVendorTagsControl3A[numTags]      = { 0 };
    UINT64       vendorTagsControl3AOffset[numTags] = { 0 };

    GetDataList(vendorTagsControl3A, pVendorTagsControl3A, vendorTagsControl3AOffset, numTags);

    // Pointers in pVendorTagsControl3A[] guaranteed to be non-NULL by GetDataList() for InputMetadataSectionMask
    Utils::Memcpy(pInputData->pAECStatsUpdateData, pVendorTagsControl3A[BPSVendorTagAECStats], sizeof(AECStatsControl));
    Utils::Memcpy(pInputData->pAWBStatsUpdateData, pVendorTagsControl3A[BPSVendorTagAWBStats], sizeof(AWBStatsControl));
    Utils::Memcpy(pInputData->pAFStatsUpdateData, pVendorTagsControl3A[BPSVendorTagAFStats], sizeof(AFStatsControl));
    Utils::Memcpy(pInputData->pAECUpdateData, pVendorTagsControl3A[BPSVendorTagAECFrame], sizeof(AECFrameControl));
    Utils::Memcpy(pInputData->pAWBUpdateData, pVendorTagsControl3A[BPSVendorTagAWBFrame], sizeof(AWBFrameControl));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::UpdateClock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::UpdateClock(
    ExecuteProcessRequestData*   pExecuteProcessRequestData,
    BPSClockAndBandwidth*        pClockAndBandwidth)
{
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT64                  requestId        = pNodeRequestData->pCaptureRequest->requestId;
    UINT                    frameCycles;
    UINT                    FPS          = DefaultFPS;
    UINT64                  budgetNS;
    FLOAT                   overHead     = BPSClockOverhead;
    FLOAT                   efficiency   = BPSClockEfficiency;
    FLOAT                   budget;

    // Framecycles calculation considers Number of Pixels processed in the current frame, Overhead and Efficiency
    if ( 0 != m_FPS)
    {
        FPS = m_FPS;
    }

    frameCycles = pClockAndBandwidth->frameCycles;
    frameCycles = static_cast<UINT>((frameCycles * overHead) / efficiency);

    // Budget is the Max duration of current frame to process
    budget   = 1.0f / FPS;
    budgetNS = static_cast<UINT64>(budget * NanoSecondMult);

    pClockAndBandwidth->budgetNS     = budgetNS;
    pClockAndBandwidth->frameCycles  = frameCycles;
    pClockAndBandwidth->realtimeFlag = IsRealTime();

    CAMX_LOG_VERBOSE(CamxLogGroupPower, "[%s][%llu] FPS = %d budget = %lf budgetNS = %lld fc = %d",
                     NodeIdentifierString(), requestId, FPS, budget, budgetNS, frameCycles);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::CalculateBPSRdBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::CalculateBPSRdBandwidth(
    PerRequestActivePorts*  pPerRequestPorts,
    BPSClockAndBandwidth*   pClockAndBandwidth)
{
    UINT   srcWidth  = 0;
    UINT   srcHeight = 0;
    FLOAT  bppSrc    = 0;
    FLOAT  overhead  = BPSBandwidthOverhead;
    FLOAT  swMargin  = BPSSwMargin;
    UINT   FPS       = pClockAndBandwidth->FPS;
    UINT64 readBandwidthPass0;
    UINT64 readBandwidthPass1;

    pClockAndBandwidth->readBW.unCompressedBW = 0;
    pClockAndBandwidth->readBW.compressedBW   = 0;

    for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
    {
        PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];

        if (pInputPort->portId == CSLBPSInputPortImage)
        {
            const ImageFormat* pImageFormat = GetInputPortImageFormat(InputPortIndex(pInputPort->portId));

            if (NULL != pImageFormat)
            {
                srcWidth    = pImageFormat->width;
                srcHeight   = pImageFormat->height;
                bppSrc      = ImageFormatUtils::GetBytesPerPixel(pImageFormat);
            }

            break;
        }
    }

    if ((0 == srcWidth) ||
        (0 == srcHeight))
    {
        srcWidth  = m_fullInputWidth;
        srcHeight = m_fullInputHeight;
    }

    // Pass0_RdAB = (  (SrcWidth * SrcHeight * SrcBPP * Ovhd ) ) * fps
    readBandwidthPass0 = static_cast<UINT64>((srcWidth * srcHeight * bppSrc * overhead ) * FPS);

    // Pass1_RdAB = (  (SrcWidth/4/2 * SrcHeight/4/2 * 8 * Ovhd ) )  *  fps
    readBandwidthPass1 = static_cast<UINT64>(((srcWidth / 4.0f / 2.0f) * (srcHeight / 4.0f / 2.0f) * 8.0f * overhead) * FPS);

    pClockAndBandwidth->readBW.unCompressedBW = static_cast<UINT64>((readBandwidthPass0 + readBandwidthPass1 ) * swMargin);
    pClockAndBandwidth->readBW.compressedBW   = pClockAndBandwidth->readBW.unCompressedBW;

    CAMX_LOG_VERBOSE(CamxLogGroupPower,
                     "BW: sw = %d sh = %d bppSrc = %f overhead = %f FPS = %d "
                     "Pass0: %llu Pass1: %llu unCompressedBW = %llu, compressedBW = %llu",
                     srcWidth, srcHeight, bppSrc, overhead, FPS, readBandwidthPass0, readBandwidthPass1,
                     pClockAndBandwidth->readBW.unCompressedBW, pClockAndBandwidth->readBW.compressedBW);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::CalculateBPSWrBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::CalculateBPSWrBandwidth(
    PerRequestActivePorts*  pPerRequestPorts,
    BPSClockAndBandwidth*   pClockAndBandwidth)
{
    UINT   dstWidth   = 0;
    UINT   dstHeight  = 0;
    FLOAT  bppDst     = BPSBpp8Bit;
    FLOAT  overhead   = BPSBandwidthOverhead;
    FLOAT  swMargin   = BPSSwMargin;
    FLOAT  BPSUbwcCr  = BPSUBWCWrCompressionRatio;
    UINT   FPS        = pClockAndBandwidth->FPS;
    BOOL   UBWCEnable = FALSE;
    UINT64 writeBandwidthPass0;
    UINT64 writeBandwidthPass1;

    pClockAndBandwidth->writeBW.unCompressedBW = 0;
    pClockAndBandwidth->writeBW.compressedBW   = 0;

    for (UINT i = 0; i < pPerRequestPorts->numOutputPorts; i++)
    {
        PerRequestOutputPortInfo* pOutputPort = &pPerRequestPorts->pOutputPorts[i];

        if (pOutputPort->portId == CSLBPSOutputPortIdPDIImageFull)
        {
            const ImageFormat* pImageFormat = GetOutputPortImageFormat(OutputPortIndex(pOutputPort->portId));

            if (NULL != pImageFormat)
            {
                dstWidth   = pImageFormat->width;
                dstHeight  = pImageFormat->height;
                UBWCEnable = ImageFormatUtils::IsUBWC(pImageFormat->format);

                if (TRUE == ImageFormatUtils::Is10BitFormat(pImageFormat->format))
                {
                    bppDst = BPSBpp10Bit;
                }
            }
            break;
        }
    }

    if ((0 == dstWidth) ||
        (0 == dstHeight))
    {
        dstWidth  = m_fullInputWidth;
        dstHeight = m_fullInputHeight;
    }

    // Pass0_WrAB = ((DstWidth * DstHeight * DstBPP * Ovhd ) / BPS_UBWC_WrCr + (DstWidth/4/2 * DstHeight/4/2 * 8 ) )  *  fps
    writeBandwidthPass0 = static_cast<UINT64>(
                          (((dstWidth * dstHeight * bppDst * overhead) * FPS) / BPSUbwcCr) +
                          (((dstWidth / 4.0f / 2.0f) * (dstHeight / 4.0f / 2.0f) * 8.0f) * FPS));

    // Pass1_WrAB = ((DstWidth/16/2 * DstHeight/16/2 * 8 )  + (DstWidth/64/2 * DstHeight/64/2 * 8 ) )  *  fps
    writeBandwidthPass1 = static_cast<UINT64>(
                          (((dstWidth / 16.0f / 2.0f) * (dstHeight / 16.0f / 2.0f) * 8.0f ) * FPS) +
                          (((dstWidth / 64.0f / 2.0f) * (dstHeight / 64.0f / 2.0f) * 8.0f ) * FPS));

    pClockAndBandwidth->writeBW.unCompressedBW = static_cast<UINT64>((writeBandwidthPass0 + writeBandwidthPass1) * swMargin);

    CAMX_LOG_VERBOSE(CamxLogGroupPower, "uncompressed: dstw = %d sh = %d bppDst = %f overhead = %f FPS = %d BPSUbwcCr = %f"
                     "Pass0: %llu Pass1: %llu BW = %llu",
                     dstWidth, dstHeight, bppDst, overhead, FPS, BPSUbwcCr,
                     writeBandwidthPass0, writeBandwidthPass1, pClockAndBandwidth->writeBW.unCompressedBW);

    if (TRUE == UBWCEnable)
    {
        if (BPSBpp10Bit == bppDst)
        {
            BPSUbwcCr = BPSUBWCWrCompressionRatio10Bit;
        }
        else
        {
            BPSUbwcCr = BPSUBWCWrCompressionRatio8Bit;
        }

        writeBandwidthPass0 = static_cast<UINT64>(
                              (((dstWidth * dstHeight * bppDst * overhead) * FPS) / BPSUbwcCr) +
                              (((dstWidth / 4.0f / 2.0f) * (dstHeight / 4.0f / 2.0f) * 8.0f) * FPS));

        pClockAndBandwidth->writeBW.compressedBW = static_cast<UINT64>((writeBandwidthPass0 + writeBandwidthPass1) * swMargin);

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "compressed: dstw = %d sh = %d bppDst = %f overhead = %f FPS = %d BPSUbwcCr = %f "
                         "Pass0: %llu Pass1: %llu BW = %d",
                         dstWidth, dstHeight, bppDst, overhead, FPS, BPSUbwcCr,
                         writeBandwidthPass0, writeBandwidthPass1, pClockAndBandwidth->writeBW.compressedBW);
    }
    else
    {
        pClockAndBandwidth->writeBW.compressedBW = pClockAndBandwidth->writeBW.unCompressedBW;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPower, "Wr: cbw = %llu bw = %llu", pClockAndBandwidth->writeBW.compressedBW,
        pClockAndBandwidth->writeBW.unCompressedBW);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::UpdateBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::UpdateBandwidth(
    ExecuteProcessRequestData*   pExecuteProcessRequestData,
    BPSClockAndBandwidth*        pClockAndBandwidth)
{
    PerRequestActivePorts*  pPerRequestPorts = pExecuteProcessRequestData->pEnabledPortsInfo;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT64                  requestId        = pNodeRequestData->pCaptureRequest->requestId;
    UINT                    FPS              = DefaultFPS;

    if (0 != m_FPS)
    {
        FPS = m_FPS;
    }

    pClockAndBandwidth->FPS = FPS;

    CalculateBPSRdBandwidth(pPerRequestPorts, pClockAndBandwidth);
    CalculateBPSWrBandwidth(pPerRequestPorts, pClockAndBandwidth);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::CheckAndUpdateClockBW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::CheckAndUpdateClockBW(
    CmdBuffer*                   pCmdBuffer,
    ExecuteProcessRequestData*   pExecuteProcessRequestData,
    BPSClockAndBandwidth*        pClockAndBandwidth)
{
    PerRequestActivePorts*  pPerRequestPorts = pExecuteProcessRequestData->pEnabledPortsInfo;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT64                  requestId = pNodeRequestData->pCaptureRequest->requestId;

    UpdateClock(pExecuteProcessRequestData, pClockAndBandwidth);
    UpdateBandwidth(pExecuteProcessRequestData, pClockAndBandwidth);

    if (CSLICPGenericBlobCmdBufferClkV2 == m_pPipeline->GetICPClockAndBandWidthConfigurationVersion())
    {
        CSLICPClockBandwidthRequestV2* pICPClockBandwidthRequestV2 =
            reinterpret_cast<CSLICPClockBandwidthRequestV2*>(m_pGenericClockAndBandwidthData);
        pICPClockBandwidthRequestV2->budgetNS     = pClockAndBandwidth->budgetNS;
        pICPClockBandwidthRequestV2->frameCycles  = pClockAndBandwidth->frameCycles;
        pICPClockBandwidthRequestV2->realtimeFlag = pClockAndBandwidth->realtimeFlag;

        // One for Read one for Write
        pICPClockBandwidthRequestV2->numPaths = 2;

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "[%s][%llu] : fps=%d, budget= %lu, frameCycles=%u, realtimeFlag=%d, paths %d",
                         NodeIdentifierString(),
                         requestId,
                         pClockAndBandwidth->FPS,
                         pICPClockBandwidthRequestV2->budgetNS,
                         pICPClockBandwidthRequestV2->frameCycles,
                         pICPClockBandwidthRequestV2->realtimeFlag,
                         pICPClockBandwidthRequestV2->numPaths);

        CSLAXIperPathBWVote* pBWPathPerVote = &pICPClockBandwidthRequestV2->outputPathBWInfo[0];
        pBWPathPerVote->transactionType     = CSLAXITransactionRead;
        pBWPathPerVote->pathDataType        = CSLAXIPathDataALL;
        pBWPathPerVote->camnocBW            = pClockAndBandwidth->readBW.unCompressedBW;
        pBWPathPerVote->mnocABBW            = pClockAndBandwidth->readBW.compressedBW;
        pBWPathPerVote->mnocIBBW            = pClockAndBandwidth->readBW.compressedBW;

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "idx0: [%s][%llu] transactionType %d,path %d,camnocBW:[%lu], mnocAB-IB:[%lu, %lu]",
                         NodeIdentifierString(),
                         requestId,
                         pBWPathPerVote->transactionType,
                         pBWPathPerVote->pathDataType,
                         pBWPathPerVote->camnocBW,
                         pBWPathPerVote->mnocABBW,
                         pBWPathPerVote->mnocIBBW);

        pBWPathPerVote                      = &pICPClockBandwidthRequestV2->outputPathBWInfo[1];
        pBWPathPerVote->transactionType     = CSLAXITransactionWrite;
        pBWPathPerVote->pathDataType        = CSLAXIPathDataALL;
        pBWPathPerVote->camnocBW            = pClockAndBandwidth->writeBW.unCompressedBW;
        pBWPathPerVote->mnocABBW            = pClockAndBandwidth->writeBW.compressedBW;
        pBWPathPerVote->mnocIBBW            = pClockAndBandwidth->writeBW.compressedBW;

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "idx1: [%s][%llu] transactionType %d,path %d,camnocBW:[%lu], mnocAB-IB:[%lu, %lu]",
                         NodeIdentifierString(),
                         requestId,
                         pBWPathPerVote->transactionType,
                         pBWPathPerVote->pathDataType,
                         pBWPathPerVote->camnocBW,
                         pBWPathPerVote->mnocABBW,
                         pBWPathPerVote->mnocIBBW);


        if ((pICPClockBandwidthRequestV2->outputPathBWInfo[0].camnocBW == 0) || // camnoc read zero
            (pICPClockBandwidthRequestV2->outputPathBWInfo[1].camnocBW == 0) || // camnoc write zero
            (pICPClockBandwidthRequestV2->outputPathBWInfo[0].mnocABBW == 0) || // mnoc read zero
            (pICPClockBandwidthRequestV2->outputPathBWInfo[1].mnocABBW == 0))   // mnoc write zero
        {
            CAMX_LOG_ERROR(CamxLogGroupPower, "%s: req[%llu]: CAMNOC / MNOC BW zero"
                           "camnoc  read %lu write %lu, mnoc read %lu, write %lu",
                           NodeIdentifierString(), requestId,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[0].camnocBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[1].camnocBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[0].mnocABBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[1].mnocABBW);
            OsUtils::RaiseSignalAbort();
        }

        PacketBuilder::WriteGenericBlobData(pCmdBuffer,
            CSLICPGenericBlobCmdBufferClkV2,
            (sizeof(CSLICPClockBandwidthRequestV2) +
            (sizeof(CSLAXIperPathBWVote))),
            reinterpret_cast<BYTE*>(pICPClockBandwidthRequestV2));
    }
    else
    {
        CSLICPClockBandwidthRequest* pICPClockBandwidthRequest =
            reinterpret_cast<CSLICPClockBandwidthRequest*>(m_pGenericClockAndBandwidthData);

        pICPClockBandwidthRequest->budgetNS     = pClockAndBandwidth->budgetNS;
        pICPClockBandwidthRequest->frameCycles  = pClockAndBandwidth->frameCycles;
        pICPClockBandwidthRequest->realtimeFlag = pClockAndBandwidth->realtimeFlag;

        pICPClockBandwidthRequest->unCompressedBW = pClockAndBandwidth->readBW.unCompressedBW   +
                                                    pClockAndBandwidth->writeBW.unCompressedBW;
        pICPClockBandwidthRequest->compressedBW   = pClockAndBandwidth->readBW.compressedBW     +
                                                    pClockAndBandwidth->writeBW.compressedBW;

        PacketBuilder::WriteGenericBlobData(pCmdBuffer,
            CSLICPGenericBlobCmdBufferClk,
            sizeof(CSLICPClockBandwidthRequest),
            reinterpret_cast<BYTE*>(pClockAndBandwidth));

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "[%s][%llu] : fps=%d, "
                         "Read BandWidth (uncompressed = %llu compressed = %llu), "
                         "WriteBandwidth (uncompressed = %llu compressed = %llu), "
                         "TotalBandwidth (uncompressed = %llu compressed = %llu), ",
                         NodeIdentifierString(), requestId, pClockAndBandwidth->FPS,
                         pClockAndBandwidth->readBW.unCompressedBW, pClockAndBandwidth->readBW.compressedBW,
                         pClockAndBandwidth->writeBW.unCompressedBW, pClockAndBandwidth->writeBW.compressedBW,
                         pICPClockBandwidthRequest->unCompressedBW, pICPClockBandwidthRequest->compressedBW);

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DumpDebug
///
/// @brief: This is called when firmware signal an error and UMD needs firmware dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult DumpDebug(
    BPSCmdBufferId          index,
    CmdBuffer*              pBuffer,
    UINT64                  requestId,
    UINT32                  realtime,
    BPSIntanceProperty      instanceProperty,
    const CHAR*             pPipelineName)
{
    CamxResult result = CamxResultSuccess;
    CHAR       filename[512];

    if (index == BPSCmdBufferBLMemory)
    {
        switch (index)
        {
            case BPSCmdBufferBLMemory:
                CAMX_LOG_ERROR(CamxLogGroupBPS, "dump bl buffer");
                CamX::OsUtils::SNPrintF(filename, sizeof(filename),
                    "%s/BPSBLMemoryDump_%s_%llu_realtime-%d_processTYpe_%d_profileId_%d.txt",
                    ConfigFileDirectory, pPipelineName, requestId,
                    realtime, instanceProperty.processingType, instanceProperty.profileId);

                break;
            default:
                result = CamxResultEInvalidArg;
                break;
        }
        if (CamxResultSuccess == result)
        {
            FILE* pFile = CamX::OsUtils::FOpen(filename, "wb");
            if (!pFile)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Can't open file");
                return CamxResultEFailed;
            }
            CamX::OsUtils::FWrite(pBuffer->GetHostAddr(), pBuffer->GetMaxLength(), 1, pFile);
            CamX::OsUtils::FClose(pFile);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSNode::NotifyRequestProcessingError()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::NotifyRequestProcessingError(
    NodeFenceHandlerData* pFenceHandlerData,
    UINT                  unSignaledFenceCount)
{
    CAMX_ASSERT(NULL != pFenceHandlerData);
    CSLFenceResult  fenceResult    = pFenceHandlerData->fenceResult;
    BOOL            enableDump     = ((((0x2 == m_BPSHangDumpEnable) && (CSLFenceResultFailed == fenceResult)) ||
                                      ((0x1 == m_BPSHangDumpEnable) && (0 == unSignaledFenceCount))) ? TRUE : FALSE);

    if (TRUE == enableDump)
    {
        CAMX_LOG_INFO(CamxLogGroupBPS, "notify error fence back for request %d", pFenceHandlerData->requestId);
        CmdBuffer* pBuffer = NULL;
        pBuffer =
            CheckCmdBufferWithRequest(pFenceHandlerData->requestId, m_pBPSCmdBufferManager[BPSCmdBufferBLMemory]);
        if (!pBuffer)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "cant find buffer");
            return;
        }

        DumpDebug(BPSCmdBufferBLMemory, pBuffer, pFenceHandlerData->requestId,
            IsRealTime(), m_instanceProperty, NodeIdentifierString());
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::PopulateNCLibWarpGrid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::PopulateNCLibWarpGrid(
    NcLibIcaGrid* pGrid)
{
    CamxResult   result = CamxResultSuccess;

    // Fill Default information
    if (NULL  != pGrid)
    {
        UINT32 QFactor    = Q4;

        pGrid->enable                   = FALSE;
        pGrid->reuseGridTransform       = 0;
        pGrid->transformDefinedOnWidth  = IcaVirtualDomainWidth;
        pGrid->transformDefinedOnHeight = IcaVirtualDomainHeight;
        pGrid->geometry                 = NcLibIcaGrid67x51;
        pGrid->extrapolatedCorners      = 0;

        for (UINT i = 0; i < 4 ; i++)
        {
            pGrid->gridCorners[i].x = 0.0f;
            pGrid->gridCorners[i].y = 0.0f;
        }

        for (UINT i = 0; i < (ICA30GridTransformWidth * ICA30GridTransformHeight); i++)
        {
            pGrid->grid[i].x = gridArrayX30[i] / QFactor;
            pGrid->grid[i].y = gridArrayY30[i] / QFactor;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetGeoLibDisplayOutputSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::GetGeoLibDisplayOutputSize()
{
    CamxResult result = CamxResultSuccess;
    UINT32     metaTag = 0;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.snapshotOutputDimension",
                                                      "DisplayOutputDimension",
                                                      &metaTag);
    if (CamxResultSuccess == result)
    {
        static const UINT SnapshotDimensionTags[] =
        {
            metaTag | InputMetadataSectionMask
        };

        UINT length                             = CAMX_ARRAY_SIZE(SnapshotDimensionTags);
        VOID* pData[1]                          = { 0 };
        UINT64 snapshotDimensionDataOffset[1]   = { 0 };

        GeoLibStreamOutput* pDisplayOutputDimension;

        result = GetDataList(SnapshotDimensionTags, pData, snapshotDimensionDataOffset, length);
        if (CamxResultSuccess == result)
        {
            pDisplayOutputDimension    = (static_cast<GeoLibStreamOutput*>(pData[0]));
            if (NULL == pDisplayOutputDimension     ||
                0 == pDisplayOutputDimension->width ||
                0 == pDisplayOutputDimension->height)
            {
                CAMX_LOG_INFO(CamxLogGroupBPS, "Invalid display output dimension data");
                result = CamxResultENoSuch;
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupBPS, "Display output dimension (%d, %d)",
                    pDisplayOutputDimension->width, pDisplayOutputDimension->height);
                m_displayOutSize.widthPixels = pDisplayOutputDimension->width;
                m_displayOutSize.heightLines = pDisplayOutputDimension->height;
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_INFO(CamxLogGroupBPS, "No display output output dimension");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetGeoLibStillOutputSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::GetGeoLibStillOutputSize()
{
    CamxResult result  = CamxResultSuccess;
    UINT32     metaTag = 0;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.snapshotOutputDimension",
                                                      "StillOutputDimension",
                                                      &metaTag);
    if (CamxResultSuccess == result)
    {
        static const UINT SnapshotDimensionTags[] =
        {
            metaTag |InputMetadataSectionMask
        };

        UINT length                             = CAMX_ARRAY_SIZE(SnapshotDimensionTags);
        VOID* pData[1]                          = { 0 };
        UINT64 snapshotDimensionDataOffset[1]   = { 0 };

        GeoLibStreamOutput* pStillOutputDimension;

        GetDataList(SnapshotDimensionTags, pData, snapshotDimensionDataOffset, length);

        pStillOutputDimension    = (static_cast<GeoLibStreamOutput*>(pData[0]));
        if (NULL == pStillOutputDimension)
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid snapshot output dimension data");
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "Still output dimension (%d, %d)",
                pStillOutputDimension->width, pStillOutputDimension->height);
            m_stillOutSize.widthPixels = pStillOutputDimension->width;
            m_stillOutSize.heightLines = pStillOutputDimension->height;
        }
        CAMX_LOG_INFO(CamxLogGroupBPS, "stillOutSize (%d, %d)", m_stillOutSize.widthPixels, m_stillOutSize.heightLines);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Can not get snapshot output dimension tag");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ResampleAndCopyGrid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::ResampleAndCopyGrid(
    const SensorMode* pSensorMode)
{

    CamxResult result  = CamxResultSuccess;
    INT32 ret          = 0;
    NcLibIcaGrid    warpGridDataIn;
    NcLibIcaGrid    warpGridDataOut;

    if ((TRUE == m_ICAGridOut2InEnabled) && (TRUE == m_ICAGridIn2OutEnabled))
    {
        CAMX_LOG_INFO(CamxLogGroupBPS, " %s:, LD w %d, h %d, Sen x %d, h%d Sen fp %d, fl %d",
                       NodeIdentifierString(), m_ldFullOutSize.widthPixels,
                       m_ldFullOutSize.heightLines, m_sensorSize.widthPixels,
                       m_sensorSize.heightLines, pSensorMode->cropInfo.firstPixel,
                       pSensorMode->cropInfo.firstLine);

        if ((0 != m_ldFullOutSize.widthPixels) &&
            (0 != m_ldFullOutSize.heightLines))
        {
            if ((m_sensorSize.heightLines < m_ldFullOutSize.heightLines) ||
                (m_sensorSize.widthPixels < m_ldFullOutSize.widthPixels))
            {
                NcLibWindowRegion distortedWindowRegion;
                distortedWindowRegion.fullWidth    = m_sensorSize.widthPixels;
                distortedWindowRegion.fullHeight   = m_sensorSize.heightLines;
                distortedWindowRegion.windowLeft   = pSensorMode->activeArrayCropWindow.left;
                distortedWindowRegion.windowTop    = pSensorMode->activeArrayCropWindow.top;
                distortedWindowRegion.windowWidth  = pSensorMode->activeArrayCropWindow.width;
                distortedWindowRegion.windowHeight = pSensorMode->activeArrayCropWindow.height;

                Utils::Memcpy(&warpGridDataIn, &m_ldFullInToOut, sizeof(warpGridDataIn));
                Utils::Memcpy(&warpGridDataOut, &m_ldFullOutToIn, sizeof(warpGridDataOut));

                NcLibWindowRegion undistortedWindowRegion;
                ret = NcLibCalcMaxOutputWindow(&distortedWindowRegion,
                                               &distortedWindowRegion,
                                               &m_ldFullInToOut,
                                               &undistortedWindowRegion);

                ret = NcLibResampleGrid(&m_ldFullInToOut,
                                        &undistortedWindowRegion,
                                        &distortedWindowRegion,
                                        &warpGridDataIn);

                if ((m_ldFullInToOut.transformDefinedOnWidth  < undistortedWindowRegion.fullWidth)   ||
                    (m_ldFullInToOut.transformDefinedOnHeight < undistortedWindowRegion.fullHeight))
                {
                    CAMX_LOG_WARN(CamxLogGroupBPS, "m_ldFullInToOut W:%u, H:%u < undistortedWindowRegion W:%u, H:%u",
                                  m_ldFullInToOut.transformDefinedOnWidth,
                                  m_ldFullInToOut.transformDefinedOnHeight,
                                  undistortedWindowRegion.fullWidth,
                                  undistortedWindowRegion.fullHeight);
                }

                if (0 != ret)
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Resample In 2 Out Grid failed %d", ret);
                    m_ldFullOutSize.widthPixels = 0;
                    m_ldFullOutSize.heightLines = 0;
                    result                      = CamxResultEInvalidArg;
                }
                else
                {
                    ret = NcLibResampleGrid(&m_ldFullOutToIn,
                                            &distortedWindowRegion,
                                            &undistortedWindowRegion,
                                            &warpGridDataOut);
                    if (0 != ret)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "Resample Out to In Grid failed %d", ret);
                        m_ldFullOutSize.widthPixels = 0;
                        m_ldFullOutSize.heightLines = 0;
                        result                      = CamxResultEInvalidArg;
                    }
                }

                if (CamxResultSuccess == result)
                {
                    Utils::Memcpy(&m_ldFullInToOut, &warpGridDataIn, sizeof(warpGridDataIn));
                    Utils::Memcpy(&m_ldFullOutToIn, &warpGridDataOut, sizeof(warpGridDataOut));

                    m_ldFullOutSize.widthPixels = m_sensorSize.widthPixels;
                    m_ldFullOutSize.heightLines = m_sensorSize.heightLines;
                }
            }
            else if ((m_sensorSize.widthPixels > m_ldFullOutSize.widthPixels) ||
                     (m_sensorSize.heightLines > m_ldFullOutSize.heightLines))
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "LDoutsize greater than sensorsize reset LD size,Sensor w %d,h %d,ld w %d,h %d",
                    m_sensorSize.widthPixels, m_sensorSize.heightLines,
                    m_ldFullOutSize.widthPixels, m_ldFullOutSize.heightLines);
                m_ldFullOutSize.widthPixels = 0;
                m_ldFullOutSize.heightLines = 0;
                result = CamxResultEInvalidArg;
            }
            else
            {
                // Use the same grid no resample
            }
        }
        else
        {
            m_ldFullOutSize.widthPixels = 0;
            m_ldFullOutSize.heightLines = 0;
        }
    }
    return result;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::PopulateGeoLibInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::PopulateGeoLibInput(
    CropWindow*         pCropWindow,
    RefCropWindowSize*  pRefCropWindow,
    BOOL*               pDisplayOutputExist)
{
    CamxResult result  = CamxResultSuccess;
    UINT32     metaTag = 0;

    CAMX_LOG_INFO(CamxLogGroupBPS, "================= Setup GeoLIb Input ====================");

    // Flow mode should be at this moment
    CAMX_LOG_INFO(CamxLogGroupBPS, "FlowMode=%d", m_flowMode);

    //   has been filled while getting ICA grid information
    CAMX_LOG_INFO(CamxLogGroupBPS, "LDFullOutSize (%d, %d)", m_ldFullOutSize.widthPixels, m_ldFullOutSize.heightLines);

    // Fill regOutSize
    if (CamxResultSuccess == result)
    {
        // need to get from chromatix.  <-------- need to link with chromatix
        // this field has been set in the setup device resource stage. At here, we need to check the chromatix
        // to see if there is any changes after acquire the device resoruce
        CAMX_LOG_INFO(CamxLogGroupBPS, "regOutSize (%d, %d)", m_regOutSize.widthPixels, m_regOutSize.heightLines);
    }

    // Fill Sensor Size
    if (CamxResultSuccess == result)
    {
        const SensorMode* pSensorMode = GetSensorModeData(FALSE);
        if (NULL != pSensorMode)
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "Sensor resolution: (%d, %d), cropinfo ((%d, %d) (%d, %d))",
                pSensorMode->resolution.outputWidth, pSensorMode->resolution.outputHeight,
                pSensorMode->cropInfo.firstPixel, pSensorMode->cropInfo.lastPixel,
                pSensorMode->cropInfo.firstLine, pSensorMode->cropInfo.lastLine);

            m_sensorSize.widthPixels = pSensorMode->cropInfo.lastPixel - pSensorMode->cropInfo.firstPixel + 1;
            m_sensorSize.heightLines = pSensorMode->cropInfo.lastLine - pSensorMode->cropInfo.firstLine + 1;
            CAMX_LOG_INFO(CamxLogGroupBPS, "SensorSize (%d, %d)", m_sensorSize.widthPixels, m_sensorSize.heightLines);

            ResampleAndCopyGrid(pSensorMode);
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Can not get sensor mode data");
        }

    }

    // Fill Still Output Size
    if (CamxResultSuccess == result)
    {
        result = GetGeoLibStillOutputSize();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Can not get still output size for GeoLib");
        }
    }

    // Fill Display Output Size
    *pDisplayOutputExist = FALSE;
    if ((CamxResultSuccess == result) && (CamxResultSuccess == GetGeoLibDisplayOutputSize()))
    {
        *pDisplayOutputExist = TRUE;
    }

    // Fill Zoom window
    m_zoomWindow.offset.x = pCropWindow->left / static_cast<float>(pRefCropWindow->width);
    m_zoomWindow.offset.y = pCropWindow->top / static_cast<float>(pRefCropWindow->height);
    m_zoomWindow.size.x   = pCropWindow->width / static_cast<float>(pRefCropWindow->width);
    m_zoomWindow.size.y   = pCropWindow->height / static_cast<float>(pRefCropWindow->height);
    CAMX_LOG_INFO(CamxLogGroupBPS, "zoomWindow: Offset (%f, %f), size (%f, %f) ",
        m_zoomWindow.offset.x, m_zoomWindow.offset.y, m_zoomWindow.size.x, m_zoomWindow.size.y);

    if (FALSE == GetStaticSettings()->enablepGeolibOffCenterZoom)
    {
        if (FALSE == (Utils::FEqual(((m_zoomWindow.offset.x * 2.0f) + m_zoomWindow.size.x), 1.0f)))
        {
            CAMX_LOG_WARN(CamxLogGroupBPS, " non centered zoom window onx from feature offset x %f,  size x %f",
                          m_zoomWindow.offset.x, m_zoomWindow.size.x);
            m_zoomWindow.offset.x = (1.0f - m_zoomWindow.size.x) / 2;
        }
        if (FALSE == (Utils::FEqual(((m_zoomWindow.offset.y * 2.0f) + m_zoomWindow.size.y), 1.0f)))
        {
            CAMX_LOG_WARN(CamxLogGroupBPS, " non centered zoom window on y from feature offset y %f,  size y %f",
                          m_zoomWindow.offset.y, m_zoomWindow.size.y);
            m_zoomWindow.offset.y = (1.0f - m_zoomWindow.size.y) / 2;
        }
    }

    // Fill LDC grid information
    if (FALSE == m_ICAGridOut2InEnabled)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Fill default out2in Grid information");
        PopulateNCLibWarpGrid(&m_ldFullOutToIn);
    }

    if (FALSE == m_ICAGridIn2OutEnabled)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Fill default in2out Grid information");
        PopulateNCLibWarpGrid(&m_ldFullInToOut);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::DumpGeolibResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::DumpGeolibResult(
    UINT64                  requestId,
    BOOL                    outputOnly,
    BOOL                    hasDisplayOutput)
{
#if defined (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // NOWHINE PR002 <- Win32 definition
    CHAR dataPath[]        = "/data/vendor/camera";
#else
    CHAR dataPath[]        = "/data/misc/camera";
#endif // Android-P or later

    CHAR dumpFilename[256] = { 0 };

    // m_mfMode has been assigned to a valid index here. But add a check just to be safe
    if (m_mfMode < GEOLIB_MF_MODE_NUM)
    {
        OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/BPS%d_request_%d_%s_flow_%d_mfmode_%d.log",
            dataPath, InstanceID(), requestId, BPSGeolibMetaTagNameList[m_mfMode], m_flowMode, m_mfMode);
        FILE* pFile = OsUtils::FOpen(dumpFilename, "wb");
        if (NULL == pFile)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Can not create dump file=%s", dumpFilename);
        }
        else
        {
            if (TRUE != outputOnly)
            {
                // Dump Input
                fprintf(pFile, "==================== Still Input ====================\n");
                Dump_GeoLibStillFrameInput(m_flowMode,
                                           m_mfMode,
                                           &m_sensorSize,
                                           &m_ldFullOutToIn,
                                           &m_ldFullInToOut,
                                           &m_stillOutSize,
                                           ((TRUE == hasDisplayOutput) ? &m_displayOutSize : NULL),
                                           ((GEOLIB_MF_FINAL == m_mfMode) ? NULL : &m_regOutSize),
                                           &m_zoomWindow,
                                           pFile);
            }

            // Dump output
            fprintf(pFile, "==================== Still Output ====================\n");
            Dump_GeoLibStillFrameConfig(&m_stillFrameConfig[m_mfMode], pFile);
        }

        if (NULL != pFile)
        {
            OsUtils::FClose(pFile);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid multiframe pass %d", m_mfMode);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::CalculateGeoLibParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::CalculateGeoLibParameters(
    UINT64                  requestId)
{
    CamxResult result = CamxResultSuccess;
    INT geolibResult  = NC_LIB_SUCCESS;


    // flow mode and MF mode should be filled at this moment
    CAMX_LOG_INFO(CamxLogGroupBPS, "Invoke GeoLib with flowMode=%d, mfmode=%d, display output=%d",
                  m_flowMode, m_mfMode, m_displayOutputFlag);

    geolibResult = GeoLibStillFrameCalc(m_flowMode,
                                        m_mfMode,
                                        &m_sensorSize,
                                        &m_ldFullOutToIn,
                                        &m_ldFullInToOut,
                                        (((0 == m_ldFullOutSize.widthPixels) || (0 == m_ldFullOutSize.heightLines)) ?
                                            NULL : &m_ldFullOutSize),
                                        &m_stillOutSize,
                                        ((TRUE == m_displayOutputFlag) ? &m_displayOutSize : NULL),
                                        ((GEOLIB_MF_FINAL == m_mfMode) ? NULL : &m_regOutSize),
                                        &m_zoomWindow,
                                        &m_stillFrameConfig[m_mfMode]);
    if (NC_LIB_SUCCESS != geolibResult)
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "GeoLIb calculation failed mfmode %d flow mode %d", m_mfMode, m_flowMode);
        result = CamxResultEFailed;
    }
    else
    {
        BOOL status = Validate_GeoLibStillCalcFrameOutput(m_flowMode,
                                                          m_mfMode,
                                                         (((GEOLIB_MF_FINAL == m_mfMode) && (TRUE == m_displayOutputFlag)) ?
                                                         &m_displayOutSize : NULL),
                                                         &m_stillFrameConfig[m_mfMode]);
        if (TRUE != status)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Geolib frame configuration output");
            result = CamxResultEInvalidArg;
        }
        else
        {
            if (TRUE == m_dumpGeolibResult)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Dump geolib in prefilter stage");
                DumpGeolibResult(requestId, FALSE, m_displayOutputFlag);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::SetScaleRatios
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::SetScaleRatios(
    ISPInputData* pInputData)
{
    IFECropInfo   cropInfo;
    UINT32        metaTag = 0;
    CamxResult    result  = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ref.cropsize",
                                                                     "RefCropSize",
                                                                     &metaTag);
    if (CamxResultSuccess == result)
    {
        metaTag |= InputMetadataSectionMask;
        CropWindow*        pCropWindow    = NULL;
        RefCropWindowSize* pRefCropWindow = NULL;

        UINT32  fullInputWidth  = pInputData->pipelineBPSData.width;
        UINT32  fullInputHeight = pInputData->pipelineBPSData.height;

        static const UINT PropertiesBPS[] =
        {
            InputScalerCropRegion,
            metaTag
        };
        UINT length                          = CAMX_ARRAY_SIZE(PropertiesBPS);
        VOID* pData[2]                       = { 0 };
        UINT64 propertyDataBPSOffset[2]      = { 0 , 0};

        GetDataList(PropertiesBPS, pData, propertyDataBPSOffset, length);

        pCropWindow    = (static_cast<CropWindow*>(pData[0]));
        pRefCropWindow = (static_cast<RefCropWindowSize*>(pData[1]));

        if ((NULL != pCropWindow) && (NULL != pRefCropWindow))
        {
            if ((0 == pRefCropWindow->width) || (0 == pRefCropWindow->height))
            {
                pRefCropWindow->width  = fullInputWidth;
                pRefCropWindow->height = fullInputHeight;
            }

            CAMX_LOG_INFO(CamxLogGroupBPS, "ZDBG IPE crop Window [%d, %d, %d, %d] full size %dX%d active %dX%d",
                pCropWindow->left,
                pCropWindow->top,
                pCropWindow->width,
                pCropWindow->height,
                fullInputWidth,
                fullInputHeight,
                pRefCropWindow->width,
                pRefCropWindow->height);

            cropInfo.fullPath.left   = (pCropWindow->left * fullInputWidth) / pRefCropWindow->width;
            cropInfo.fullPath.top    = (pCropWindow->top * fullInputHeight) / pRefCropWindow->height;
            cropInfo.fullPath.width  = (pCropWindow->width * fullInputWidth) / pRefCropWindow->width;
            cropInfo.fullPath.height = (pCropWindow->height * fullInputHeight) / pRefCropWindow->height;

            FLOAT ratio1 = static_cast<FLOAT>(cropInfo.fullPath.width) / static_cast<FLOAT>(fullInputWidth);
            FLOAT ratio2 = static_cast<FLOAT>(cropInfo.fullPath.height) / static_cast<FLOAT>(fullInputHeight);
            pInputData->postScaleRatio = (ratio1 > ratio2) ? ratio2 : ratio1;
            pInputData->preScaleRatio  = 1.0f;

            CAMX_LOG_INFO(CamxLogGroupBPS,
                "BPS:%d crop w %d, h %d, fw %d, fh %d, iw %d, ih %d, preScaleRatio %f, postScaleRatio %f",
                InstanceID(),
                cropInfo.fullPath.width, cropInfo.fullPath.height,
                fullInputWidth, fullInputHeight,
                m_curIntermediateDimension.width, m_curIntermediateDimension.height,
                pInputData->preScaleRatio, pInputData->postScaleRatio);

            m_cropWindowCurrent = *pCropWindow;
            m_refCropWindow     = *pRefCropWindow;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "crop window are NULL ");
            result = CamxResultENoSuch;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "cannot find vendor tag ref.cropsize");
        result = CamxResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ProcessGeoLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 CamxResult BPSNode::ProcessGeoLib(
    ISPInputData* pInputData,
    CmdBuffer*    pCmdBufferBlob)
{
    CamxResult result = CamxResultSuccess;

    if (BPSProcessingType::BPSProcessingMFSR == m_instanceProperty.processingType)
    {
        switch (m_instanceProperty.profileId)
        {
            case BPSProfileId::BPSProfileIdPrefilter:
            {
                result = ProcessGeoLibInPrefilter(pInputData, pCmdBufferBlob);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "[%s]: Update configIOData failed", NodeIdentifierString());
                }
                break;
            }
            case BPSProfileId::BPSProfileIdBlend:
            case BPSProfileId::BPSProfileIdBlendPost:
            {
                CAMX_LOG_INFO(CamxLogGroupBPS, "[%s]: Process Geolib information in profile %d",
                    NodeIdentifierString(), m_instanceProperty.profileId);

                result = GetMetadataGeoLibResult(pInputData->frameNum, m_instanceProperty.profileId);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "[%s}: Can not Get Geolib parameters in profile %d",
                        NodeIdentifierString(), m_instanceProperty.profileId);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupBPS, "[%s]: Update config IO data......", NodeIdentifierString());
                    result = UpdateConfigIOWithGeoLibParameters(
                        pInputData, pCmdBufferBlob, &m_stillFrameConfig[m_mfMode]);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupBPS, "[%s]: Update configIOData failed", NodeIdentifierString());
                    }
                }
                break;
            }
            case BPSProfileId::BPSProfileIdHNR:
            {
                if (m_curIntermediateDimension.ratio > 1.0f)
                {
                    // Update preScaleRatio and postScaleRatio for MFSR usecase
                    pInputData->preScaleRatio = 1.0f / m_curIntermediateDimension.ratio;
                    pInputData->postScaleRatio = pInputData->postScaleRatio / pInputData->preScaleRatio;
                    if (FALSE == Utils::FEqual(m_curIntermediateDimension.ratio, m_prevIntermediateDimension.ratio))
                    {
                        CAMX_LOG_INFO(CamxLogGroupBPS, "[%s]: Update Config IO......", NodeIdentifierString());
                        UpdateConfigIO(
                            pCmdBufferBlob, m_curIntermediateDimension.width, m_curIntermediateDimension.height);
                        m_prevIntermediateDimension = m_curIntermediateDimension;
                    }
                }
                break;
            }
            default:
            {
                result = CamxResultEInvalidArg;
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid BPS profile Id %d", m_instanceProperty.profileId);
                break;
            }
        }

        if (CamxResultSuccess == result)
        {
            m_cropWindowPrevious = m_cropWindowCurrent;
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::ProcessGeoLibInPrefilter
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::ProcessGeoLibInPrefilter(
    ISPInputData*       pInputData,
    CmdBuffer*          pCmdBufferBlob)
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_INFO(CamxLogGroupBPS, "[%s]: Invoke Geolib......", NodeIdentifierString());

    // need to check if we need MFNR or MFSR. Hardcode it to MFSR for now
    result = GetGeoLibFlowMode(pInputData);
    if (CamxResultSuccess == result)
    {
        CAMX_LOG_INFO(CamxLogGroupBPS, "Setup GeoLib parameters.......");
        result = PopulateGeoLibInput(&m_cropWindowCurrent, &m_refCropWindow, &m_displayOutputFlag);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Can not get GeoLib flow mode");
    }

    if (CamxResultSuccess == result)
    {
        // calculate the still frame configuration for prefilter stage
        m_mfMode    = GEOLIB_MF_ANCHOR;
        result = CalculateGeoLibParameters(pInputData->frameNum);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Can not setup Geolib Input parameters");
    }

    if (CamxResultSuccess == result)
    {
        // Post MetaData for prefilter
        result = PostMetadataGeoLibResult(m_mfMode);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "Post Geolib Parmaeters failed for prefilter");
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "Update config IO data......");
            result =  UpdateConfigIOWithGeoLibParameters(pInputData, pCmdBufferBlob, &m_stillFrameConfig[m_mfMode]);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "[%s]: Update config IO Data failed", NodeIdentifierString());
            }
            else
            {
                // calculate the still frame configuation for blending stage
                m_mfMode    = GEOLIB_MF_BLEND;
                result = CalculateGeoLibParameters(pInputData->frameNum);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "GeoLib calculation failure for prefilter stage");
    }

    if (CamxResultSuccess == result)
    {
        // Post metaData for blending stage
        result = PostMetadataGeoLibResult(m_mfMode);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "Post Geolib Parmaeters failed for blending");
        }
        else
        {
            // calcualte the still frame configuration for postfilter stage
            m_mfMode    = GEOLIB_MF_FINAL;
            result = CalculateGeoLibParameters(pInputData->frameNum);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "GeoLib calculation failure for blending stage");
    }

    if (CamxResultSuccess == result)
    {
        // Post metaData for postfilter stage
        result = PostMetadataGeoLibResult(m_mfMode);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Post Geolib Parmaeters failed for postfilter");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Geo Lib calculation failure for postfilter stage");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetGeoLibFlowMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::GetGeoLibFlowMode(
    ISPInputData*           pInputData)
{
    CamxResult                      result = CamxResultSuccess;
    mf_1_1_0::chromatix_mf11Type*   pChromatix;
    FLOAT                           sensorGainThreshold;


    pChromatix = pInputData->pTuningDataManager->GetChromatix()->GetModule_mf11_sw(
                       reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                       pInputData->pTuningData->noOfSelectionParameter);
    sensorGainThreshold = pChromatix->chromatix_mf11_reserve.mfsr_disable_gain;

    if (sensorGainThreshold > pInputData->triggerData.AECGain)
    {
        m_flowMode  = GEOLIB_STILL_MFSR;
    }
    else
    {
        m_flowMode  = GEOLIB_STILL_MFNR;
    }

    CAMX_LOG_INFO(CamxLogGroupBPS, "[%s]: threshold=%f, gain=%f, GeoLib flow mode is %d",
        NodeIdentifierString(), sensorGainThreshold, pInputData->triggerData.AECGain, m_flowMode);

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetMetadataGeoLibResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::GetMetadataGeoLibResult(
    UINT64          requestId,
    BPSProfileId    profileId)
{
    CamxResult result = CamxResultSuccess;
    UINT32 tag        = 0;

    result = GetGeoLibModeIdFromBPSProfileId(profileId, &m_mfMode);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid BPS profile Id input: %d", profileId);
    }

    if (CamxResultSuccess == result && m_mfMode < GEOLIB_MF_MODE_NUM)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.geoLibStillFrameConfig",
            BPSGeolibMetaTagNameList[m_mfMode], &tag);
        if (CamxResultSuccess == result)
        {
            UINT        geoLibFrameconfigTag[]    = { tag | InputMetadataSectionMask };
            const UINT  length                    = CAMX_ARRAY_SIZE(geoLibFrameconfigTag);
            VOID*       pData[length]             = { 0 };
            UINT64      pDataOffset[length]       = { 0 };

            CAMX_LOG_INFO(CamxLogGroupBPS, "[%s]: Retrieve GelibMetadata for %s: mfmode=%d, tagId=0x%x", NodeIdentifierString(),
                BPSGeolibMetaTagNameList[m_mfMode], m_mfMode, geoLibFrameconfigTag[0]);
            GetDataList(geoLibFrameconfigTag, pData, pDataOffset, length);
            if (NULL != pData[0])
            {
                Utils::Memcpy(&m_stillFrameConfig[m_mfMode], pData[0], sizeof(m_stillFrameConfig[m_mfMode]));
                if (TRUE == m_dumpGeolibResult)
                {
                    DumpGeolibResult(requestId, TRUE, TRUE);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "[%s]: Can not get Geo lib frame configuration", NodeIdentifierString());
                result = CamxResultENoSuch;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::PostMetadataGeoLibResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::PostMetadataGeoLibResult(
    GeoLibMultiFrameMode    mfMode)
{
    CamxResult result = CamxResultEFailed;
    UINT32 tag = 0;

    if (mfMode < GEOLIB_MF_MODE_NUM)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.geoLibStillFrameConfig",
            BPSGeolibMetaTagNameList[mfMode], &tag);
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupBPS, "Post GeolibMetadata for %s: tagId=0x%x", BPSGeolibMetaTagNameList[mfMode], tag);
            UINT geoLibFrameConfigTag[]                     = { tag };
            const UINT        dataLength                    = CAMX_ARRAY_SIZE(geoLibFrameConfigTag);
            const VOID*       pConfigData[dataLength]       = { &m_stillFrameConfig[mfMode] };
            UINT              pConfigDataCount[dataLength]  = { sizeof(m_stillFrameConfig[mfMode]) };

            result = WriteDataList(geoLibFrameConfigTag, pConfigData, pConfigDataCount, dataLength);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "[%s] Can not publish the Geolib frame configuration, error %d",
                    NodeIdentifierString(),  result);
            }
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::UpdateConfigIOWithGeoLibParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::UpdateConfigIOWithGeoLibParameters(
    ISPInputData*           pInputData,
    CmdBuffer*              pCmdBuffer,
    GeoLibStillFrameConfig* pStilFrameconfig)
{
    CamxResult       result        = CamxResultSuccess;
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    CAMX_UNREFERENCED_PARAM(pCmdBuffer);

    CAMX_LOG_INFO(CamxLogGroupBPS, "[%s]: Update Fetch window if needed", NodeIdentifierString());
    if (pStilFrameconfig->bpsOutFullCrop.inputImageSize.widthPixels !=
        pStilFrameconfig->bpsOutFullCrop.outputImageSize.widthPixels        ||
        pStilFrameconfig->bpsOutFullCrop.inputImageSize.heightLines !=
        pStilFrameconfig->bpsOutFullCrop.outputImageSize.heightLines)
    {
        pBPSIQSettings->fetchCropEN = 1;

        pBPSIQSettings->fetchCropWindow.fullWidth =
            Utils::EvenCeilingUINT32(pStilFrameconfig->bpsOutFullCrop.inputImageSize.widthPixels);

        pBPSIQSettings->fetchCropWindow.fullHeight =
            Utils::EvenCeilingUINT32(pStilFrameconfig->bpsOutFullCrop.inputImageSize.heightLines);

        pBPSIQSettings->fetchCropWindow.windowLeft =
            Utils::EvenCeilingUINT32(pStilFrameconfig->bpsOutFullCrop.inputImageRoi.offset.x *
                pStilFrameconfig->bpsOutFullCrop.inputImageSize.widthPixels);

        pBPSIQSettings->fetchCropWindow.windowTop =
            Utils::EvenCeilingUINT32(pStilFrameconfig->bpsOutFullCrop.inputImageRoi.offset.y *
                pStilFrameconfig->bpsOutFullCrop.inputImageSize.heightLines);

        pBPSIQSettings->fetchCropWindow.windowWidth =
            Utils::EvenCeilingUINT32(pStilFrameconfig->bpsOutFullCrop.inputImageRoi.size.x *
                pStilFrameconfig->bpsOutFullCrop.inputImageSize.widthPixels);

        pBPSIQSettings->fetchCropWindow.windowHeight =
            Utils::EvenCeilingUINT32(pStilFrameconfig->bpsOutFullCrop.inputImageRoi.size.y *
                pStilFrameconfig->bpsOutFullCrop.inputImageSize.heightLines);

        CAMX_LOG_INFO(CamxLogGroupBPS, "FW fetch: Input Window (%d, %d), fetch window (%d, %d, %d, %d)",
            pBPSIQSettings->fetchCropWindow.fullWidth, pBPSIQSettings->fetchCropWindow.fullHeight,
            pBPSIQSettings->fetchCropWindow.windowLeft, pBPSIQSettings->fetchCropWindow.windowTop,
            pBPSIQSettings->fetchCropWindow.windowWidth, pBPSIQSettings->fetchCropWindow.windowHeight);
    }


    BpsConfigIo*     pConfigIO     = reinterpret_cast<BpsConfigIo*>(m_configIOMem.pVirtualAddr);
    BpsConfigIoData* pConfigIOData = &pConfigIO->cmdData;
    BOOL             bUpdate = FALSE;


    if (pConfigIOData->images[BPS_INPUT_IMAGE].info.dimensions.widthPixels !=
            pStilFrameconfig->bpsOutFullCrop.inputImageSize.widthPixels            ||
        pConfigIOData->images[BPS_INPUT_IMAGE].info.dimensions.heightLines !=
            pStilFrameconfig->bpsOutFullCrop.inputImageSize.heightLines)
    {
        pConfigIOData->images[BPS_INPUT_IMAGE].info.dimensions.widthPixels       =
            pStilFrameconfig->bpsOutFullCrop.inputImageSize.widthPixels;
        pConfigIOData->images[BPS_INPUT_IMAGE].info.dimensions.heightLines       =
            pStilFrameconfig->bpsOutFullCrop.inputImageSize.heightLines;

        CAMX_LOG_INFO(CamxLogGroupBPS, "FW ConfigIOData: Input Size:(%d, %d)",
            pConfigIOData->images[BPS_INPUT_IMAGE].info.dimensions.widthPixels,
            pConfigIOData->images[BPS_INPUT_IMAGE].info.dimensions.heightLines);
        bUpdate = TRUE;
    }


    if (pConfigIOData->images[BPS_OUTPUT_IMAGE_FULL].info.dimensions.widthPixels !=
            pStilFrameconfig->bpsOutSizeFull.widthPixels                           ||
        pConfigIOData->images[BPS_OUTPUT_IMAGE_FULL].info.dimensions.heightLines !=
            pStilFrameconfig->bpsOutSizeFull.heightLines)
    {
        pConfigIOData->images[BPS_OUTPUT_IMAGE_FULL].info.dimensions.widthPixels =
            pStilFrameconfig->bpsOutSizeFull.widthPixels;
        pConfigIOData->images[BPS_OUTPUT_IMAGE_FULL].info.dimensions.heightLines =
            pStilFrameconfig->bpsOutSizeFull.heightLines;

        CAMX_LOG_INFO(CamxLogGroupBPS, "FW ConfigIOData: Output Full: (%d, %d)",
            pConfigIOData->images[BPS_OUTPUT_IMAGE_FULL].info.dimensions.widthPixels,
            pConfigIOData->images[BPS_OUTPUT_IMAGE_FULL].info.dimensions.heightLines);
        bUpdate = TRUE;
    }

    if (pConfigIOData->images[BPS_OUTPUT_IMAGE_DS4].info.dimensions.widthPixels !=
            pStilFrameconfig->bpsOutSizeDS4.widthPixels                            ||
        pConfigIOData->images[BPS_OUTPUT_IMAGE_DS4].info.dimensions.heightLines !=
            pStilFrameconfig->bpsOutSizeDS4.heightLines)
    {
        pConfigIOData->images[BPS_OUTPUT_IMAGE_DS4].info.dimensions.widthPixels =
            pStilFrameconfig->bpsOutSizeDS4.widthPixels;
        pConfigIOData->images[BPS_OUTPUT_IMAGE_DS4].info.dimensions.heightLines =
            pStilFrameconfig->bpsOutSizeDS4.heightLines;

        CAMX_LOG_INFO(CamxLogGroupBPS, "FW ConfigIOData: Output DS4: (%d, %d)",
            pConfigIOData->images[BPS_OUTPUT_IMAGE_DS4].info.dimensions.widthPixels,
            pConfigIOData->images[BPS_OUTPUT_IMAGE_DS4].info.dimensions.heightLines);
        bUpdate = TRUE;
    }

    if (pConfigIOData->images[BPS_OUTPUT_IMAGE_DS16].info.dimensions.widthPixels !=
            pStilFrameconfig->bpsOutSizeDS16.widthPixels                           ||
        pConfigIOData->images[BPS_OUTPUT_IMAGE_DS16].info.dimensions.heightLines !=
            pStilFrameconfig->bpsOutSizeDS16.heightLines)
    {
        pConfigIOData->images[BPS_OUTPUT_IMAGE_DS16].info.dimensions.widthPixels =
            pStilFrameconfig->bpsOutSizeDS16.widthPixels;
        pConfigIOData->images[BPS_OUTPUT_IMAGE_DS16].info.dimensions.heightLines =
            pStilFrameconfig->bpsOutSizeDS16.heightLines;

        CAMX_LOG_INFO(CamxLogGroupBPS, "FW ConfigIOData: Output DS16: (%d, %d)",
            pConfigIOData->images[BPS_OUTPUT_IMAGE_DS16].info.dimensions.widthPixels,
            pConfigIOData->images[BPS_OUTPUT_IMAGE_DS16].info.dimensions.heightLines);
        bUpdate = TRUE;
    }

    if (pConfigIOData->images[BPS_OUTPUT_IMAGE_DS64].info.dimensions.widthPixels !=
            pStilFrameconfig->bpsOutSizeDS64.widthPixels                           ||
        pConfigIOData->images[BPS_OUTPUT_IMAGE_DS64].info.dimensions.heightLines !=
            pStilFrameconfig->bpsOutSizeDS64.heightLines)
    {
        pConfigIOData->images[BPS_OUTPUT_IMAGE_DS64].info.dimensions.widthPixels =
            pStilFrameconfig->bpsOutSizeDS64.widthPixels;
        pConfigIOData->images[BPS_OUTPUT_IMAGE_DS64].info.dimensions.heightLines =
            pStilFrameconfig->bpsOutSizeDS64.heightLines;

        CAMX_LOG_INFO(CamxLogGroupBPS, "FW ConfigIOData: Output DS64: (%d, %d)",
            pConfigIOData->images[BPS_OUTPUT_IMAGE_DS64].info.dimensions.widthPixels,
            pConfigIOData->images[BPS_OUTPUT_IMAGE_DS64].info.dimensions.heightLines);
        bUpdate = TRUE;
    }

    CAMX_LOG_INFO(CamxLogGroupBPS, "FW ConfigIOData: orig RegOut: (%d, %d)",
        pConfigIOData->images[BPS_OUTPUT_IMAGE_REG1].info.dimensions.widthPixels,
        pConfigIOData->images[BPS_OUTPUT_IMAGE_REG1].info.dimensions.heightLines);

    if (pConfigIOData->images[BPS_OUTPUT_IMAGE_REG1].info.dimensions.widthPixels !=
            pStilFrameconfig->bpsRegCropAndScale.outputImageSize.widthPixels           ||
        pConfigIOData->images[BPS_OUTPUT_IMAGE_REG1].info.dimensions.heightLines !=
            pStilFrameconfig->bpsRegCropAndScale.outputImageSize.heightLines)
    {
        pConfigIOData->images[BPS_OUTPUT_IMAGE_REG1].info.dimensions.widthPixels =
            pStilFrameconfig->bpsRegCropAndScale.outputImageSize.widthPixels;
        pConfigIOData->images[BPS_OUTPUT_IMAGE_REG1].info.dimensions.heightLines =
            pStilFrameconfig->bpsRegCropAndScale.outputImageSize.heightLines;

        CAMX_LOG_INFO(CamxLogGroupBPS, "FW ConfigIOData: RegOut: (%d, %d)",
            pConfigIOData->images[BPS_OUTPUT_IMAGE_REG1].info.dimensions.widthPixels,
            pConfigIOData->images[BPS_OUTPUT_IMAGE_REG1].info.dimensions.heightLines);
        bUpdate = TRUE;
    }

    if (TRUE == bUpdate)
    {
        CAMX_LOG_INFO(CamxLogGroupBPS, "Update config IO data with GeoLib result");
        DeInitializeStripingParams();
        result = InitializeStripingParams(pConfigIOData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Initialize Striping params failed %d", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::UpdateConfigIO
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::UpdateConfigIO(
    CmdBuffer* pCmdBuffer,
    UINT32     intermediateWidth,
    UINT32     intermediateHeight)
{
    CamxResult       result        = CamxResultSuccess;
    BpsConfigIo*     pConfigIO     = reinterpret_cast<BpsConfigIo*>(m_configIOMem.pVirtualAddr);
    BpsConfigIoData* pConfigIOData = &pConfigIO->cmdData;

    // MFSR: updating intermediate size in Config IO
    ImageDimensions  ds4Dimension = { 0 };
    ImageDimensions  ds16Dimension = { 0 };
    ImageDimensions  ds64Dimension = { 0 };
    GetDownscaleDimension(intermediateWidth, intermediateHeight, &ds4Dimension, &ds16Dimension, &ds64Dimension);

    pConfigIOData->images[BPS_INPUT_IMAGE].info.dimensions.widthPixels       = intermediateWidth;
    pConfigIOData->images[BPS_INPUT_IMAGE].info.dimensions.heightLines       = intermediateHeight;
    pConfigIOData->images[BPS_OUTPUT_IMAGE_FULL].info.dimensions.widthPixels = intermediateWidth;
    pConfigIOData->images[BPS_OUTPUT_IMAGE_FULL].info.dimensions.heightLines = intermediateHeight;
    pConfigIOData->images[BPS_OUTPUT_IMAGE_DS4].info.dimensions.widthPixels  = ds4Dimension.widthPixels;
    pConfigIOData->images[BPS_OUTPUT_IMAGE_DS4].info.dimensions.heightLines  = ds4Dimension.heightLines;
    pConfigIOData->images[BPS_OUTPUT_IMAGE_DS16].info.dimensions.widthPixels = ds16Dimension.widthPixels;
    pConfigIOData->images[BPS_OUTPUT_IMAGE_DS16].info.dimensions.heightLines = ds16Dimension.heightLines;
    pConfigIOData->images[BPS_OUTPUT_IMAGE_DS64].info.dimensions.widthPixels = ds64Dimension.widthPixels;
    pConfigIOData->images[BPS_OUTPUT_IMAGE_DS64].info.dimensions.heightLines = ds64Dimension.heightLines;

    DeInitializeStripingParams();
    result = InitializeStripingParams(pConfigIOData);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Initialize Striping params failed %d", result);
    }
    else
    {
        m_configIORequest.pDeviceResourceParam = m_deviceResourceRequest.pDeviceResourceParam;
        result = PacketBuilder::WriteGenericBlobData(pCmdBuffer, CSLICPGenericBlobCmdBufferConfigIO,
                                                     sizeof(ConfigIORequest), reinterpret_cast<BYTE*>(&m_configIORequest));

        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "BPS:%d WriteGenericBlobData() return %d", InstanceID(), result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::DumpConfigIOData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::DumpConfigIOData()
{
    BpsConfigIo*     pConfigIO;
    BpsConfigIoData* pConfigIOData;

    pConfigIO     = reinterpret_cast<BpsConfigIo*>(m_configIOMem.pVirtualAddr);
    pConfigIOData = &pConfigIO->cmdData;

    CAMX_LOG_INFO(CamxLogGroupBPS, "BPS:%d Config IO Data Dump:", InstanceID());
    for (UINT i = 0; i < BPS_IO_IMAGES_MAX; i++)
    {
        CAMX_LOG_INFO(CamxLogGroupBPS, "BPS:%d image %d format=%d, width=%d, height=%d",
                       InstanceID(), i,
                       pConfigIOData->images[i].info.format,
                       pConfigIOData->images[i].info.dimensions.widthPixels,
                       pConfigIOData->images[i].info.dimensions.heightLines);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result        = CamxResultSuccess;
    UINT32     tagCount      = 0;
    UINT32     tagID;

    if (NumBPSMetadataOutputTags + BPSMaxOutputICAVendorTags < MaxTagsPublished)
    {
        for (UINT32 tagIndex = 0; tagIndex < NumBPSMetadataOutputTags; ++tagIndex)
        {
            pPublistTagList->tagArray[tagCount++] = BPSMetadataOutputTags[tagIndex];
        }

        if (TRUE == m_publishLDCGridData)
        {
            for (UINT32 tagIndex = 0; tagIndex < BPSMaxOutputICAVendorTags; ++tagIndex)
            {
                result = VendorTagManager::QueryVendorTagLocation(
                    BPSOutputICAVendorTags[tagIndex].pSectionName,
                    BPSOutputICAVendorTags[tagIndex].pTagName,
                    &tagID);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                                   BPSOutputICAVendorTags[tagIndex].pSectionName,
                                   BPSOutputICAVendorTags[tagIndex].pTagName);
                    break;
                }
                pPublistTagList->tagArray[tagCount++] = tagID;
                m_vendorTagArray[tagIndex]            = tagID;
            }
        }

        if (TRUE == GetStaticSettings()->enableStreamCropZoom)
        {
            for (UINT32 tagIndex = 0; tagIndex < BPSMaxOutputStreamCropVendorTags; ++tagIndex)
            {
                result = VendorTagManager::QueryVendorTagLocation(
                    BPSOutputStreamCropVendorTags[tagIndex].pSectionName,
                    BPSOutputStreamCropVendorTags[tagIndex].pTagName,
                    &tagID);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                            BPSOutputStreamCropVendorTags[tagIndex].pSectionName,
                            BPSOutputStreamCropVendorTags[tagIndex].pTagName);
                    break;
                }

                pPublistTagList->tagArray[tagCount++] = tagID;
            }
        }
    }
    else
    {
        result = CamxResultEOutOfBounds;
        CAMX_LOG_ERROR(CamxLogGroupMeta, "ERROR More space needed to add publish tags (%d, %d)",
            NumBPSMetadataOutputTags, BPSMaxOutputICAVendorTags);
    }

    if (CamxResultSuccess == result)
    {
        pPublistTagList->tagCount = tagCount;
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tags will be published", tagCount);
    }
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetMetadataContrastLevel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::GetMetadataContrastLevel(
    ISPHALTagsData* pHALTagsData)
{
    UINT32     metaTagContrast = 0;
    CamxResult resultContrast  = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.contrast",
                                                                            "level",
                                                                            &metaTagContrast);
    if (CamxResultSuccess == resultContrast)
    {
        static const UINT VendorTagContrast[] =
        {
            metaTagContrast | InputMetadataSectionMask,
        };

        const static UINT lengthContrast                    = CAMX_ARRAY_SIZE(VendorTagContrast);
        VOID* pDataContrast[lengthContrast]                 = { 0 };
        UINT64 vendorTagsContrastBPSOffset[lengthContrast]  = { 0 };

        GetDataList(VendorTagContrast, pDataContrast, vendorTagsContrastBPSOffset, lengthContrast);
        if (NULL != pDataContrast[0])
        {
            UINT8 appLevel = *(static_cast<UINT8*>(pDataContrast[0]));
            if (appLevel > 0)
            {
                pHALTagsData->contrastLevel = appLevel - 1;
            }
            else
            {
                pHALTagsData->contrastLevel = 5;
            }
            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "Manual Contrast Level = %d", pHALTagsData->contrastLevel);
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupBPS, "Cannot obtain Contrast Level. Set default to 5");
            pHALTagsData->contrastLevel = 5;
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "No Contrast Level available. Set default to 5");
        pHALTagsData->contrastLevel = 5; // normal without contrast change
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetMetadataTonemapCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::GetMetadataTonemapCurve(
    ISPHALTagsData* pHALTagsData)
{
    // Deep copy tone map curves, only when the tone map is contrast curve
    if (TonemapModeContrastCurve == pHALTagsData->tonemapCurves.tonemapMode)
    {
        ISPTonemapPoint* pBlueTonemapCurve  = NULL;
        ISPTonemapPoint* pGreenTonemapCurve = NULL;
        ISPTonemapPoint* pRedTonemapCurve   = NULL;

        static const UINT VendorTagsBPS[] =
        {
            InputTonemapCurveBlue,
            InputTonemapCurveGreen,
            InputTonemapCurveRed,
        };

        const static UINT length                    = CAMX_ARRAY_SIZE(VendorTagsBPS);
        VOID* pData[length]                         = { 0 };
        UINT64 vendorTagsTonemapBPSOffset[length]   = { 0 };

        GetDataList(VendorTagsBPS, pData, vendorTagsTonemapBPSOffset, length);

        if (NULL != pData[0])
        {
            pBlueTonemapCurve = static_cast<ISPTonemapPoint*>(pData[0]);
        }

        if (NULL != pData[1])
        {
            pGreenTonemapCurve = static_cast<ISPTonemapPoint*>(pData[1]);
        }

        if (NULL != pData[2])
        {
            pRedTonemapCurve = static_cast<ISPTonemapPoint*>(pData[2]);
        }

        CAMX_ASSERT(NULL != pBlueTonemapCurve);
        CAMX_ASSERT(NULL != pGreenTonemapCurve);
        CAMX_ASSERT(NULL != pRedTonemapCurve);

        pHALTagsData->tonemapCurves.curvePoints = static_cast<INT32>(
            GetDataCountFromPipeline(InputTonemapCurveBlue, 0, GetPipeline()->GetPipelineId(), TRUE));

        if ((pHALTagsData->tonemapCurves.curvePoints > 0) && (NULL != pBlueTonemapCurve))
        {
            // Blue tone map curve
            Utils::Memcpy(pHALTagsData->tonemapCurves.tonemapCurveBlue,
                          pBlueTonemapCurve,
                          (sizeof(ISPTonemapPoint) * pHALTagsData->tonemapCurves.curvePoints));
        }

        pHALTagsData->tonemapCurves.curvePoints = static_cast<INT32>(
            GetDataCountFromPipeline(InputTonemapCurveGreen, 0, GetPipeline()->GetPipelineId(), TRUE));

        if ((pHALTagsData->tonemapCurves.curvePoints > 0) && (NULL != pGreenTonemapCurve))
        {
            // Green tone map curve
            Utils::Memcpy(pHALTagsData->tonemapCurves.tonemapCurveGreen,
                          pGreenTonemapCurve,
                          (sizeof(ISPTonemapPoint) * pHALTagsData->tonemapCurves.curvePoints));
        }

        pHALTagsData->tonemapCurves.curvePoints = static_cast<INT32>(
            GetDataCountFromPipeline(InputTonemapCurveRed, 0, GetPipeline()->GetPipelineId(), TRUE));

        if ((pHALTagsData->tonemapCurves.curvePoints > 0) && (NULL != pRedTonemapCurve))
        {
            // Red tone map curve
            Utils::Memcpy(pHALTagsData->tonemapCurves.tonemapCurveRed,
                          pRedTonemapCurve,
                          (sizeof(ISPTonemapPoint) * pHALTagsData->tonemapCurves.curvePoints));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSNode::GetInSensorSeamlessControltState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::GetInSensorSeamlessControltState(
    ISPInputData*   pInputData)
{
    CamxResult result  = CamxResultSuccess;
    UINT metaTag       = 0;

    result = VendorTagManager::QueryVendorTagLocation("com.qti.insensor_control", "seamless_insensor_state", &metaTag);

    if (CamxResultSuccess == result)
    {
        UINT              propertiesBPS[] = { metaTag | InputMetadataSectionMask };
        static const UINT Length          = CAMX_ARRAY_SIZE(propertiesBPS);
        VOID*             pData[Length]   = { 0 };
        UINT64            offsets[Length] = { 0 };

        result = GetDataList(propertiesBPS, pData, offsets, Length);

        if (CamxResultSuccess == result)
        {
            if (NULL != pData[0])
            {
                pInputData->seamlessInSensorState = *reinterpret_cast<SeamlessInSensorState*>(pData[0]);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupBPS, "BPS:%s Can't get in-sensor seamless control state!!!, pData is NULL",
                                 NodeIdentifierString());
            }
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "BPS:%s Can't query vendor tag: seamless_insensor_state",
                         NodeIdentifierString());
    }

    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "BPS:%s Get tag: seamless_insensor_state = %u",
                     NodeIdentifierString(),
                     pInputData->seamlessInSensorState);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetMetadataTMC12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::GetMetadataTMC12(
    ISPInputData*    pModuleInput)
{
    CamxResult  result = CamxResultSuccess;

    UINT    propertiesTMC12[] =
    {
        PropertyIDIFEADRCInfoOutput,
    };

    if (TRUE != IsNodeInPipeline(IFE))
    {
        propertiesTMC12[0] |= InputMetadataSectionMask;
    }

    const SIZE_T numTags                          = CAMX_ARRAY_SIZE(propertiesTMC12);
    VOID*        pPropertyDataTMC12[numTags]      = { 0 };
    UINT64       propertyDataTMC12Offset[numTags] = { 0 };

    result = GetDataList(propertiesTMC12, pPropertyDataTMC12, propertyDataTMC12Offset, numTags);

    if (CamxResultSuccess == result)
    {
        //  Tag: PropertyIDIFEADRCInfoOutput
        if (NULL != pPropertyDataTMC12[0])
        {
            //  Get the the previousCalculatedHistogram/previousCalculatedCDF,
            //  which are posted in realtime pipeline with offset = 1
            pModuleInput->triggerData.pPreviousCalculatedHistogram =
                                                (static_cast<ADRCData*>(pPropertyDataTMC12[0]))->previousCalculatedHistogram;
            pModuleInput->triggerData.pPreviousCalculatedCDF       =
                                                (static_cast<ADRCData*>(pPropertyDataTMC12[0]))->previousCalculatedCDF;
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupBPS, "BPS:%u Failed to get PropertyIDIFEADRCInfoOutput for TMC12", InstanceID());
    }

    GetMetadataDarkBoostOffset(pModuleInput);
    GetMetadataFourthToneAnchor(pModuleInput);

    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "BPS:%u TMC12 override data: dark boost offset = %f, fourth tone anchor = %f",
                     InstanceID(),
                     pModuleInput->triggerData.overrideDarkBoostOffset,
                     pModuleInput->triggerData.overrideFourthToneAnchor);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::SetupICAGrid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::SetupICAGrid()
{
    CamxResult result = CamxResultSuccess;

    m_publishLDCGridData = RequireLDCGridPublishing();

    if (TRUE == m_publishLDCGridData)
    {
        result = AllocateICAGridData();
        if (CamxResultSuccess == result)
        {
            UINT32 cameraId = GetPipeline()->GetCameraId();
            UpdateICAChromatixGridData(cameraId, NULL);
        }
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "%s, LDC Grid setup failed", NodeIdentifierString());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::AllocateICAGridData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSNode::AllocateICAGridData()
{
    CamxResult result          = CamxResultSuccess;

    for (UINT i = 0; i < GridMaxType; i++)
    {
        switch(i)
        {
            case In2OutGrid:
                m_pWarpGridData[i] = &m_ldFullInToOut;
                break;
            case Out2InGrid:
                m_pWarpGridData[i] = &m_ldFullOutToIn;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Grid type %d", i);
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::DeallocateICAGridData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::DeallocateICAGridData()
{
    for (UINT i = 0; i < GridMaxType; i++)
    {
        m_pWarpGridData[i] = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSNode::UpdateICAChromatixGridData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::UpdateICAChromatixGridData(
    UINT32 cameraId,
    ChiTuningModeParameter* pTuningModeData)
{
    BOOL                            result              = FALSE;
    TuningDataManager*              pTuningManager      = HwEnvironment::GetInstance()->GetTuningDataManager(cameraId);
    const StaticSettings*           pStaticSettings     = HwEnvironment::GetInstance()->GetStaticSettings();
    VOID*                           pICAChromatix       = NULL;
    VOID*                           pICARgnData         = NULL;
    NcLibIcaGrid*                   pWarpInGridOut2In   = m_pWarpGridData[Out2InGrid];
    NcLibIcaGrid*                   pWarpInGridIn2Out   = m_pWarpGridData[In2OutGrid];
    UINT                            numSelectors        = 1;
    TuningMode                      defaultSelectors[1] = { { ModeType::Default, { 0 } } };
    TuningMode*                     pSelectors          = &defaultSelectors[0];

    if (NULL != pTuningModeData)
    {
        numSelectors       = pTuningModeData->noOfSelectionParameter;
        pSelectors         = reinterpret_cast<TuningMode*>(&pTuningModeData->TuningMode[0]);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "#Of selectors %d", numSelectors);

    if ((NULL != pTuningManager) &&
        (TRUE == pTuningManager->IsValidChromatix()))
    {
        switch (m_capability.ICAVersion)
        {
            case ICAVersion30:
                pICAChromatix =
                    static_cast<VOID *>(pTuningManager->GetChromatix()->GetModule_ica30_ipe_module1(
                        pSelectors,
                        numSelectors));
                break;
            case ICAVersion20:
                pICAChromatix =
                    static_cast<VOID *>(pTuningManager->GetChromatix()->GetModule_ica20_ipe_module1(
                        pSelectors,
                        numSelectors));
                break;
            case ICAVersion10:
                pICAChromatix =
                    static_cast<VOID *>(pTuningManager->GetChromatix()->GetModule_ica10_ipe_module1(
                        pSelectors,
                        numSelectors));
                break;
            default:
                break;
        }
    }

    if (NULL != pICAChromatix)
    {
        switch (m_capability.ICAVersion)
        {
            case ICAVersion30:
                m_ICAGridOut2InEnabled =
                    ((TRUE == !!((static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                        enable_section.ctc_transform_grid_enable)) &&
                        (TRUE == !!((static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                            chromatix_ica30_reserve.ld_u2i_grid_valid))) ? TRUE : FALSE;

                m_ICAGridIn2OutEnabled =
                    ((TRUE == !!((static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                        enable_section.ctc_transform_grid_enable)) &&
                        (TRUE == !!((static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                            chromatix_ica30_reserve.ld_i2u_grid_valid))) ? TRUE : FALSE;

                m_ICAGridGeometry = (1 == (static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                    chromatix_ica30_reserve.ld_i2u_grid_geometry) ? ICAGeometryCol67Row51 : ICAGeometryCol35Row27;

                pICARgnData = &(static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                    chromatix_ica30_core.mod_ica30_lens_posn_data[0].
                    lens_posn_data.mod_ica30_lens_zoom_data[0].
                    lens_zoom_data.mod_ica30_aec_data[0].
                    ica30_rgn_data;
                if ((TRUE == m_ICAGridIn2OutEnabled) && (TRUE == m_ICAGridOut2InEnabled))
                {
                    m_ldFullOutSize.widthPixels = (static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                        chromatix_ica30_reserve.ld_full_out_width;
                    m_ldFullOutSize.heightLines = (static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                        chromatix_ica30_reserve.ld_full_out_height;
                }

                CAMX_LOG_INFO(CamxLogGroupBPS, "ldFullOutSize: (width=%d, height=%d)",
                    m_ldFullOutSize.widthPixels, m_ldFullOutSize.heightLines);
                break;
            case ICAVersion20:
                m_ICAGridOut2InEnabled = ((TRUE == !!((static_cast<ica_2_0_0::chromatix_ica20Type *>(pICAChromatix))->
                    enable_section.ctc_transform_grid_enable)) &&
                    (TRUE == !!((static_cast<ica_2_0_0::chromatix_ica20Type *>(pICAChromatix))->
                        chromatix_ica20_reserve.undistorted_to_lens_distorted_output_ld_grid_valid))) ? TRUE : FALSE;

                m_ICAGridIn2OutEnabled = ((TRUE == !!((static_cast<ica_2_0_0::chromatix_ica20Type *>(pICAChromatix))->
                    enable_section.ctc_transform_grid_enable)) &&
                    (TRUE == !!((static_cast<ica_2_0_0::chromatix_ica20Type *>(pICAChromatix))->
                        chromatix_ica20_reserve.distorted_input_to_undistorted_ldc_grid_valid))) ? TRUE : FALSE;

                m_ICAGridGeometry = ICAGeometryCol35Row27;

                pICARgnData = &(static_cast<ica_2_0_0::chromatix_ica20Type *>(pICAChromatix))->
                    chromatix_ica20_core.mod_ica20_lens_posn_data[0].
                    lens_posn_data.mod_ica20_lens_zoom_data[0].
                    lens_zoom_data.mod_ica20_aec_data[0].
                    ica20_rgn_data;
                break;
            case ICAVersion10:
                m_ICAGridOut2InEnabled = ((TRUE == !!((static_cast<ica_1_0_0::chromatix_ica10Type *>(pICAChromatix))->
                    enable_section.ctc_transform_grid_enable)) &&
                    (TRUE == !!((static_cast<ica_1_0_0::chromatix_ica10Type *>(pICAChromatix))->
                        chromatix_ica10_reserve.undistorted_to_lens_distorted_output_ld_grid_valid))) ? TRUE : FALSE;

                m_ICAGridIn2OutEnabled = ((TRUE == !!((static_cast<ica_1_0_0::chromatix_ica10Type *>(pICAChromatix))->
                    enable_section.ctc_transform_grid_enable)) &&
                    (TRUE == !!((static_cast<ica_1_0_0::chromatix_ica10Type *>(pICAChromatix))->
                        chromatix_ica10_reserve.distorted_input_to_undistorted_ldc_grid_valid))) ? TRUE : FALSE;

                m_ICAGridGeometry = ICAGeometryCol33Row25;

                pICARgnData = &(static_cast<ica_1_0_0::chromatix_ica10Type *>(pICAChromatix))->
                    chromatix_ica10_core.mod_ica10_lens_posn_data[0].
                    lens_posn_data.mod_ica10_lens_zoom_data[0].
                    lens_zoom_data.mod_ica10_aec_data[0].
                    ica10_rgn_data;
                break;
            default:
                break;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid warp grid chromatix");
    }

    if (NULL != pICARgnData)
    {
        UINT32 numRows    = 0;
        UINT32 numColumns = 0;
        UINT32 QFactor    = Q4;

        switch (m_ICAGridGeometry)
        {
            case ICAGeometryCol67Row51:
                numColumns = ICA30GridTransformWidth;
                numRows    = ICA30GridTransformHeight;
                QFactor    = Q4;
                break;
            case ICAGeometryCol35Row27:
                numColumns = ICA20GridTransformWidth;
                numRows    = ICA20GridTransformHeight;
                QFactor    = (CSLCameraTitanVersion::CSLTitan480 == m_titanVersion) ? Q4 : Q3;
                break;
            case ICAGeometryCol33Row25:
                numColumns = ICA10GridTransformWidth;
                numRows    = ICA10GridTransformHeight;
                QFactor    = Q3;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid ica grid geometry");
                break;
        }

        // out2in grid data
        if ((TRUE == m_ICAGridOut2InEnabled) && (NULL != pWarpInGridOut2In))
        {
            pWarpInGridOut2In->enable                          = 1;
            pWarpInGridOut2In->reuseGridTransform              = 0;
            pWarpInGridOut2In->transformDefinedOnWidth         = IcaVirtualDomainWidth;
            pWarpInGridOut2In->transformDefinedOnHeight        = IcaVirtualDomainHeight;
            pWarpInGridOut2In->geometry                        = static_cast<NcLibIcaGridGeometry>(m_ICAGridGeometry);
            pWarpInGridOut2In->extrapolatedCorners             = 0;
            Utils::Memset(&pWarpInGridOut2In->gridCorners, 0x0, sizeof(pWarpInGridOut2In->gridCorners));

            switch (m_capability.ICAVersion)
            {
                case ICAVersion30:
                    for (UINT i = 0; i < (numColumns * numRows); i++)
                    {
                        // u2i , out2in grid is essentially ctc grid
                        pWarpInGridOut2In->grid[i].x = ((static_cast<ica_3_0_0::ica30_rgn_dataType *>(pICARgnData))->
                            ctc_grid_x_tab.ctc_grid_x[i] / QFactor);
                        pWarpInGridOut2In->grid[i].y = ((static_cast<ica_3_0_0::ica30_rgn_dataType *>(pICARgnData))
                            ->ctc_grid_y_tab.ctc_grid_y[i] / QFactor);
                    }
                    break;
                case ICAVersion20:
                    for (UINT i = 0; i < (numColumns * numRows); i++)
                    {
                        // u2i , out2in grid is essentially ctc grid
                        pWarpInGridOut2In->grid[i].x = ((static_cast<ica_2_0_0::ica20_rgn_dataType *>(pICARgnData))->
                            ctc_grid_x_tab.ctc_grid_x[i] / QFactor);
                        pWarpInGridOut2In->grid[i].y = ((static_cast<ica_2_0_0::ica20_rgn_dataType *>(pICARgnData))
                            ->ctc_grid_y_tab.ctc_grid_y[i] / QFactor);
                    }
                    break;
                case ICAVersion10:
                    for (UINT i = 0; i < (numColumns * numRows); i++)
                    {
                        // u2i , out2in grid is essentially ctc grid
                        pWarpInGridOut2In->grid[i].x = ((static_cast<ica_1_0_0::ica10_rgn_dataType *>(pICARgnData))->
                            ctc_grid_x_tab.ctc_grid_x[i] / QFactor);
                        pWarpInGridOut2In->grid[i].y = ((static_cast<ica_1_0_0::ica10_rgn_dataType *>(pICARgnData))
                            ->ctc_grid_y_tab.ctc_grid_y[i] / QFactor);
                    }
                    for (UINT i = 0; i < 4; i++)
                    {
                        pWarpInGridOut2In->gridCorners[i].x = ((static_cast<ica_1_0_0::ica10_rgn_dataType *>(pICARgnData))->
                            ctc_grid_x_tab.ctc_grid_x[(numColumns * numRows) + i] / QFactor);
                        pWarpInGridOut2In->gridCorners[i].y = ((static_cast<ica_1_0_0::ica10_rgn_dataType *>(pICARgnData))->
                            ctc_grid_y_tab.ctc_grid_y[(numColumns * numRows) + i] / QFactor);
                    }
                    pWarpInGridOut2In->extrapolatedCorners = 1;
                    break;
                default:
                    break;
            }
        }

        // in2out grid data
        if ((TRUE == m_ICAGridIn2OutEnabled) && (NULL != pWarpInGridIn2Out))
        {
            pWarpInGridIn2Out->enable                          = 1;
            pWarpInGridIn2Out->reuseGridTransform              = 0;
            pWarpInGridIn2Out->transformDefinedOnWidth         = IcaVirtualDomainWidth;
            pWarpInGridIn2Out->transformDefinedOnHeight        = IcaVirtualDomainHeight;
            pWarpInGridIn2Out->geometry                        = static_cast<NcLibIcaGridGeometry>(m_ICAGridGeometry);
            pWarpInGridIn2Out->extrapolatedCorners             = 0;
            Utils::Memset(&pWarpInGridIn2Out->gridCorners, 0x0, sizeof(pWarpInGridIn2Out->gridCorners));

            switch (m_capability.ICAVersion)
            {
                case ICAVersion30:
                    for (UINT i = 0; i < (numColumns * numRows); i++)
                    {
                        pWarpInGridIn2Out->grid[i].x = ((static_cast<ica_3_0_0::ica30_rgn_dataType *>(pICARgnData))->
                            ld_i2u_grid_x_tab.ld_i2u_grid_x[i] / QFactor);
                        pWarpInGridIn2Out->grid[i].y = ((static_cast<ica_3_0_0::ica30_rgn_dataType *>(pICARgnData))->
                            ld_i2u_grid_y_tab.ld_i2u_grid_y[i] / QFactor);
                    }
                    break;
                case ICAVersion20:
                    for (UINT i = 0; i < (numColumns * numRows); i++)
                    {
                        pWarpInGridIn2Out->grid[i].x = ((static_cast<ica_2_0_0::ica20_rgn_dataType *>(pICARgnData))->
                            distorted_input_to_undistorted_ldc_grid_x_tab.distorted_input_to_undistorted_ldc_grid_x[i] /
                            QFactor);
                        pWarpInGridIn2Out->grid[i].y = ((static_cast<ica_2_0_0::ica20_rgn_dataType *>(pICARgnData))->
                            distorted_input_to_undistorted_ldc_grid_y_tab.distorted_input_to_undistorted_ldc_grid_y[i] /
                            QFactor);
                    }
                    break;
                case ICAVersion10:
                    for (UINT i = 0; i < (numColumns * numRows); i++)
                    {
                        pWarpInGridIn2Out->grid[i].x = ((static_cast<ica_1_0_0::ica10_rgn_dataType *>(pICARgnData))->
                            distorted_input_to_undistorted_ldc_grid_x_tab.distorted_input_to_undistorted_ldc_grid_x[i] /
                            QFactor);
                        pWarpInGridIn2Out->grid[i].y = ((static_cast<ica_1_0_0::ica10_rgn_dataType *>(pICARgnData))->
                            distorted_input_to_undistorted_ldc_grid_y_tab.distorted_input_to_undistorted_ldc_grid_y[i] /
                            QFactor);
                    }
                    for (UINT i = 0; i < 4; i++)
                    {
                        pWarpInGridIn2Out->gridCorners[i].x = ((static_cast<ica_1_0_0::ica10_rgn_dataType *>(pICARgnData))->
                            distorted_input_to_undistorted_ldc_grid_x_tab.
                            distorted_input_to_undistorted_ldc_grid_x[(numColumns * numRows) + i] /
                            QFactor);
                        pWarpInGridIn2Out->gridCorners[i].y = ((static_cast<ica_1_0_0::ica10_rgn_dataType *>(pICARgnData))->
                            distorted_input_to_undistorted_ldc_grid_y_tab.
                            distorted_input_to_undistorted_ldc_grid_y[(numColumns * numRows) + i] /
                            QFactor);
                    }
                    pWarpInGridIn2Out->extrapolatedCorners = 1;
                    break;
                default:
                    break;
            }
        }
        CAMX_LOG_INFO(CamxLogGroupBPS, "LDC: ICA grid geometry %d, InGrid Out2In enabled %d, In2Out enabled %d",
                      m_ICAGridGeometry, m_ICAGridOut2InEnabled, m_ICAGridIn2OutEnabled);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid chromatix ICA data ptr");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::PublishICAGridTransform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::PublishICAGridTransform(
    NcLibIcaGrid** ppWarpGridData)
{
    CamxResult    result                = CamxResultSuccess;
    BOOL          isFullOutPortEnabled  = FALSE;
    UINT32        out2inGridtagId       = m_vendorTagArray[ICAInGridOut2InTransformIdx];
    UINT32        in2outGridtagId       = m_vendorTagArray[ICAInGridIn2OutTransformIdx];
    NcLibIcaGrid* pWarpOutGridOut2In    = ppWarpGridData[Out2InGrid];
    NcLibIcaGrid* pWarpOutGridIn2Out    = ppWarpGridData[In2OutGrid];
    UINT32        numRows               = 0;
    UINT32        numColumns            = 0;

    switch (m_ICAGridGeometry)
    {
        case ICAGeometryCol67Row51:
            numColumns = ICA30GridTransformWidth;
            numRows    = ICA30GridTransformHeight;
            break;
        case ICAGeometryCol35Row27:
            numColumns = ICA20GridTransformWidth;
            numRows    = ICA20GridTransformHeight;
            break;
        case ICAGeometryCol33Row25:
            numColumns = ICA10GridTransformWidth;
            numRows    = ICA10GridTransformHeight;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid ica grid geometry");
            break;
    }

    for (UINT portIndex = 0; portIndex < GetNumOutputPorts(); portIndex++)
    {
        OutputPort* pOutputPort = GetOutputPort(portIndex);

        if (BPSOutputPortFull == pOutputPort->portId)
        {
            isFullOutPortEnabled = TRUE;
        }
    }

    UINT              propertiesICAGrid[1] = { 0 };
    const VOID*       pICAGridData[1]      = { 0 };
    UINT              pICAGridDataCount[1] = { 0 };

    IPEICAGridTransform gridOut2InTransform;
    Utils::Memset(&gridOut2InTransform, 0x0, sizeof(IPEICAGridTransform));

    IPEICAGridTransform gridIn2OutTransform;
    Utils::Memset(&gridIn2OutTransform, 0x0, sizeof(IPEICAGridTransform));

    if (NULL != pWarpOutGridOut2In)
    {
        gridOut2InTransform.gridTransformEnable      = (TRUE == m_ICAGridOut2InEnabled);
        if (TRUE == m_ICAGridOut2InEnabled)
        {
            gridOut2InTransform.reuseGridTransform       = 0;
            gridOut2InTransform.transformDefinedOnWidth  = IcaVirtualDomainWidth;
            gridOut2InTransform.transformDefinedOnHeight = IcaVirtualDomainHeight;
            gridOut2InTransform.geometry                 = static_cast<ICAGridGeometry>(m_ICAGridGeometry);

            for (UINT idx = 0; idx < (numColumns * numRows); idx++)
            {
                gridOut2InTransform.gridTransformArray[idx].x = pWarpOutGridOut2In->grid[idx].x;
                gridOut2InTransform.gridTransformArray[idx].y = pWarpOutGridOut2In->grid[idx].y;
            }
            // Grid Corners are filled for ICA 1.0 only. For others they are set to zero.
            for (UINT idx = 0; idx < 4; idx++)
            {
                gridOut2InTransform.gridTransformArrayCorners[idx].x = pWarpOutGridOut2In->gridCorners[idx].x;
                gridOut2InTransform.gridTransformArrayCorners[idx].y = pWarpOutGridOut2In->gridCorners[idx].y;
            }
            gridOut2InTransform.gridTransformArrayExtrapolatedCorners =
                (pWarpOutGridOut2In->extrapolatedCorners == EXTRAPOLATION_TYPE_FOUR_CORNERS) ? 1 : 0;

            gridOut2InTransform.gridTransformArrayExtrapolatedCorners = 0;
        }
        propertiesICAGrid[0] = { out2inGridtagId };
        pICAGridData[0]      = &gridOut2InTransform;
        pICAGridDataCount[0] = sizeof(IPEICAGridTransform);

        if (TRUE == isFullOutPortEnabled)
        {
            WritePSDataList(BPSOutputPortFull, propertiesICAGrid, pICAGridData, pICAGridDataCount, 1);
            PublishPSData(out2inGridtagId, NULL);
            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "%s: published out2in ica grid data", NodeIdentifierString());
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "%s: Full port not enabled, can't publish", NodeIdentifierString());
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid out2in ica grid ptr");
    }

    if (NULL != pWarpOutGridIn2Out)
    {
        gridIn2OutTransform.gridTransformEnable      = (TRUE == m_ICAGridIn2OutEnabled);
        if (TRUE == m_ICAGridIn2OutEnabled)
        {
            gridIn2OutTransform.reuseGridTransform       = 0;
            gridIn2OutTransform.transformDefinedOnWidth  = IcaVirtualDomainWidth;
            gridIn2OutTransform.transformDefinedOnHeight = IcaVirtualDomainHeight;
            gridIn2OutTransform.geometry                 = static_cast<ICAGridGeometry>(m_ICAGridGeometry);

            for (UINT idx = 0; idx < (numColumns * numRows); idx++)
            {
                gridIn2OutTransform.gridTransformArray[idx].x = pWarpOutGridIn2Out->grid[idx].x;
                gridIn2OutTransform.gridTransformArray[idx].y = pWarpOutGridIn2Out->grid[idx].y;
            }
            // Grid Corners are filled for ICA 1.0 only. For others they are set to zero.
            for (UINT idx = 0; idx < 4; idx++)
            {
                gridIn2OutTransform.gridTransformArrayCorners[idx].x = pWarpOutGridIn2Out->gridCorners[idx].x;
                gridIn2OutTransform.gridTransformArrayCorners[idx].y = pWarpOutGridIn2Out->gridCorners[idx].y;
            }
            gridIn2OutTransform.gridTransformArrayExtrapolatedCorners =
                (pWarpOutGridIn2Out->extrapolatedCorners == EXTRAPOLATION_TYPE_FOUR_CORNERS) ? 1 : 0;

            gridIn2OutTransform.gridTransformArrayExtrapolatedCorners = 0;
        }
        propertiesICAGrid[0] = { in2outGridtagId };
        pICAGridData[0]      = &gridIn2OutTransform;
        pICAGridDataCount[0] = sizeof(IPEICAGridTransform);

        if (TRUE == isFullOutPortEnabled)
        {
            WritePSDataList(BPSOutputPortFull, propertiesICAGrid, pICAGridData, pICAGridDataCount, 1);
            PublishPSData(in2outGridtagId, NULL);
            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "published in2out ica grid data");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "%s: Full port not enabled, can't publish", NodeIdentifierString());
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid in2out ica grid ptr");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::IsRegOutputPortWithoutAlignment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSNode::IsRegOutputPortWithoutAlignment(
    UINT32                      outputPortId,
    OutputPortNegotiationData*  pOutputPortNegotiationData)
{
    BOOL isValue = FALSE;

    if (((BPSProcessingMFNR == m_instanceProperty.processingType)   ||
         (BPSProcessingMFSR == m_instanceProperty.processingType))  &&
        ((outputPortId == BPSOutputPortReg1)                        ||
         (outputPortId == BPSOutputPortReg2))                       &&
        (TRUE == IsPortWithoutAlignment(pOutputPortNegotiationData)))
    {
        isValue = TRUE;
    }

    return isValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::IsPortWithoutAlignment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSNode::IsPortWithoutAlignment(
    OutputPortNegotiationData*  pOutputPortNegotiationData)
{
    BOOL isWithoutAlignment = TRUE;

    for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification && TRUE == isWithoutAlignment;
                                                                                                inputIndex++)
    {
        BufferRequirement*  pInputPortRequirement = &pOutputPortNegotiationData->inputPortRequirement[inputIndex];

        for (UINT planeIdx = 0; planeIdx < FormatsMaxPlanes && TRUE == isWithoutAlignment; planeIdx++)
        {
            if ((0 != pInputPortRequirement->planeAlignment[planeIdx].strideAlignment) ||
                (0 != pInputPortRequirement->planeAlignment[planeIdx].scanlineAlignment))
            {
                isWithoutAlignment = FALSE;
                CAMX_LOG_VERBOSE(CamxLogGroupBPS, "%s, Need alignment, inputIndex=%u, planeIdx=%u, alignment: %u, %u",
                                 NodeIdentifierString(),
                                 inputIndex,
                                 planeIdx,
                                 pInputPortRequirement->planeAlignment[planeIdx].strideAlignment,
                                 pInputPortRequirement->planeAlignment[planeIdx].scanlineAlignment);
            }
        }
    }


    return isWithoutAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetMetadataDarkBoostOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::GetMetadataDarkBoostOffset(
    ISPInputData*    pModuleInput)
{
    CamxResult  result                      = CamxResultSuccess;
    UINT        tagIsValidDarkBoostOffset   = 0;
    UINT        tagDarkBoostOffset          = 0;

    //  Set overrideDarkBoostOffset to -1 as default value, which means not enable this optional function
    pModuleInput->triggerData.overrideDarkBoostOffset = -1;

    //  Get the TMC user control setting from Apps
    //  Tag: isValidDarkBoostOffset
    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.tmcusercontrol",
                                                      "isValidDarkBoostOffset",
                                                      &tagIsValidDarkBoostOffset);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "BPS:%u Can't query vendor tag: isValidDarkBoostOffset", InstanceID());
    }

    //  Tag: darkBoostOffset
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.tmcusercontrol",
                                                          "darkBoostOffset",
                                                          &tagDarkBoostOffset);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "BPS:%u Can't query vendor tag: darkBoostOffset", InstanceID());
        }
    }

    static const UINT PropertiesTMC[] =
    {
        tagIsValidDarkBoostOffset   | InputMetadataSectionMask,
        tagDarkBoostOffset          | InputMetadataSectionMask,
    };

    const SIZE_T numTags                        = CAMX_ARRAY_SIZE(PropertiesTMC);
    VOID*        pPropertyDataTMC[numTags]      = { 0 };
    UINT64       propertyDataTMCOffset[numTags] = { 0 };

    if (CamxResultSuccess == result)
    {
        result = GetDataList(PropertiesTMC, pPropertyDataTMC, propertyDataTMCOffset, numTags);
    }

    if (CamxResultSuccess == result)
    {
        //  Tag: tagIsValidDarkBoostOffset and tagDarkBoostOffset
        if (NULL != pPropertyDataTMC[0])
        {
            BOOL isValid = *(static_cast<BOOL*>(pPropertyDataTMC[0]));
            if ((TRUE == isValid) && (NULL != pPropertyDataTMC[1]))
            {
                //  Get the override dark boost offset for user control
                pModuleInput->triggerData.overrideDarkBoostOffset = *(static_cast<FLOAT*>(pPropertyDataTMC[1]));
            }
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "BPS:%u Failed to get TMC user control DarkBoostOffset", InstanceID());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::GetMetadataFourthToneAnchor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::GetMetadataFourthToneAnchor(
    ISPInputData*    pModuleInput)
{
    CamxResult  result                      = CamxResultSuccess;
    UINT        tagIsValidFourthToneAnchor  = 0;
    UINT        tagFourthToneAnchor         = 0;

    //  Set overrideFourthToneAnchor to -1 as defailt value, which means not enable this optional function
    pModuleInput->triggerData.overrideFourthToneAnchor = -1;

    //  Get the TMC user control setting from Apps
    //  Tag: isValidFourthToneAnchor
    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.tmcusercontrol",
                                                      "isValidFourthToneAnchor",
                                                      &tagIsValidFourthToneAnchor);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "BPS:%u Can't query vendor tag: isValidFourthToneAnchor", InstanceID());
    }

    if (CamxResultSuccess == result)
    {
        //  Tag: fourthToneAnchor
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.tmcusercontrol",
                                                          "fourthToneAnchor",
                                                          &tagFourthToneAnchor);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "BPS:%u Can't query vendor tag: fourthToneAnchor", InstanceID());
        }
    }

    static const UINT PropertiesTMC[] =
    {
        tagIsValidFourthToneAnchor  | InputMetadataSectionMask,
        tagFourthToneAnchor         | InputMetadataSectionMask,
    };

    const SIZE_T numTags                        = CAMX_ARRAY_SIZE(PropertiesTMC);
    VOID*        pPropertyDataTMC[numTags]      = { 0 };
    UINT64       propertyDataTMCOffset[numTags] = { 0 };

    if (CamxResultSuccess == result)
    {
        result = GetDataList(PropertiesTMC, pPropertyDataTMC, propertyDataTMCOffset, numTags);
    }

    if (CamxResultSuccess == result)
    {
        //  Tag: tagIsValidFourthToneAnchor and tagFourthToneAnchor
        if (NULL != pPropertyDataTMC[0])
        {
            BOOL isValid = *(static_cast<BOOL*>(pPropertyDataTMC[0]));
            if ((TRUE == isValid) && (NULL != pPropertyDataTMC[1]))
            {
                //  Get the override fourth tone anchor for user control
                pModuleInput->triggerData.overrideFourthToneAnchor = *(static_cast<FLOAT*>(pPropertyDataTMC[1]));
            }
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "BPS:%u Failed to get TMC user control FourthToneAnchor", InstanceID());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSNode::CalculateRegistrationOutputSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSNode::CalculateRegistrationOutputSize(
    const ImageFormat*  pImageFormat,
    ImageFormat*        pRegImageFormat)
{
    UINT32 MFSRInputWidth  = pImageFormat->width  >> MFSRDownscaleRatioShift;
    UINT32 MFSRInputHeight = pImageFormat->height >> MFSRDownscaleRatioShift;
    UINT32 resolutionIndex = 0;

    for ( ; resolutionIndex < MaxIndexRegInputResolutionMap; ++resolutionIndex)
    {
        if ((MFSRInputWidth  >= RegInputResolutionMap[resolutionIndex].width)  &&
            (MFSRInputHeight >= RegInputResolutionMap[resolutionIndex].height))
        {
            break;
        }
    }

    if (resolutionIndex >= MaxIndexRegInputResolutionMap)
    {
        // If we cannot match any resolution in the map, then this index will be equal to MaxIndexRegInputResolutionMap.
        resolutionIndex = MaxIndexRegInputResolutionMap - 1;

        CAMX_LOG_WARN(CamxLogGroupBPS, "%s, Cannot match any registration size, so use the smallest size.",
                      NodeIdentifierString());
    }

    pRegImageFormat->width  = RegInputResolutionMap[resolutionIndex].width;
    pRegImageFormat->height = RegInputResolutionMap[resolutionIndex].height;
}

CAMX_NAMESPACE_END
