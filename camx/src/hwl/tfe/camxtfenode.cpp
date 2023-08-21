////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtfenode.cpp
/// @brief TFE Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcsljumptable.h"
#include "camxcslresourcedefs.h"
#include "camxthreadmanager.h"
#include "camxhal3module.h"
#include "camxhwcontext.h"
#include "camxtrace.h"
#include "camxvendortags.h"
#include "camxnode.h"
#include "camxpipeline.h"
#include "camxpropertyblob.h"
#include "parametertuningtypes.h"
#include "camxispstatsmodule.h"
#include "camximagesensormoduledata.h"
#include "camxiqinterface.h"
#include "camxisppipeline.h"
#include "camxispiqmodule.h"
#include "chipinforeaderdefs.h"
#include "camxtfeproperty.h"
#include "camxtfenode.h"
#include "camxisppipeline.h"

#define STRIPE_FIELD_PRINT(field)    \
    CAMX_LOG_ERROR(CamxLogGroupISP, "dualTFE %s = %d", #field, (field))

#define STRIPE_FIELD_PRINT_LL(field) \
    CAMX_LOG_ERROR(CamxLogGroupISP, "dualTFE %s = %lld", #field, (field))

#define PRINT_CROP_1D(_in)                          \
do                                                  \
{                                                   \
    STRIPE_FIELD_PRINT(_in.enable);                 \
    STRIPE_FIELD_PRINT(_in.inDim);                  \
    STRIPE_FIELD_PRINT(_in.firstOut);               \
    STRIPE_FIELD_PRINT(_in.lastOut);                \
} while (0, 0)


#define PRINT_MNDS_V16(_in)                         \
do                                                  \
{                                                   \
    STRIPE_FIELD_PRINT(_in.en);                     \
    STRIPE_FIELD_PRINT(_in.input_l);                \
    STRIPE_FIELD_PRINT(_in.output_l);               \
    STRIPE_FIELD_PRINT(_in.pixel_offset);           \
    STRIPE_FIELD_PRINT(_in.cnt_init);               \
    STRIPE_FIELD_PRINT(_in.interp_reso);            \
    STRIPE_FIELD_PRINT(_in.rounding_option_v);      \
    STRIPE_FIELD_PRINT(_in.rounding_option_h);      \
    STRIPE_FIELD_PRINT(_in.right_pad_en);           \
    STRIPE_FIELD_PRINT(_in.input_processed_length); \
    STRIPE_FIELD_PRINT(_in.phase_init);             \
    STRIPE_FIELD_PRINT(_in.phase_step);             \
} while (0, 0)

#define PRINT_WE_STRIPE(_in)                        \
do                                                  \
{                                                   \
    STRIPE_FIELD_PRINT(_in.enable1);                \
    STRIPE_FIELD_PRINT(_in.hInit1);                 \
    STRIPE_FIELD_PRINT(_in.stripeWidth1);           \
    STRIPE_FIELD_PRINT(_in.enable2);                \
    STRIPE_FIELD_PRINT(_in.hInit2);                 \
    STRIPE_FIELD_PRINT(_in.stripeWidth2);           \
} while (0, 0)

CAMX_NAMESPACE_BEGIN

// TPG defaults, need to derive the value based on the dimension and FPS
static const UINT   TPGPIXELCLOCK   = 400000000;
static const FLOAT  TPGFPS          = 30.0f;

static UINT64 TFEStatsModuleOutputPorts[] =
{
    TFEOutputPortStatsAECBG,
    TFEOutputPortStatsBHIST,
    TFEOutputPortStatsTLBE,
    TFEOutputPortStatsBAF,
    TFEOutputPortStatsAWBBG
};

/// @brief Command buffer identifiers
enum class TFECmdBufferId: UINT32
{
    Packet = 0,     ///< Packet
    Main,           ///< Main IQ command buffer
    Left,           ///< Left IQ command buffer
    Right,          ///< Right IQ command buffer
    DMI32,          ///< DMI32 command buffer
    DMI64,          ///< DMI64 command buffer
    DualConfig,     ///< Dual TFE config command buffer
    GenericBlob,    ///< Generic command buffer
    NumCmdBuffers   ///< Max number of command buffers
};

static const UINT   TFEKMDCmdBufferMaxSize  = 8 * 1024;                 ///< Reserved KMD Cmd Buffer Size

// @brief list of usecase tags published by TFE node
/// @todo (CAMX-4913) Update with TFE tag after adding to vendor tags.
static const UINT32 TFEUsecaseTags[] =
{
    PropertyIDUsecaseIFEPDAFInfo,
};

/// @todo (CAMX-4913) Update after adding register map files
static const UINT32 TFELeftPartialTileShift     = IFE_IFE_0_BUS_WR_CLIENT_3_TILE_CFG_PARTIAL_TILE_LEFT_SHIFT;
static const UINT32 TFELeftPartialTileMask      = IFE_IFE_0_BUS_WR_CLIENT_3_TILE_CFG_PARTIAL_TILE_LEFT_MASK;
static const UINT32 TFERightPartialTileShift    = IFE_IFE_0_BUS_WR_CLIENT_3_TILE_CFG_PARTIAL_TILE_RIGHT_SHIFT;
static const UINT32 TFERightPartialTileMask     = IFE_IFE_0_BUS_WR_CLIENT_3_TILE_CFG_PARTIAL_TILE_RIGHT_MASK;

// Make this a setting
static const BOOL TFEOverwriteModuleWithStriping = TRUE;    ///< Overwrite IQ modules using striping output

/// @todo (CAMX-4913) Update based on actual value
static const UINT32 TFEMaxPdafPixelsPerBlock     = (16 * 16); ///< Maximum Pdaf Pixels Per block

static const UINT32 DefaultBHistStatsRegionWidth    = 2;

/// @todo (CAMX-4913) Update values based on HW
static const UINT32 DefaultDMISelShift      = 63;
static const UINT32 DefaultDMIIndexShift    = 54;
static const UINT32 DefaultDMILeftShift     = 41;
static const UINT32 DefaultDMILeftBits      = 0x1FFF;
static const UINT32 DefaultDMITopShift      = 27;
static const UINT32 DefaultDMITopBits       = 0x3FFF;
static const UINT32 DefaultDMIWidthShift    = 14;
static const UINT32 DefaultDMIWidthBits     = 0xFFF;
static const UINT32 DefaultDMIHeightShift   = 0;
static const UINT32 DefaultDMIHeightBits    = 0x1FFF;

// BF v2.5 (Titan480)
static const UINT32 DefaultBF25DMISelShift   = 73;  // Upper 64-bit from striping library
static const UINT32 DefaultBF25DMIIndexShift = 55;  // Note this is different from BF v2.3 and BF v2.4 (not bit position 54)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TranslateFormatToISPImageFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 TranslateFormatToISPImageFormat()
{
    UINT32 ISPFormat = TFEFormatUndefined;

    return ISPFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::TranslateCSIDataTypeToCSIDecodeFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 TFENode::TranslateCSIDataTypeToCSIDecodeFormat()
{
    UINT8 CSIDecodeFormat = 0;

    return CSIDecodeFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::TranslateBitDepthToCSIDecodeFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 TFENode::TranslateBitDepthToCSIDecodeFormat()
{
    UINT8 CSIDecodeFormat = 0;

    return CSIDecodeFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::TranslateSensorStreamConfigTypeToPortSourceType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TFENode::TranslateSensorStreamConfigTypeToPortSourceType()
{
    UINT portSourceTypeId = PortSrcTypeUndefined;

    return portSourceTypeId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::TranslatePortSourceTypeToSensorStreamConfigType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 TFENode::TranslatePortSourceTypeToSensorStreamConfigType()
{
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::TranslateColorFilterPatternToISPPattern
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 TFENode::TranslateColorFilterPatternToISPPattern()
{
    UINT32 ISPPattern = 0;

    return ISPPattern;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::FindSensorStreamConfigIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::FindSensorStreamConfigIndex(
    StreamType  streamType,
    UINT*       pStreamIndex)
{
    BOOL foundStream = FALSE;

    for (UINT32 index = 0; index < m_pSensorModeData->streamConfigCount; index++)
    {
        if (m_pSensorModeData->streamConfig[index].type == streamType)
        {
            foundStream = TRUE;
            if (NULL != pStreamIndex)
            {
                *pStreamIndex = index;
            }
            break;
        }
    }

    return foundStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CheckOutputPortIndexIfUnsupported
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::CheckOutputPortIndexIfUnsupported()
{
    BOOL needOutputPortDisabled = FALSE;

    return needOutputPortDisabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CheckIfPDAFType3Supported
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::CheckIfPDAFType3Supported()
{
    BOOL enableCAMIFSubsample    = FALSE;

    return enableCAMIFSubsample;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::IsPixelOutputPortSourceType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::IsPixelOutputPortSourceType(
    UINT outputPortId)
{
    BOOL isPixelPortSourceType = FALSE;

    const UINT portSourceTypeId = GetOutputPortSourceType(OutputPortIndex(outputPortId));

    switch (portSourceTypeId)
    {
        case PortSrcTypeUndefined:
        case PortSrcTypePixel:
            isPixelPortSourceType = TRUE;
            break;

        default:
            break;
    }

    return isPixelPortSourceType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::SetRDIOutputPortFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::SetRDIOutputPortFormat()
{
    CamxResult result = CamxResultSuccess;

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::TFENode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TFENode::TFENode()
{
    m_pNodeName = "TFE";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ReleaseDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ReleaseDevice()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::~TFENode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TFENode::~TFENode()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TFENode* TFENode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    if ((NULL != pCreateInputData) && (NULL != pCreateInputData->pNodeInfo))
    {
        UINT32           propertyCount   = pCreateInputData->pNodeInfo->nodePropertyCount;
        PerNodeProperty* pNodeProperties = pCreateInputData->pNodeInfo->pNodeProperties;

        TFENode* pNodeObj = CAMX_NEW TFENode;

        if (NULL != pNodeObj)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "nodePropertyCount %d", propertyCount);

            // There can be multiple TFE instances in a pipeline, each instance can have different properties
            for (UINT32 count = 0; count < propertyCount; count++)
            {
                UINT32 nodePropertyId     = pNodeProperties[count].id;
                VOID*  pNodePropertyValue = pNodeProperties[count].pValue;

                switch (nodePropertyId)
                {
                    /// @todo (CAMX-4913) Update with TFE node properties or change to ISP
                    case NodePropertyIFECSIDHeight:
                        pNodeObj->m_instanceProperty.TFECSIDHeight = atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyIFECSIDWidth:
                        pNodeObj->m_instanceProperty.TFECSIDWidth = atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyIFECSIDLeft:
                        pNodeObj->m_instanceProperty.TFECSIDLeft = atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyIFECSIDTop:
                        pNodeObj->m_instanceProperty.TFECSIDTop = atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyStatsSkipPattern:
                        // Incase of 60 fps preview, stats should run at 30 fps(tintless at 15),
                        // this property defines the skip pattern.
                        pNodeObj->m_instanceProperty.TFEStatsSkipPattern = (2 * (*static_cast<UINT*>(pNodePropertyValue)));
                        break;
                    case NodePropertyForceSingleIFEOn:
                        pNodeObj->m_instanceProperty.TFESingleOn = atoi(static_cast<CHAR*>(pNodePropertyValue));
                        break;
                    default:
                        CAMX_LOG_WARN(CamxLogGroupPProc, "Unhandled node property Id %d", nodePropertyId);
                        break;
                }
            }

            CAMX_LOG_INFO(CamxLogGroupPProc, "TFE instance CSIDHeight: %d, CSIDWidth: %d, CSIDLeft: %d, CSIDTop: %d, "
                "SkipPattern: %d, ForceSingleTFEOn: %d",
                pNodeObj->m_instanceProperty.TFECSIDHeight,
                pNodeObj->m_instanceProperty.TFECSIDWidth,
                pNodeObj->m_instanceProperty.TFECSIDLeft,
                pNodeObj->m_instanceProperty.TFECSIDTop,
                pNodeObj->m_instanceProperty.TFEStatsSkipPattern,
                pNodeObj->m_instanceProperty.TFESingleOn);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to create TFENode, no memory");
        }

        return pNodeObj;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null input pointer");
        return NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);

    CamxResult  result                = CamxResultSuccess;
    INT32       deviceIndex           = -1;
    UINT        indicesLengthRequired = 0;
    UINT32      TFETestImageSizeWidth;
    UINT32      TFETestImageSizeHeight;

    CAMX_ASSERT(TFE == Type());

    /// @todo (CAMX-4913) Update with mimas5xcontext once ready
    Titan17xContext* pContext = NULL;

    pContext             = static_cast<Titan17xContext *>(GetHwContext());
    m_OEMIQSettingEnable = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->IsOEMIQSettingEnable;

    UINT inputPortId[MaxTFEInputPorts];

    pCreateOutputData->maxOutputPorts = MaxDefinedTFEOutputPorts;
    pCreateOutputData->maxInputPorts  = MaxTFEInputPorts;

    GetHwContext()->GetDeviceVersion(CSLDeviceTypeTFE, &m_version);

    m_maxNumOfCSLTFEPortId = CSLTFEPortIdMaxNumPortResources;

    GetAllInputPortIds(&m_totalInputPorts, &inputPortId[0]);
    CAMX_LOG_INFO(CamxLogGroupISP, "m_totalInputPorts = %u", m_totalInputPorts);

    UINT numOutputPorts = 0;
    UINT outputPortId[MaxBufferComposite];

    GetAllOutputPortIds(&numOutputPorts, &outputPortId[0]);

    CAMX_ASSERT_MESSAGE(numOutputPorts > 0, "Failed to get output port.");


    if (TRUE == IsSecureMode())
    {
        for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
        {
            // Except stats,set all ports as secured if node is secure node
            if (TRUE == IsStatsOutputPort(outputPortId[outputPortIndex]))
            {
                ResetSecureMode(outputPortId[outputPortIndex]);
            }
        }
    }

    CAMX_ASSERT(MaxBufferComposite >= numOutputPorts);

    UINT32 groupID = TFEOutputGroupIdMAX;
    for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
    {
        switch (outputPortId[outputPortIndex])
        {

            case TFEOutputPortFull:
                pCreateOutputData->bufferComposite.portGroupID[TFEOutputPortFull] = TFEOutputGroupId0;
                break;

            case TFEOutputPortRawDump:
                pCreateOutputData->bufferComposite.portGroupID[TFEOutputPortRawDump] = TFEOutputGroupId1;
                break;

            case TFEOutputPortStatsAECBG:
            case TFEOutputPortStatsBHIST:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                    TFEOutputGroupId2;
                break;

            case TFEOutputPortStatsTLBE:
            case TFEOutputPortStatsAWBBG:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                    TFEOutputGroupId3;
                break;

            case TFEOutputPortStatsBAF:
                pCreateOutputData->bufferComposite.portGroupID[TFEOutputPortStatsBAF] =
                    TFEOutputGroupId4;
                break;

            case TFEOutputPortRDI0:
                pCreateOutputData->bufferComposite.portGroupID[TFEOutputPortRDI0] =
                    TFEOutputGroupId5;
                break;

            case TFEOutputPortRDI1:
                pCreateOutputData->bufferComposite.portGroupID[TFEOutputPortRDI1] =
                    TFEOutputGroupId6;
                break;

            case TFEOutputPortRDI2:
            case TFEOutputPortPDAF:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                    TFEOutputGroupId7;
                break;

            default:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                    groupID++;
                break;
        }
    }

    pCreateOutputData->bufferComposite.hasCompositeMask = TRUE;
    Utils::Memcpy(&m_bufferComposite, &pCreateOutputData->bufferComposite, sizeof(BufferGroup));

    /// @todo (CAMX-4913) Update with TFE after getting settings xml
    TFETestImageSizeWidth  = GetStaticSettings()->IFETestImageSizeWidth;
    TFETestImageSizeHeight = GetStaticSettings()->IFETestImageSizeHeight;
    m_disableManual3ACCM   = GetStaticSettings()->DisableManual3ACCM;

    m_testGenModeData.format                  = PixelFormat::BayerRGGB;
    m_testGenModeData.numPixelsPerLine        = TFETestImageSizeWidth;
    m_testGenModeData.numLinesPerFrame        = TFETestImageSizeHeight;
    m_testGenModeData.resolution.outputWidth  = TFETestImageSizeWidth;
    m_testGenModeData.resolution.outputHeight = TFETestImageSizeHeight;
    m_testGenModeData.cropInfo.firstPixel     = 0;
    m_testGenModeData.cropInfo.firstLine      = 0;
    m_testGenModeData.cropInfo.lastPixel      = TFETestImageSizeWidth - 1;
    m_testGenModeData.cropInfo.lastLine       = TFETestImageSizeHeight - 1;
    m_testGenModeData.streamConfigCount       = 1;
    m_testGenModeData.streamConfig[0].type    = StreamType::IMAGE;
    m_testGenModeData.outPixelClock           = TPGPIXELCLOCK;
    m_testGenModeData.maxFPS                  = TPGFPS;

    // Add device indices
    result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeTFE, &deviceIndex, 1, &indicesLengthRequired);

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(indicesLengthRequired == 1);
        result = AddDeviceIndex(deviceIndex);
    }

    /// @todo (CAMX-4913) Update with tuningDumpDataSizeTFE xml after getting settings xml
    // If TFE tuning-data enable, initialize debug-data writer
    if ((CamxResultSuccess == result) &&
        (TRUE == GetStaticSettings()->enableTuningMetadata) &&
        (0 != GetStaticSettings()->tuningDumpDataSizeIFE))
    {
        // We would disable dual TFE when supporting tuning data
        m_pTuningMetadata = static_cast<IFETuningMetadata*>(CAMX_CALLOC(sizeof(IFETuningMetadata)));
        if (NULL == m_pTuningMetadata)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to allocate Tuning metadata.");
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            m_pDebugDataWriter = CAMX_NEW TDDebugDataWriter();
            if (NULL == m_pDebugDataWriter)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to allocate Tuning metadata.");
                result = CamxResultENoMemory;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        result = CheckForRDIOnly();

        // Check if stats node is connected/disabled
        if (FALSE == GetPipeline()->HasStatsNode())
        {
            m_hasStatsNode = FALSE;
        }
        else
        {
            m_hasStatsNode = TRUE;
        }

        m_OEMStatsConfig = GetStaticSettings()->IsOEMStatSettingEnable;
    }

    // Configure TFE Capability
    result = ConfigureTFECapability();

    if (CamxResultSuccess == result)
    {
        // For RDI only use case too we need to allocate cmd buffer for kernel use
        m_totalIQCmdSizeDWord += TFEKMDCmdBufferMaxSize;
    }

    // register to update config done for initial PCR
    pCreateOutputData->createFlags.willNotifyConfigDone = TRUE;

    m_genericBlobCmdBufferSizeBytes =
        (sizeof(TFEResourceHFRConfig) + (sizeof(TFEPortHFRConfig) * (MaxDefinedTFEOutputPorts - 1))) +  // HFR configuration
        sizeof(TFEResourceClockConfig) + (sizeof(UINT64) * (TFERDIMaxNum - 1)) +
        sizeof(TFEResourceBWConfig) + (sizeof(TFEResourceBWVote) * (TFERDIMaxNum - 1));

    // For RDI only use case too we need to allocate cmd buffer for kernel use
    m_totalIQCmdSizeDWord += TFEKMDCmdBufferMaxSize;

    if (CamxResultSuccess != result)
    {
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
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CAMX_UNREFERENCED_PARAM(pFinalizeInitializationData);

    // Save required static metadata
    GetStaticMetadata();

    if ((CSLPresilEnabled == GetCSLMode()) || (CSLPresilRUMIEnabled == GetCSLMode()) || (TRUE == IsTPGMode()))
    {
        m_pSensorModeData     = &m_testGenModeData;
        m_pSensorModeRes0Data = &m_testGenModeData;
    }
    else
    {
        GetSensorModeData(&m_pSensorModeData);
        GetSensorModeRes0Data(&m_pSensorModeRes0Data);

        if (NULL != m_pSensorCaps)
        {
            m_pOTPData = &m_pSensorCaps->OTPData;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Sensor static capabilities not available");
        }
    }

    if (NULL != m_pSensorModeData)
    {
        m_TFEinputResolution.horizontalFactor   = 1.0f;
        m_TFEinputResolution.verticalFactor     = 1.0f;
        m_TFEinputResolution.resolution.width   = m_pSensorModeData->cropInfo.lastPixel -
            m_pSensorModeData->cropInfo.firstPixel + 1;
        m_TFEinputResolution.resolution.height  = m_pSensorModeData->cropInfo.lastLine -
            m_pSensorModeData->cropInfo.firstLine + 1;
        m_TFEinputResolution.CAMIFWindow.left   = m_pSensorModeData->cropInfo.firstPixel;
        m_TFEinputResolution.CAMIFWindow.top    = m_pSensorModeData->cropInfo.firstLine;
        m_TFEinputResolution.CAMIFWindow.width  = m_pSensorModeData->cropInfo.lastPixel -
            m_pSensorModeData->cropInfo.firstPixel + 1;
        m_TFEinputResolution.CAMIFWindow.height = m_pSensorModeData->cropInfo.lastLine -
            m_pSensorModeData->cropInfo.firstLine + 1;

        PublishTFEInputToUsecasePool(&m_TFEinputResolution);
    }

    m_resourcePolicy = pFinalizeInitializationData->resourcePolicy;

    return CamxResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TFENode::PublishTFEInputToUsecasePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::PublishTFEInputToUsecasePool(
    TFEInputResolution* pTFEResolution)
{

    MetadataPool*           pPerUsecasePool = GetPerFramePool(PoolType::PerUsecase);
    MetadataSlot*           pPerUsecaseSlot = pPerUsecasePool->GetSlot(0);
    UsecasePropertyBlob*    pPerUsecaseBlob = NULL;
    CamxResult              result          = CamxResultSuccess;

    CAMX_ASSERT(NULL != pTFEResolution);

    /// @todo (CAMX-4913) Add tag for TFE once it is added to vendor tags
    const UINT  TFEResolutionTag[] = { PropertyIDUsecaseIFEInputResolution };
    const VOID* pData[1]           = { pTFEResolution };
    UINT        pDataCount[1]      = { sizeof(TFEInputResolution) };

    result = WriteDataList(TFEResolutionTag, pData, pDataCount, 1);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupFD, "Falied to publish TFE Downscale ratio uscasepool");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::InitializeOutputPathImageInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::InitializeOutputPathImageInfo()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::HardcodeSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TFENode::HardcodeSettings()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::HardcodeTintlessSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TFENode::HardcodeTintlessSettings()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::DynamicCAMIFCrop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TFENode::DynamicCAMIFCrop()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CreateCmdBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::CreateCmdBuffers()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::FetchCmdBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::FetchCmdBuffers()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::AddCmdBufferReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::AddCmdBufferReference()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ConfigBufferIO
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ConfigBufferIO()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CommitAndSubmitPacket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::CommitAndSubmitPacket()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CheckForRDIOnly
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::CheckForRDIOnly()
{

    UINT       outputPortId[MaxDefinedTFEOutputPorts];
    UINT       totalOutputPort = 0;
    CamxResult result          = CamxResultSuccess;

    // Set RDI only as default case
    m_RDIOnlyUseCase = TRUE;

    // Get Output Port List
    GetAllOutputPortIds(&totalOutputPort, &outputPortId[0]);

    if (totalOutputPort == 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Error: no output port");
        result = CamxResultEInvalidArg;
    }

    for (UINT index = 0; index < totalOutputPort; index++)
    {
        if ((TFEOutputPortRDI0 != outputPortId[index]) &&
            (TFEOutputPortRDI1 != outputPortId[index]) &&
            (TFEOutputPortRDI2 != outputPortId[index]))
        {
            m_RDIOnlyUseCase = FALSE;
            CAMX_LOG_VERBOSE(CamxLogGroupISP, " m_RDIOnlyUseCase False");
            break;
        }
    }
    CAMX_LOG_VERBOSE(CamxLogGroupISP, " m_RDIOnlyUseCase %d", m_RDIOnlyUseCase);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ReadDefaultStatsConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ReadDefaultStatsConfig()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ReleaseResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ReleaseResources(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CAMX_UNREFERENCED_PARAM(modeBitmask);

    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::UpdateInitIQSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::UpdateInitIQSettings()
{
    CamxResult result = CamxResultSuccess;

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::UpdateInitSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::UpdateInitSettings()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::AcquireResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::AcquireResources()
{
    CamxResult result = CamxResultSuccess;

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::NewActiveStreamsSetup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::NewActiveStreamsSetup(
    UINT activeStreamIdMask)
{
    CAMX_UNREFERENCED_PARAM(activeStreamIdMask);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::PostPipelineCreate()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::InitialSetupandConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::InitialSetupandConfig()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::UpdateIQStateConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::UpdateIQStateConfiguration()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::GetOEMIQSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::GetOEMIQSettings()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::GetOEMStatsConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::GetOEMStatsConfig()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::GetTintlessStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::GetTintlessStatus()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::Get3AFrameConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::Get3AFrameConfig()
{
    return CamxResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::UpdateCamifSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::UpdateCamifSettings()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CAMX_UNREFERENCED_PARAM(pExecuteProcessRequestData);

    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::SetupDeviceResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::SetupDeviceResource()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::SetupChannelResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::SetupChannelResource()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::MapPortIdToChannelId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::MapPortIdToChannelId()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::GetPDAFInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::GetPDAFInformation()
{
    UINT   sensorPDAFInfoTag[1] = { PropertyIDSensorPDAFInfo };
    VOID*  pDataOutput[]        = { 0 };
    UINT64 PDAFdataOffset[1]    = { 0 };

    GetDataList(sensorPDAFInfoTag, pDataOutput, PDAFdataOffset, 1);
    if (NULL != pDataOutput[0])
    {
        Utils::Memcpy(&m_ISPInputSensorData.sensorPDAFInfo, pDataOutput[0], sizeof(SensorPDAFInfo));
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "PDAF not enabled");
    }

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TFENode::IsPDHwEnabled()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::IsPDHwEnabled()
{
    BOOL isPDHwEnabled = FALSE;
    return isPDHwEnabled;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::AcquireDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::AcquireDevice()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::UpdateTFECapabilityBasedOnCameraPlatform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::UpdateTFECapabilityBasedOnCameraPlatform()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ConfigureTFECapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ConfigureTFECapability()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::HasOutputPortForIQModulePathConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::HasOutputPortForIQModulePathConfig() const
{
    BOOL hasOutputPort = FALSE;

    return hasOutputPort;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CreateTFEIQModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::CreateTFEIQModules()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CreateTFEStatsModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::CreateTFEStatsModules()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::Cleanup()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::Cleanup()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PopulateGeneralTuningMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::PopulateGeneralTuningMetadata()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::DumpTuningMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::DumpTuningMetadata()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ProgramIQEnable()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ProgramIQEnable()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PrepareBAFStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::PrepareBAFStatsMetadata()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PrepareAWBBGStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::PrepareAWBBGStatsMetadata()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PrepareAECBGStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::PrepareAECBGStatsMetadata()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PrepareBHistStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::PrepareBHistStatsMetadata()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PrepareTintlessBEStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::PrepareTintlessBEStatsMetadata()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PostMetadataRaw
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::PostMetadataRaw()
{
    CamxResult result = CamxResultSuccess;

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PostMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::PostMetadata()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PrepareTFEProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::PrepareTFEProperties()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PrepareTFEVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::PrepareTFEVendorTags()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PrepareTFEHALTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::PrepareTFEHALTags()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ComputeNeutralPoint
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ComputeNeutralPoint()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TFENode::PrepareStripingParameters()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::PrepareStripingParameters()
{
    CamxResult result = CamxResultSuccess;

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ProgramIQConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ProgramIQConfig()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::GetStaticMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::GetStaticMetadata()
{
    HwCameraInfo    cameraInfo;

    HwEnvironment::GetInstance()->GetCameraInfo(GetPipeline()->GetCameraId(), &cameraInfo);

    // Initialize default metadata
    m_HALTagsData.blackLevelLock                 = BlackLevelLockOff;
    m_HALTagsData.colorCorrectionMode            = ColorCorrectionModeFast;
    m_HALTagsData.controlAEMode                  = ControlAEModeOn;
    m_HALTagsData.controlAWBMode                 = ControlAWBModeAuto;
    m_HALTagsData.controlMode                    = ControlModeAuto;
    m_HALTagsData.controlPostRawSensitivityBoost = cameraInfo.pPlatformCaps->minPostRawSensitivityBoost;
    m_HALTagsData.noiseReductionMode             = NoiseReductionModeFast;
    m_HALTagsData.shadingMode                    = ShadingModeFast;
    m_HALTagsData.statisticsHotPixelMapMode      = StatisticsHotPixelMapModeOff;
    m_HALTagsData.statisticsLensShadingMapMode   = StatisticsLensShadingMapModeOff;
    m_HALTagsData.tonemapCurves.tonemapMode      = TonemapModeFast;

    // Cache sensor capability information for this camera
    m_pSensorCaps = cameraInfo.pSensorCaps;

    m_sensorActiveArrayWidth  = m_pSensorCaps->activeArraySize.width;
    m_sensorActiveArrayHeight = m_pSensorCaps->activeArraySize.height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::GetMetadataTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::GetMetadataTags()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ProgramStripeConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ProgramStripeConfig()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CalculateIQCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::CalculateIQCmdSize()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::SetDependencies()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{

    CAMX_ASSERT(NULL != pBufferNegotiationData);

    CamxResult      result                         = CamxResultSuccess;
    UINT32          optimalInputWidth              = 0;
    UINT32          optimalInputHeight             = 0;
    UINT32          minInputWidth                  = 0;
    UINT32          minInputHeight                 = 0;
    UINT32          maxInputWidth                  = 0;
    UINT32          maxInputHeight                 = 0;
    UINT32          perOutputPortOptimalWidth      = 0;
    UINT32          perOutputPortOptimalHeight     = 0;
    UINT32          perOutputPortMinWidth          = 0;
    UINT32          perOutputPortMinHeight         = 0;
    UINT32          perOutputPortMaxWidth          = TFEMaxOutputWidthFull * 2;
    UINT32          perOutputPortMaxHeight         = TFEMaxOutputHeightFull;
    UINT32          totalInputPorts                = 0;
    AlignmentInfo   alignmentLCM[FormatsMaxPlanes] = { { 0 } };
    UINT32          inputPortId[MaxTFEInputPorts];

    // Get Input Port List
    GetAllInputPortIds(&totalInputPorts, &inputPortId[0]);

    // The TFE node will have to loop through all the output ports which are connected to a child node or a HAL target.
    // The input buffer requirement will be the super resolution after looping through all the output ports.
    // The super resolution may have different aspect ratio compared to the original output port aspect ratio, but
    // this will be taken care of by the crop hardware associated with the output port.
    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        UINT                       outputPortIndex            =
            pBufferNegotiationData->pOutputPortNegotiationData[index].outputPortIndex;
        UINT                       outputPortId               = GetOutputPortId(outputPortIndex);

        if ((FALSE == IsStatsOutputPort(outputPortId)) &&
            (TRUE == IsPixelOutputPortSourceType(outputPortId)))
        {
            perOutputPortOptimalWidth  = 0;
            perOutputPortOptimalHeight = 0;
            perOutputPortMinWidth      = 0;
            perOutputPortMinHeight     = 0;
            perOutputPortMaxWidth      = TFEMaxOutputWidthFull * 2;
            perOutputPortMaxHeight     = TFEMaxOutputHeightFull;

            if (TRUE == m_RDIOnlyUseCase)
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "ignore Single TFE output limitatation for RDI only usecase");
                perOutputPortMaxWidth  = TFEMAXOutputWidthRDIOnly;
                perOutputPortMaxHeight = TFEMAXOutputHeightRDIOnly;
            }

            Utils::Memset(&pOutputPortNegotiationData->outputBufferRequirementOptions, 0, sizeof(BufferRequirement));

            // Go through the requirements of the destination ports connected to a given output port of TFE
            for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
            {
                BufferRequirement* pInputPortRequirement = &pOutputPortNegotiationData->inputPortRequirement[inputIndex];

                // Optimal width per port is the super resolution of all the connected destination ports' optimal needs.
                perOutputPortOptimalWidth  =
                    Utils::MaxUINT32(pInputPortRequirement->optimalWidth, perOutputPortOptimalWidth);
                perOutputPortOptimalHeight =
                    Utils::MaxUINT32(pInputPortRequirement->optimalHeight, perOutputPortOptimalHeight);
                perOutputPortMinWidth      =
                    Utils::MaxUINT32(pInputPortRequirement->minWidth, perOutputPortMinWidth);
                perOutputPortMinHeight     =
                    Utils::MaxUINT32(pInputPortRequirement->minHeight, perOutputPortMinHeight);
                perOutputPortMaxWidth      =
                    Utils::MinUINT32(pInputPortRequirement->maxWidth, perOutputPortMaxWidth);
                perOutputPortMaxHeight     =
                    Utils::MinUINT32(pInputPortRequirement->maxHeight, perOutputPortMaxHeight);

                for (UINT planeIdx = 0; planeIdx < FormatsMaxPlanes; planeIdx++)
                {
                    alignmentLCM[planeIdx].strideAlignment  =
                        Utils::CalculateLCM(
                            static_cast<INT32>(alignmentLCM[planeIdx].strideAlignment),
                            static_cast<INT32>(pInputPortRequirement->planeAlignment[planeIdx].strideAlignment));
                    alignmentLCM[planeIdx].scanlineAlignment =
                        Utils::CalculateLCM(
                            static_cast<INT32>(alignmentLCM[planeIdx].scanlineAlignment),
                            static_cast<INT32>(pInputPortRequirement->planeAlignment[planeIdx].scanlineAlignment));
                }
            }

            // Ensure optimal dimensions are within min and max dimensions. There are chances that the optimal dimension goes
            // over the max. Correct for the same.
            UINT32 originalOptimalWidth = perOutputPortOptimalWidth;
            perOutputPortOptimalWidth   =
                Utils::ClampUINT32(perOutputPortOptimalWidth, perOutputPortMinWidth, perOutputPortMaxWidth);

            // Calculate OptimalHeight if witdh is clamped.
            if (originalOptimalWidth != 0)
            {
                perOutputPortOptimalHeight =
                    Utils::RoundFLOAT(static_cast<FLOAT>(
                    (perOutputPortOptimalWidth * perOutputPortOptimalHeight) / originalOptimalWidth));
            }
            perOutputPortOptimalHeight =
                Utils::ClampUINT32(perOutputPortOptimalHeight, perOutputPortMinHeight, perOutputPortMaxHeight);

            // Store the buffer requirements for this output port which will be reused to set, during forward walk.
            // The values stored here could be final output dimensions unless it is overridden by forward walk.
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth  = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight = perOutputPortOptimalHeight;

            pOutputPortNegotiationData->outputBufferRequirementOptions.minWidth  = perOutputPortMinWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.minHeight = perOutputPortMinHeight;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxWidth  = perOutputPortMaxWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxHeight = perOutputPortMaxHeight;

            Utils::Memcpy(&pOutputPortNegotiationData->outputBufferRequirementOptions.planeAlignment[0],
                &alignmentLCM[0],
                sizeof(AlignmentInfo) * FormatsMaxPlanes);

            Utils::Memset(&alignmentLCM[0], 0, sizeof(AlignmentInfo) * FormatsMaxPlanes);

            optimalInputWidth  = Utils::MaxUINT32(perOutputPortOptimalWidth, optimalInputWidth);
            optimalInputHeight = Utils::MaxUINT32(perOutputPortOptimalHeight, optimalInputHeight);
            minInputWidth      = Utils::MaxUINT32(perOutputPortMinWidth, minInputWidth);
            minInputHeight     = Utils::MaxUINT32(perOutputPortMinHeight, minInputHeight);
            maxInputWidth      = Utils::MaxUINT32(perOutputPortMaxWidth, maxInputWidth);
            maxInputHeight     = Utils::MaxUINT32(perOutputPortMaxHeight, maxInputHeight);
        }
        else // Stats and DS ports do not take part in buffer negotiation
        {
            const ImageFormat* pImageFormat = GetOutputPortImageFormat(OutputPortIndex(outputPortId));

            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth  = pImageFormat->width;
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight = pImageFormat->height;
        }
    }

    if ((TRUE == IsTPGMode()) && (optimalInputWidth == 0) && (optimalInputHeight == 0))
    {
        optimalInputWidth  = m_testGenModeData.resolution.outputWidth;
        optimalInputHeight = m_testGenModeData.resolution.outputHeight;
        CAMX_LOG_INFO(CamxLogGroupISP, "TFE: TPG MODE: Width x Height = %d x %d", optimalInputWidth, optimalInputHeight);
    }

    CheckForRDIOnly();

    if ((FALSE == m_RDIOnlyUseCase) &&
        ((0 == optimalInputWidth) || (0 == optimalInputHeight)))
    {
        result = CamxResultEFailed;

        CAMX_LOG_ERROR(CamxLogGroupISP,
            "ERROR: Buffer Negotiation Failed, W:%d x H:%d!\n",
            optimalInputWidth,
            optimalInputHeight);
    }
    else
    {
        if ((minInputWidth > maxInputWidth) || (minInputHeight > maxInputHeight))
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Min > Max, unable to use current format");
            result = CamxResultEFailed;
        }

        // Ensure optimal dimensions are within min and max dimensions. There are chances that the optimal dimension goes
        // over the max. Correct for the same.
        optimalInputWidth  = Utils::ClampUINT32(optimalInputWidth, minInputWidth, maxInputWidth);
        optimalInputHeight = Utils::ClampUINT32(optimalInputHeight, minInputHeight, maxInputHeight);

        pBufferNegotiationData->numInputPorts = totalInputPorts;

        for (UINT input = 0; input < totalInputPorts; input++)
        {
            UINT currentInputPortSourceTypeId = GetInputPortSourceType(input);
            if ((PortSrcTypeUndefined != currentInputPortSourceTypeId) &&
                (PortSrcTypePixel     != currentInputPortSourceTypeId))
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "Skip input buffer options for port source type of non pixel.")
                    continue;
            }

            pBufferNegotiationData->inputBufferOptions[input].nodeId     = Type();
            pBufferNegotiationData->inputBufferOptions[input].instanceId = InstanceID();
            pBufferNegotiationData->inputBufferOptions[input].portId     = inputPortId[input];

            BufferRequirement* pInputBufferRequirement = &pBufferNegotiationData->inputBufferOptions[input].bufferRequirement;

            pInputBufferRequirement->optimalWidth  = optimalInputWidth;
            pInputBufferRequirement->optimalHeight = optimalInputHeight;
            // If OPE is enabling SIMO and if one of the output is smaller than the other, then the scale capabilities (min,max)
            // needs to be adjusted after accounting for the scaling needed on the smaller output port.
            pInputBufferRequirement->minWidth  = minInputWidth;
            pInputBufferRequirement->minHeight = minInputHeight;
            pInputBufferRequirement->maxWidth  = maxInputWidth;
            pInputBufferRequirement->maxHeight = maxInputHeight;

            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                "Buffer Negotiation dims, Port %d Optimal %d x %d, Min %d x %d, Max %d x %d\n",
                inputPortId[input],
                optimalInputWidth,
                optimalInputHeight,
                minInputWidth,
                minInputHeight,
                maxInputWidth,
                maxInputHeight);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::ExtractCAMIFDecimatedPattern
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::ExtractCAMIFDecimatedPattern()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PreparePDAFInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::PreparePDAFInformation()
{
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    UINT32 finalSelectedOutputWidth  = 0;
    UINT32 finalSelectedOutputHeight = 0;

    UINT32 fullPortOutputWidth  = 0;
    UINT32 fullPortOutputHeight = 0;

    UINT16 pixelSkipPattern           = 0;
    UINT16 lineSkipPattern            = 0;
    UINT32 numberOfPixels             = 0;
    UINT32 numberOfLines              = 0;
    UINT32 pdafPixelCount             = 0;
    UINT32 residualWidth              = 0;
    UINT32 residualHeight             = 0;
    UINT16 residualWidthPattern       = 0;
    UINT16 residualHeightPattern      = 0;
    UINT16 offsetX                    = 0;
    UINT16 offsetY                    = 0;
    FLOAT  perOutputPortAspectRatio   = 0.0f;
    FLOAT  inputSensorAspectRatio     = 0.0f;
    FLOAT  curStreamAspectRatio       = 0.0f;
    UINT32 perOutputPortOptimalWidth  = 0;
    UINT32 perOutputPortOptimalHeight = 0;
    UINT32 perOutputPortMinWidth      = 0;
    UINT32 perOutputPortMinHeight     = 0;
    UINT32 perOutputPortMaxWidth      = TFEMaxOutputWidthFull * 2;  // Since for DUAL TFE , Max width port width * 2
    UINT32 perOutputPortMaxHeight     = TFEMaxOutputHeightFull;

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    /// @todo (CAMX-561) Handle sensor mode changes per request. May need to keep an array of m_pSensorModeData[..]
    if ((CSLPresilEnabled == GetCSLMode()) || (CSLPresilRUMIEnabled == GetCSLMode()) || (TRUE == IsTPGMode()))
    {
        m_pSensorModeData     = &m_testGenModeData;
        m_pSensorModeRes0Data = &m_testGenModeData;
    }
    else
    {
        if (NULL == m_pSensorModeData)
        {
            GetSensorModeData(&m_pSensorModeData);
        }
    }
    if (NULL == m_pOTPData)
    {
        if (!((CSLPresilEnabled == GetCSLMode()) || (CSLPresilRUMIEnabled == GetCSLMode())))
        {
            if (NULL != m_pSensorCaps)
            {
                m_pOTPData = &m_pSensorCaps->OTPData;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Sensor static capabilities not available");
            }
        }
    }

    // Gets the PDAF Information from Sensor
    GetPDAFInformation();


    // The input can be from the sensor or a test pattern generator (CAMIF TPG or CSID TPG)
    UINT32 finalSelectedInputWidth  = m_pSensorModeData->resolution.outputWidth;
    UINT32 finalSelectedInputHeight = m_pSensorModeData->resolution.outputHeight;

    if (0 != finalSelectedInputHeight)
    {
        inputSensorAspectRatio = static_cast<FLOAT>(finalSelectedInputWidth) /
            static_cast<FLOAT>(finalSelectedInputHeight);
    }

    // CSID crop override
    if (TRUE == EnableCSIDCropOverridingForSingleTFE())
    {
        finalSelectedInputWidth = m_instanceProperty.TFECSIDWidth;

        if (TRUE == IsSensorModeFormatYUV(m_pSensorModeData->format))
        {
            finalSelectedInputWidth >>= 1;
        }
        finalSelectedInputHeight = m_instanceProperty.TFECSIDHeight;

        CAMX_LOG_INFO(CamxLogGroupISP, "CSID crop override: finalSelectedInputWidth = %d, finalSelectedInputHeight = %d",
            finalSelectedInputWidth, finalSelectedInputHeight);
    }

    CAMX_LOG_INFO(CamxLogGroupISP, "finalSelectedInputWidth = %d, finalSelectedInputHeight = %d",
        finalSelectedInputWidth, finalSelectedInputHeight);

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        UINT                       outputPortIndex            =
            pBufferNegotiationData->pOutputPortNegotiationData[index].outputPortIndex;
        UINT                       outputPortId               = GetOutputPortId(outputPortIndex);
        FLOAT newOutputPortAspectRatio                        = 0.0f;

        if ((FALSE == IsStatsOutputPort(outputPortId)) &&
            (TRUE == IsPixelOutputPortSourceType(outputPortId)))
        {
            perOutputPortOptimalWidth  = 0;
            perOutputPortOptimalHeight = 0;
            perOutputPortMinWidth      = 0;
            perOutputPortMinHeight     = 0;
            perOutputPortMaxWidth      = TFEMaxOutputWidthFull * 2;
            perOutputPortMaxHeight     = TFEMaxOutputHeightFull;
            perOutputPortAspectRatio   = 0.0f;

            if (TRUE == m_RDIOnlyUseCase)
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "ignore Single TFE output limitatation for RDI only usecase");
                perOutputPortMaxWidth  = TFEMAXOutputWidthRDIOnly;
                perOutputPortMaxHeight = TFEMAXOutputHeightRDIOnly;
            }

            // Go through the requirements of the destination ports connected to a given output port of IFE
            for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
            {
                BufferRequirement* pInputPortRequirement = &pOutputPortNegotiationData->inputPortRequirement[inputIndex];

                // Optimal width per port is the super resolution of all the connected destination ports' optimal needs.
                perOutputPortOptimalWidth  =
                    Utils::MaxUINT32(pInputPortRequirement->optimalWidth, perOutputPortOptimalWidth);
                perOutputPortOptimalHeight =
                    Utils::MaxUINT32(pInputPortRequirement->optimalHeight, perOutputPortOptimalHeight);

                if (FALSE == Utils::FEqual(pInputPortRequirement->optimalHeight, 0.0f))
                {
                    curStreamAspectRatio = static_cast<FLOAT>(pInputPortRequirement->optimalWidth) /
                        static_cast<FLOAT>(pInputPortRequirement->optimalHeight);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupISP, "pInputPortRequirement->optimalHeight is 0");
                }
                // Get new OutputPortAspectRatio because perOutputPortOptimalWidth/Height may be modified,
                // this may not be standard ratio, just for further checking
                if (FALSE == Utils::FEqual(perOutputPortOptimalHeight, 0.0f))
                {
                    newOutputPortAspectRatio = static_cast<FLOAT>(perOutputPortOptimalWidth) /
                        static_cast<FLOAT>(perOutputPortOptimalHeight);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupISP, "perOutputPortOptimalHeight is 0");
                }

                if ((perOutputPortAspectRatio != 0) &&
                    (perOutputPortAspectRatio != curStreamAspectRatio || perOutputPortAspectRatio != newOutputPortAspectRatio))
                {
                    FLOAT adjustAspectRatio = perOutputPortAspectRatio > curStreamAspectRatio ?
                        curStreamAspectRatio : perOutputPortAspectRatio;
                    adjustAspectRatio       = inputSensorAspectRatio > adjustAspectRatio ?
                        inputSensorAspectRatio : adjustAspectRatio;

                    if (TRUE == Utils::FEqualCoarse(adjustAspectRatio, newOutputPortAspectRatio))
                    {
                        // The dimensions are fine. Do nothing
                    }
                    else if (adjustAspectRatio > newOutputPortAspectRatio)
                    {
                        perOutputPortOptimalWidth =
                            Utils::EvenFloorUINT32(static_cast<UINT32>(perOutputPortOptimalHeight * adjustAspectRatio));
                        CAMX_LOG_INFO(CamxLogGroupISP, "NonConformant AspectRatio:%f Change Width %d using AR:%f",
                            newOutputPortAspectRatio, perOutputPortOptimalWidth, adjustAspectRatio);
                    }
                    else
                    {
                        perOutputPortOptimalHeight = Utils::EvenFloorUINT32(
                            static_cast<UINT32>(perOutputPortOptimalWidth / adjustAspectRatio));
                        CAMX_LOG_INFO(CamxLogGroupISP, "NonConformant AspectRatio:%f Change Height %d using AR:%f",
                            newOutputPortAspectRatio, perOutputPortOptimalHeight, adjustAspectRatio);
                    }
                }

                if (FALSE == Utils::FEqual(perOutputPortOptimalHeight, 0.0f))
                {
                    perOutputPortAspectRatio = static_cast<FLOAT>(perOutputPortOptimalWidth) /
                        static_cast<FLOAT>(perOutputPortOptimalHeight);
                }

                perOutputPortMinWidth  =
                    Utils::MaxUINT32(pInputPortRequirement->minWidth, perOutputPortMinWidth);
                perOutputPortMinHeight =
                    Utils::MaxUINT32(pInputPortRequirement->minHeight, perOutputPortMinHeight);
                perOutputPortMaxWidth  =
                    Utils::MinUINT32(pInputPortRequirement->maxWidth, perOutputPortMaxWidth);
                perOutputPortMaxHeight =
                    Utils::MinUINT32(pInputPortRequirement->maxHeight, perOutputPortMaxHeight);
            }

            // Ensure optimal dimensions are within min and max dimensions. There are chances that the optimal dimension goes
            // over the max. Correct for the same.
            perOutputPortOptimalWidth  =
                Utils::ClampUINT32(perOutputPortOptimalWidth, perOutputPortMinWidth, perOutputPortMaxWidth);
            perOutputPortOptimalHeight =
                Utils::ClampUINT32(perOutputPortOptimalHeight, perOutputPortMinHeight, perOutputPortMaxHeight);

            // Store the buffer requirements for this output port which will be reused to set, during forward walk.
            // The values stored here could be final output dimensions unless it is overridden by forward walk.
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth  = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight = perOutputPortOptimalHeight;
        }
    }

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        BufferProperties*          pOutputBufferProperties    = pOutputPortNegotiationData->pFinalOutputBufferProperties;
        UINT                       outputPortId               = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);

        // Assume only singleTFE, cap output buffer to TFE limitation
        /// @todo (CAMX-4913) Change to capResolutionForSingleTFE once settings xml is ready
        if (TRUE == pStaticSettings->capResolutionForSingleIFE)
        {
            if (pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth >(TFEMaxOutputWidthFull))
            {
                pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth = TFEMaxOutputWidthFull;
            }
        }
        finalSelectedOutputWidth  = pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth;
        finalSelectedOutputHeight = pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight;

        CAMX_LOG_INFO(CamxLogGroupISP, "outputPortId = %d, Output buffer requirement options: optimalWidth = %d",
            outputPortId, finalSelectedOutputWidth);
        CAMX_LOG_INFO(CamxLogGroupISP, "outputPortId = %d, Output buffer requirement options: optimalHeight = %d",
            outputPortId, finalSelectedOutputHeight);

        // If clipping needed, apply only for non-stats type output ports.
        if ((FALSE == IsStatsOutputPort(outputPortId)) &&
            (TRUE  == IsPixelOutputPortSourceType(outputPortId)))
        {
            /// Clip if output greater than input since IFE cannot do upscale.
            if (finalSelectedOutputWidth > finalSelectedInputWidth)
            {
                finalSelectedOutputWidth = finalSelectedInputWidth;

                CAMX_LOG_INFO(CamxLogGroupISP, "outputPortId = %d, Output buffer clipped width = %d",
                    outputPortId, finalSelectedOutputWidth);
            }

            /// Clip if output greater than input, clamp since IFE cannot do upscale.
            if (finalSelectedOutputHeight > finalSelectedInputHeight)
            {
                finalSelectedOutputHeight = finalSelectedInputHeight;

                CAMX_LOG_INFO(CamxLogGroupISP, "outputPortId = %d, Output buffer clipped height = %d",
                    outputPortId, finalSelectedOutputHeight);
            }
        }

        // Dont apply scale limits for nonstats ports
        if ((FALSE == IsStatsOutputPort(outputPortId)) &&
            (TRUE  == IsPixelOutputPortSourceType(outputPortId)))
        {
            /// If downscale ratio is beyond tfe limits, cap the output dimension.
            if (0 != finalSelectedOutputWidth)
            {
                if ((finalSelectedInputWidth / finalSelectedOutputWidth) > TFEMaxDownscaleLimt)
                {
                    finalSelectedOutputWidth = static_cast<UINT32>(finalSelectedInputWidth * TFEMaxDownscaleLimt);
                    CAMX_LOG_WARN(CamxLogGroupISP, "Scaleratio beyond limit, inp height = %d, out height = %d",
                        finalSelectedInputWidth, finalSelectedOutputWidth);
                }
            }
            if (0 != finalSelectedOutputHeight)
            {
                if (finalSelectedInputHeight / finalSelectedOutputHeight > TFEMaxDownscaleLimt)
                {
                    finalSelectedOutputHeight = static_cast<UINT32>(finalSelectedInputHeight * TFEMaxDownscaleLimt);
                    CAMX_LOG_WARN(CamxLogGroupISP, "Scaleratio beyond limit, inp height = %d, out height = %d",
                        finalSelectedInputHeight, finalSelectedOutputHeight);
                }
            }
        }
        UINT outputPortSourceTypeId = GetOutputPortSourceType(pOutputPortNegotiationData->outputPortIndex);
        BOOL isSinkPortWithBuffer   = IsSinkPortWithBuffer(pOutputPortNegotiationData->outputPortIndex);

        if (FALSE == isSinkPortWithBuffer)
        {
            switch (outputPortId)
            {
                case TFEOutputPortFull:
                    pOutputBufferProperties->imageFormat.width  = finalSelectedOutputWidth;
                    pOutputBufferProperties->imageFormat.height = finalSelectedOutputHeight;

                    Utils::Memcpy(&pOutputBufferProperties->imageFormat.planeAlignment[0],
                        &pOutputPortNegotiationData->outputBufferRequirementOptions.planeAlignment[0],
                        sizeof(AlignmentInfo) * FormatsMaxPlanes);
                    break;

                /// @note * 2 below for stats is for handling dual TFE (conservatively). This can be optimized.
                case TFEOutputPortStatsAECBG:
                    pOutputBufferProperties->imageFormat.width  = AECBGTFEStatsMaxWidth * 2;
                    pOutputBufferProperties->imageFormat.height = AECBGTFEStatsMaxHeight;
                    break;

                case TFEOutputPortStatsAWBBG:
                    pOutputBufferProperties->imageFormat.width  = AWBBGTFEStatsMaxWidth * 2;
                    pOutputBufferProperties->imageFormat.height = AWBBGTFEStatsMaxHeight;
                    break;

                case TFEOutputPortStatsTLBE:
                    pOutputBufferProperties->imageFormat.width  = TintlessBETFEStatsWidth * 2;
                    pOutputBufferProperties->imageFormat.height = TintlessBETFEStatsHeight;
                    break;

                case TFEOutputPortStatsBHIST:
                    pOutputBufferProperties->imageFormat.width  = BHistTFEStatsWidth * 2;
                    pOutputBufferProperties->imageFormat.height = 1;
                    break;

                case TFEOutputPortStatsBAF:
                    pOutputBufferProperties->imageFormat.width  = BAFTFEStatsMaxWidth * 2;
                    pOutputBufferProperties->imageFormat.height = BAFTFEStatsMaxHeight;
                    break;

                    // For RDI output port cases, set the output buffer dimension to the TFE input buffer dimension.
                case TFEOutputPortRDI0:
                case TFEOutputPortRDI1:
                case TFEOutputPortRDI2:
                    pOutputBufferProperties->imageFormat.width  = finalSelectedInputWidth;
                    pOutputBufferProperties->imageFormat.height = finalSelectedInputHeight;

                    // Overwrite if the RDI port is associated with PDAF port source type.
                    if (PortSrcTypePDAF == outputPortSourceTypeId)
                    {
                        UINT streamIndex;
                        if (TRUE == FindSensorStreamConfigIndex(StreamType::PDAF, &streamIndex))
                        {
                            UINT32 PDAFWidth  = m_pSensorModeData->streamConfig[streamIndex].frameDimension.width;
                            UINT32 PDAFHeight = m_pSensorModeData->streamConfig[streamIndex].frameDimension.height;

                            // For type PDAF Type2 or 2PD SW-based type (i.e. dual PD) over RDI port case
                            if (Format::RawPlain16 == pOutputBufferProperties->imageFormat.format)
                            {

                                // Read PDAFBufferFormat from sensor PDAF Info
                                const PDBufferFormat sensorPDBufferFormat =
                                    m_ISPInputSensorData.sensorPDAFInfo.PDAFBufferFormat;

                                // Check NativeBufferFormat if Sensor is PDAF Type2/DualPD
                                if (PDLibSensorType2  ==
                                    static_cast<PDLibSensorType>(m_ISPInputSensorData.sensorPDAFInfo.PDAFSensorType) ||
                                    PDLibSensorDualPD ==
                                    static_cast<PDLibSensorType>(m_ISPInputSensorData.sensorPDAFInfo.PDAFSensorType))
                                {
                                    // Read PDAFNativeBufferFormat from sensor PDAF Info
                                    const PDBufferFormat sensorPDNativeBufferFormat =
                                        m_ISPInputSensorData.sensorPDAFInfo.PDAFNativeBufferFormat;

                                    switch (sensorPDNativeBufferFormat)
                                    {
                                        case PDBufferFormat::UNPACKED16:
                                            // Need to handle the padding case for the default blob sensor
                                            pOutputBufferProperties->imageFormat.width  = PDAFWidth * PDAFHeight * 2;
                                            pOutputBufferProperties->imageFormat.height = 1;
                                            pOutputBufferProperties->imageFormat.format = Format::Blob;
                                            break;

                                        case PDBufferFormat::MIPI10:

                                            switch (sensorPDBufferFormat)
                                            {
                                                case PDBufferFormat::UNPACKED16:
                                                    pOutputBufferProperties->imageFormat.width  = PDAFWidth;
                                                    pOutputBufferProperties->imageFormat.height = PDAFHeight;
                                                    pOutputBufferProperties->imageFormat.format = Format::RawPlain16;
                                                    break;
                                                default:
                                                    pOutputBufferProperties->imageFormat.width  = PDAFWidth;
                                                    pOutputBufferProperties->imageFormat.height = PDAFHeight;
                                                    break;
                                            }
                                            break;

                                        default:
                                            CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported PDNativeBufferFormat = %d",
                                                sensorPDNativeBufferFormat);
                                            break;
                                    }
                                    CAMX_LOG_VERBOSE(CamxLogGroupISP, "PDNativeBufferFormat = %d", sensorPDNativeBufferFormat);
                                }
                                else
                                {
                                    switch (sensorPDBufferFormat)
                                    {
                                        case PDBufferFormat::UNPACKED16:
                                            // Need to handle the padding case for the default blob sensor
                                            pOutputBufferProperties->imageFormat.width  = PDAFWidth * PDAFHeight * 2;
                                            pOutputBufferProperties->imageFormat.height = 1;
                                            pOutputBufferProperties->imageFormat.format = Format::Blob;
                                            break;

                                        case PDBufferFormat::MIPI10:
                                            pOutputBufferProperties->imageFormat.width  = PDAFWidth;
                                            pOutputBufferProperties->imageFormat.height = PDAFHeight;
                                            break;

                                        default:
                                            CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported PDBufferFormat = %d",
                                                sensorPDBufferFormat);
                                            break;
                                    }
                                }
                                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                    "PDAF buffer format = RawPlain16, PDBufferFormat = %d, "
                                    "width = %u, height = %u",
                                    sensorPDBufferFormat,
                                    pOutputBufferProperties->imageFormat.width,
                                    pOutputBufferProperties->imageFormat.height);
                            }
                            else if (Format::RawPlain64 == pOutputBufferProperties->imageFormat.format)
                            {
                                pOutputBufferProperties->imageFormat.width  = PDAFWidth;
                                pOutputBufferProperties->imageFormat.height = PDAFHeight;

                                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                    "PDAF buffer format = RawPlain16, width = %u, height = %u",
                                    pOutputBufferProperties->imageFormat.width,
                                    pOutputBufferProperties->imageFormat.height);
                            }
                            else
                            {
                                // Unsupported PDAF format
                                pOutputBufferProperties->imageFormat.width  = 0;
                                pOutputBufferProperties->imageFormat.height = 0;

                                CAMX_LOG_ERROR(CamxLogGroupISP,
                                    "Unspported PDAF buffer format = %u",
                                    pOutputBufferProperties->imageFormat.format);
                            }
                        }
                        else
                        {
                            pOutputBufferProperties->imageFormat.width         = 0;
                            pOutputBufferProperties->imageFormat.height         = 0;
                            pOutputBufferProperties->immediateAllocImageBuffers = 0;
                            CAMX_LOG_INFO(CamxLogGroupISP, "PDAF stream not configured by sensor");

                        }
                    }
                    else if (PortSrcTypeMeta == outputPortSourceTypeId)
                    {
                        UINT streamIndex;
                        if (TRUE == FindSensorStreamConfigIndex(StreamType::META, &streamIndex))
                        {
                            UINT32 metaWidth  = m_pSensorModeData->streamConfig[streamIndex].frameDimension.width;
                            UINT32 metaHeight = m_pSensorModeData->streamConfig[streamIndex].frameDimension.height;

                            if (Format::Blob == pOutputBufferProperties->imageFormat.format)
                            {
                                // The default blob sensor
                                pOutputBufferProperties->imageFormat.width  = metaWidth * metaHeight;
                                pOutputBufferProperties->imageFormat.height = 1;

                                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                    "Meta buffer format = Blob, width = %u, height = %u",
                                    pOutputBufferProperties->imageFormat.width,
                                    pOutputBufferProperties->imageFormat.height);
                            }
                            else
                            {
                                // Unsupported Meta buffer format
                                pOutputBufferProperties->imageFormat.width  = 0;
                                pOutputBufferProperties->imageFormat.height = 0;

                                CAMX_LOG_ERROR(CamxLogGroupISP,
                                    "Unspported meta buffer format = %u",
                                    pOutputBufferProperties->imageFormat.format);
                            }
                        }
                    }
                    else if (PortSrcTypeHDR == outputPortSourceTypeId)
                    {
                        UINT streamIndex;
                        if (TRUE == FindSensorStreamConfigIndex(StreamType::HDR, &streamIndex))
                        {
                            UINT32 HDRWidth  = m_pSensorModeData->streamConfig[streamIndex].frameDimension.width;
                            UINT32 HDRHeight = m_pSensorModeData->streamConfig[streamIndex].frameDimension.height;

                            if (Format::Blob == pOutputBufferProperties->imageFormat.format)
                            {
                                // The default blob sensor
                                pOutputBufferProperties->imageFormat.width  = HDRWidth * HDRHeight;
                                pOutputBufferProperties->imageFormat.height = 1;

                                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                    "HDR buffer format = Blob, width = %u, height = %u",
                                    pOutputBufferProperties->imageFormat.width,
                                    pOutputBufferProperties->imageFormat.height);
                            }
                            else if (Format::RawMIPI == pOutputBufferProperties->imageFormat.format)
                            {
                                pOutputBufferProperties->imageFormat.width  = HDRWidth;
                                pOutputBufferProperties->imageFormat.height = HDRHeight;

                                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                    "HDR buffer format = RawMIPI, width = %u, height = %u",
                                    pOutputBufferProperties->imageFormat.width,
                                    pOutputBufferProperties->imageFormat.height);
                            }
                            else
                            {
                                // Unsupported Meta buffer format
                                pOutputBufferProperties->imageFormat.width  = 0;
                                pOutputBufferProperties->imageFormat.height = 0;

                                CAMX_LOG_ERROR(CamxLogGroupISP,
                                    "Unspported HDR buffer format = %u",
                                    pOutputBufferProperties->imageFormat.format);
                            }
                        }
                    }
                    break;

                case TFEOutputPortRawDump:
                    pOutputBufferProperties->imageFormat.width  = finalSelectedInputWidth;
                    pOutputBufferProperties->imageFormat.height = finalSelectedInputHeight;
                    break;

                case TFEOutputPortPDAF:
                    pdafPixelCount = m_ISPInputSensorData.sensorPDAFInfo.PDAFPixelCount;


                    for (UINT coordinate = 0;
                        (coordinate < pdafPixelCount) && (coordinate < TFEMaxPdafPixelsPerBlock);
                        coordinate++)
                    {
                        pixelSkipPattern |=
                            (1 << (m_ISPInputSensorData.sensorPDAFInfo.PDAFPixelCoords[coordinate].PDXCoordinate % 16));
                        lineSkipPattern  |=
                            (1 << (m_ISPInputSensorData.sensorPDAFInfo.PDAFPixelCoords[coordinate].PDYCoordinate % 16));
                    }

                    // Find number of pixels
                    numberOfPixels = GetPixelsInSkipPattern(pixelSkipPattern);
                    numberOfLines  = GetPixelsInSkipPattern(lineSkipPattern);

                    offsetX = m_ISPInputSensorData.CAMIFCrop.firstPixel % 16;
                    offsetY = m_ISPInputSensorData.CAMIFCrop.firstLine % 16;

                    pixelSkipPattern = ((pixelSkipPattern << offsetX) | (pixelSkipPattern >> (16 - offsetX)));
                    lineSkipPattern  = ((lineSkipPattern << offsetY) | (lineSkipPattern >> (16 - offsetY)));

                    m_PDAFInfo.bufferWidth  = (m_pSensorModeData->resolution.outputWidth / 16) * numberOfPixels;
                    m_PDAFInfo.bufferHeight = (m_pSensorModeData->resolution.outputHeight / 16) * numberOfLines;

                    residualWidth  = m_pSensorModeData->resolution.outputWidth % 16;
                    residualHeight = m_pSensorModeData->resolution.outputHeight % 16;

                    if (0 != residualWidth)
                    {
                        residualWidthPattern = ((static_cast<UINT16>(~0)) >> (16 - residualWidth));
                    }

                    if (0 != residualHeight)
                    {
                        residualHeightPattern = ((static_cast<UINT16>(~0)) >> (16 - residualHeight));
                    }

                    m_PDAFInfo.bufferWidth                      +=
                        GetPixelsInSkipPattern(pixelSkipPattern & residualWidthPattern);
                    m_PDAFInfo.bufferHeight                     +=
                        GetPixelsInSkipPattern(lineSkipPattern & residualHeightPattern);
                    m_PDAFInfo.pixelSkipPattern                 = pixelSkipPattern;
                    m_PDAFInfo.lineSkipPattern                  = lineSkipPattern;
                    m_PDAFInfo.enableSubsample                  = CheckIfPDAFType3Supported();
                    pOutputBufferProperties->imageFormat.width  = m_PDAFInfo.bufferWidth;
                    pOutputBufferProperties->imageFormat.height = m_PDAFInfo.bufferHeight;

                    PreparePDAFInformation();

                    CAMX_LOG_VERBOSE(CamxLogGroupISP, "PDAF Buffer Width %d Height %d",
                        m_PDAFInfo.bufferWidth, m_PDAFInfo.bufferHeight);
                    break;

                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Unhandled output portID");
                    break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::IsSensorModeFormatBayer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::IsSensorModeFormatBayer(
    PixelFormat format
    ) const
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
// TFENode::IsSensorModeFormatMono
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::IsSensorModeFormatMono(
    PixelFormat format
    ) const
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
/// TFENode::IsSensorModeFormatYUV
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::IsSensorModeFormatYUV(
    PixelFormat format
    ) const
{
    BOOL isYUV = FALSE;

    if ((PixelFormat::YUVFormatUYVY == format) ||
        (PixelFormat::YUVFormatYUYV == format))
    {
        isYUV = TRUE;
    }

    return isYUV;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TFENode::IsTPGMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::IsTPGMode()
{
    BOOL isTPG = FALSE;
    const StaticSettings*   pSettings = GetStaticSettings();
    if (NULL != pSettings)
    {
        isTPG = pSettings->enableTPG;
    }

    return isTPG;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::EvaluateDualTFEMode
IFEModuleMode DualTFEUtils::EvaluateDualTFEMode()
{
    IFEModuleMode mode = IFEModuleMode::SingleIFENormal;

    return mode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::FillCfgFromOneStripe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DualTFEUtils::FillCfgFromOneStripe()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::FetchCfgWithStripeOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DualTFEUtils::FetchCfgWithStripeOutput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::ComputeSplitParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DualTFEUtils::ComputeSplitParams()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::GetDefaultDualTFEStatsConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DualTFEUtils::GetDefaultDualTFEStatsConfig()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::ReleaseDualTFEPassResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DualTFEUtils::ReleaseDualTFEPassResult()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::PrintDualTFEInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DualTFEUtils::PrintDualTFEInput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::PrintDualTFEOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DualTFEUtils::PrintDualTFEOutput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::PrintDualTFEFrame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DualTFEUtils::PrintDualTFEFrame()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::UpdateDualTFEConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DualTFEUtils::UpdateDualTFEConfig()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualTFEUtils::UpdateStripingInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DualTFEUtils::UpdateStripingInput()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DualTFEUtils::TranslateOutputFormatToStripingLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int16_t DualTFEUtils::TranslateOutputFormatToStripingLib()
{
    int16_t format = IMAGE_FORMAT_INVALID;

    return format;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DualTFEUtils::TranslateInputFormatToStripingLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int16_t DualTFEUtils::TranslateInputFormatToStripingLib()
{
    int16_t format = IMAGE_FORMAT_INVALID;

    return format;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::HardcodeSettingsSetDefaultBAFFilterInputConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::HardcodeSettingsSetDefaultBAFFilterInputConfig()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::HardcodeSettingsSetDefaultBAFROIInputConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::HardcodeSettingsSetDefaultBAFROIInputConfig()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::HardcodeSettingsBAFStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TFENode::HardcodeSettingsBAFStats()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::SetupHFRInitialConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::SetupHFRInitialConfig()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::SetupBusReadInitialConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::SetupBusReadInitialConfig()
{
    CamxResult result = CamxResultSuccess;

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::GetTFEInputWidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 TFENode::GetTFEInputWidth()
{
    UINT32 width = 0;

    return width;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CalculatePixelClockRate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 TFENode::CalculatePixelClockRate()
{
    UINT64  pixClockHz = 0;

    return pixClockHz;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::GetPixelClockRate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 TFENode::GetPixelClockRate()
{
    UINT64 pixClockHz = 0;

    return pixClockHz;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::PrepareStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::PrepareStreamOn()
{
    // As of now, nothing to do. Can be used later for setup that is needed before stream.
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::OnStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::OnStreamOff(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CAMX_UNREFERENCED_PARAM(modeBitmask);

    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CalculateRDIClockRate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 TFENode::CalculateRDIClockRate()
{
    UINT64 clockRate = 0;

    return clockRate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CalculateSensorLineDuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT TFENode::CalculateSensorLineDuration()
{
    FLOAT lineDuration = 0;

    return lineDuration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::GetOverrideBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::GetOverrideBandwidth()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CalculatePixelPortLineBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 TFENode::CalculatePixelPortLineBandwidth()
{
    UINT64 camnocBWbytes = 0;

    return camnocBWbytes;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CalculateBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::CalculateBandwidth()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::SetupResourceClockConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::SetupResourceClockConfig()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::SetupResourceBWConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::SetupResourceBWConfig()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::CanSkipAlgoProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TFENode::CanSkipAlgoProcessing() const
{
    BOOL skipRequired = FALSE;

    return skipRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TFENode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TFENode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CAMX_UNREFERENCED_PARAM(pPublistTagList);

    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TFENode::GetMetadataContrastLevel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TFENode::GetMetadataContrastLevel()
{
}

CAMX_NAMESPACE_END
