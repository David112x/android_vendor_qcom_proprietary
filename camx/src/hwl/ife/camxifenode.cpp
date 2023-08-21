////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifenode.cpp
/// @brief IFE Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-1295) - Need to figure out the proper way to deal with TPG/SensorEmulation + RealDevice/Presil
#include "camxcsljumptable.h"
#include "camxcslresourcedefs.h"
#include "camxthreadmanager.h"
#include "camxhal3module.h"
#include "camxtrace.h"
#include "camxvendortags.h"
#include "camxpipeline.h"
#include "parametertuningtypes.h"
#include "camxifehvx.h"
#include "camximagesensormoduledata.h"
#include "camxiqinterface.h"
#include "camxtitan17xdefs.h"
#include "camxifenode.h"
#include "camxswtmc11.h"
#include "camxtitan150ife.h"
#include "camxtitan170ife.h"
#include "camxtitan175ife.h"
#include "camxtitan480ife.h"
#include "camxtuningdatamanager.h"

#define STRIPE_FIELD_PRINT(fd, field)    \
    CAMX_LOG_TO_FILE(fd, 0, "dualIFE %s = %d", #field, (field))

#define STRIPE_FIELD_PRINT_LL(fd, field) \
    CAMX_LOG_TO_FILE(fd, 0, "dualIFE %s = %lld", #field, (field))

#define IFE_SETTING_DUMP(fd, field)   \
    CAMX_LOG_TO_FILE(fd, 0, "IFE Settings %s = %d", #field, (field))

#define IFE_SETTING_DUMP_LL(fd, field)   \
    CAMX_LOG_TO_FILE(fd, 0, "dualIFE %s = %lld", #field, (field))

#define IFE_SETTING_DUMP_FLOAT(fd, field)   \
    CAMX_LOG_TO_FILE(fd, 0, "dualIFE %s = %f", #field, (field))

#define PRINT_CROP_1D(fd, _in)                       \
do                                               \
{                                                \
    STRIPE_FIELD_PRINT(fd, _in.enable);              \
    STRIPE_FIELD_PRINT(fd, _in.inDim);               \
    STRIPE_FIELD_PRINT(fd, _in.firstOut);            \
    STRIPE_FIELD_PRINT(fd, _in.lastOut);             \
} while (0, 0)


#define PRINT_MNDS_V16(fd, _in)                       \
do                                                \
{                                                 \
    STRIPE_FIELD_PRINT(fd, _in.enable);               \
    STRIPE_FIELD_PRINT(fd, _in.input);                \
    STRIPE_FIELD_PRINT(fd, _in.output);               \
    STRIPE_FIELD_PRINT(fd, _in.pixelOffset);          \
    STRIPE_FIELD_PRINT(fd, _in.cntInit);              \
    STRIPE_FIELD_PRINT(fd, _in.interpReso);           \
    STRIPE_FIELD_PRINT(fd, _in.roundingOptionVer);    \
    STRIPE_FIELD_PRINT(fd, _in.roundingOptionHor);    \
    STRIPE_FIELD_PRINT(fd, _in.rightPadEnable);       \
    STRIPE_FIELD_PRINT(fd, _in.inputProcessedLength); \
    STRIPE_FIELD_PRINT(fd, _in.phaseInit);            \
    STRIPE_FIELD_PRINT(fd, _in.phaseStep);            \
} while (0, 0)

#define PRINT_MNDS_V20(fd, _in)                      \
do                                               \
{                                                \
    STRIPE_FIELD_PRINT(fd, _in.enable);              \
    STRIPE_FIELD_PRINT(fd, _in.input);               \
    STRIPE_FIELD_PRINT(fd, _in.output);              \
    STRIPE_FIELD_PRINT(fd, _in.pixelOffset);         \
    STRIPE_FIELD_PRINT(fd, _in.interpReso);          \
    STRIPE_FIELD_PRINT(fd, _in.roundingOptionVer);   \
    STRIPE_FIELD_PRINT(fd, _in.roundingOptionHor);   \
    STRIPE_FIELD_PRINT(fd, _in.dropFirstOutput);   \
    STRIPE_FIELD_PRINT(fd, _in.phaseInit);           \
    STRIPE_FIELD_PRINT(fd, _in.phaseStep);           \
} while (0, 0)


#define PRINT_WE_STRIPE(fd, _in)                        \
do                                                  \
{                                                   \
    STRIPE_FIELD_PRINT(fd, _in.enable1);                \
    STRIPE_FIELD_PRINT(fd, _in.hInit1);                 \
    STRIPE_FIELD_PRINT(fd, _in.stripeWidth1);           \
    STRIPE_FIELD_PRINT(fd, _in.enable2);                \
    STRIPE_FIELD_PRINT(fd, _in.hInit2);                 \
    STRIPE_FIELD_PRINT(fd, _in.stripeWidth2);           \
} while (0, 0)

CAMX_NAMESPACE_BEGIN

// TPG defaults, need to derive the value based on the dimension and FPS
static const UINT   TPGPIXELCLOCK   = 400000000;
static const FLOAT  TPGFPS          = 30.0f;

static UINT64 IFEStatsModuleOutputPorts[] =
{
    IFEOutputPortStatsRS,
    IFEOutputPortStatsCS,
    IFEOutputPortStatsIHIST,
    IFEOutputPortStatsBHIST,
    IFEOutputPortStatsHDRBE,
    IFEOutputPortStatsHDRBHIST,
    IFEOutputPortStatsTLBG,
    IFEOutputPortStatsBF,
    IFEOutputPortStatsAWBBG
};

/// @brief Command buffer identifiers
enum class IFECmdBufferId: UINT32
{
    Packet = 0,       ///< Packet
    Main,             ///< Main IQ command buffer
    Left,             ///< Left IQ command buffer
    Right,            ///< Right IQ command buffer
    DMI32,            ///< DMI32 command buffer
    DMI64,            ///< DMI64 command buffer
    DualConfig,       ///< Dual IFE config command buffer
    GenericBlob,      ///< Generic command buffer
    LeftGenericBlob,  ///< Left Blob Command Buffer
    RightGenericBlob, ///< Right Blob Command Buffer
    RegDumpBuffer,    ///< Per Frame Reg Dump Buffers
    FlushDumpBuffer,  ///< Flush Time Reg Dump Buffers
    HangDumpBuffer,   ///< Hang or Error time Buffer
    NumCmdBuffers     ///< Max number of command buffers
};

static const UINT   IFEKMDCmdBufferMaxSize  = 8 * 1024;                 ///< Reserved KMD Cmd Buffer Size

UINT32 CGCOnCmds[] =
{
    regIFE_IFE_0_VFE_MODULE_LENS_CGC_OVERRIDE,    0xFFFFFFFF,
    regIFE_IFE_0_VFE_MODULE_STATS_CGC_OVERRIDE,   0xFFFFFFFF,
    regIFE_IFE_0_VFE_MODULE_COLOR_CGC_OVERRIDE,   0xFFFFFFFF,
    regIFE_IFE_0_VFE_MODULE_ZOOM_CGC_OVERRIDE,    0xFFFFFFFF,
    regIFE_IFE_0_VFE_MODULE_BUS_CGC_OVERRIDE,     0xFFFFFFFF,
    regIFE_IFE_0_VFE_MODULE_DUAL_PD_CGC_OVERRIDE, 0x1
};

// @brief list of usecase tags published by IFE node
static const UINT32 IFEUsecaseTags[] =
{
    PropertyIDUsecaseIFEPDAFInfo,
};

static const UINT32 IFECGCNumRegs = sizeof(CGCOnCmds) / (2 * sizeof(UINT32));

static const UINT32 IFELeftPartialTileShift     = IFE_IFE_0_BUS_WR_CLIENT_3_TILE_CFG_PARTIAL_TILE_LEFT_SHIFT;
static const UINT32 IFELeftPartialTileMask      = IFE_IFE_0_BUS_WR_CLIENT_3_TILE_CFG_PARTIAL_TILE_LEFT_MASK;
static const UINT32 IFERightPartialTileShift    = IFE_IFE_0_BUS_WR_CLIENT_3_TILE_CFG_PARTIAL_TILE_RIGHT_SHIFT;
static const UINT32 IFERightPartialTileMask     = IFE_IFE_0_BUS_WR_CLIENT_3_TILE_CFG_PARTIAL_TILE_RIGHT_MASK;

// Make this a setting
static const BOOL IFEOverwriteModuleWithStriping = TRUE;    ///< Overwrite IQ modules using striping output

static const UINT32 IFEMaxPdafPixelsPerBlock     = (16 * 16); ///< Maximum Pdaf Pixels Per block

static const UINT32 DefaultIHistStatsRegionWidth    = 2;
static const UINT32 DefaultHDRBHistStatsRegionWidth = 2;
static const UINT32 DefaultBHistStatsRegionWidth    = 2;
static const UINT32 DefaultBFGammaEntries           = 32;

static const UINT32 DefaultDMISelShift      = 63;
static const UINT32 DefaultDMIIndexShift    = 54;
static const UINT32 DefaultDMIIndexBits     = 0xFF;     // 8-bit mask [61:54]
static const UINT32 DefaultDMILeftShift     = 41;
static const UINT32 DefaultDMILeftBits      = 0x1FFF;   // 13-bit mask
static const UINT32 DefaultDMITopShift      = 27;
static const UINT32 DefaultDMITopBits       = 0x3FFF;   // 14-bit mask
static const UINT32 DefaultDMIWidthShift    = 14;
static const UINT32 DefaultDMIWidthBits     = 0xFFF;    // 12-bit mask
static const UINT32 DefaultDMIHeightShift   = 0;
static const UINT32 DefaultDMIHeightBits    = 0x1FFF;   // 13-bit mask

static const UINT32 MaxCommandBuffersDualIFE = 10;
static const UINT32 MaxCommandBuffers        = 5;

static const UINT32 SingleIFE = 1;
static const UINT32 DualIFE   = 2;

// Default minimum horizontal blanking lanes.
// This is same across targets
static const UINT32 IFEDefaultMinHBI    = 64;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TranslateFormatToISPImageFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 TranslateFormatToISPImageFormat(
    CamX::Format format,
    UINT8        CSIDecodeBitWidth)
{
    UINT32 ISPFormat = ISPFormatUndefined;

    switch (format)
    {
        case CamX::Format::YUV420NV12:
            ISPFormat = ISPFormatNV12;
            break;

        case CamX::Format::YUV420NV21:
            ISPFormat = ISPFormatNV21;
            break;

        case CamX::Format::UBWCTP10:
            ISPFormat = ISPFormatUBWCTP10;
            break;

        case CamX::Format::UBWCNV12:
        case CamX::Format::UBWCNV12FLEX:
            ISPFormat = ISPFormatUBWCNV12;
            break;

        case CamX::Format::UBWCNV124R:
            ISPFormat = ISPFormatUBWCNV124R;
            break;
        case CamX::Format::YUV420NV12TP10:
            ISPFormat = ISPFormatTP10;
            break;
        case CamX::Format::RawMIPI8:
            ISPFormat = ISPFormatMIPIRaw8;
            break;
        case CamX::Format::P010:
            ISPFormat = ISPFormatPlain1610;
            break;
        case CamX::Format::RawMIPI:
            switch (CSIDecodeBitWidth)
            {
                case CSIDecode6Bit:
                    ISPFormat = ISPFormatMIPIRaw6;
                    break;
                case CSIDecode8Bit:
                    ISPFormat = ISPFormatMIPIRaw8;
                    break;
                case CSIDecode10Bit:
                    ISPFormat = ISPFormatMIPIRaw10;
                    break;
                case CSIDecode12Bit:
                    ISPFormat = ISPFormatMIPIRaw12;
                    break;
                case CSIDecode14Bit:
                    ISPFormat = ISPFormatMIPIRaw14;
                    break;
                default:
                    ISPFormat = ISPFormatUndefined;
                    CAMX_ASSERT_ALWAYS_MESSAGE("Invalid CSID bit width %d", CSIDecodeBitWidth);
                    break;
            }
            break;

        case CamX::Format::RawPlain16:
            switch (CSIDecodeBitWidth)
            {
                case CSIDecode8Bit:
                    ISPFormat = ISPFormatPlain168;
                    break;
                case CSIDecode10Bit:
                    ISPFormat = ISPFormatPlain1610;
                    break;
                case CSIDecode12Bit:
                    ISPFormat = ISPFormatPlain1612;
                    break;
                case CSIDecode14Bit:
                    ISPFormat = ISPFormatPlain1614;
                    break;
                default:
                    ISPFormat = ISPFormatUndefined;
                    CAMX_ASSERT_ALWAYS_MESSAGE("Invalid CSID bit width %d", CSIDecodeBitWidth);
                    break;
            }
            break;

        case CamX::Format::RawPlain64:
            ISPFormat = ISPFormatPlain64;
            break;

        case CamX::Format::RawYUV8BIT:
            ISPFormat = ISPFormatPlain8;
            break;
        case CamX::Format::Y8:
            ISPFormat = ISPFormatY;
            break;
        case CamX::Format::Jpeg:
        case CamX::Format::YUV422NV16:
        case CamX::Format::Y16:
        case CamX::Format::Blob:
        case CamX::Format::RawPrivate:
        case CamX::Format::RawMeta8BIT:
        default:
            ISPFormat = ISPFormatUndefined;
            CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported format %d", format);
            break;
    }

    return ISPFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFENode::InitIFEPerFrameConfig
///
/// @brief  Initialize IFE Per-Frmae Debug Config
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::InitIFEPerFrameConfig()
{
    for (UINT index = 0; index < IFERequestQueueDepth; index++)
    {
        // After AcquireResource certain configs like StripeConfigs are initialized and
        // that init config is resumed for first new EPR(), for EPR we use m_IFEPerFrameData
        // structures on behalf of Init Structures like m_frameConfig, etc . Hence we need
        // Init config to be in place and also the new request doesn't start always with "1"
        // it can also resume with old requestId (In case of Multi camera Active/Deactivate).
        // So, the init config is copied to m_IFEPerFrameData[index] till the request queue Depth.
        m_IFEPerFrameData[index].frameConfig.stateLSC                    = m_frameConfig.stateLSC;
        // Initialize pointer variables with valid per frame data structures
        m_IFEPerFrameData[index].frameConfig.pFrameLevelData             = &m_IFEPerFrameData[index].ISPFrameData;
        m_IFEPerFrameData[index].frameConfig.pFrameLevelData->pFrameData = &m_IFEPerFrameData[index].ISPFramelevelData;

        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            m_IFEPerFrameData[index].dualIFEConfigData.stripingInput = *m_pStripingInput;
            // Initialize the Pointers in Striping Lib output stucture
            m_IFEPerFrameData[index].dualIFEConfigData.passOut.pStripeOutput[0] =
                &(m_IFEPerFrameData[index].dualIFEConfigData.stripeOutput[0]);
            m_IFEPerFrameData[index].dualIFEConfigData.passOut.pStripeOutput[1] =
                &(m_IFEPerFrameData[index].dualIFEConfigData.stripeOutput[1]);
        }

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFENode::UpdateIFEDebugConfig
///
/// @brief  Update IFE Debug Config
///
/// @param  pInputData    Pointer to isp inputdata
/// @param  requestID     requestId
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::UpdateIFEDebugConfig(
    ISPInputData* pInputData,
    UINT64 requestID)
{
    UINT perFrameDataIndex = GetIFEPerFrameDataIndex(requestID);

    m_IFEPerFrameData[perFrameDataIndex].requestID          = requestID;
    m_IFEPerFrameData[perFrameDataIndex].isValid            = TRUE;
    m_IFEPerFrameData[perFrameDataIndex].forceTriggerUpdate = pInputData->forceTriggerUpdate;

    if ((NULL != m_pPassOut) && (IFEModuleMode::DualIFENormal == m_mode))
    {
        // Copy Striping Library Output Data
        m_IFEPerFrameData[perFrameDataIndex].dualIFEConfigData.passOut.numStripes = m_pPassOut->numStripes;

        Utils::Memcpy(&m_IFEPerFrameData[perFrameDataIndex].dualIFEConfigData.stripeOutput[0], m_pPassOut->pStripeOutput[0],
            sizeof(IFEStripeInterfaceOutput));
        Utils::Memcpy(&m_IFEPerFrameData[perFrameDataIndex].dualIFEConfigData.stripeOutput[1], m_pPassOut->pStripeOutput[1],
            sizeof(IFEStripeInterfaceOutput));
    }

    // Copy Per request Tuning mode data
    Utils::Memcpy(&(m_IFEPerFrameData[perFrameDataIndex].tuningData), &m_tuningData, sizeof(m_tuningData));

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::TranslateCSIDataTypeToCSIDecodeFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 IFENode::TranslateCSIDataTypeToCSIDecodeFormat(
    const UINT8 CSIDataType)
{
    UINT8 CSIDecodeFormat = 0;

    switch (CSIDataType)
    {
        case CSIDataTypeYUV422_8:
        case CSIDataTypeRaw8:
            CSIDecodeFormat = CSIDecode8Bit;
            break;

        case CSIDataTypeRaw10:
            CSIDecodeFormat = CSIDecode10Bit;
            break;

        case CSIDataTypeRaw12:
            CSIDecodeFormat = CSIDecode12Bit;
            break;

        case CSIDataTypeRaw14:
            CSIDecodeFormat = CSIDecode14Bit;
            break;

        default:
            CSIDecodeFormat = CSIDecode10Bit;
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "Unable to translate DT = %02x to CSI decode bit format. Assuming 10 bit!",
                           CSIDataType);
            break;
    }

    return CSIDecodeFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::TranslateBitDepthToCSIDecodeFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 IFENode::TranslateBitDepthToCSIDecodeFormat(
    const UINT32 bitWidth)
{
    UINT8 CSIDecodeFormat;

    switch (bitWidth)
    {
        case 6:
            CSIDecodeFormat = CSIDecode6Bit;
            break;

        case 8:
            CSIDecodeFormat = CSIDecode8Bit;
            break;

        case 10:
            CSIDecodeFormat = CSIDecode10Bit;
            break;

        case 12:
            CSIDecodeFormat = CSIDecode12Bit;
            break;

        case 14:
            CSIDecodeFormat = CSIDecode14Bit;
            break;

        default:
            CSIDecodeFormat = CSIDecode10Bit;
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "Unable to translate bit width = %u to CSI decode bit format. Assuming 10 bit!",
                           bitWidth);
            break;
    }

    return CSIDecodeFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::TranslateSensorStreamConfigTypeToPortSourceType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFENode::TranslateSensorStreamConfigTypeToPortSourceType(
    StreamType streamType)
{
    UINT portSourceTypeId = PortSrcTypeUndefined;

    switch (streamType)
    {
        case StreamType::IMAGE:
            portSourceTypeId = PortSrcTypePixel;
            break;

        case StreamType::PDAF:
            portSourceTypeId = PortSrcTypePDAF;
            break;

        case StreamType::HDR:
            portSourceTypeId = PortSrcTypeHDR;
            break;

        case StreamType::META:
            portSourceTypeId = PortSrcTypeMeta;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "Unable to translate sensor stream type = %u",
                           streamType);
            break;
    }

    return portSourceTypeId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::TranslatePortSourceTypeToSensorStreamConfigType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFENode::TranslatePortSourceTypeToSensorStreamConfigType(
    UINT portSourceTypeId)
{
    StreamType streamType;

    switch (portSourceTypeId)
    {
        case PortSrcTypePixel:
            streamType = StreamType::IMAGE;
            break;

        case PortSrcTypePDAF:
            streamType = StreamType::PDAF;
            break;

        case PortSrcTypeHDR:
            streamType = StreamType::HDR;
            break;

        case PortSrcTypeMeta:
            streamType = StreamType::META;
            break;

        case PortSrcTypeUndefined:
        default:
            streamType = StreamType::BLOB;
            break;
    }

    return static_cast<UINT32>(streamType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::TranslateColorFilterPatternToISPPattern
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFENode::TranslateColorFilterPatternToISPPattern(
    const enum SensorInfoColorFilterArrangementValues colorFilterArrangementValue)
{
    UINT32 ISPPattern = 0;

    switch (colorFilterArrangementValue)
    {
        case SensorInfoColorFilterArrangementRggb:
            ISPPattern = ISPPatternBayerRGRGRG;
            break;

        case SensorInfoColorFilterArrangementGrbg:
            ISPPattern = ISPPatternBayerGRGRGR;
            break;

        case SensorInfoColorFilterArrangementGbrg:
            ISPPattern = ISPPatternBayerGBGBGB;
            break;

        case SensorInfoColorFilterArrangementBggr:
            ISPPattern = ISPPatternBayerBGBGBG;
            break;

        case SensorInfoColorFilterArrangementY:
            ISPPattern = ISPPatternBayerRGRGRG;
            break;

        default:
            ISPPattern = ISPPatternBayerRGRGRG;

            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "Unable to translate SensorInfoColorFilterArrangementValue = %d to ISP Bayer pattern."
                           "Assuming ISPPatternBayerRGRGRG!",
                           colorFilterArrangementValue);
            break;
    }

    return ISPPattern;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::FindSensorStreamConfigIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::FindSensorStreamConfigIndex(
    StreamType  streamType,
    UINT*       pStreamIndex)
{
    BOOL foundStream = FALSE;

    for (UINT32 index = 0; index < m_pSensorModeData->streamConfigCount; index++)
    {
        if (m_pSensorModeData->streamConfig[index].type == streamType)
        {
            foundStream  = TRUE;
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
// IFENode::CheckOutputPortIndexIfUnsupported
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::CheckOutputPortIndexIfUnsupported(
    UINT outputPortIndex)
{
    BOOL needOutputPortDisabled = FALSE;

    if ((FALSE           == FindSensorStreamConfigIndex(StreamType::PDAF, NULL)) &&
        (PortSrcTypePDAF == GetOutputPortSourceType(outputPortIndex)))
    {
        if ((ISPHwTitan480 != m_hwMask) || (FALSE == CheckIfPDAFType3Supported()))
        {
            needOutputPortDisabled = TRUE;
        }
    }
    else if ((FALSE          == FindSensorStreamConfigIndex(StreamType::HDR, NULL)) &&
             (PortSrcTypeHDR == GetOutputPortSourceType(outputPortIndex)))
    {
        needOutputPortDisabled = TRUE;
    }
    else if ((FALSE           == FindSensorStreamConfigIndex(StreamType::META, NULL)) &&
             (PortSrcTypeMeta == GetOutputPortSourceType(outputPortIndex)))
    {
        needOutputPortDisabled = TRUE;
    }

    return needOutputPortDisabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetPDAFSensorType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PDLibSensorType IFENode::GetPDAFSensorType()
{
    UINT32                       cameraID                  = GetPipeline()->GetCameraId();
    const ImageSensorModuleData* pSensorModuleData         = GetHwContext()->GetImageSensorModuleData(cameraID);
    UINT                         currentMode               = 0;
    BOOL                         isSensorModeSupportPDAF   = FALSE;
    PDLibSensorType              sensorModePDAFType        = PDLibSensorInvalid;
    PDLibSensorType              sensorCurrentModePDAFType = PDLibSensorInvalid;

    static const UINT GetProps[] =
    {
        PropertyIDUsecaseSensorCurrentMode
    };

    static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]   = { 0 };
    UINT64            offsets[GetPropsLength] = { 0 };

    GetDataList(GetProps, pData, offsets, GetPropsLength);

    if (NULL != pData[0])
    {
        currentMode = *reinterpret_cast<UINT*>(pData[0]);
        pSensorModuleData->GetPDAFInformation(currentMode,
            &isSensorModeSupportPDAF, reinterpret_cast<PDAFType*>(&sensorCurrentModePDAFType));
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "PDAFType is %d", sensorCurrentModePDAFType);

    // Check if PDAF is disabled with camx settings
    if (FALSE == GetStaticSettings()->disablePDAF)
    {
        if (TRUE == isSensorModeSupportPDAF)
        {
            sensorModePDAFType = sensorCurrentModePDAFType;
        }
    }

    return sensorModePDAFType;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CheckIfPDAFType3Supported
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::CheckIfPDAFType3Supported()
{
    BOOL isPDAFT3Enabled = FALSE;

    if (PDLibSensorType3 == GetPDAFSensorType())
    {
        isPDAFT3Enabled = TRUE;
    }

    return isPDAFT3Enabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::IsPixelOutputPortSourceType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::IsPixelOutputPortSourceType(
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
// IFENode::SetRDIOutputPortFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetRDIOutputPortFormat(
    IFEOutputResourceInfo*  pOutputResource,
    Format                  format,
    UINT                    outputPortId,
    UINT                    portSourceTypeId)
{
    CamxResult result = CamxResultSuccess;

    switch (portSourceTypeId)
    {
        case PortSrcTypeUndefined:
        case PortSrcTypePixel:
            pOutputResource->format = TranslateFormatToISPImageFormat(format, m_CSIDecodeBitWidth);
            break;

        case PortSrcTypePDAF:
            switch (format)
            {
                case Format::Blob:
                    pOutputResource->format = ISPFormatPlain128;
                    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                     "For PDAF stream input resource, configure output portId = %u as Plain128",
                                     outputPortId);
                    break;

                case Format::RawPlain16:
                    pOutputResource->format = TranslateFormatToISPImageFormat(format, m_PDAFCSIDecodeFormat);
                    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                     "For PDAF stream input resource, configure output portId = %u as RawPlain16",
                                     outputPortId);
                    break;

                case Format::RawPlain64:
                    pOutputResource->format = TranslateFormatToISPImageFormat(format, m_PDAFCSIDecodeFormat);
                    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                     "For PDAF stream input resource, configure output portId = %u as RawPlain16",
                                     outputPortId);
                    break;

                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP,
                                   "Unsupported RDI output format for PDAF stream input resource. Format = %u",
                                   format);
                    result = CamxResultEUnsupported;
                    break;
            }
            break;

        case PortSrcTypeMeta:
            switch (format)
            {
                case Format::Blob:
                    pOutputResource->format = ISPFormatPlain128;
                    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                     "For META stream input resource, configure output portId = %u as Plain128",
                                     outputPortId);
                    break;

                case Format::RawPlain16:
                    pOutputResource->format = TranslateFormatToISPImageFormat(format, m_metaCSIDecodeFormat);
                    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                     "For META stream input resource, configure output portId = %u as RawPlain16",
                                     outputPortId);
                    break;

                case Format::RawMIPI:
                    pOutputResource->format = TranslateFormatToISPImageFormat(format, m_metaCSIDecodeFormat);
                    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                     "For META stream input resource, configure output portId = %u as RawMIPI",
                                     outputPortId);
                    break;

                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP,
                                   "Unsupported RDI output format for META stream input resource. Format = %u",
                                   format);
                    result = CamxResultEUnsupported;
                    break;
            }
            break;

        case PortSrcTypeHDR:
            switch (format)
            {
                case Format::Blob:
                    pOutputResource->format = ISPFormatPlain128;
                    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                     "For HDR stream input resource, configure output portId = %u as Plain128",
                                     outputPortId);
                    break;

                case Format::RawMIPI:
                    pOutputResource->format = TranslateFormatToISPImageFormat(format, m_HDRCSIDecodeFormat);
                    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                     "For HDR stream input resource, configure output portId = %u as RawMIPI",
                                     outputPortId);
                    break;

                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP,
                                   "Unsupported RDI output format for HDR stream input resource. Format = %u",
                                   format);
                    result = CamxResultEUnsupported;
                    break;
            }
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "Unsupported port source type = %u for RDI output port = %u",
                           portSourceTypeId,
                           outputPortId);
            result = CamxResultEUnsupported;
            break;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::IFENode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFENode::IFENode()
    : m_mode(IFEModuleMode::SingleIFENormal)
    , m_useStatsAlgoConfig(TRUE)
    , m_initialConfigPending (TRUE)
    , m_highInitialBWCnt(0)
    , m_pIFEResourceInfo(NULL)
    , m_isIFEResourceAcquried(FALSE)
    , m_enableBusRead(FALSE)
    , m_IFEPixelRawPort(0)
{
    m_ISPFrameData.pFrameData   = &m_ISPFramelevelData;
    m_ISPInputSensorData.dGain  = 1.0f;
    m_pNodeName                 = "IFE";

    // LDC related
    m_ICAGridOut2InEnabled   = FALSE;
    m_ICAGridIn2OutEnabled   = FALSE;

    m_publishLDCGridData     = FALSE;
    m_ICAGridGeometry        = 0;
    m_ifeOutputImageSize     = { 0 };

    for (UINT path = 0; path < LDCMaxPath; path++)
    {
        m_ifeZoomWindowInDomain[path] = { 0 };
    }

    for (UINT i = 0; i < GridMaxType; i++)
    {
        m_pWarpGridDataIn[i]  = NULL;

        for (UINT j = 0; j < LDCMaxPath; j++)
        {
            m_pWarpGridDataOut[i][j] = NULL;
        }
    }
    // Clear IFEDebugData structure; so that accidentally we will not access/dump
    // those structures
    ClearIFEDebugConfig();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ReleaseDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ReleaseDevice()
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != GetHwContext()) && (0 != m_hDevice))
    {
        result = CSLReleaseDevice(GetCSLSession(), m_hDevice);
        CAMX_LOG_INFO(CamxLogGroupCore, "Releasing - IFEDevice : %s, Result: %d", NodeIdentifierString(), result);
        if (CamxResultSuccess == result)
        {
            SetDeviceAcquired(FALSE);
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to release device");
        }
        if (NULL != m_pIFEResourceInfo)
        {
            CAMX_FREE(m_pIFEResourceInfo);
            m_pIFEResourceInfo = NULL;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Failed: m_hDevice = %d HwContext = %p", m_hDevice, GetHwContext());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::~IFENode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFENode::~IFENode()
{
    Cleanup();

    if (TRUE == IsDeviceAcquired())
    {
        IQInterface::IQSettingModuleUninitialize(&m_libInitialData);

        ReleaseDevice();
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupISP, "No ReleaseDevice, IsDeviceAcquired = %d ", IsDeviceAcquired());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFENode* IFENode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    if ((NULL != pCreateInputData) && (NULL != pCreateInputData->pNodeInfo))
    {
        UINT32           propertyCount   = pCreateInputData->pNodeInfo->nodePropertyCount;
        PerNodeProperty* pNodeProperties = pCreateInputData->pNodeInfo->pNodeProperties;

        IFENode* pNodeObj = CAMX_NEW IFENode;

        if (NULL != pNodeObj)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "nodePropertyCount %d", propertyCount);

            // There can be multiple IFE instances in a pipeline, each instance can have different properties
            for (UINT32 count = 0; count < propertyCount; count++)
            {
                UINT32 nodePropertyId     = pNodeProperties[count].id;
                VOID*  pNodePropertyValue = pNodeProperties[count].pValue;

                switch (nodePropertyId)
                {
                    case NodePropertyProfileId:
                        pNodeObj->m_instanceProperty.profileId = static_cast<IFEProfileId>(
                           atoi(static_cast<const CHAR*>(pNodePropertyValue)));
                        break;
                    case NodePropertyIFECSIDHeight:
                        pNodeObj->m_instanceProperty.IFECSIDHeight = atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyIFECSIDWidth:
                        pNodeObj->m_instanceProperty.IFECSIDWidth = atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyIFECSIDLeft:
                        pNodeObj->m_instanceProperty.IFECSIDLeft = atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyIFECSIDTop:
                        pNodeObj->m_instanceProperty.IFECSIDTop = atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyStatsSkipPattern:
                        // Incase of 60 fps preview, stats should run at 30 fps(tintless at 15),
                        // this property defines the skip pattern.
                        pNodeObj->m_instanceProperty.IFEStatsSkipPattern = (2 * (*static_cast<UINT*>(pNodePropertyValue)));
                        break;
                    case NodePropertyForceSingleIFEOn:
                        pNodeObj->m_instanceProperty.IFESingleOn = atoi(static_cast<CHAR*>(pNodePropertyValue));
                        break;
                    default:
                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Unhandled node property Id %d", nodePropertyId);
                        break;
                }
            }

            CAMX_LOG_INFO(CamxLogGroupISP, "IFE instance CSIDHeight: %d, CSIDWidth: %d, CSIDLeft: %d, CSIDTop: %d, "
                          "SkipPattern: %d, ForceSingleIFEOn: %d, profile ID %d",
                          pNodeObj->m_instanceProperty.IFECSIDHeight,
                          pNodeObj->m_instanceProperty.IFECSIDWidth,
                          pNodeObj->m_instanceProperty.IFECSIDLeft,
                          pNodeObj->m_instanceProperty.IFECSIDTop,
                          pNodeObj->m_instanceProperty.IFEStatsSkipPattern,
                          pNodeObj->m_instanceProperty.IFESingleOn,
                          pNodeObj->m_instanceProperty.profileId);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFENode, no memory");
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
// IFENode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{

    CamxResult  result                  = CamxResultSuccess;
    INT32       deviceIndex             = -1;
    UINT        indicesLengthRequired   = 0;
    UINT32      IFETestImageSizeWidth;
    UINT32      IFETestImageSizeHeight;

    CAMX_ASSERT(IFE == Type());

    Titan17xContext* pContext = NULL;

    pContext             = static_cast<Titan17xContext *>(GetHwContext());
    m_OEMIQSettingEnable = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->IsOEMIQSettingEnable;

    UINT inputPortId[MaxDefinedIFEInputPorts];

    pCreateOutputData->maxOutputPorts = MaxDefinedIFEOutputPorts;
    pCreateOutputData->maxInputPorts  = MaxDefinedIFEInputPorts;

    GetHwContext()->GetDeviceVersion(CSLDeviceTypeIFE, &m_version);

    GetCSIDBinningInfo(&m_csidBinningInfo);
    if (TRUE == m_csidBinningInfo.isBinningEnabled)
    {
        CAMX_LOG_CONFIG(CamxLogGroupISP, "CSID: binning enabled:%d, binningMode:%d",
            m_csidBinningInfo.isBinningEnabled, m_csidBinningInfo.binningMode);
    }

    m_maxNumOfCSLIFEPortId = CSLIFEPortIdMaxNumPortResourcesOfPlatform(pContext->GetTitanVersion());

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

    UINT32 groupID = ISPOutputGroupIdMAX;
    for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
    {
        switch (outputPortId[outputPortIndex])
        {

            case IFEOutputPortFull:
            case IFEOutputPortDS4:
            case IFEOutputPortDS16:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                                                                               ISPOutputGroupId0;
                break;

            case IFEOutputPortDisplayFull:
            case IFEOutputPortDisplayDS4:
            case IFEOutputPortDisplayDS16:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                                                                               ISPOutputGroupId1;
                break;

            case IFEOutputPortStatsHDRBHIST:
            case IFEOutputPortStatsAWBBG:
            case IFEOutputPortStatsHDRBE:
            case IFEOutputPortStatsIHIST:
            case IFEOutputPortStatsCS:
            case IFEOutputPortStatsRS:
            case IFEOutputPortStatsTLBG:
            case IFEOutputPortStatsBHIST:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                    ISPOutputGroupId2;
                break;

            case IFEOutputPortStatsBF:
                pCreateOutputData->bufferComposite.portGroupID[IFEOutputPortStatsBF] = ISPOutputGroupId3;
                break;

            case IFEOutputPortFD:
                pCreateOutputData->bufferComposite.portGroupID[IFEOutputPortFD] = ISPOutputGroupId4;
                break;

            case IFEOutputPortCAMIFRaw:
            case IFEOutputPortLSCRaw:
            case IFEOutputPortGTMRaw:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                    ISPOutputGroupId5;
                break;

            case IFEOutputPortPDAF:
                pCreateOutputData->bufferComposite.portGroupID[IFEOutputPortPDAF] = ISPOutputGroupId6;
                break;

            default:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                    groupID++;
                break;
        }
    }

    pCreateOutputData->bufferComposite.hasCompositeMask = TRUE;
    Utils::Memcpy(&m_bufferComposite, &pCreateOutputData->bufferComposite, sizeof(BufferGroup));

    IFETestImageSizeWidth   = GetStaticSettings()->IFETestImageSizeWidth;
    IFETestImageSizeHeight  = GetStaticSettings()->IFETestImageSizeHeight;
    m_disableManual3ACCM    = GetStaticSettings()->DisableManual3ACCM;

    /// @todo (CAMX-561) Config the TPG setting properly, right now hardcode
    m_testGenModeData.format                    = PixelFormat::BayerRGGB;
    m_testGenModeData.numPixelsPerLine          = IFETestImageSizeWidth;
    m_testGenModeData.numLinesPerFrame          = IFETestImageSizeHeight;
    m_testGenModeData.resolution.outputWidth    = IFETestImageSizeWidth;
    m_testGenModeData.resolution.outputHeight   = IFETestImageSizeHeight;
    m_testGenModeData.cropInfo.firstPixel       = 0;
    m_testGenModeData.cropInfo.firstLine        = 0;
    m_testGenModeData.cropInfo.lastPixel        = IFETestImageSizeWidth - 1;
    m_testGenModeData.cropInfo.lastLine         = IFETestImageSizeHeight - 1;
    m_testGenModeData.streamConfigCount         = 1;
    m_testGenModeData.streamConfig[0].type      = StreamType::IMAGE;
    m_testGenModeData.outPixelClock             = TPGPIXELCLOCK;
    m_testGenModeData.maxFPS                    = TPGFPS;

    // Add device indices
    result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeIFE, &deviceIndex, 1, &indicesLengthRequired);

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(indicesLengthRequired == 1);
        result = AddDeviceIndex(deviceIndex);
    }

    // If IFE tuning-data enable, initialize debug-data writer
    if ((CamxResultSuccess  == result)                                      &&
        (TRUE               == GetStaticSettings()->enableTuningMetadata)   &&
        (0                  != GetStaticSettings()->tuningDumpDataSizeIFE))
    {
        // We would disable dual IFE when supporting tuning data
        m_pTuningMetadata = static_cast<IFETuningMetadata*>(CAMX_CALLOC(sizeof(IFETuningMetadata)));
        if (NULL == m_pTuningMetadata)
        {
            CAMX_LOG_ERROR(CamxLogGroupDebugData, "Failed to allocate Tuning metadata.");
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            m_pDebugDataWriter = CAMX_NEW TDDebugDataWriter();
            if (NULL == m_pDebugDataWriter)
            {
                CAMX_LOG_ERROR(CamxLogGroupDebugData, "Failed to allocate Tuning metadata.");
                result = CamxResultENoMemory;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        result = CheckForRDIOnly();
    }

    // Fetch settings, and link info
    if (CamxResultSuccess == result)
    {
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

        /// @todo (CAMX-833) Update ISP input data from the port and disable hardcoded flag from settings
        m_enableHardcodedConfig = static_cast<Titan17xContext *>(
            GetHwContext())->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->IFEEnableHardcodedConfig;


        m_useStatsAlgoConfig = (FALSE == m_enableHardcodedConfig) && (TRUE == m_hasStatsNode);


        CAMX_LOG_VERBOSE(CamxLogGroupISP, "useStatsAlgo %d, Hardcod En %d, Stats Node %d, OEM Stats %d",
                         m_useStatsAlgoConfig,
                         m_enableHardcodedConfig,
                         m_hasStatsNode,
                         m_OEMStatsConfig);
    }

    // Configure IFE Capability
    result = ConfigureIFECapability();

    Utils::Memset(&m_HVXInputData, 0, sizeof(IFEHVXInfo));

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "enableHVXStreaming %d  pHVXAlgoCallbacks %p",
        GetStaticSettings()->enableHVXStreaming, pCreateInputData->pHVXAlgoCallbacks);

    if ((TRUE == GetStaticSettings()->enableHVXStreaming) &&
        (NULL != pCreateInputData->pHVXAlgoCallbacks))
    {
        m_HVXInputData.pHVXAlgoCallbacks = pCreateInputData->pHVXAlgoCallbacks;
        m_HVXInputData.enableHVX        = TRUE;

        result = CreateIFEHVXModules();
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE ==
            HwEnvironment::GetInstance()->IsHWBugWorkaroundEnabled(Titan17xWorkarounds::Titan17xWorkaroundsCDMDMICGCBug))
        {
            // Adding CGC on/off register write requirements
            m_totalIQCmdSizeDWord += PacketBuilder::RequiredWriteInterleavedRegsSizeInDwords(IFECGCNumRegs) * 2;
        }

        // For RDI only use case too we need to allocate cmd buffer for kernel use
        m_totalIQCmdSizeDWord += IFEKMDCmdBufferMaxSize;
    }

    m_totalIQCmdSizeDWord += m_pIFEPipeline->GetRegCmdSize();

    // register to update config done for initial PCR
    pCreateOutputData->createFlags.willNotifyConfigDone = TRUE;

    m_genericBlobCmdBufferSizeBytes =
        (sizeof(IFEResourceHFRConfig) + (sizeof(IFEPortHFRConfig) * (MaxDefinedIFEOutputPorts - 1))) +  // HFR configuration
        sizeof(IFEResourceClockConfig) + (sizeof(UINT64) * (RDIMaxNum - 1)) +
        sizeof(IFEResourceBWConfig) + (sizeof(IFEResourceBWVote) * (RDIMaxNum - 1)) +
        sizeof(IFEResourceBWConfigVer2) +
        sizeof(CSLResourceUBWCConfigV2) + (sizeof(CSLPortUBWCConfigV2) *
        (CSLMaxNumPlanes - 1) * (MaxDefinedIFEOutputPorts - 1))                     +
        sizeof(IFEOutConfig);

    // For RDI only use case too we need to allocate cmd buffer for kernel use
    m_totalIQCmdSizeDWord += IFEKMDCmdBufferMaxSize;

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
    // Get full size sensor Aspect Ratio
    const UINT sensorInfoTag[] =
    {
        StaticSensorInfoActiveArraySize,
    };
    const UINT length = CAMX_ARRAY_SIZE(sensorInfoTag);
    VOID*      pDataOut[length] = { 0 };
    UINT64     offset[length] = { 0 };

    result = GetDataList(sensorInfoTag, pDataOut, offset, length);
    if (CamxResultSuccess == result)
    {
        if (NULL != pDataOut[0])
        {
            Region region = *static_cast<Region*>(pDataOut[0]);
            if (region.height != 0)
            {
                m_sensorActiveArrayAR =
                    static_cast<FLOAT>(region.width) / static_cast<FLOAT>(region.height);
            }
            else
            {
                result = CamxResultEInvalidState;
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Sensor Active Array Dimensions");
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pFinalizeInitializationData);

    // Fetch static info with respect to sensor and initiatilize  the HAL3 IFE metadata
    GetStaticMetadata();

    // Fetch sesnor info
    result = FetchSensorInfo();

    // Update IFE input data based on sensor info and also HVX data
    if ((CamxResultSuccess == result) && (NULL != m_pSensorModeData))
    {
        m_IFEinputResolution.horizontalFactor   = 1.0f;
        m_IFEinputResolution.verticalFactor     = 1.0f;
        m_IFEinputResolution.resolution.width   = m_pSensorModeData->cropInfo.lastPixel -
            m_pSensorModeData->cropInfo.firstPixel + 1;
        m_IFEinputResolution.resolution.height  = m_pSensorModeData->cropInfo.lastLine -
            m_pSensorModeData->cropInfo.firstLine + 1;
        m_IFEinputResolution.CAMIFWindow.left   = m_pSensorModeData->cropInfo.firstPixel;
        m_IFEinputResolution.CAMIFWindow.top    = m_pSensorModeData->cropInfo.firstLine;
        m_IFEinputResolution.CAMIFWindow.width  = m_pSensorModeData->cropInfo.lastPixel -
            m_pSensorModeData->cropInfo.firstPixel + 1;
        m_IFEinputResolution.CAMIFWindow.height = m_pSensorModeData->cropInfo.lastLine -
            m_pSensorModeData->cropInfo.firstLine + 1;

        if (TRUE == m_csidBinningInfo.isBinningEnabled)
        {
            m_IFEinputResolution.resolution.width   >>= 1;
            m_IFEinputResolution.resolution.height  >>= 1;
            m_IFEinputResolution.CAMIFWindow.left   >>= 1;
            m_IFEinputResolution.CAMIFWindow.top    >>= 1;
            m_IFEinputResolution.CAMIFWindow.width  >>= 1;
            m_IFEinputResolution.CAMIFWindow.height >>= 1;
        }

        ISPInputData moduleInput;

        moduleInput.HVXData.sensorInput.width = m_pSensorModeData->resolution.outputWidth;
        moduleInput.HVXData.sensorInput.height = m_pSensorModeData->resolution.outputHeight;
        moduleInput.HVXData.format = m_pSensorModeData->format;

        // Update the input to IFE in case of HVX binning
        if (TRUE == m_HVXInputData.enableHVX)
        {

            result = static_cast<IFEHVX*>(m_pIFEHVXModule)->GetHVXInputResolution(&moduleInput);

            if (CamxResultSuccess == result)
            {
                m_HVXInputData.HVXConfiguration = moduleInput.HVXData;
                m_IFEinputResolution.resolution.width = moduleInput.HVXData.HVXOut.width;
                m_IFEinputResolution.resolution.height = moduleInput.HVXData.HVXOut.height;
                m_IFEinputResolution.CAMIFWindow.left = 0;
                m_IFEinputResolution.CAMIFWindow.top = 0;
                m_IFEinputResolution.CAMIFWindow.width = moduleInput.HVXData.HVXOut.width - 1;
                m_IFEinputResolution.CAMIFWindow.height = moduleInput.HVXData.HVXOut.height - 1;
                m_IFEinputResolution.horizontalFactor =
                    static_cast<FLOAT>(moduleInput.HVXData.HVXOut.width) /
                    static_cast<FLOAT>(moduleInput.HVXData.sensorInput.width);
                m_IFEinputResolution.verticalFactor =
                    static_cast<FLOAT>(moduleInput.HVXData.HVXOut.height) /
                    static_cast<FLOAT>(moduleInput.HVXData.sensorInput.height);
            }
            else
            {
                m_HVXInputData.enableHVX = FALSE;
            }

            CAMX_LOG_INFO(CamxLogGroupISP, "[HVX_DBG]: Ds enabled %d",
                m_HVXInputData.HVXConfiguration.DSEnabled);
        }

        // Publish IFE input and HW capabilities for other nodes
        PublishIFEInputToUsecasePool(&m_IFEinputResolution);
        PublishPDAFCapabilityToUsecasePool();
    }

    m_resourcePolicy = pFinalizeInitializationData->resourcePolicy;

    return CamxResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFENode::PublishIFEInputToUsecasePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PublishIFEInputToUsecasePool(
    IFEInputResolution* pIFEResolution)
{
    CAMX_UNREFERENCED_PARAM(pIFEResolution);

    MetadataPool*           pPerUsecasePool = GetPerFramePool(PoolType::PerUsecase);
    MetadataSlot*           pPerUsecaseSlot = pPerUsecasePool->GetSlot(0);
    UsecasePropertyBlob*    pPerUsecaseBlob = NULL;
    CamxResult              result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pIFEResolution);

    const UINT  IFEResolutionTag[] = { PropertyIDUsecaseIFEInputResolution };
    const VOID* pData[1]           = { pIFEResolution };
    UINT        pDataCount[1]      = { sizeof(IFEInputResolution) };

    result = WriteDataList(IFEResolutionTag, pData, pDataCount, 1);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupFD, "Falied to publish IFE Downscale ratio uscasepool");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFENode::PublishPDAFCapabilityToUsecasePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PublishPDAFCapabilityToUsecasePool()
{
    MetadataPool*           pPerUsecasePool = GetPerFramePool(PoolType::PerUsecase);
    MetadataSlot*           pPerUsecaseSlot = pPerUsecasePool->GetSlot(0);
    UsecasePropertyBlob*    pPerUsecaseBlob = NULL;
    PDHwAvailablity         pdHWInfo        = { 0 };
    CamxResult              result = CamxResultSuccess;

    if (NULL != m_pIFEPipeline)
    {
        m_pIFEPipeline->GetPDHWCapability(&pdHWInfo, m_instanceProperty.profileId);
    }
    const UINT  IFEPDHWInfoTag[] = { PropertyIDUsecaseIFEPDHWInfo };
    const VOID* pData[1] = { &pdHWInfo };
    UINT        pDataCount[1] = { sizeof(PDHwAvailablity) };

    result = WriteDataList(IFEPDHWInfoTag, pData, pDataCount, 1);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupFD, "Falied to publish IFE PDHWInfo uscasepool");
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::InitializeOutputPathImageInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::InitializeOutputPathImageInfo()
{
    UINT32 totalOutputPorts = 0;
    UINT32 outputPortId[MaxDefinedIFEOutputPorts];

    const ImageFormat*  pImageFormat = NULL;

    /// @todo (CAMX-1015) Get this only once in ProcessingNodeInitialize() and save it off in the IFENode class
    GetAllOutputPortIds(&totalOutputPorts, outputPortId);

    /// @todo (CAMX-1221) Change read only type of ISPInput data structure to pointer to avoid memcpy
    for (UINT outputPortIndex = 0; outputPortIndex < totalOutputPorts; outputPortIndex++)
    {
        pImageFormat = GetOutputPortImageFormat(OutputPortIndex(outputPortId[outputPortIndex]));

        if (NULL != pImageFormat)
        {
            CAMX_LOG_INFO(CamxLogGroupISP, "IFE output path[%d] dimension [%d * %d]",
                outputPortId[outputPortIndex],
                pImageFormat->width,
                pImageFormat->height);

            switch (outputPortId[outputPortIndex])
            {
                case IFEOutputPortFull:
                    m_ISPInputHALData.stream[FullOutput].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[FullOutput].height = pImageFormat->height;
                    m_ISPInputHALData.format[FullOutput]        = pImageFormat->format;
                    break;

                case IFEOutputPortFD:
                    m_ISPInputHALData.stream[FDOutput].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[FDOutput].height = pImageFormat->height;
                    m_ISPInputHALData.format[FDOutput]        = pImageFormat->format;

                    break;

                case IFEOutputPortDS4:
                    m_ISPInputHALData.stream[DS4Output].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[DS4Output].height = pImageFormat->height;
                    m_ISPInputHALData.format[DS4Output]        = pImageFormat->format;

                    break;

                case IFEOutputPortDS16:
                    m_ISPInputHALData.stream[DS16Output].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[DS16Output].height = pImageFormat->height;
                    m_ISPInputHALData.format[DS16Output]        = pImageFormat->format;

                    break;

                case IFEOutputPortCAMIFRaw:
                    m_IFEPixelRawPort                               = IFEOutputPortCAMIFRaw;
                    m_ISPInputHALData.stream[PixelRawOutput].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[PixelRawOutput].height = pImageFormat->height;
                    m_ISPInputHALData.format[PixelRawOutput]        = pImageFormat->format;
                    break;

                case IFEOutputPortLSCRaw:
                    m_IFEPixelRawPort                               = IFEOutputPortLSCRaw;
                    m_ISPInputHALData.stream[PixelRawOutput].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[PixelRawOutput].height = pImageFormat->height;
                    m_ISPInputHALData.format[PixelRawOutput]        = pImageFormat->format;
                    break;

                case IFEOutputPortGTMRaw:
                    m_IFEPixelRawPort                               = IFEOutputPortGTMRaw;
                    m_ISPInputHALData.stream[PixelRawOutput].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[PixelRawOutput].height = pImageFormat->height;
                    m_ISPInputHALData.format[PixelRawOutput]        = pImageFormat->format;
                    break;

                case IFEOutputPortDisplayFull:
                    m_ISPInputHALData.stream[DisplayFullOutput].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[DisplayFullOutput].height = pImageFormat->height;
                    m_ISPInputHALData.format[DisplayFullOutput]        = pImageFormat->format;
                    break;

                case IFEOutputPortDisplayDS4:
                    m_ISPInputHALData.stream[DisplayDS4Output].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[DisplayDS4Output].height = pImageFormat->height;
                    m_ISPInputHALData.format[DisplayDS4Output]        = pImageFormat->format;
                    break;

                case IFEOutputPortDisplayDS16:
                    m_ISPInputHALData.stream[DisplayDS16Output].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[DisplayDS16Output].height = pImageFormat->height;
                    m_ISPInputHALData.format[DisplayDS16Output]        = pImageFormat->format;
                    break;

                case IFEOutputPortLCR:
                    m_ISPInputHALData.stream[LCROutput].width  = pImageFormat->width;
                    m_ISPInputHALData.stream[LCROutput].height = pImageFormat->height;
                    m_ISPInputHALData.format[LCROutput]        = pImageFormat->format;
                    break;

                default:
                    // For non-image output format, such as stats output, ignore and the configuration.
                    break;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Output port is not defined");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::HardcodeSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IFENode::HardcodeSettings(
    ISPInputData*     pModuleInput,
    ISPStripeConfig*  pStripeConfig,
    BOOL              initalConfig)
{

    Utils::Memcpy(&pModuleInput->HALData, &m_ISPInputHALData, sizeof(ISPHALConfigureData));
    Utils::Memcpy(&pModuleInput->sensorData, &m_ISPInputSensorData, sizeof(ISPSensorConfigureData));

    // Assume single IFE and set the first (only) stripe's crop to the full crop. This will be overwritten if in dual mode.
    pStripeConfig->CAMIFCrop = pModuleInput->sensorData.CAMIFCrop;
    // CSID crop override: Update CAMIFCrop.
    if (TRUE == EnableCSIDCropOverridingForSingleIFE())
    {
        pStripeConfig->CAMIFCrop.firstPixel = m_instanceProperty.IFECSIDLeft;
        pStripeConfig->CAMIFCrop.lastPixel  = m_instanceProperty.IFECSIDLeft + m_instanceProperty.IFECSIDWidth - 1;
        pStripeConfig->CAMIFCrop.firstLine  = m_instanceProperty.IFECSIDTop;
        pStripeConfig->CAMIFCrop.lastLine   = m_instanceProperty.IFECSIDTop + m_instanceProperty.IFECSIDHeight - 1;
    }

    pStripeConfig->CAMIFSubsampleInfo.enableCAMIFSubsample                   = m_PDAFInfo.enableSubsample;
    pStripeConfig->CAMIFSubsampleInfo.CAMIFSubSamplePattern.pixelSkipPattern = m_PDAFInfo.pixelSkipPattern;
    pStripeConfig->CAMIFSubsampleInfo.CAMIFSubSamplePattern.lineSkipPattern  = m_PDAFInfo.lineSkipPattern;

    pStripeConfig->pCSIDSubsampleInfo = m_CSIDSubSampleInfo;

    if (TRUE == initalConfig)
    {
        if (((pStripeConfig->CAMIFCrop.lastPixel - pStripeConfig->CAMIFCrop.firstPixel + 1) <
            static_cast<UINT32>(pModuleInput->pHALTagsData->HALCrop.left + pModuleInput->pHALTagsData->HALCrop.width)) ||
            ((pStripeConfig->CAMIFCrop.lastLine - pStripeConfig->CAMIFCrop.firstLine + 1) <
            static_cast<UINT32>(pModuleInput->pHALTagsData->HALCrop.top + pModuleInput->pHALTagsData->HALCrop.height)))
        {
            CAMX_LOG_INFO(CamxLogGroupISP, "Overrwriting CROP Window");
            pModuleInput->pHALTagsData->HALCrop.left   = pStripeConfig->CAMIFCrop.firstPixel;
            pModuleInput->pHALTagsData->HALCrop.width  =
                pStripeConfig->CAMIFCrop.lastPixel - pStripeConfig->CAMIFCrop.firstPixel + 1;
            pModuleInput->pHALTagsData->HALCrop.top    = pStripeConfig->CAMIFCrop.firstLine;
            pModuleInput->pHALTagsData->HALCrop.height =
                pStripeConfig->CAMIFCrop.lastLine - pStripeConfig->CAMIFCrop.firstLine + 1;
        }
    }

    pStripeConfig->HALCrop[FDOutput]          = pModuleInput->pHALTagsData->HALCrop;
    pStripeConfig->HALCrop[FullOutput]        = pModuleInput->pHALTagsData->HALCrop;
    pStripeConfig->HALCrop[DS4Output]         = pModuleInput->pHALTagsData->HALCrop;
    pStripeConfig->HALCrop[DS16Output]        = pModuleInput->pHALTagsData->HALCrop;
    pStripeConfig->HALCrop[PixelRawOutput]    = pModuleInput->pHALTagsData->HALCrop;
    pStripeConfig->HALCrop[DisplayFullOutput] = pModuleInput->pHALTagsData->HALCrop;
    pStripeConfig->HALCrop[DisplayDS4Output]  = pModuleInput->pHALTagsData->HALCrop;
    pStripeConfig->HALCrop[DisplayDS16Output] = pModuleInput->pHALTagsData->HALCrop;

    pStripeConfig->stream[FDOutput].width           = pModuleInput->HALData.stream[FDOutput].width;
    pStripeConfig->stream[FDOutput].height          = pModuleInput->HALData.stream[FDOutput].height;
    pStripeConfig->stream[FullOutput].width         = pModuleInput->HALData.stream[FullOutput].width;
    pStripeConfig->stream[FullOutput].height        = pModuleInput->HALData.stream[FullOutput].height;
    pStripeConfig->stream[DS4Output].width          = pModuleInput->HALData.stream[DS4Output].width;
    pStripeConfig->stream[DS4Output].height         = pModuleInput->HALData.stream[DS4Output].height;
    pStripeConfig->stream[DS16Output].width         = pModuleInput->HALData.stream[DS16Output].width;
    pStripeConfig->stream[DS16Output].height        = pModuleInput->HALData.stream[DS16Output].height;
    pStripeConfig->stream[PixelRawOutput].width     = pModuleInput->HALData.stream[PixelRawOutput].width;
    pStripeConfig->stream[PixelRawOutput].height    = pModuleInput->HALData.stream[PixelRawOutput].height;
    pStripeConfig->stream[PDAFRawOutput].width      = m_PDAFInfo.bufferWidth;
    pStripeConfig->stream[PDAFRawOutput].height     = m_PDAFInfo.bufferHeight;
    pStripeConfig->stream[DisplayFullOutput].width  = pModuleInput->HALData.stream[DisplayFullOutput].width;
    pStripeConfig->stream[DisplayFullOutput].height = pModuleInput->HALData.stream[DisplayFullOutput].height;
    pStripeConfig->stream[DisplayDS4Output].width   = pModuleInput->HALData.stream[DisplayDS4Output].width;
    pStripeConfig->stream[DisplayDS4Output].height  = pModuleInput->HALData.stream[DisplayDS4Output].height;
    pStripeConfig->stream[DisplayDS16Output].width  = pModuleInput->HALData.stream[DisplayDS16Output].width;
    pStripeConfig->stream[DisplayDS16Output].height = pModuleInput->HALData.stream[DisplayDS16Output].height;
    pStripeConfig->stream[LCROutput].width          = pModuleInput->HALData.stream[LCROutput].height;
    pStripeConfig->stream[LCROutput].height         = pModuleInput->HALData.stream[LCROutput].width;

    UINT32 inputWidth  = m_ISPInputSensorData.CAMIFCrop.lastPixel - m_ISPInputSensorData.CAMIFCrop.firstPixel + 1;
    UINT32 inputHeight = m_ISPInputSensorData.CAMIFCrop.lastLine - m_ISPInputSensorData.CAMIFCrop.firstLine + 1;

    // Stats tap-out setting
    pModuleInput->statsTapOut.HDRBEStatsSrcSelection    = TapoutHDRAfterLSC;
    pModuleInput->statsTapOut.HDRBHistStatsSrcSelection = TapoutHDRAfterLSC;
    // IHist tap-out depends in Y Hist Stretch feature
    if (NULL != pModuleInput->pTuningDataManager)
    {
        BOOL                enableYHistStretch              = FALSE;
        ChiTuningMode       tuningSelectors[MaxTuningMode]  = {};
        TuningSetManager*   pTuningManager                  = NULL;

        // Getting from default chromatix data, tap-out shall be set before acquire device
        tuningSelectors[0].mode = ChiModeType::Default;
        pTuningManager          = pModuleInput->pTuningDataManager->GetChromatix();
        if (NULL != pTuningManager)
        {
            TuningMode* pSelectors = reinterpret_cast<TuningMode*>(&tuningSelectors[0]);
            enableYHistStretch =
                pTuningManager->GetModule_Extension(pSelectors, MaxTuningMode)->YHistStretch.enableYHistStretch;
        }

        if (TRUE == enableYHistStretch)
        {
            pModuleInput->statsTapOut.IHistStatsSrcSelection = TapoutIHistAfterGLUT;
        }
        else
        {
            pModuleInput->statsTapOut.IHistStatsSrcSelection = TapoutIHistBeforeGLUT;
        }
    }

    if (TRUE == m_HVXInputData.HVXConfiguration.DSEnabled)
    {
        inputWidth  = m_HVXInputData.HVXConfiguration.HVXOut.width;
        inputHeight = m_HVXInputData.HVXConfiguration.HVXOut.height;
        m_HVXInputData.HVXConfiguration.HVXCrop.firstLine = 0;
        m_HVXInputData.HVXConfiguration.HVXCrop.firstPixel = 0;
        m_HVXInputData.HVXConfiguration.HVXCrop.lastPixel = inputWidth - 1;
        m_HVXInputData.HVXConfiguration.HVXCrop.lastLine = inputHeight - 1;
        m_HVXInputData.HVXConfiguration.origCAMIFCrop = pStripeConfig->CAMIFCrop;
        m_HVXInputData.HVXConfiguration.origHALWindow = pModuleInput->pHALTagsData->HALCrop;

        pStripeConfig->CAMIFCrop = m_HVXInputData.HVXConfiguration.HVXCrop;
        pStripeConfig->HALCrop[FullOutput].left   = 0;
        pStripeConfig->HALCrop[FullOutput].top    = 0;
        pStripeConfig->HALCrop[FullOutput].width  = inputWidth;
        pStripeConfig->HALCrop[FullOutput].height = inputHeight;

        pStripeConfig->HALCrop[FDOutput]   = pStripeConfig->HALCrop[FullOutput];
        pStripeConfig->HALCrop[DS4Output]  = pStripeConfig->HALCrop[FullOutput];
        pStripeConfig->HALCrop[DS16Output] = pStripeConfig->HALCrop[FullOutput];

        pStripeConfig->HALCrop[DisplayFullOutput] = pStripeConfig->HALCrop[FullOutput];
        pStripeConfig->HALCrop[DisplayDS4Output]  = pStripeConfig->HALCrop[FullOutput];
        pStripeConfig->HALCrop[DisplayDS16Output] = pStripeConfig->HALCrop[FullOutput];
    }

    /// @todo (CAMX-3276) Hard code YUV Sensor Crop configure for now.
    if (TRUE == pModuleInput->sensorData.isYUV)
    {
        inputWidth >>= 1;
    }

    // Currently Internal 3A publishes AEC, AWB and AF update, read it from propery pool
    if ((TRUE == m_enableHardcodedConfig) || (TRUE == initalConfig))
    {
        BGBEConfig* pBEConfig = &pModuleInput->pAECStatsUpdateData->statsConfig.BEConfig;

        pBEConfig->channelGainThreshold[ChannelIndexR]  = (1 << IFEPipelineBitWidth) - 1;
        pBEConfig->channelGainThreshold[ChannelIndexGR] = (1 << IFEPipelineBitWidth) - 1;
        pBEConfig->channelGainThreshold[ChannelIndexB]  = (1 << IFEPipelineBitWidth) - 1;
        pBEConfig->channelGainThreshold[ChannelIndexGB] = (1 << IFEPipelineBitWidth) - 1;
        pBEConfig->horizontalNum                        = 64;
        pBEConfig->verticalNum                          = 48;
        pBEConfig->ROI.left                             = 0;
        pBEConfig->ROI.top                              = 0;
        pBEConfig->ROI.width                            = inputWidth - (inputWidth / 10);
        pBEConfig->ROI.height                           = inputHeight - (inputHeight / 10);
        pBEConfig->outputBitDepth                       = 0;
        pBEConfig->outputMode                           = BGBERegular;

        // For AWB BG, store the configuration in pAWBStatsUpdateData.
        BGBEConfig* pBGConfig = &pModuleInput->pAWBStatsUpdateData->statsConfig.BGConfig;

        pBGConfig->channelGainThreshold[ChannelIndexR]  = (1 << IFEPipelineBitWidth) - 1;
        pBGConfig->channelGainThreshold[ChannelIndexGR] = (1 << IFEPipelineBitWidth) - 1;
        pBGConfig->channelGainThreshold[ChannelIndexB]  = (1 << IFEPipelineBitWidth) - 1;
        pBGConfig->channelGainThreshold[ChannelIndexGB] = (1 << IFEPipelineBitWidth) - 1;
        pBGConfig->horizontalNum                        = 64;
        pBGConfig->verticalNum                          = 48;
        pBGConfig->ROI.left                             = 0;
        pBGConfig->ROI.top                              = 0;
        pBGConfig->ROI.width                            = inputWidth - (inputWidth / 10);
        pBGConfig->ROI.height                           = inputHeight - (inputHeight / 10);
        pBGConfig->outputBitDepth                       = 0;
        pBGConfig->outputMode                           = BGBERegular;

        pModuleInput->pAECUpdateData->luxIndex                      = 220.0f;
        pModuleInput->pAECUpdateData->predictiveGain                = 1.0f;
        for (UINT i = 0; i < ExposureIndexCount; i++)
        {
            pModuleInput->pAECUpdateData->exposureInfo[i].exposureTime  = 1;
            pModuleInput->pAECUpdateData->exposureInfo[i].linearGain    = 1.0f;
            pModuleInput->pAECUpdateData->exposureInfo[i].sensitivity   = 1.0f;
        }

        pModuleInput->pAECUpdateData->stretchControl.enable  = FALSE;
        pModuleInput->pAECUpdateData->stretchControl.scaling = 1.0f;
        pModuleInput->pAECUpdateData->stretchControl.clamp   = 0.0f;

        pModuleInput->pAWBUpdateData->colorTemperature = 0;
        pModuleInput->pAWBUpdateData->AWBGains.gGain   = 1.0f;
        pModuleInput->pAWBUpdateData->AWBGains.bGain   = 1.493855f;
        pModuleInput->pAWBUpdateData->AWBGains.rGain   = 2.043310f;

        pModuleInput->pAFUpdateData->exposureCompensationEnable = 0;

        HardcodeSettingsBFStats(pStripeConfig);

        // RS stats configuration
        pModuleInput->pAFDStatsUpdateData->statsConfig.statsHNum                        = m_defaultConfig.RSStatsHorizRegions;
        pModuleInput->pAFDStatsUpdateData->statsConfig.statsVNum                        = m_defaultConfig.RSStatsVertRegions;
        pModuleInput->pAFDStatsUpdateData->statsConfig.statsRSCSColorConversionEnable   = TRUE;

        // CS stats configuration
        pModuleInput->pCSStatsUpdateData->statsConfig.statsHNum = m_defaultConfig.CSStatsHorizRegions;
        pModuleInput->pCSStatsUpdateData->statsConfig.statsVNum = m_defaultConfig.CSStatsVertRegions;

        // BHist configuration
        BHistConfig* pBHistConfig = &pModuleInput->pAECStatsUpdateData->statsConfig.BHistConfig;

        pBHistConfig->ROI.top     = 0;
        pBHistConfig->ROI.left    = 0;
        pBHistConfig->ROI.width   = inputWidth - (inputWidth / 10);
        pBHistConfig->ROI.height  = inputHeight - (inputHeight / 10);
        pBHistConfig->channel     = ColorChannel::ColorChannelY;
        pBHistConfig->uniform     = TRUE;

        // HDR Bhist config
        HDRBHistConfig* pHDRBHistConfig = &pModuleInput->pAECStatsUpdateData->statsConfig.HDRBHistConfig;

        pHDRBHistConfig->ROI.top            = 0;
        pHDRBHistConfig->ROI.left           = 0;
        pHDRBHistConfig->ROI.width          = inputWidth;
        pHDRBHistConfig->ROI.height         = inputHeight;
        pHDRBHistConfig->greenChannelInput  = HDRBHistSelectGR;
        pHDRBHistConfig->inputFieldSelect   = HDRBHistInputAll;

    }

    if ((FALSE == m_OEMStatsConfig) || (TRUE == m_enableHardcodedConfig) || (TRUE == initalConfig))
    {
        // Currently Internal 3A does not publishes Tintless BG, IHist & CS stats, cofing default values.
        // OEM not using stats node must configure these stats.
        IHistStatsConfig* pIHistConfig = &pModuleInput->pIHistStatsUpdateData->statsConfig;

        pIHistConfig->ROI.top           = 0;
        pIHistConfig->ROI.left          = 0;
        pIHistConfig->ROI.width         = inputWidth - (inputWidth / 10);
        pIHistConfig->ROI.height        = inputHeight - (inputHeight / 10);
        pIHistConfig->channelYCC        = IHistYCCChannelY;
        pIHistConfig->maxPixelSumPerBin = 0;

        // Tintless BG config
        // For Tintless BG, store the configuration in pAECStatsUpdateData.
        BGBEConfig* pBGConfig = &pModuleInput->pAECStatsUpdateData->statsConfig.TintlessBGConfig;

        pBGConfig->channelGainThreshold[ChannelIndexR]  = (1 << IFEPipelineBitWidth) - 1;
        pBGConfig->channelGainThreshold[ChannelIndexGR] = (1 << IFEPipelineBitWidth) - 1;
        pBGConfig->channelGainThreshold[ChannelIndexB]  = (1 << IFEPipelineBitWidth) - 1;
        pBGConfig->channelGainThreshold[ChannelIndexGB] = (1 << IFEPipelineBitWidth) - 1;

        pBGConfig->horizontalNum                        = 32;
        pBGConfig->verticalNum                          = 24;
        pBGConfig->ROI.left                             = 0;
        pBGConfig->ROI.top                              = 0;
        pBGConfig->ROI.width                            = inputWidth;
        pBGConfig->ROI.height                           = inputHeight;
        pBGConfig->outputBitDepth                       = 14;
        pBGConfig->outputMode                           = BGBERegular;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::HardcodeTintlessSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IFENode::HardcodeTintlessSettings(
    ISPInputData*     pModuleInput)
{
    BGBEConfig* pTintlessBGConfig = &pModuleInput->pAECStatsUpdateData->statsConfig.TintlessBGConfig;
    UINT32 CAMIFWidth  = m_ISPInputSensorData.CAMIFCrop.lastPixel - m_ISPInputSensorData.CAMIFCrop.firstPixel + 1;
    UINT32 CAMIFHeight = m_ISPInputSensorData.CAMIFCrop.lastLine - m_ISPInputSensorData.CAMIFCrop.firstLine + 1;

    /// @todo (CAMX-3276) Hard code YUV Sensor Crop configure for now.
    if (TRUE == pModuleInput->sensorData.isYUV)
    {
        CAMIFWidth >>= 1;
    }

    pTintlessBGConfig->channelGainThreshold[ChannelIndexR]  = (1 << IFEPipelineBitWidth) - 1;
    pTintlessBGConfig->channelGainThreshold[ChannelIndexGR] = (1 << IFEPipelineBitWidth) - 1;
    pTintlessBGConfig->channelGainThreshold[ChannelIndexB]  = (1 << IFEPipelineBitWidth) - 1;
    pTintlessBGConfig->channelGainThreshold[ChannelIndexGB] = (1 << IFEPipelineBitWidth) - 1;
    pTintlessBGConfig->horizontalNum  = 32;
    pTintlessBGConfig->verticalNum    = 24;
    pTintlessBGConfig->ROI.left       = 0;
    pTintlessBGConfig->ROI.top        = 0;
    pTintlessBGConfig->ROI.width      = CAMIFWidth;
    pTintlessBGConfig->ROI.height     = CAMIFHeight;
    pTintlessBGConfig->outputBitDepth = 14;
    pTintlessBGConfig->outputMode     = BGBERegular;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::DynamicCAMIFCrop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IFENode::DynamicCAMIFCrop(
    ISPStripeConfig*  pStripeConfig)
{
    if (TRUE == EnableCSIDCropOverridingForSingleIFE())
    {
        CropWindow crop;
        crop.left   = m_instanceProperty.IFECSIDLeft;
        crop.top    = m_instanceProperty.IFECSIDTop;
        crop.width  = m_instanceProperty.IFECSIDWidth;
        crop.height = m_instanceProperty.IFECSIDHeight;

        pStripeConfig->HALCrop[FDOutput]          = crop;
        pStripeConfig->HALCrop[FullOutput]        = crop;
        pStripeConfig->HALCrop[DS4Output]         = crop;
        pStripeConfig->HALCrop[DS16Output]        = crop;
        pStripeConfig->HALCrop[PixelRawOutput]    = crop;
        pStripeConfig->HALCrop[DisplayFullOutput] = crop;
        pStripeConfig->HALCrop[DisplayDS4Output]  = crop;
        pStripeConfig->HALCrop[DisplayDS16Output] = crop;

        pStripeConfig->CAMIFCrop.firstPixel = crop.left;
        pStripeConfig->CAMIFCrop.lastPixel  = crop.width + pStripeConfig->CAMIFCrop.firstPixel - 1;
        pStripeConfig->CAMIFCrop.firstLine  = crop.top;
        pStripeConfig->CAMIFCrop.lastLine   = crop.height + pStripeConfig->CAMIFCrop.firstLine - 1;

        CAMX_LOG_INFO(CamxLogGroupISP, "Dynamic Crop Info being used width=%d height=%d left=%d top=%d",
            crop.width,
            crop.height,
            crop.left,
            crop.top);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CreateCmdBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::CreateCmdBuffers()
{
    CamxResult            result    = CamxResultSuccess;

    // Initializate Command Buffer
    result = InitializeCmdBufferManagerList(static_cast<UINT32>(IFECmdBufferId::NumCmdBuffers));

    if (CamxResultSuccess == result)
    {
        ResourceParams resourceIQParams = { 0 };

        resourceIQParams.usageFlags.packet                  = 1;
        // 3 Command Buffers for GenericBlob, Common (and possibly Left/Right IFEs in dual IFE mode)
        resourceIQParams.packetParams.maxNumCmdBuffers      =
            (IFEModuleMode::DualIFENormal == m_mode) ? MaxCommandBuffersDualIFE : MaxCommandBuffers;

        resourceIQParams.packetParams.maxNumIOConfigs       = MaxDefinedIFEOutputPorts;
        resourceIQParams.packetParams.maxNumPatches         = IFEMaxNumPatches;
        resourceIQParams.packetParams.enableAddrPatching    = 1;
        resourceIQParams.resourceSize                       = Packet::CalculatePacketSize(&resourceIQParams.packetParams);

        // Same number as cmd buffers
        resourceIQParams.poolSize                           = m_IFECmdBlobCount * resourceIQParams.resourceSize;
        resourceIQParams.alignment                          = CamxPacketAlignmentInBytes;
        resourceIQParams.pDeviceIndices                     = NULL;
        resourceIQParams.numDevices                         = 0;
        resourceIQParams.memFlags                           = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("IQPacketManager", &resourceIQParams, &m_pIQPacketManager);

        if (CamxResultSuccess == result)
        {
            ResourceParams params = { 0 };

            // Need to reserve a buffer in the Cmd Buffer for the KMD to do the patching
            params.resourceSize                     = m_totalIQCmdSizeDWord * sizeof(UINT32);
            params.poolSize                         = m_IFECmdBlobCount * params.resourceSize;
            params.usageFlags.cmdBuffer             = 1;
            params.cmdParams.type                   = CmdType::CDMDirect;
            params.alignment                        = CamxCommandBufferAlignmentInBytes;
            params.cmdParams.enableAddrPatching     = 1;
            params.cmdParams.maxNumNestedAddrs      = IFEMaxNumNestedAddrsCommon;
            /// @todo (CAMX-561) Put in IFE device index (when acquire is implemented)
            params.pDeviceIndices                   = DeviceIndices();
            params.numDevices                       = DeviceIndexCount();
            params.memFlags                         = CSLMemFlagUMDAccess;
            if (GetStaticSettings()->ifeSWCDMEnable == TRUE)
            {
                params.resourceSize                        = (m_totalIQCmdSizeDWord * sizeof(UINT32))    +
                                                             (m_total32bitDMISizeDWord * sizeof(UINT32)) +
                                                             (m_total64bitDMISizeDWord * sizeof(UINT32));
                params.poolSize                            = m_IFECmdBlobCount * params.resourceSize;
                params.cmdParams.enableAddrPatching        = 0;
                params.cmdParams.mustInlineIndirectBuffers = 1;
                params.memFlags                            = CSLMemFlagUMDAccess | CSLMemFlagKMDAccess;
                params.cmdParams.maxNumNestedAddrs         = 0;
            }

            result = CreateCmdBufferManager("IQMainCmdBufferManager", &params, &m_pIQMainCmdBufferManager);

            // Create Generic Blob Buffer
            if ((CamxResultSuccess == result) && (m_genericBlobCmdBufferSizeBytes > 0))
            {
                params = { 0 };

                params.resourceSize                     = m_genericBlobCmdBufferSizeBytes;
                params.poolSize                         = m_IFECmdBlobCount * params.resourceSize;
                params.usageFlags.cmdBuffer             = 1;
                params.cmdParams.type                   = CmdType::Generic;
                params.alignment                        = CamxCommandBufferAlignmentInBytes;
                params.cmdParams.enableAddrPatching     = 0;
                params.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
                params.pDeviceIndices                   = NULL;
                params.numDevices                       = 0;

                result = CreateCmdBufferManager("GenericBlobBufferManager", &params, &m_pGenericBlobBufferManager);
            }

            // Create 64 bit DMI Buffer
            if ((CamxResultSuccess == result) && (m_total64bitDMISizeDWord > 0))
            {
                params = { 0 };

                params.resourceSize                     = m_total64bitDMISizeDWord * sizeof(UINT32);
                params.poolSize                         = m_IFECmdBlobCount * params.resourceSize;
                params.usageFlags.cmdBuffer             = 1;
                params.cmdParams.type                   = CmdType::CDMDMI64;
                params.alignment                        = CamxCommandBufferAlignmentInBytes;
                params.cmdParams.enableAddrPatching     = 0;
                params.memFlags                         = CSLMemFlagUMDAccess;
                params.pDeviceIndices                   = DeviceIndices();
                params.numDevices                       = DeviceIndexCount();

                result = CreateCmdBufferManager("64bitDMIBufferManager", &params, &m_p64bitDMIBufferManager);
            }

            if ((CamxResultSuccess == result) && (m_total32bitDMISizeDWord > 0))
            {
                params = { 0 };

                params.resourceSize                     = m_total32bitDMISizeDWord * sizeof(UINT32);
                params.poolSize                         = m_IFECmdBlobCount * params.resourceSize;
                params.usageFlags.cmdBuffer             = 1;
                params.cmdParams.type                   = QueryCDMDMIType(m_titanVersion);
                params.alignment                        = CamxCommandBufferAlignmentInBytes;
                params.cmdParams.enableAddrPatching     = 0;
                params.memFlags                         = CSLMemFlagUMDAccess;
                params.pDeviceIndices                   = DeviceIndices();
                params.numDevices                       = DeviceIndexCount();

                result = CreateCmdBufferManager("32bitDMIBufferManager", &params, &m_p32bitDMIBufferManager);
            }

            // Create Flush Dump Buffer
            if ((CamxResultSuccess == result) && (m_pIFEPipeline->GetFlushDumpBufferSize() > 0))
            {
                params = { 0 };

                params.resourceSize                 = m_pIFEPipeline->GetFlushDumpBufferSize();
                params.poolSize                     = params.resourceSize;
                params.usageFlags.cmdBuffer         = 1;
                params.cmdParams.type               = CmdType::Generic;
                params.alignment                    = CamxCommandBufferAlignmentInBytes;
                params.cmdParams.enableAddrPatching = 0;
                params.memFlags                     = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
                params.pDeviceIndices               = NULL;
                params.numDevices                   = 0;

                result = CreateCmdBufferManager("FlushDumpBufferManager", &params, &m_pFlushDumpBufferManager);
            }

            if ((CamxResultSuccess == result) && (m_pIFEPipeline->GetHangDumpBufferSize() > 0))
            {
                params = { 0 };

                params.resourceSize                 = m_pIFEPipeline->GetHangDumpBufferSize();
                params.poolSize                     = params.resourceSize;
                params.usageFlags.cmdBuffer         = 1;
                params.cmdParams.type               = CmdType::Generic;
                params.alignment                    = CamxCommandBufferAlignmentInBytes;
                params.cmdParams.enableAddrPatching = 0;
                params.memFlags                     = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
                params.pDeviceIndices               = NULL;
                params.numDevices                   = 0;

                result = CreateCmdBufferManager("HangDumpBufferManager", &params, &m_pHangDumpBufferManager);
            }

            if ((CamxResultSuccess == result) && (m_pIFEPipeline->GetRegDumpBufferSize() > 0))
            {
                params                              = { 0 };
                params.resourceSize                 = m_pIFEPipeline->GetRegDumpBufferSize();
                params.poolSize                     = m_IFECmdBlobCount * params.resourceSize;
                params.usageFlags.cmdBuffer         = 1;
                params.cmdParams.type               = CmdType::Generic;
                params.alignment                    = CamxCommandBufferAlignmentInBytes;
                params.cmdParams.enableAddrPatching = 0;
                params.memFlags                     = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
                params.pDeviceIndices               = NULL;
                params.numDevices                   = 0;

                result = CreateCmdBufferManager("RegDumpBufferManager", &params, &m_pRegDumpBufferManager);
            }

            if ((CamxResultSuccess == result) && (IFEModuleMode::DualIFENormal == m_mode))
            {
                params = { 0 };

                // m_totalIQCmdSizeDWord is probably too much but pretty safe.
                params.resourceSize                     = m_totalIQCmdSizeDWord * sizeof(UINT32);
                params.poolSize                         = m_IFECmdBlobCount * params.resourceSize;
                params.usageFlags.cmdBuffer             = 1;
                params.cmdParams.type                   = CmdType::CDMDirect;
                params.alignment                        = CamxCommandBufferAlignmentInBytes;
                params.cmdParams.enableAddrPatching     = 1;
                params.cmdParams.maxNumNestedAddrs      = IFEMaxNumNestedAddrsDualIFESensitive;
                params.pDeviceIndices                   = DeviceIndices();
                params.numDevices                       = DeviceIndexCount();
                params.memFlags                         = CSLMemFlagUMDAccess;

                if (GetStaticSettings()->ifeSWCDMEnable == TRUE)
                {
                    params.resourceSize                        = (m_totalIQCmdSizeDWord * sizeof(UINT32))    +
                                                                 (m_total32bitDMISizeDWord * sizeof(UINT32)) +
                                                                 (m_total64bitDMISizeDWord * sizeof(UINT32));
                    params.poolSize                            = m_IFECmdBlobCount * params.resourceSize;
                    params.cmdParams.enableAddrPatching        = 0;
                    params.cmdParams.mustInlineIndirectBuffers = 1;
                    params.memFlags                            = CSLMemFlagUMDAccess | CSLMemFlagKMDAccess;
                    params.cmdParams.maxNumNestedAddrs         = 0;
                }
                result = CreateCmdBufferManager("IQLeftCmdBufferManager", &params, &m_pIQLeftCmdBufferManager);
                if (CamxResultSuccess == result)
                {
                    result = CreateCmdBufferManager("IQRightCmdBufferManager", &params, &m_pIQRightCmdBufferManager);
                }

                if (CamxResultSuccess == result)
                {
                    params.resourceSize                     = DualIFEUtils::GetMaxStripeConfigSize(m_maxNumOfCSLIFEPortId);
                    params.poolSize                         = m_IFECmdBlobCount * params.resourceSize;
                    params.usageFlags.cmdBuffer             = 1;
                    params.cmdParams.type                   = CmdType::Generic;
                    params.alignment                        = CamxCommandBufferAlignmentInBytes;
                    params.cmdParams.enableAddrPatching     = 0;
                    params.pDeviceIndices                   = NULL;
                    params.numDevices                       = 0;
                    params.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

                    result = CreateCmdBufferManager("DualIFEConfigCmdBufferManager", &params,
                                                    &m_pDualIFEConfigCmdBufferManager);
                }

                if ((CamxResultSuccess == result) && (m_genericBlobCmdBufferSizeBytes > 0))
                {
                    params = { 0 };

                    params.resourceSize                     = m_genericBlobCmdBufferSizeBytes;
                    params.poolSize                         = m_IFECmdBlobCount * params.resourceSize;
                    params.usageFlags.cmdBuffer             = 1;
                    params.cmdParams.type                   = CmdType::Generic;
                    params.alignment                        = CamxCommandBufferAlignmentInBytes;
                    params.cmdParams.enableAddrPatching     = 0;
                    params.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
                    params.pDeviceIndices                   = NULL;
                    params.numDevices                       = 0;

                    result = CreateCmdBufferManager("RightBlobBufferManager", &params, &m_pGenericBlobRightBufferManager);
                }

                if ((CamxResultSuccess == result) && (m_genericBlobCmdBufferSizeBytes > 0))
                {
                    params = { 0 };

                    params.resourceSize                     = m_genericBlobCmdBufferSizeBytes;
                    params.poolSize                         = m_IFECmdBlobCount * params.resourceSize;
                    params.usageFlags.cmdBuffer             = 1;
                    params.cmdParams.type                   = CmdType::Generic;
                    params.alignment                        = CamxCommandBufferAlignmentInBytes;
                    params.cmdParams.enableAddrPatching     = 0;
                    params.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
                    params.pDeviceIndices                   = NULL;
                    params.numDevices                       = 0;

                    result = CreateCmdBufferManager("LeftBlobBufferManager", &params, &m_pGenericBlobLeftBufferManager);
                }
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Failed to Create Cmd Buffer Manager or Command Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::FetchCmdBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::FetchCmdBuffers(
    UINT64  requestId,
    BOOL    isInitPacket)
{
    const StaticSettings*   pSettings   = HwEnvironment::GetInstance()->GetStaticSettings();
    CamxResult              result      = CamxResultSuccess;

    CAMX_ASSERT((NULL != m_pIQPacketManager)                                              &&
                ((m_totalIQCmdSizeDWord <= 0)           || (NULL != m_pIQMainCmdBufferManager)) &&
                ((m_total64bitDMISizeDWord <= 0)        || (NULL != m_p64bitDMIBufferManager))  &&
                ((m_total32bitDMISizeDWord <= 0)        || (NULL != m_p32bitDMIBufferManager))  &&
                ((m_genericBlobCmdBufferSizeBytes <= 0) || (NULL != m_pGenericBlobBufferManager)));

    // Reset all the previous command buffers
    m_pIQPacket                  = NULL;
    m_pCommonCmdBuffer           = NULL;
    m_p64bitDMIBuffer            = NULL;
    m_p32bitDMIBuffer            = NULL;
    m_pLeftCmdBuffer             = NULL;
    m_pRightCmdBuffer            = NULL;
    m_pDualIFEConfigCmdBuffer    = NULL;
    m_pGenericBlobCmdBuffer      = NULL;
    m_pLeftGenericBlobCmdBuffer  = NULL;
    m_pRightGenericBlobCmdBuffer = NULL;
    m_pRegDumpCmdBuffer          = NULL;

    m_pIQPacket = GetPacketForRequest(requestId, m_pIQPacketManager);
    CAMX_ASSERT(NULL != m_pIQPacket);

    if (m_totalIQCmdSizeDWord > 0)
    {
        m_pCommonCmdBuffer = GetCmdBufferForRequest(requestId, m_pIQMainCmdBufferManager);
        CAMX_ASSERT(NULL != m_pCommonCmdBuffer);
    }

    if (m_genericBlobCmdBufferSizeBytes > 0)
    {
        m_pGenericBlobCmdBuffer = GetCmdBufferForRequest(requestId, m_pGenericBlobBufferManager);
        CAMX_ASSERT(NULL != m_pGenericBlobCmdBuffer);
    }

    if (m_total64bitDMISizeDWord > 0)
    {
        m_p64bitDMIBuffer = GetCmdBufferForRequest(requestId, m_p64bitDMIBufferManager);
        if (NULL != m_p64bitDMIBuffer)
        {
            // Reserve the whole DMI Buffer for this frame
            m_p64bitDMIBufferAddr = static_cast<UINT32*>(m_p64bitDMIBuffer->BeginCommands(m_total64bitDMISizeDWord));
        }
    }

    if (m_total32bitDMISizeDWord > 0)
    {
        m_p32bitDMIBuffer = GetCmdBufferForRequest(requestId, m_p32bitDMIBufferManager);
        if (NULL != m_p32bitDMIBuffer)
        {
            // Reserve the whole DMI Buffer for this frame
            m_p32bitDMIBufferAddr = static_cast<UINT32*>(m_p32bitDMIBuffer->BeginCommands(m_total32bitDMISizeDWord));
        }
    }

    // Get the Flush and HangDump Coammnd Buffers for Only Init Request
    if (FALSE == m_RDIOnlyUseCase)
    {
        if (TRUE == isInitPacket)
        {
            if (m_pIFEPipeline->GetFlushDumpBufferSize() > 0)
            {
                m_pFlushDumpCmdBuffer = GetCmdBufferForRequest(requestId, m_pFlushDumpBufferManager);
                if (NULL != m_pFlushDumpCmdBuffer)
                {
                    m_pFlushDumpBufferAddr =
                        static_cast<UINT32*>(m_pFlushDumpCmdBuffer->BeginCommands(
                                            (m_pIFEPipeline->GetFlushDumpBufferSize() / sizeof(UINT32))));
                }
            }

            if (m_pIFEPipeline->GetHangDumpBufferSize() > 0)
            {
                m_pHangDumpCmdBuffer = GetCmdBufferForRequest(requestId, m_pHangDumpBufferManager);
                if (NULL != m_pHangDumpCmdBuffer)
                {
                    m_pHangDumpBufferAddr =
                        static_cast<UINT32*>(m_pHangDumpCmdBuffer->BeginCommands(
                                            (m_pIFEPipeline->GetHangDumpBufferSize() / sizeof(UINT32))));
                }
            }
        }
    }

    // Get the RegDump Command buffers per frame Request
    if ((TRUE == pSettings->enableIFERegDump) &&
        (FALSE == isInitPacket)                         &&
        (m_pIFEPipeline->GetRegDumpBufferSize() > 0))
    {
        m_pRegDumpCmdBuffer = GetCmdBufferForRequest(requestId, m_pRegDumpBufferManager);
        if (NULL != m_pRegDumpCmdBuffer)
        {
            m_pRegDumpBufferAddr =
                static_cast<UINT32*>(m_pRegDumpCmdBuffer->BeginCommands(
                                    (m_pIFEPipeline->GetRegDumpBufferSize() / sizeof(UINT32))));
            m_pIFEPipeline->SetupHangRegDump(m_pRegDumpBufferAddr, m_mode, TRUE);
        }
    }

    // Error Check
    if ((NULL == m_pIQPacket)                                                       ||
        ((m_totalIQCmdSizeDWord    > 0)         && (NULL == m_pCommonCmdBuffer))    ||
        ((m_total64bitDMISizeDWord > 0)         && (NULL == m_p64bitDMIBufferAddr)) ||
        ((m_total32bitDMISizeDWord > 0)         && (NULL == m_p32bitDMIBufferAddr)) ||
        ((m_genericBlobCmdBufferSizeBytes > 0)  && (NULL == m_pGenericBlobCmdBuffer)))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Not able to fetch Cmd buffer for  requestId %llu", requestId);
        result = CamxResultENoMemory;
    }

    if ((CamxResultSuccess            == result)            &&
        (IFEModuleMode::DualIFENormal == m_mode)            &&
        (FALSE                        == m_RDIOnlyUseCase))
    {
        CAMX_ASSERT((NULL != m_pIQLeftCmdBufferManager) && (NULL != m_pIQRightCmdBufferManager));

        m_pLeftCmdBuffer             = GetCmdBufferForRequest(requestId, m_pIQLeftCmdBufferManager);
        m_pRightCmdBuffer            = GetCmdBufferForRequest(requestId, m_pIQRightCmdBufferManager);
        m_pDualIFEConfigCmdBuffer    = GetCmdBufferForRequest(requestId, m_pDualIFEConfigCmdBufferManager);
        m_pLeftGenericBlobCmdBuffer  = GetCmdBufferForRequest(requestId, m_pGenericBlobLeftBufferManager);
        m_pRightGenericBlobCmdBuffer = GetCmdBufferForRequest(requestId, m_pGenericBlobRightBufferManager);

        if ((NULL == m_pLeftCmdBuffer)             ||
            (NULL == m_pRightCmdBuffer)            ||
            (NULL == m_pLeftGenericBlobCmdBuffer)  ||
            (NULL == m_pRightGenericBlobCmdBuffer) ||
            (NULL == m_pDualIFEConfigCmdBuffer))
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not able to fetch Dual IFE Cmd buffer for  requestId %llu", requestId);
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::AddCmdBufferReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::AddCmdBufferReference(
    UINT64              requestId,
    CSLPacketOpcodesIFE opcode,
    BOOL                isInitPacket)
{
    CamxResult result         = CamxResultSuccess;
    UINT32     cmdBufferIndex = 0;

    m_pIQPacket->SetRequestId(GetCSLSyncId(requestId));

    /// @todo (CAMX-656) Need to define the Metadata of the packet
    m_pCommonCmdBuffer->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdCommon));
    result = m_pIQPacket->AddCmdBufferReference(m_pCommonCmdBuffer, &cmdBufferIndex);

    // We may not need Generic Blob Buffer always, only insert if we have some information in it
    if ((NULL != m_pGenericBlobCmdBuffer) && (m_pGenericBlobCmdBuffer->GetResourceUsedDwords() > 0))
    {
        m_pGenericBlobCmdBuffer->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdGenericBlob));
        result = m_pIQPacket->AddCmdBufferReference(m_pGenericBlobCmdBuffer, NULL);
    }

    if (TRUE == isInitPacket)
    {
        // Add Command Buffer Referce
        if (NULL != m_pFlushDumpCmdBuffer)
        {
            m_pFlushDumpCmdBuffer->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdFlushDump));
            result = m_pFlushDumpCmdBuffer->CommitCommands();
            result = m_pIQPacket->AddCmdBufferReference(m_pFlushDumpCmdBuffer, NULL);
        }

        if (NULL != m_pHangDumpCmdBuffer)
        {
            m_pHangDumpCmdBuffer->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdHangDump));
            result = m_pHangDumpCmdBuffer->CommitCommands();
            result = m_pIQPacket->AddCmdBufferReference(m_pHangDumpCmdBuffer, NULL);
        }

    }


    if (NULL != m_pRegDumpCmdBuffer)
    {
        m_pRegDumpCmdBuffer->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdGenericDump));
        result = m_pRegDumpCmdBuffer->CommitCommands();
        result = m_pIQPacket->AddCmdBufferReference(m_pRegDumpCmdBuffer, NULL);
    }

    if ((CamxResultSuccess == result) && (IFEModuleMode::DualIFENormal == m_mode))
    {
        m_pIQPacket->SetOpcode(CSLDeviceType::CSLDeviceTypeIFE, opcode);

        m_pRightCmdBuffer->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdRight));
        result = m_pIQPacket->AddCmdBufferReference(m_pRightCmdBuffer, NULL);

        if (CamxResultSuccess == result)
        {
            m_pLeftCmdBuffer->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdLeft));
            result = m_pIQPacket->AddCmdBufferReference(m_pLeftCmdBuffer, NULL);
        }

        if (CamxResultSuccess == result)
        {
            m_pDualIFEConfigCmdBuffer->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdDualConfig));
            result = m_pIQPacket->AddCmdBufferReference(m_pDualIFEConfigCmdBuffer, NULL);
        }

        if ((NULL != m_pLeftGenericBlobCmdBuffer) && (m_pLeftGenericBlobCmdBuffer->GetResourceUsedDwords() > 0))
        {
            m_pLeftGenericBlobCmdBuffer->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdGenericBlobLeft));
            result = m_pIQPacket->AddCmdBufferReference(m_pLeftGenericBlobCmdBuffer, NULL);
        }

        if ((NULL != m_pRightGenericBlobCmdBuffer) && (m_pRightGenericBlobCmdBuffer->GetResourceUsedDwords() > 0))
        {
            m_pRightGenericBlobCmdBuffer->SetMetadata(static_cast<UINT32>(CSLIFECmdBufferIdGenericBlobRight));
            result = m_pIQPacket->AddCmdBufferReference(m_pRightGenericBlobCmdBuffer, NULL);
        }

    }
    else if (CamxResultSuccess == result)
    {
        m_pIQPacket->SetOpcode(CSLDeviceType::CSLDeviceTypeIFE, opcode);
    }

    if (CamxResultSuccess == result)
    {
        result = m_pIQPacket->SetKMDCmdBufferIndex(cmdBufferIndex,
                                                   (m_pCommonCmdBuffer->GetResourceUsedDwords() * sizeof(UINT32)));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ValidateBufferConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ValidateBufferConfig(
    ImageBuffer* pBuffer,
    UINT         portId)
{
    CamxResult result      = CamxResultSuccess;
    UINT32     width       = 0;
    UINT32     height      = 0;

    if (NULL != pBuffer)
    {
        width  = m_pSensorModeData->cropInfo.lastPixel - m_pSensorModeData->cropInfo.firstPixel + 1;
        height = m_pSensorModeData->cropInfo.lastLine - m_pSensorModeData->cropInfo.firstLine + 1;

        const ImageFormat* pFormat = pBuffer->GetFormat();

        if (NULL != pFormat)
        {
            if ((pFormat->width < width) || (pFormat->height < height))
            {
                CAMX_LOG_ERROR(CamxLogGroupISP,
                               "Buffer Validation Failed..Sensor Dimesions [%d x %d]"
                               "Buffer Dimensions [%d x %d] Port id %d",
                                width, height, pFormat->width, pFormat->height, portId);
                result = CamxResultEFailed;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Buffer is NULL.. Port Id %d", portId);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ConfigBufferIO
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ConfigBufferIO(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CAMX_ASSERT(NULL != pExecuteProcessRequestData);

    CamxResult                result                                 = CamxResultSuccess;
    NodeProcessRequestData*   pNodeRequestData                       = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*    pPerRequestPorts                       = pExecuteProcessRequestData->pEnabledPortsInfo;
    BOOL                      isSnapshot                             = FALSE;
    PerRequestOutputPortInfo* pOutputPort                            = NULL;
    BOOL                      groupFenceSignaled[MaxBufferComposite] = { 0 };
    UINT                      index                                  =
        GetIFEPerFrameDataIndex(pNodeRequestData->pCaptureRequest->requestId);
    UINT&                     rPortIndex                             = m_IFEPerFrameData[index].numIOConfig;

    CAMX_ASSERT(NULL != pNodeRequestData);
    CAMX_ASSERT(NULL != pPerRequestPorts);

    // Initialize m_IFEDebugData[index].numIOConfig
    rPortIndex = 0;

    /// @todo (CAMX-4167) Base node should have called BindBuffers by now, no need to call again.
    // If LateBinding is enabled, output ImageBuffers may not have backing buffers yet, so lets bind the buffers as we are
    // starting to prepare IO configuration
    result = BindInputOutputBuffers(pExecuteProcessRequestData->pEnabledPortsInfo, TRUE, TRUE);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Failed in Binding backing buffers to output ImageBuffers, result=%d", result);
    }

    BOOL isFSSnapshot = IsFSSnapshot(pExecuteProcessRequestData, pNodeRequestData->pCaptureRequest->requestId);

    PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[0];
    // Provide input buffer for offline mode
    if (IFEProfileIdOffline == m_instanceProperty.profileId)
    {
        if (1 == pPerRequestPorts->numInputPorts)
        {
            // There can be only one input buffer,
            result = m_pIQPacket->AddIOConfig(pInputPort->pImageBuffer,
                                              IFEInputBusRead,
                                              CSLIODirection::CSLIODirectionInput,
                                              pInputPort->phFence,
                                              1,
                                              NULL,
                                              NULL,
                                              0);

            CAMX_LOG_INFO(CamxLogGroupISP,
                          "FS: IFE:%d BusRead I/O config, hFence=%d, imgBuf=%p, reqID=%llu rc:%d",
                          InstanceID(),
                          pInputPort->phFence,
                          pInputPort->pImageBuffer,
                          pNodeRequestData->pCaptureRequest->requestId,
                          result);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid number of input ports %d", pPerRequestPorts->numInputPorts);
        }
    }

    for (UINT i = 0; (i < pPerRequestPorts->numOutputPorts) && (CamxResultSuccess == result); i++)
    {
        UINT numBatchedFrames = pNodeRequestData->pCaptureRequest->numBatchedFrames;
        pOutputPort           = &pPerRequestPorts->pOutputPorts[i];
        if (NULL != pOutputPort)
        {
            for (UINT bufferIndex = 0; bufferIndex < pOutputPort->numOutputBuffers; bufferIndex++)
            {
                FrameSubsampleConfig currentConfig;
                UINT32               channelId      = 0;
                ImageBuffer*         pImageBuffer   = pOutputPort->ppImageBuffer[bufferIndex];
                BOOL                 isPortIOConfig = TRUE;

                /// @todo (CAMX-1015) See if this per request translation can be avoided
                result = MapPortIdToChannelId(pOutputPort->portId, &channelId);
                if (((IFEOutputPortRDI0   == pOutputPort->portId)  ||
                    (IFEOutputPortRDI1    == pOutputPort->portId)  ||
                    (IFEOutputPortRDI2    == pOutputPort->portId)) &&
                    (TRUE == IsPixelOutputPortSourceType(pOutputPort->portId)))
                {
                    result = ValidateBufferConfig(pImageBuffer, pOutputPort->portId);
                }

                if (CamxResultSuccess == result)
                {
                    if (NULL != pImageBuffer)
                    {
                        if (TRUE == m_isDisabledOutputPort[OutputPortIndex(pOutputPort->portId)])
                        {
                                // There won't be any HW signal, so signal the fence right away.
                            if ((NULL != pOutputPort) && (CSLInvalidHandle != *pOutputPort->phFence))
                            {
                                CSLFenceSignal(*pOutputPort->phFence, CSLFenceResultSuccess);
                                isPortIOConfig = FALSE;
                            }
                        }
                        else
                        {
                            if (numBatchedFrames > 1)
                            {
                                if (TRUE == pOutputPort->flags.isBatch)
                                {
                                    // Set Pattern to 1 and Period to 0 for no frame dropping
                                    currentConfig.frameDropPattern = 1;
                                    currentConfig.frameDropPeriod  = 0;
                                    currentConfig.subsamplePattern = 1 << (numBatchedFrames - 1);
                                    currentConfig.subsamplePeriod  = (numBatchedFrames - 1);
                                }
                                else
                                {
                                    currentConfig.frameDropPattern = 1;
                                    currentConfig.frameDropPeriod  = (numBatchedFrames - 1);
                                    currentConfig.subsamplePattern = 1;
                                    currentConfig.subsamplePeriod  = 0;
                                }

                                result = m_pIQPacket->AddIOConfig(pImageBuffer,
                                                                  channelId,
                                                                  CSLIODirection::CSLIODirectionOutput,
                                                                  pOutputPort->phFence,
                                                                  1,
                                                                  NULL,
                                                                  &currentConfig,
                                                                  0);
                            }
                            else if (TRUE == m_enableBusRead)
                            {
                                if (TRUE == isFSSnapshot)
                                {
                                    // In FS Snapshot case configure only RDI1
                                    if (IFEOutputPortRDI1 == pOutputPort->portId)
                                    {
                                        CAMX_LOG_INFO(CamxLogGroupISP, "FS: IFE:%d Snapshot - Configure only RDI1",
                                            InstanceID());
                                        result = m_pIQPacket->AddIOConfig(pImageBuffer,
                                                                          channelId,
                                                                          CSLIODirection::CSLIODirectionOutput,
                                                                          pOutputPort->phFence,
                                                                          1,
                                                                          NULL,
                                                                          NULL,
                                                                          0);
                                    }
                                    else
                                    {
                                        UINT groupID   = m_bufferComposite.portGroupID[pOutputPort->portId];
                                        isPortIOConfig = FALSE;
                                        /* For rest of the ports signal the fence right away.
                                           Grouped fences need to be signaled only once */
                                        if ((NULL != pOutputPort)                       &&
                                            (CSLInvalidHandle != *pOutputPort->phFence) &&
                                            (FALSE == groupFenceSignaled[groupID]))
                                        {
                                            CSLFenceSignal(*pOutputPort->phFence, CSLFenceResultSuccess);
                                            groupFenceSignaled[groupID] = TRUE;
                                            CAMX_LOG_INFO(CamxLogGroupISP,
                                                          "FS: IFE:%d Snapshot Signal hFence=%d request=%llu groupID=%u",
                                                          InstanceID(),
                                                          *pOutputPort->phFence,
                                                          pNodeRequestData->pCaptureRequest->requestId,
                                                          groupID);
                                        }
                                    }
                                }
                                else
                                {
                                    // FS - Preview
                                    result = m_pIQPacket->AddIOConfig(pImageBuffer,
                                                                      channelId,
                                                                      CSLIODirection::CSLIODirectionOutput,
                                                                      pOutputPort->phFence,
                                                                      1,
                                                                      NULL,
                                                                      NULL,
                                                                      0);

                                    if (IFEOutputPortRDI0 == pOutputPort->portId)
                                    {
                                        // In FS mode RDI0 WM o/p will be BusRead i/p
                                        result = m_pIQPacket->AddIOConfig(pImageBuffer,
                                                                          IFEInputBusRead,
                                                                          CSLIODirection::CSLIODirectionInput,
                                                                          pOutputPort->phFence,
                                                                          1,
                                                                          NULL,
                                                                          NULL,
                                                                          0);

                                        CAMX_LOG_INFO(CamxLogGroupISP,
                                                      "FS: IFE:%d BusRead I/O config, hFence=%d, imgBuf=%p, reqID=%llu rc:%d",
                                                      InstanceID(),
                                                      *pOutputPort->phFence,
                                                      pImageBuffer,
                                                      pNodeRequestData->pCaptureRequest->requestId,
                                                      result);
                                    }
                                }
                            }
                            else
                            {
                                result = m_pIQPacket->AddIOConfig(pImageBuffer,
                                                                  channelId,
                                                                  CSLIODirection::CSLIODirectionOutput,
                                                                  pOutputPort->phFence,
                                                                  1,
                                                                  NULL,
                                                                  NULL,
                                                                  0);
                            }
                        }
                        const ImageFormat* pOutputImageFormat = pImageBuffer->GetFormat();

                        if ((TRUE == isPortIOConfig) && (NULL != pOutputImageFormat))
                        {
                            CAMX_LOG_INFO(CamxLogGroupISP,
                                          "IFE:%d reporting I/O config, portId=%d, hFence=%d, imgBuf=%p is Pixel Type %d "
                                          "memHandle=0x%X WxH=%dx%d request=%llu, format %d",
                                          InstanceID(),
                                          pOutputPort->portId,
                                          *pOutputPort->phFence,
                                          pImageBuffer,
                                          IsPixelOutputPortSourceType(pOutputPort->portId),
                                          pImageBuffer->GetCSLBufferInfo()->hHandle,
                                          pOutputImageFormat->width,
                                          pOutputImageFormat->height,
                                          pNodeRequestData->pCaptureRequest->requestId,
                                          pOutputImageFormat->format);

                            // Update config io buffer to debug structure
                            m_IFEPerFrameData[index].ioConfig[rPortIndex].portId      = pOutputPort->portId;
                            m_IFEPerFrameData[index].ioConfig[rPortIndex].hFence      = *pOutputPort->phFence;
                            m_IFEPerFrameData[index].ioConfig[rPortIndex].hMemHandle  =
                                pImageBuffer->GetCSLBufferInfo()->hHandle;
                            rPortIndex++;
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP, "Image Buffer is Null for output buffer %d", bufferIndex);
                        result = CamxResultEInvalidState;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Not able to find matching Ouput ID");
                    result = CamxResultEInvalidState;
                }

                if (CamxResultSuccess != result)
                {
                    break;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CommitAndSubmitPacket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::CommitAndSubmitPacket()
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_p64bitDMIBuffer)
    {
        result = m_p64bitDMIBuffer->CommitCommands();
        CAMX_ASSERT(CamxResultSuccess == result);
    }

    if ((CamxResultSuccess == result) && (NULL != m_p32bitDMIBuffer))
    {
        result = m_p32bitDMIBuffer->CommitCommands();
        CAMX_ASSERT(CamxResultSuccess == result);
    }

    if (CamxResultSuccess == result)
    {
        result = m_pIQPacket->CommitPacket();
        CAMX_ASSERT(CamxResultSuccess == result);
    }

    if (CamxResultSuccess == result)
    {
        result = GetHwContext()->Submit(GetCSLSession(), m_hDevice, m_pIQPacket);
    }

    if (CamxResultSuccess != result)
    {
        if (CamxResultECancelledRequest == result)
        {
            CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s CSL Submit Failed: %s due to Ongoing flush",
                NodeIdentifierString(), Utils::CamxResultToString(result));
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s CSL Submit Failed: %s",
                NodeIdentifierString(), Utils::CamxResultToString(result));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CheckForRDIOnly
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::CheckForRDIOnly()
{
    UINT       outputPortId[MaxDefinedIFEOutputPorts];
    UINT       totalOutputPort                  = 0;
    CamxResult result                           = CamxResultSuccess;

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
        if ((IFEOutputPortRDI0        != outputPortId[index]) &&
            (IFEOutputPortRDI1        != outputPortId[index]) &&
            (IFEOutputPortRDI2        != outputPortId[index]) &&
            (IFEOutputPortRDI3        != outputPortId[index]) &&
            (IFEOutputPortStatsDualPD != outputPortId[index]))
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
// IFENode::ReadDefaultStatsConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ReadDefaultStatsConfig(
    ISPInputData* pModuleInput)
{
    CamxResult              result               = CamxResultSuccess;
    static const UINT Properties3A[] =
    {
        PropertyIDUsecaseAECFrameControl,
        PropertyIDUsecaseAWBFrameControl,
        PropertyIDUsecaseAECStatsControl,
        PropertyIDUsecaseAWBStatsControl,
        PropertyIDAFDStatsControl,
        PropertyIDUsecaseHWPDConfig
    };

    VOID* pPropertyData3A[CAMX_ARRAY_SIZE(Properties3A)]       = { 0 };
    UINT64 propertyData3AOffset[CAMX_ARRAY_SIZE(Properties3A)] = { 0 };
    UINT length                    = CAMX_ARRAY_SIZE(Properties3A);

    GetDataList(Properties3A, pPropertyData3A, propertyData3AOffset, length);

    if (NULL != pPropertyData3A[0])
    {
        Utils::Memcpy(pModuleInput->pAECUpdateData, pPropertyData3A[0], sizeof(AECFrameControl));
    }
    if (NULL != pPropertyData3A[1])
    {
        Utils::Memcpy(pModuleInput->pAWBUpdateData, pPropertyData3A[1], sizeof(AWBFrameControl));
    }
    if (NULL != pPropertyData3A[2])
    {
        Utils::Memcpy(pModuleInput->pAECStatsUpdateData, pPropertyData3A[2], sizeof(AECStatsControl));
    }
    if (NULL != pPropertyData3A[3])
    {
        Utils::Memcpy(pModuleInput->pAWBStatsUpdateData, pPropertyData3A[3], sizeof(AWBStatsControl));
    }
    if (NULL != pPropertyData3A[4])
    {
        Utils::Memcpy(pModuleInput->pAFDStatsUpdateData, pPropertyData3A[4], sizeof(AFDStatsControl));
    }
    if (NULL != pPropertyData3A[5])
    {
        Utils::Memcpy(pModuleInput->pPDHwConfig, pPropertyData3A[5], sizeof(PDHwConfig));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "PDAF HW Configuration is NULL");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ReleaseResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ReleaseResources(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult            result  = CamxResultSuccess;
    UINT        perFrameDataIndex = GetIFEPerFrameDataIndex(m_IFELastAcceptedRequestId);

    if (CHIDeactivateModeSensorStandby & modeBitmask)
    {
        CAMX_LOG_INFO(CamxLogGroupISP,
                      "IFE:%d IFE %p Skipping Resource Release ModeMask 0x%X device handle %d",
                      InstanceID(),
                      this,
                      modeBitmask,
                      m_hDevice);
    }
    else
    {
        result = CSLReleaseHardware(GetCSLSession(),
                                    m_hDevice);
        RecycleAllCmdBuffers();
        if (result == CamxResultSuccess)
        {
            m_isIFEResourceAcquried = FALSE;
            CAMX_LOG_INFO(CamxLogGroupISP,
                      "IFE:%d IFE %p Resource Release ModeMask 0x%X device handle %d",
                      InstanceID(),
                      this,
                      modeBitmask,
                      m_hDevice);
        }
    }

    // Initialize Stripe config with latest accepted requestId just before streamoff
    if ((m_IFELastAcceptedRequestId != CamxInvalidRequestId) &&
        (m_IFELastAcceptedRequestId == m_IFEPerFrameData[perFrameDataIndex].requestID))
    {
        // This Config will be used to updateIQConfiguration during AcquireResource
        m_frameConfig.stateLSC = m_IFEPerFrameData[perFrameDataIndex].frameConfig.stateLSC;
    }
    // Release all the command buffers to the free pool.
    // Note that the command buffers and its buffer managers are not destroyed.
    RecycleAllCmdBuffers();

    // Reset the CSL sync ID lookup table.
    ResetCSLSyncIDLookupTable();

    ClearIFEDebugConfig();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::UpdateInitIQSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::UpdateInitIQSettings()
{
    CamxResult result = CamxResultSuccess;

    m_IFELastAcceptedRequestId                    = CamxInvalidRequestId;
    m_IFEInitData.pOTPData                        = m_pOTPData;
    m_IFEInitData.frameNum                        = FirstValidRequestId;
    m_IFEInitData.pHwContext                      = GetHwContext();
    m_IFEInitData.pAECUpdateData                  = &m_frameConfig.AECUpdateData;
    m_IFEInitData.pAECStatsUpdateData             = &m_frameConfig.AECStatsUpdateData;
    m_IFEInitData.pAWBUpdateData                  = &m_frameConfig.AWBUpdateData;
    m_IFEInitData.pAWBStatsUpdateData             = &m_frameConfig.AWBStatsUpdateData;
    m_IFEInitData.pAFUpdateData                   = &m_frameConfig.AFUpdateData;
    m_IFEInitData.pAFStatsUpdateData              = &m_frameConfig.AFStatsUpdateData;
    m_IFEInitData.pAFDStatsUpdateData             = &m_frameConfig.AFDStatsUpdateData;
    m_IFEInitData.pIHistStatsUpdateData           = &m_frameConfig.IHistStatsUpdateData;
    m_IFEInitData.pPDHwConfig                     = &m_frameConfig.pdHwConfig;
    m_IFEInitData.pCSStatsUpdateData              = &m_frameConfig.CSStatsUpdateData;
    m_IFEInitData.pStripeConfig                   = &m_frameConfig;
    m_IFEInitData.pHALTagsData                    = &m_HALTagsData;
    m_IFEInitData.disableManual3ACCM              = m_disableManual3ACCM;
    m_IFEInitData.isDualcamera                    = GetPipeline()->IsMultiCamera();
    m_IFEInitData.useHardcodedRegisterValues      = CheckToUseHardcodedRegValues(m_IFEInitData.pHwContext);
    m_IFEInitData.pTuningData                     = &m_tuningData;
    m_IFEInitData.tuningModeChanged               = ISPIQModule::IsTuningModeDataChanged(m_IFEInitData.pTuningData,
                                                                                         NULL);
    m_IFEInitData.maxOutputWidthFD                = m_maxOutputWidthFD;
    m_IFEInitData.titanVersion                    = m_titanVersion;
    m_IFEInitData.forceTriggerUpdate              = TRUE;
    m_IFEInitData.isInitPacket                    = TRUE;

    m_IFEInitData.sensorBitWidth                  = m_pSensorCaps->sensorConfigs[0].streamConfigs[0].bitWidth;
    m_IFEInitData.sensorData.sensorBinningFactor  = m_ISPInputSensorData.sensorBinningFactor;
    m_IFEInitData.sensorData.fullResolutionWidth  = m_pSensorModeRes0Data->resolution.outputWidth;
    m_IFEInitData.sensorData.fullResolutionHeight = m_pSensorModeRes0Data->resolution.outputHeight;
    m_IFEInitData.pFrameBased                     = &m_frameBased[0];
    m_IFEInitData.pRDIStreams                     = &m_RDIStreams[0];
    m_IFEInitData.IFEPixelRawPort                 = m_IFEPixelRawPort;

    if (TRUE == m_csidBinningInfo.isBinningEnabled)
    {
        m_IFEInitData.sensorData.fullResolutionWidth    >>= 1;
        m_IFEInitData.sensorData.fullResolutionHeight   >>= 1;
    }


    m_IFEInitData.IFENodeInstanceProperty = m_instanceProperty;

    HardcodeSettings(&m_IFEInitData, &m_frameConfig, TRUE);

    DynamicCAMIFCrop(&m_frameConfig);

    if (TRUE == m_useStatsAlgoConfig)
    {
        // OEM can not update config as part of config streams, use hardcode values
        if (FALSE == m_OEMStatsConfig)
        {
            // Overwrites with the proper stats configuration information
            result = ReadDefaultStatsConfig(&m_IFEInitData);
            // HardCode Tintless Settings
            HardcodeTintlessSettings(&m_IFEInitData);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to read the default stream on stats configuration!");
        }
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::UpdateInitSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::UpdateInitSettings()
{
    CamxResult result             = CamxResultSuccess;
    INT32      numIFEsUsed        = SingleIFE;

    BOOL       forceSingleIFEForPDAFType3     = FALSE;

    Utils::Memset(&m_IFEInitData, 0, sizeof(ISPInputData));

    m_IFEInitData.pIFEOutputPathInfo    = m_IFEOutputPathInfo;
    m_IFEInitData.pTuningDataManager    = GetTuningDataManager();
    m_IFEInitData.csidBinningInfo       = m_csidBinningInfo;

    Utils::Memcpy(&m_IFEInitData.sensorData, &m_ISPInputSensorData, sizeof(ISPSensorConfigureData));

    CSLCameraTitanChipVersion titanChipVersion =
     static_cast <Titan17xContext *> (GetHwContext())->GetTitanChipVersion();

    if ((TRUE == CheckIfPDAFType3Supported()) &&
        (titanChipVersion == CSLCameraTitanChipVersion::CSLTitan170V200))
    {
        forceSingleIFEForPDAFType3 = TRUE;
    }

    if (TRUE == m_HVXInputData.enableHVX)
    {
        SetupHVXInitialConfig(&m_IFEInitData);
    }

    UpdateInitIQSettings();

    // Check for FastShutter (FS) Mode. Enable IFE read/fetch patch
    result = IsFSModeEnabled(&m_enableBusRead);
    if (TRUE == m_enableBusRead)
    {
        m_genericBlobCmdBufferSizeBytes += sizeof(IFEBusReadConfig);
        CAMX_LOG_INFO(CamxLogGroupISP, "FS: IFE Read Path Enabled");
    }

    if (CamxResultSuccess == result)
    {
        ISPIQTuningDataBuffer   IQOEMTriggerData;

        // Get optional OEM trigger data if available
        if (NULL != m_IFEInitData.pIFETuningMetadata)
        {
            IQOEMTriggerData.pBuffer    = m_IFEInitData.pIFETuningMetadata->oemTuningData.IQOEMTuningData;
            IQOEMTriggerData.size       = sizeof(m_IFEInitData.pIFETuningMetadata->oemTuningData.IQOEMTuningData);
        }
        else
        {
            IQOEMTriggerData.pBuffer    = NULL;
            IQOEMTriggerData.size       = 0;
        }

        m_IFEInitData.minRequiredSingleIFEClock = CalculatePixelClockRate(m_pSensorModeData->resolution.outputWidth);

        IQInterface::IQSetupTriggerData(&m_IFEInitData, this, GetMaximumPipelineDelay(), &IQOEMTriggerData);

        m_IFEInitData.forceIFEflag = m_instanceProperty.IFESingleOn;

        m_IFEInitData.RDIOnlyCase = m_RDIOnlyUseCase;

        m_mode = DualIFEUtils::EvaluateDualIFEMode(&m_IFEInitData, m_enableBusRead, forceSingleIFEForPDAFType3);
        CAMX_LOG_INFO(CamxLogGroupISP, "IFEMode %d vfeClk %lld sensorWidth %d",
                m_mode, m_IFEInitData.minRequiredSingleIFEClock, m_pSensorModeData->resolution.outputWidth);

        m_IFEInitData.pipelineIFEData.moduleMode = m_mode;
        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            numIFEsUsed = DualIFE;
            PFStripeInterface  pStripeInterfaceAlgoEntry = NULL;
            pStripeInterfaceAlgoEntry = reinterpret_cast<PFStripeInterface>(OsUtils::LoadPreBuiltLibrary("libcamxifestriping",
                    "CreateIFEStripeInterface", &m_hHandle));

            if (NULL != pStripeInterfaceAlgoEntry)
            {
                pStripeInterfaceAlgoEntry(&m_pIFEStripeInterface);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to Load Striping Library");
            }

            m_IFEInitData.pIFEStripeInterface = m_pIFEStripeInterface;

            m_stripeConfigs[0].overwriteStripes = IFEOverwriteModuleWithStriping;
            m_stripeConfigs[1].overwriteStripes = IFEOverwriteModuleWithStriping;

            m_pStripingInput = static_cast<IFEStripingInput*>(CAMX_CALLOC(sizeof(IFEStripingInput)));
            m_pPassOut = static_cast<IFEStripingPassOutput*>(CAMX_CALLOC(sizeof(IFEStripingPassOutput)));
            if ((NULL == m_pStripingInput) || (NULL == m_pPassOut))
            {
                result = CamxResultENoMemory;
            }
            if (CamxResultSuccess == result)
            {
                // Store bankSelect for interpolation to store mesh_table_l[bankSelect] and mesh_table_r[bankSelect]
                m_frameConfig.pFrameLevelData                    = &m_ISPFrameData;
                m_IFEInitData.pStripingInput                     = m_pStripingInput;
                m_IFEInitData.pStripeConfig                      = &m_frameConfig;
                m_IFEInitData.pCalculatedData                    = &m_ISPFramelevelData;
                m_IFEInitData.pCalculatedMetadata                = &m_ISPMetadata;

                result = PrepareStripingParameters(&m_IFEInitData);
                if (CamxResultSuccess == result)
                {
                    result = DualIFEUtils::ComputeSplitParams(&m_IFEInitData,
                                                              &m_dualIFESplitParams,
                                                              m_pSensorModeData,
                                                              m_pIFEStripeInterface);

                }
            }
        }
    }

    // Update usecase pool vendor tag to indicate the number of IFEs used for this instance.
    if (CamxResultSuccess == result)
    {
        UINT32 metaTag = 0;

        result = VendorTagManager::QueryVendorTagLocation(
            "org.quic.camera.ISPConfigData", "numIFEsUsed", &metaTag);

        if (CamxResultSuccess == result)
        {
            const UINT  metadataTag[] = { metaTag | UsecaseMetadataSectionMask };
            const VOID* pDstData[1]   = { &numIFEsUsed };
            UINT        pDataCount[1] = { 1 };

            result = WriteDataList(metadataTag, pDstData, pDataCount, 1);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Node::%s Failed to publish num of IFEs used for this instance",
                    NodeIdentifierString());
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::AcquireResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::AcquireResources()
{
    CamxResult              result = CamxResultSuccess;


    if (FALSE == m_isIFEResourceAcquried)
    {
        CAMX_LOG_INFO(CamxLogGroupISP,
                      "IFE:%d IFE %p Acquiring Resource device handle %d",
                      InstanceID(),
                      this,
                      m_hDevice);
        // Acquire IFE device resources
        CSLDeviceResource deviceResource;

        result = SetupDeviceResource();

        if (m_numResource <= 0)
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupISP, "No Resource Needs to be Set up");
        }
        else
        {
            if (CamxResultSuccess == result)
            {
                Utils::Memset(&m_IFEAcquiredHWInfo, 0, sizeof(m_IFEAcquiredHWInfo));

                deviceResource.deviceResourceParamSize = m_IFEResourceSize;
                deviceResource.pDeviceResourceParam    = m_pIFEResourceInfo;
                deviceResource.resourceID              = ISPResourceIdPort;
                deviceResource.pReturnParams           = &m_IFEAcquiredHWInfo;

                result = CSLAcquireHardware(GetCSLSession(),
                                            m_hDevice,
                                            &deviceResource,
                                            NodeIdentifierString(),
                                            m_pIFEPipeline->GetAcquireHWStructVersion());
            }
            // Create and Submit initial config packet
            if (CamxResultSuccess == result)
            {
                m_isIFEResourceAcquried = TRUE;
                result                  = FetchCmdBuffers(0, TRUE);
            }

            if (CamxResultSuccess == result)
            {
                if (IFEModuleMode::DualIFENormal == m_mode)
                {
                    m_stripeConfigs[0].stateLSC       = m_frameConfig.stateLSC;
                    m_stripeConfigs[1].stateLSC       = m_frameConfig.stateLSC;
                    m_frameConfig.pFrameLevelData     = &m_ISPFrameData;
                    m_IFEInitData.pStripeConfig       = &m_frameConfig;
                    m_IFEInitData.pCalculatedData     = &m_ISPFramelevelData;
                    m_IFEInitData.pCalculatedMetadata = &m_ISPMetadata;
                    m_IFEInitData.pStripingInput      = m_pStripingInput;

                    result = DualIFEUtils::UpdateDualIFEConfig(&m_IFEInitData,
                                                               m_PDAFInfo,
                                                               m_stripeConfigs,
                                                               &m_dualIFESplitParams,
                                                               m_pPassOut,
                                                               m_pSensorModeData,
                                                               m_pIFEStripeInterface,
                                                               NodeIdentifierString());
                }
                else
                {
                    CamX::Utils::Memset(m_stripeConfigs[0].stateDS, 0, sizeof(m_stripeConfigs[0].stateDS));

                    // Each module has stripe specific configuration that needs to be updated before calling upon module
                    // Execute function. In single IFE case, there's only one stripe and hence we will use
                    // m_stripeConfigs[0] to hold the input configuration
                    UpdateIQStateConfiguration(&m_frameConfig, &m_stripeConfigs[0]);
                }

                if (TRUE == m_libInitialData.isSucceed)
                {
                    m_IFEInitData.pLibInitialData = m_libInitialData.pLibData;
                }

                m_IFEInitData.p64bitDMIBuffer     = m_p64bitDMIBuffer;
                m_IFEInitData.p32bitDMIBuffer     = m_p32bitDMIBuffer;
                m_IFEInitData.p32bitDMIBufferAddr = m_p32bitDMIBufferAddr;
                m_IFEInitData.p64bitDMIBufferAddr = m_p64bitDMIBufferAddr;
                m_IFEInitData.pStripeConfig       = &m_frameConfig;
                m_frameConfig.pFrameLevelData     = &m_ISPFrameData;
                m_IFEInitData.pCalculatedMetadata = &m_ISPMetadata;


                m_IFEInitData.pipelineIFEData.moduleMode = m_mode;

                // CAMIF needs to be programed during the initialization time
                m_IFEInitData.pipelineIFEData.programCAMIF        = TRUE;
                m_IFEInitData.pipelineIFEData.numBatchedFrames    = m_usecaseNumBatchedFrames;
                m_IFEInitData.dualPDPipelineData.programCAMIFLite = TRUE;
                m_IFEInitData.dualPDPipelineData.numBatchedFrames = m_usecaseNumBatchedFrames;

                // Check if KMD return valid acquired hw info, only hw v2 has valid acuqired hw
                if (0 < m_IFEAcquiredHWInfo.valid_acquired_hw &&
                    IFEHWMaxCounts > m_IFEAcquiredHWInfo.valid_acquired_hw)
                {
                    result = InitialCAMIFConfig();
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupISP, "valid hw %d, hw struct version %d",
                                    m_IFEAcquiredHWInfo.valid_acquired_hw,
                                    m_pIFEPipeline->GetAcquireHWStructVersion());
                }

                // Ensure that the init packet DMI bank value is zero, every time pipeline is reactivated
                m_IFEInitData.isInitPacket          = TRUE;
                m_IFEInitData.forceTriggerUpdate    = TRUE;
                m_pIFEPipeline->UpdateDMIBankSelectValue(&m_IFEInitData.bankUpdate, TRUE);
                CAMX_LOG_INFO(CamxLogGroupISP, "Node::%s Update DMI bank to 0 for init packet.", NodeIdentifierString());

                // Execute IQ module configuration and updated module enable info
                result = ProgramIQConfig(&m_IFEInitData);

                if ((CamxResultSuccess == result) && (FALSE == m_RDIOnlyUseCase))
                {
                    result = ProgramIQEnable(&m_IFEInitData);
                }
                m_IFELastAcceptedRequestId = CamxInvalidRequestId;
            }

            // Initailiaze the non IQ settings like Batch mode, clock settings and UBWC config
            if (CamxResultSuccess == result)
            {
                result = InitialSetupandConfig(&m_IFEInitData);
            }

            if (CamxResultSuccess == result)
            {
                InitIFEPerFrameConfig();
            }

            // Add all command buffer reference to the packet
            if (CamxResultSuccess == result)
            {
                if (IFEModuleMode::DualIFENormal == m_mode)
                {
                    result = AddCmdBufferReference(0, CSLPacketOpcodesDualIFEInitialConfig, TRUE);
                }
                else
                {
                    result = AddCmdBufferReference(0, CSLPacketOpcodesIFEInitialConfig, TRUE);
                }
            }

            // Commit DMI command buffers and do a CSL submit of the request
            if (CamxResultSuccess == result)
            {
                result = CommitAndSubmitPacket();
            }
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupISP,
                      "IFE:%d IFE %p Skipping Resource Acquire device handle %d",
                      InstanceID(),
                      this,
                      m_hDevice);
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::NewActiveStreamsSetup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::NewActiveStreamsSetup(
    UINT activeStreamIdMask)
{
    BOOL isNodeEnabled         = FALSE;
    UINT fullPortEnableMask    = 0x0;
    UINT displayPortEnableMask = 0x0;
    UINT numOutputPortsEnabled = 0;

    // Mask all DS4/DS16 and Full port mask
    // Such that all ports are enabled if any port is enabled
    for (UINT portIndex = 0; portIndex < GetNumOutputPorts(); portIndex++)
    {
        OutputPort* pOutputPort =  GetOutputPort(portIndex);

        if ((IFEOutputPortDS4 == pOutputPort->portId) ||
            (IFEOutputPortDS16 == pOutputPort->portId) ||
            (IFEOutputPortFull == pOutputPort->portId))
        {
            fullPortEnableMask |= pOutputPort->enabledInStreamMask;
        }
        else if ((IFEOutputPortDisplayDS4 == pOutputPort->portId) ||
                 (IFEOutputPortDisplayDS16 == pOutputPort->portId) ||
                 (IFEOutputPortDisplayFull == pOutputPort->portId))
        {
            displayPortEnableMask |= pOutputPort->enabledInStreamMask;
        }
    }

    for (UINT portIndex = 0; portIndex < GetNumOutputPorts(); portIndex++)
    {
        OutputPort* pOutputPort = GetOutputPort(portIndex);

        UINT mask = 0x0;

        if ((IFEOutputPortDS4 == pOutputPort->portId) ||
            (IFEOutputPortDS16 == pOutputPort->portId) ||
            (IFEOutputPortFull == pOutputPort->portId))
        {
            // Replace EnableInstream mask with the calculated mask
            mask = fullPortEnableMask;
        }
        else if ((IFEOutputPortDisplayDS4 == pOutputPort->portId) ||
                 (IFEOutputPortDisplayDS16 == pOutputPort->portId) ||
                 (IFEOutputPortDisplayFull == pOutputPort->portId))
        {
            mask = displayPortEnableMask;
        }
        else
        {
            mask = pOutputPort->enabledInStreamMask;
        }

        // "enabledInStreamMask" is the list of streams for which the output port needs to be enabled. So if any stream in
        // "activeStreamIdMask" is also set in "enabledInStreamMask", it means we need to enable the output port
        if (0 != (mask & activeStreamIdMask) &&
            pOutputPort->numInputPortsConnected > pOutputPort->numInputPortsDisabled)
        {
            pOutputPort->flags.isEnabled = TRUE;
            numOutputPortsEnabled++;
        }
        else
        {
            pOutputPort->flags.isEnabled = FALSE;
        }
    }


    isNodeEnabled = ((0 == GetNumOutputPorts()) || (0 < numOutputPortsEnabled)) ? TRUE : FALSE;

    SetNodeEnabled(isNodeEnabled);

    UINT inputPortId[MaxDefinedIFEInputPorts];
    UINT totalInputPort                        = 0;
    // Get Input Port List

    GetAllInputPortIds(&totalInputPort, &inputPortId[0]);

    /// @note It is assumed that if any output port is enabled, all the input ports are active
    ///       i.e. all inputs are required to generate any (and all) outputs
    for (UINT portIndex = 0; portIndex < totalInputPort; portIndex++)
    {
        InputPort* pInputPort       = GetInputPort(portIndex);
        if (FALSE == pInputPort->portDisabled)
        {
            pInputPort->flags.isEnabled = isNodeEnabled;
        }
        else
        {
            pInputPort->flags.isEnabled = FALSE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetSensorAspectRatioMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFENode::GetSensorAspectRatioMode()
{
    CamxResult              result                     = CamxResultSuccess;
    FLOAT                   sensorAspectRatio          = 0.0f;
    UINT32                  sensorAspectRatioMode      = 0;

    sensorAspectRatio         =
        static_cast<FLOAT>(m_pSensorModeData->resolution.outputWidth) /
        static_cast<FLOAT>(m_pSensorModeData->resolution.outputHeight);
    if (sensorAspectRatio <= (m_sensorActiveArrayAR + AspectRatioTolerance))
    {
        sensorAspectRatioMode = SensorAspectRatioMode::FULLMODE;
    }
    else
    {
        sensorAspectRatioMode = SensorAspectRatioMode::NON_FULLMODE;
    }

    return sensorAspectRatioMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::PostPipelineCreate()
{
    CamxResult              result                     = CamxResultSuccess;
    const StaticSettings*   pSettings                  = HwEnvironment::GetInstance()->GetStaticSettings();

    m_tuningData.noOfSelectionParameter = 1;
    m_tuningData.TuningMode[0].mode     = ChiModeType::Default;

    m_highInitialBWCnt = 0;

    if (CamxResultSuccess == result)
    {
        // Assemble IFE IQ Modules
        result = CreateIFEIQModules();

        // Assemble IFE Stats Module
        if (CamxResultSuccess == result && FALSE == m_RDIOnlyUseCase)
        {
            result = CreateIFEStatsModules();
        }

        if (CamxResultSuccess == result)
        {
            CalculateIQCmdSize();
            // Each member in IFEModuleEnableConfig points to an 32 bit register, and dont have continuous offset
            // Each register needs a header info while writing into pCmdBuffer.
            m_totalIQCmdSizeDWord += (sizeof(IFEModuleEnableConfig) / RegisterWidthInBytes) *
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(RegisterWidthInBytes);
        }

        if (CamxResultSuccess != result)
        {
            Cleanup();
        }
    }

    // IFE sink only use case, where FinalizeBufferProperties wont be called
    if (CamxResultSuccess == result)
    {
        result = FetchSensorInfo();
    }

    // Update IFE Sensor inputdata structure
    if (CamxResultSuccess == result)
    {

        // Fetch IFE node input data, update batching size and IFE output info
        if (CamxResultSuccess == result)
        {
            result = GetFPS();

            // Read numbers of frames in batch
            const UINT PropUsecase[] = { PropertyIDUsecaseBatch };
            VOID* pData[] = { 0 };
            UINT64 dataOffset[1] = { 0 };
            GetDataList(PropUsecase, pData, dataOffset, 1);
            if (NULL != pData[0])
            {
                m_usecaseNumBatchedFrames = *reinterpret_cast<UINT*>(pData[0]);
            }

            // Read dimension and format from output ports for HAL data.
            CamX::Utils::Memset(&m_ISPInputHALData, 0, sizeof(m_ISPInputHALData));
            InitializeOutputPathImageInfo();
        }

        // Set the sensor data information for ISP input
        m_ISPInputSensorData.CAMIFCrop.firstPixel      = m_pSensorModeData->cropInfo.firstPixel;
        m_ISPInputSensorData.CAMIFCrop.firstLine       = m_pSensorModeData->cropInfo.firstLine;
        m_ISPInputSensorData.CAMIFCrop.lastPixel       = m_pSensorModeData->cropInfo.lastPixel;
        m_ISPInputSensorData.CAMIFCrop.lastLine        = m_pSensorModeData->cropInfo.lastLine;

        // CSID crop override
        if (TRUE == EnableCSIDCropOverridingForSingleIFE())
        {
            m_ISPInputSensorData.CAMIFCrop.firstPixel = m_instanceProperty.IFECSIDLeft;
            m_ISPInputSensorData.CAMIFCrop.lastPixel  = m_instanceProperty.IFECSIDLeft + m_instanceProperty.IFECSIDWidth - 1;
            m_ISPInputSensorData.CAMIFCrop.firstLine  = m_instanceProperty.IFECSIDTop;
            m_ISPInputSensorData.CAMIFCrop.lastLine   = m_instanceProperty.IFECSIDTop + m_instanceProperty.IFECSIDHeight - 1;
        }

        if (TRUE == m_csidBinningInfo.isBinningEnabled)
        {
            m_ISPInputSensorData.CAMIFCrop.lastPixel >>= 1;
            m_ISPInputSensorData.CAMIFCrop.lastLine  >>= 1;
            m_ISPInputSensorData.CSIDBinningFactor     = 2;
        }
        else
        {
            m_ISPInputSensorData.CSIDBinningFactor     = 1;
        }

        m_ISPInputSensorData.format                    = m_pSensorModeData->format;
        m_ISPInputSensorData.isBayer                   = IsSensorModeFormatBayer(m_ISPInputSensorData.format);
        m_ISPInputSensorData.isYUV                     = IsSensorModeFormatYUV(m_ISPInputSensorData.format);
        m_ISPInputSensorData.isMono                    = IsSensorModeFormatMono(m_ISPInputSensorData.format);
        for (UINT32 i = 0; i < m_pSensorModeData->capabilityCount; i++)
        {
            if (m_pSensorModeData->capability[i] == SensorCapability::IHDR)
            {
                m_ISPInputSensorData.isIHDR        = TRUE;
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "sensor I-HDR mode");
                break;
            }
        }
        m_ISPInputSensorData.sensorAspectRatioMode     = GetSensorAspectRatioMode();
        m_ISPInputSensorData.sensorOut.width           = m_pSensorModeData->resolution.outputWidth;
        m_ISPInputSensorData.sensorOut.height          = m_pSensorModeData->resolution.outputHeight;

        m_ISPInputSensorData.sensorOut.offsetX         = m_pSensorModeData->offset.xStart;
        m_ISPInputSensorData.sensorOut.offsetY         = m_pSensorModeData->offset.yStart;

        m_ISPInputSensorData.dGain                     = 1.0f;
        m_ISPInputSensorData.sensorScalingFactor       = m_pSensorModeData->downScaleFactor;
        m_ISPInputSensorData.sensorBinningFactor       = static_cast<FLOAT>(m_pSensorModeData->binningTypeH);
        m_ISPInputSensorData.ZZHDRColorPattern         = m_pSensorModeData->ZZHDRColorPattern;
        m_ISPInputSensorData.ZZHDRFirstExposure        = m_pSensorModeData->ZZHDRFirstExposure;
        m_ISPInputSensorData.fullResolutionWidth       = m_pSensorModeRes0Data->resolution.outputWidth;
        m_ISPInputSensorData.fullResolutionHeight      = m_pSensorModeRes0Data->resolution.outputHeight;
        if (TRUE == m_csidBinningInfo.isBinningEnabled)
        {
            m_ISPInputSensorData.fullResolutionWidth    >>= 1;
            m_ISPInputSensorData.fullResolutionHeight   >>= 1;
        }

        m_HALTagsData.HALCrop.left   = 0;
        m_HALTagsData.HALCrop.top    = 0;
        m_HALTagsData.HALCrop.width  = m_ISPInputSensorData.CAMIFCrop.lastPixel - m_ISPInputSensorData.CAMIFCrop.firstPixel + 1;
        m_HALTagsData.HALCrop.height = m_ISPInputSensorData.CAMIFCrop.lastLine - m_ISPInputSensorData.CAMIFCrop.firstLine + 1;

        /// @todo (CAMX-3276) Hard code YUV Sensor Crop configure for now.
        if (TRUE == m_ISPInputSensorData.isYUV)
        {
            m_HALTagsData.HALCrop.width >>= 1;
        }
    }

    // Based on sensor type, disable/enable IFE ports which are enabled by default
    if (CamxResultSuccess == result)
    {
        UINT numOutputPorts = 0;
        UINT outputPortId[MaxDefinedIFEOutputPorts];
        GetAllOutputPortIds(&numOutputPorts, &outputPortId[0]);
        CAMX_LOG_INFO(CamxLogGroupISP, "totalOutputPorts = %u", numOutputPorts);

        for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
        {
            UINT outputPortNodeIndex = OutputPortIndex(outputPortId[outputPortIndex]);
            m_isDisabledOutputPort[outputPortNodeIndex] = FALSE;
            if (TRUE == CheckOutputPortIndexIfUnsupported(outputPortNodeIndex))
            {
                m_isDisabledOutputPort[outputPortNodeIndex] = TRUE;
                m_disabledOutputPorts++;
            }
        }
    }

    // Init HW/HVX setting, evaluate for Dual IFE
    if (CamxResultSuccess == result)
    {
        UpdateInitSettings();
    }

    // Initialize command buffer managers, and allocate command buffers
    if (CamxResultSuccess == result)
    {
        m_IFECmdBlobCount = GetPipeline()->GetRequestQueueDepth() + 7;  // Number of blob command buffers in circulation
                                                                        // 1 extra is for Initial Configuration packet
                                                                        // 4 more for pipeline delay
                                                                        // 2 extra for meet max request depth
        result = CreateCmdBuffers();
    }

    if (CamxResultSuccess == result)
    {
        IQInterface::IQSettingModuleInitialize(&m_libInitialData);
    }

    // Acquire IFE device resources
    if (CamxResultSuccess == result)
    {
        result = AcquireDevice();
    }

    CAMX_LOG_INFO(CamxLogGroupISP, "m_disabledOutputPorts = %u", m_disabledOutputPorts);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::InitialSetupandConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::InitialSetupandConfig(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    // Add Initial HFR config information in Generic Blob cmd buffer
    if (CamxResultSuccess == result)
    {
        result = SetupHFRInitialConfig();
    }

    // Add Clock configuration to Initial Packet so that kernel driver can configure IFE clocks
    if (CamxResultSuccess == result)
    {
        result = SetupResourceClockConfig();
    }

    // Add Flush Halt Configuration
    if (CamxResultSuccess == result)
    {
        m_pIFEPipeline->FillFlushConfig(m_pCommonCmdBuffer);

        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            m_pIFEPipeline->SetThrottlePattern(m_IFEThroughPut[static_cast<INT32>(IFESplitID::Left)],  m_pLeftCmdBuffer);
            m_pIFEPipeline->SetThrottlePattern(m_IFEThroughPut[static_cast<INT32>(IFESplitID::Right)], m_pRightCmdBuffer);
        }
        else
        {
            m_pIFEPipeline->SetThrottlePattern(m_IFEThroughPut[static_cast<INT32>(IFESplitID::Left)],  m_pCommonCmdBuffer);
        }
    }

    if (CamxResultSuccess == result)
    {
        // Add Bandwidth configuration to Initial Packet so that kernel driver can configure BW votes
        if (IFEGenericBlobTypeResourceBWConfig == m_pIFEPipeline->GetIFEBandWidthConfigurationVersion())
        {
            result = SetupResourceBWConfig(NULL, CamxInvalidRequestId);
        }
        else if (IFEGenericBlobTypeResourceBWConfigV2 == m_pIFEPipeline->GetIFEBandWidthConfigurationVersion())
        {
            result = SetupResourceBWConfigV2(NULL, CamxInvalidRequestId);
        }
    }

    // Add Clock configuration to Intial Packet so that kernel driver can configure CSID clocks
    if (CamxResultSuccess == result)
    {
        result = SetupCSIDClockConfig(pInputData->sensorBitWidth);
    }

    // Add UBWC  kernel API version 2 configuration to Initial packet so that kernel driver can configure UBWC.
    if (CamxResultSuccess == result)
    {
        result = SetupUBWCInitialConfig();
    }

    if ((CamxResultSuccess == result) && (TRUE == m_enableBusRead))
    {
        result = SetupBusReadInitialConfig();
    }

    if (CamxResultSuccess == result)
    {
        result = m_pIFEPipeline->SetupCoreConfig(m_pGenericBlobCmdBuffer,
                                                 &pInputData->statsTapOut,
                                                 reinterpret_cast<VOID*>(&m_IFECoreConfig));
    }

    if (NULL != m_pFlushDumpBufferAddr)
    {
        m_pIFEPipeline->SetupFlushRegDump(m_pFlushDumpBufferAddr);
    }

    if (NULL != m_pHangDumpBufferAddr)
    {
        m_pIFEPipeline->SetupHangRegDump(m_pHangDumpBufferAddr, m_mode, FALSE);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::UpdateIQStateConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::UpdateIQStateConfiguration(
    ISPStripeConfig* pFrameConfig,
    ISPStripeConfig* pStripeConfig)
{
    // pStripeConfig points to stripe state that persists from request to request, whereas pFrameConfig points to input state
    // for the current request. In here, the persistent IQ module state is copied first to the input config, then then whole
    // input is copied back to avoid doing a field by field assignment here. This can be later cleaned up by putting all
    // persistent state in its own structure within stripe config.
    pFrameConfig->stateABF  = pStripeConfig->stateABF;
    pFrameConfig->stateBF   = pStripeConfig->stateBF;
    pFrameConfig->stateLSC  = pStripeConfig->stateLSC;
    CamX::Utils::Memcpy(pFrameConfig->stateCrop, pStripeConfig->stateCrop, sizeof(pFrameConfig->stateCrop));
    CamX::Utils::Memcpy(pFrameConfig->stateDS, pStripeConfig->stateDS, sizeof(pFrameConfig->stateDS));
    CamX::Utils::Memcpy(pFrameConfig->stateMNDS, pStripeConfig->stateMNDS, sizeof(pFrameConfig->stateMNDS));
    CamX::Utils::Memcpy(pFrameConfig->stateRoundClamp, pStripeConfig->stateRoundClamp, sizeof(pFrameConfig->stateRoundClamp));

    // Finally update all the input configuration to stripe configuration
    *pStripeConfig = *pFrameConfig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetOEMIQSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::GetOEMIQSettings(
    VOID** ppOEMData)
{
    CamxResult result;
    UINT32     metaTag          = 0;

    CAMX_ASSERT(NULL != ppOEMData);

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.iqsettings", "OEMIFEIQSetting", &metaTag);

    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: OEMIFEIQSetting");

    if (CamxResultSuccess == result)
    {
        const UINT OEMProperty[] = { metaTag | InputMetadataSectionMask };
        VOID* OEMData[]          = { 0 };
        UINT64 OEMDataOffset[]   = { 0 };
        GetDataList(OEMProperty, OEMData, OEMDataOffset, 1);

        // Pointers in OEMData[] guaranteed to be non-NULL by GetDataList() for InputMetadataSectionMask
        *ppOEMData = OEMData[0];
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetOEMStatsConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::GetOEMStatsConfig(
    ISPStripeConfig* pFrameConfig)
{
    CamxResult result = CamxResultSuccess;

    UINT32 metadataAECFrameControl      = 0;
    UINT32 metadataAWBFrameControl      = 0;
    UINT32 metadataAECStatsControl      = 0;
    UINT32 metadataAWBStatsControl      = 0;
    UINT32 metadataAFStatsControl       = 0;
    UINT32 metadataAFDStatsControl      = 0;
    UINT32 metadataIHistStatsControl    = 0;

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
    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AFDStatsControl",
        &metadataAFDStatsControl);
    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: AFDStatsControl");
    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "IHistStatsControl",
        &metadataIHistStatsControl);
    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: IHistStatsControl");

    static const UINT vendorTagsControl3A[] =
    {
        metadataAECFrameControl     | InputMetadataSectionMask,
        metadataAWBFrameControl     | InputMetadataSectionMask,
        metadataAECStatsControl     | InputMetadataSectionMask,
        metadataAWBStatsControl     | InputMetadataSectionMask,
        metadataAFStatsControl      | InputMetadataSectionMask,
        metadataAFDStatsControl     | InputMetadataSectionMask,
        metadataIHistStatsControl   | InputMetadataSectionMask,
    };
    const SIZE_T numTags                            = CAMX_ARRAY_SIZE(vendorTagsControl3A);
    VOID*        pVendorTagsControl3A[numTags]      = { 0 };
    UINT64       vendorTagsControl3AOffset[numTags] = { 0 };

    GetDataList(vendorTagsControl3A, pVendorTagsControl3A, vendorTagsControl3AOffset, numTags);

    // Pointers in pVendorTagsControl3A[] guaranteed to be non-NULL by GetDataList() for InputMetadataSectionMask
    Utils::Memcpy(&pFrameConfig->AECUpdateData,        pVendorTagsControl3A[0], sizeof(AECFrameControl));
    Utils::Memcpy(&pFrameConfig->AWBUpdateData,        pVendorTagsControl3A[1], sizeof(AWBFrameControl));
    Utils::Memcpy(&pFrameConfig->AECStatsUpdateData,   pVendorTagsControl3A[2], sizeof(AECStatsControl));
    Utils::Memcpy(&pFrameConfig->AWBStatsUpdateData,   pVendorTagsControl3A[3], sizeof(AWBStatsControl));
    Utils::Memcpy(&pFrameConfig->AFStatsUpdateData,    pVendorTagsControl3A[4], sizeof(AFStatsControl));
    Utils::Memcpy(&pFrameConfig->AFDStatsUpdateData,   pVendorTagsControl3A[5], sizeof(AFDStatsControl));
    Utils::Memcpy(&pFrameConfig->IHistStatsUpdateData, pVendorTagsControl3A[6], sizeof(IHistStatsControl));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::Get3AFrameConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::Get3AFrameConfig(
    ISPInputData*    pModuleInput,
    ISPStripeConfig* pFrameConfig,
    UINT64           requestId)
{
    static const UINT Properties3A[] =
    {
        PropertyIDAECFrameControl,              // 0
        PropertyIDAWBFrameControl,              // 1
        PropertyIDAECStatsControl,              // 2
        PropertyIDAWBStatsControl,              // 3
        PropertyIDAFStatsControl,               // 4
        PropertyIDAFDStatsControl,              // 5
        PropertyIDPostSensorGainId,             // 6
        PropertyIDISPBHistConfig,               // 7
        PropertyIDISPTintlessBGConfig,          // 8
        PropertyIDParsedTintlessBGStatsOutput,  // 9
        PropertyIDParsedBHistStatsOutput,       // 10
        PropertyIDParsedAWBBGStatsOutput,       // 11
        PropertyIDAFFrameControl,               // 12
        PropertyIDSensorNumberOfLEDs,           // 13
        PropertyIDPDHwConfig,                   // 14
    };

    const SIZE_T numTags                 = CAMX_ARRAY_SIZE(Properties3A);
    VOID*  pPropertyData3A[numTags]      = { 0 };

    UINT64 propertyData3AOffset[numTags] = { 0 };

    // Need to read from the frame matching the buffer
    propertyData3AOffset[7] = GetMaximumPipelineDelay();
    propertyData3AOffset[8] = GetMaximumPipelineDelay();

    GetDataList(Properties3A, pPropertyData3A, propertyData3AOffset, numTags);

    if (NULL != pPropertyData3A[0] && TRUE == IsTagPresentInPublishList(Properties3A[0]))
    {
        Utils::Memcpy(&pFrameConfig->AECUpdateData,      pPropertyData3A[0], sizeof(pFrameConfig->AECUpdateData));
    }
    if (NULL != pPropertyData3A[1] && TRUE == IsTagPresentInPublishList(Properties3A[1]))
    {
        Utils::Memcpy(&pFrameConfig->AWBUpdateData,      pPropertyData3A[1], sizeof(pFrameConfig->AWBUpdateData));
    }
    if (NULL != pPropertyData3A[2] && TRUE == IsTagPresentInPublishList(Properties3A[2]))
    {
        Utils::Memcpy(&pFrameConfig->AECStatsUpdateData, pPropertyData3A[2], sizeof(pFrameConfig->AECStatsUpdateData));

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Req ID %lld, Original 3A ROI Config from 3A HNum %d VNum %d, ROI [%d %d %d %d]",
                         requestId,
                         pFrameConfig->AECStatsUpdateData.statsConfig.BEConfig.horizontalNum,
                         pFrameConfig->AECStatsUpdateData.statsConfig.BEConfig.verticalNum,
                         pFrameConfig->AECStatsUpdateData.statsConfig.BEConfig.ROI.left,
                         pFrameConfig->AECStatsUpdateData.statsConfig.BEConfig.ROI.top,
                         pFrameConfig->AECStatsUpdateData.statsConfig.BEConfig.ROI.width,
                         pFrameConfig->AECStatsUpdateData.statsConfig.BEConfig.ROI.height);

    }
    if (NULL != pPropertyData3A[3] && TRUE == IsTagPresentInPublishList(Properties3A[3]))
    {
        Utils::Memcpy(&pFrameConfig->AWBStatsUpdateData, pPropertyData3A[3], sizeof(pFrameConfig->AWBStatsUpdateData));
    }
    if (NULL != pPropertyData3A[4] && TRUE == IsTagPresentInPublishList(Properties3A[4]))
    {
        if ((reinterpret_cast<AFStatsControl*>(pPropertyData3A[4]))->statsConfig.
            BFStats.BFStatsROIConfig.numBFStatsROIDimension > 0)
        {
            Utils::Memcpy(&pFrameConfig->AFStatsUpdateData,  pPropertyData3A[4], sizeof(pFrameConfig->AFStatsUpdateData));
            Utils::Memcpy(&m_previousAFstatsControl, &pFrameConfig->AFStatsUpdateData, sizeof(m_previousAFstatsControl));
        }
        else
        {
            if (requestId <= FirstValidRequestId)
            {
                pFrameConfig->CAMIFCrop = m_ISPInputSensorData.CAMIFCrop;
                HardcodeSettingsBFStats(pFrameConfig);
                Utils::Memcpy(&m_previousAFstatsControl, &pFrameConfig->AFStatsUpdateData, sizeof(m_previousAFstatsControl));
                CAMX_LOG_INFO(CamxLogGroupISP, "Invalid BF config use hardcode for requestId %llu"
                    "CAMIF Dimension [w x h] [%d x %d]",
                    requestId,
                    m_pSensorModeData->cropInfo.lastPixel - m_pSensorModeData->cropInfo.firstPixel + 1,
                    m_pSensorModeData->cropInfo.lastLine  - m_pSensorModeData->cropInfo.firstLine+ 1);
            }
            else
            {
                Utils::Memcpy(&pFrameConfig->AFStatsUpdateData, &m_previousAFstatsControl,
                    sizeof(pFrameConfig->AFStatsUpdateData));
                CAMX_LOG_INFO(CamxLogGroupISP, "Invalid BF config use previous config requestId %llu",
                    requestId);
                UINT32 numberOfROI =
                    pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension;
                CAMX_LOG_INFO(CamxLogGroupISP, "Invalid BF COnfig request ID %llu Using Prvious Config"
                    "CAMIF Dimesniosn [w x h][%d x %d] Previous Config number of ROI %d",
                    requestId,
                    m_pSensorModeData->cropInfo.lastPixel - m_pSensorModeData->cropInfo.firstPixel + 1,
                    m_pSensorModeData->cropInfo.lastLine- m_pSensorModeData->cropInfo.firstLine+ 1,
                    numberOfROI);
                for (UINT32 i= 0; i < numberOfROI; i++)
                {
                    CAMX_LOG_INFO(CamxLogGroupISP, "Previous  ROI[%d] is vaild %d (%d %d %d %d)",
                        i,
                        pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[i].isValid,
                        pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[i].ROI.left,
                        pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[i].ROI.top,
                        pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[i].ROI.width,
                        pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[i].ROI.height);
                }
            }
        }

        for (UINT32 i= 0; i < pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension; i++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Numb ROI %d first vaild %d ROI[%d](%d %d %d %d)",
                pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension,
                pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[i].isValid,
                i,
                pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[i].ROI.left,
                pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[i].ROI.top,
                pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[i].ROI.width,
                pFrameConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[i].ROI.height);
        }
    }
    if (NULL != pPropertyData3A[5] && TRUE == IsTagPresentInPublishList(Properties3A[5]))
    {
        Utils::Memcpy(&pFrameConfig->AFDStatsUpdateData, pPropertyData3A[5], sizeof(pFrameConfig->AFDStatsUpdateData));
    }
    if (NULL != pPropertyData3A[6] && TRUE == IsTagPresentInPublishList(Properties3A[6]))
    {
        m_ISPInputSensorData.dGain = *reinterpret_cast<FLOAT*>(pPropertyData3A[6]);

        if (pFrameConfig->AECUpdateData.digitalGainForSimulation >= 1.0f)
        {
            m_ISPInputSensorData.dGain *= pFrameConfig->AECUpdateData.digitalGainForSimulation;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                     "Applying Stats Update. AWB Gain(R:%f, G : %f, B : %f) CCT(%u).AEC Gain(%f) ExpTime(%llu)",
                     pFrameConfig->AWBUpdateData.AWBGains.rGain,
                     pFrameConfig->AWBUpdateData.AWBGains.gGain,
                     pFrameConfig->AWBUpdateData.AWBGains.bGain,
                     pFrameConfig->AWBUpdateData.colorTemperature,
                     pFrameConfig->AECUpdateData.exposureInfo[ExposureIndexShort].linearGain,
                     pFrameConfig->AECUpdateData.exposureInfo[ExposureIndexShort].exposureTime);

    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                     "IFE: Processing for ReqId=%llu G:ET=%f : %llu LUX=%f stretchControl(%d %f %f) ISPDigitalGain=%f",
                     requestId,
                     pFrameConfig->AECUpdateData.exposureInfo[0].linearGain,
                     pFrameConfig->AECUpdateData.exposureInfo[0].exposureTime,
                     pFrameConfig->AECUpdateData.luxIndex,
                     pFrameConfig->AECUpdateData.stretchControl.enable,
                     pFrameConfig->AECUpdateData.stretchControl.clamp,
                     pFrameConfig->AECUpdateData.stretchControl.scaling,
                     m_ISPInputSensorData.dGain);

    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);

    if (TRUE == IsPipelineWarmedUpState(requestId))
    {
        if (NULL != pPropertyData3A[7] && TRUE == IsTagPresentInPublishList(Properties3A[7]))
        {
            Utils::Memcpy(&(pFrameConfig->statsDataForISP.BHistConfig),
                          pPropertyData3A[7],
                          sizeof(pFrameConfig->statsDataForISP.BHistConfig));
        }

        if (NULL != pPropertyData3A[8] && TRUE == IsTagPresentInPublishList(Properties3A[8]))
        {
            Utils::Memcpy(&(pFrameConfig->statsDataForISP.tintlessBGConfig),
                          pPropertyData3A[8],
                          sizeof(pFrameConfig->statsDataForISP.tintlessBGConfig));
        }

        if (NULL != pPropertyData3A[9] && TRUE == IsTagPresentInPublishList(Properties3A[9]))
        {
            pFrameConfig->statsDataForISP.pParsedTintlessBGStats =
                *(reinterpret_cast<ParsedTintlessBGStatsOutput**>(pPropertyData3A[9]));
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "TLBG Config %d %d | %d TLBG stats %x %x %x ",
                pFrameConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.ROI.height,
                pFrameConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.ROI.width,
                pFrameConfig->statsDataForISP.pParsedTintlessBGStats->m_numOfRegions,
                pFrameConfig->statsDataForISP.pParsedTintlessBGStats->GetTintlessBGStatsInfo(0),
                pFrameConfig->statsDataForISP.pParsedTintlessBGStats->GetTintlessBGStatsInfo(0),
                pFrameConfig->statsDataForISP.pParsedTintlessBGStats->GetTintlessBGStatsInfo(0));
        }

        if (NULL != pPropertyData3A[10] && TRUE == IsTagPresentInPublishList(Properties3A[10]))
        {
            pFrameConfig->statsDataForISP.pParsedBHISTStats =
                *(reinterpret_cast<ParsedBHistStatsOutput**>(pPropertyData3A[10]));
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "BHIST Config %d %d %d | %d %d BHIST stats %x %x %x",
                             pFrameConfig->statsDataForISP.BHistConfig.BHistConfig.channel,
                             pFrameConfig->statsDataForISP.BHistConfig.BHistConfig.uniform,
                             pFrameConfig->statsDataForISP.BHistConfig.numBins,
                             pFrameConfig->statsDataForISP.pParsedBHISTStats->channelType,
                             pFrameConfig->statsDataForISP.pParsedBHISTStats->numBins,
                             pFrameConfig->statsDataForISP.pParsedBHISTStats->BHistogramStats[0],
                             pFrameConfig->statsDataForISP.pParsedBHISTStats->BHistogramStats[1],
                             pFrameConfig->statsDataForISP.pParsedBHISTStats->BHistogramStats[2]);
        }

        if (NULL != pPropertyData3A[11] && TRUE == IsTagPresentInPublishList(Properties3A[11]))
        {
            pFrameConfig->statsDataForISP.pParsedAWBBGStats =
                *(reinterpret_cast<ParsedAWBBGStatsOutput**>(pPropertyData3A[11]));
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "AWB BG Config usesY : %d, hasSatInfo : %d, numROIs : %d",
                pFrameConfig->statsDataForISP.pParsedAWBBGStats->flags.usesY,
                pFrameConfig->statsDataForISP.pParsedAWBBGStats->flags.hasSatInfo,
                pFrameConfig->statsDataForISP.pParsedAWBBGStats->numROIs);
        }
    }

    if (NULL != pPropertyData3A[12] && TRUE == IsTagPresentInPublishList(Properties3A[12]))
    {
        Utils::Memcpy(&pFrameConfig->AFUpdateData, pPropertyData3A[12], sizeof(pFrameConfig->AFUpdateData));
    }

    if (NULL != pPropertyData3A[13] && TRUE == IsTagPresentInPublishList(Properties3A[13]))
    {
        pModuleInput->numberOfLED = *reinterpret_cast<UINT16*>(pPropertyData3A[13]);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Number of led %d", pModuleInput->numberOfLED);
    }

    if (NULL != pPropertyData3A[14] && TRUE == IsTagPresentInPublishList(Properties3A[14]))
    {
        Utils::Memcpy(&pFrameConfig->pdHwConfig, pPropertyData3A[14], sizeof(PDHwConfig));
    }

    CAMX_LOG_INFO(CamxLogGroupISP, "IFE: Processing for ReqId=%llu requestIdOffsetFromLastFlush %llu ", requestId,
        requestIdOffsetFromLastFlush);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::DumpPDAFSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::DumpPDAFSettings(
    INT         fd,
    PDHwConfig* pPDHWConfig)
{
    IFE_SETTING_DUMP(fd, pPDHWConfig->binConfig.enableSkipBinning);
    IFE_SETTING_DUMP(fd, pPDHWConfig->binConfig.horizontalBinningPixelCount);
    IFE_SETTING_DUMP(fd, pPDHWConfig->binConfig.horizontalBinningSkip);
    IFE_SETTING_DUMP(fd, pPDHWConfig->binConfig.verticalBinningLineCount);
    IFE_SETTING_DUMP(fd, pPDHWConfig->binConfig.verticalBinningSkip);
    IFE_SETTING_DUMP(fd, pPDHWConfig->binConfig.verticalDecimateCount);
    IFE_SETTING_DUMP(fd, pPDHWConfig->BLSConfig.leftPixelBLSCorrection);
    IFE_SETTING_DUMP(fd, pPDHWConfig->BLSConfig.rightPixelBLSCorrection);
    IFE_SETTING_DUMP(fd, pPDHWConfig->cropConfig.enablePDCrop);
    IFE_SETTING_DUMP(fd, pPDHWConfig->cropConfig.firstLineCrop);
    IFE_SETTING_DUMP(fd, pPDHWConfig->cropConfig.lastLineCrop);
    IFE_SETTING_DUMP(fd, pPDHWConfig->cropConfig.firstPixelCrop);
    IFE_SETTING_DUMP(fd, pPDHWConfig->cropConfig.lastPixelCrop);
    IFE_SETTING_DUMP(fd, pPDHWConfig->earlyInterruptTransactionCount);
    IFE_SETTING_DUMP(fd, pPDHWConfig->enableEarlyInterrupt);
    IFE_SETTING_DUMP(fd, pPDHWConfig->enablePDHw);
    IFE_SETTING_DUMP(fd, pPDHWConfig->firstPixelSelect);
    IFE_SETTING_DUMP(fd, pPDHWConfig->gainMapConfig.gainMapEnable);
    IFE_SETTING_DUMP(fd, pPDHWConfig->gainMapConfig.horizontalPhaseInit);
    IFE_SETTING_DUMP(fd, pPDHWConfig->gainMapConfig.horizontalPhaseStep);
    IFE_SETTING_DUMP(fd, pPDHWConfig->gainMapConfig.numberHorizontalGrids);
    IFE_SETTING_DUMP(fd, pPDHWConfig->gainMapConfig.numberVerticalGrids);
    IFE_SETTING_DUMP(fd, pPDHWConfig->gainMapConfig.verticalPhaseInit);
    IFE_SETTING_DUMP(fd, pPDHWConfig->gainMapConfig.verticalPhaseStep);
    for (UINT32 i = 0; i < PDLibGainMapLUTSize; i++)
    {
        IFE_SETTING_DUMP(fd, pPDHWConfig->gainMapConfig.leftLUT[i]);
    }
    for (UINT32 i = 0; i < PDLibGainMapLUTSize; i++)
    {
        IFE_SETTING_DUMP(fd, pPDHWConfig->gainMapConfig.rightLUT[i]);
    }
    IFE_SETTING_DUMP(fd, pPDHWConfig->HDRConfig.hdrEnable);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->HDRConfig.hdrThreshhold);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->HDRConfig.hdrExposureRatio);
    IFE_SETTING_DUMP(fd, pPDHWConfig->HDRConfig.hdrFirstPixel);
    IFE_SETTING_DUMP(fd, pPDHWConfig->HDRConfig.hdrModeSel);
    IFE_SETTING_DUMP(fd, pPDHWConfig->IIRConfig.IIREnable);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->IIRConfig.a0);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->IIRConfig.a1);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->IIRConfig.b0);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->IIRConfig.b1);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->IIRConfig.b2);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->IIRConfig.init0);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->IIRConfig.init1);
    IFE_SETTING_DUMP(fd, pPDHWConfig->LCRConfig.enable);
    IFE_SETTING_DUMP(fd, pPDHWConfig->LCRConfig.crop.firstPixel);
    IFE_SETTING_DUMP(fd, pPDHWConfig->LCRConfig.crop.lastPixel);
    IFE_SETTING_DUMP(fd, pPDHWConfig->LCRConfig.crop.firstLine);
    IFE_SETTING_DUMP(fd, pPDHWConfig->LCRConfig.crop.lastLine);
    IFE_SETTING_DUMP(fd, pPDHWConfig->LCRConfig.blockHeight);
    IFE_SETTING_DUMP(fd, pPDHWConfig->LCRConfig.componentMask);
    IFE_SETTING_DUMP(fd, pPDHWConfig->LCRConfig.componentShift);
    IFE_SETTING_DUMP(fd, pPDHWConfig->LCRConfig.flushMask);
    IFE_SETTING_DUMP(fd, pPDHWConfig->LCRConfig.lineMask);
    IFE_SETTING_DUMP(fd, pPDHWConfig->modeSelect);
    IFE_SETTING_DUMP(fd, pPDHWConfig->pathSelect);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.sadEnable);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.sadSelect);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.sadShift);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.config0Phase);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.config1Phase);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.horizontalNumber);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.horizontalOffset);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.regionHeight);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.regionWidth);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.verticalNumber);
    IFE_SETTING_DUMP(fd, pPDHWConfig->SADConfig.verticalOffset);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.FIRConfig.enable);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.FIRConfig.shift);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->sparseConfig.FIRConfig.a0);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->sparseConfig.FIRConfig.a1);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->sparseConfig.FIRConfig.a2);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->sparseConfig.FIRConfig.a3);
    IFE_SETTING_DUMP_FLOAT(fd, pPDHWConfig->sparseConfig.FIRConfig.a4);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.LEConfig.blockHeight);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.LEConfig.blockPDRowCount);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.LEConfig.globalOffsetLines);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.LEConfig.pdPixelWidth);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.LEConfig.verticalBlockCount);
    for (UINT32 i = 0; i < PDLibMaxPDPixelRows; i++)
    {
        IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.LEConfig.horizontalOffset[i]);
    }
    for (UINT32 i = 0; i < PDLibMaxPDPixelRows; i++)
    {
        IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.LEConfig.verticalOffset[i]);
    }
    for (UINT32 i = 0; i < PDLibMaxPDPixelRows; i++)
    {
        IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.LEConfig.halfLine[i]);
    }
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PEConfig.blockHeight);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PEConfig.blockWidth);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PEConfig.cropConfig.enablePDCrop);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PEConfig.cropConfig.firstPixelCrop);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PEConfig.cropConfig.lastPixelCrop);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PEConfig.cropConfig.firstLineCrop);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PEConfig.cropConfig.lastLineCrop);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PEConfig.enable);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PSConfig.outputHeight);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PSConfig.outputWidth);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PSConfig.pixelsPerBlock);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PSConfig.rowPerBlock);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PSConfig.outputHeight);
    IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PSConfig.outputWidth);
    for (UINT index = 0; index < PDLibMaxLeftOrRightPDPixels; index++)
    {
        IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PSConfig.pdLMap[index]);
    }
    for (UINT index = 0; index < PDLibMaxLeftOrRightPDPixels; index++)
    {
        IFE_SETTING_DUMP(fd, pPDHWConfig->sparseConfig.PSConfig.pdRMap[index]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::DumpIFESettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::DumpIFESettings(
    ISPInputData* pInputData)
{
    if ((NULL != pInputData) && (NULL != pInputData->pHwContext))
    {
        if (TRUE == IsIFESettingsDumpEnabled(pInputData->pHwContext))
        {
            CHAR  dumpFilename[256];
            FILE* pFile = NULL;

            if (TRUE == pInputData->isInitPacket)
            {
                OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                    "%s/IFE_InitPacketDump.txt", ConfigFileDirectory);
            }
            else
            {
                OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                    "%s/IFE_DumpFrameNum_%llu.txt", ConfigFileDirectory, pInputData->frameNum);
            }
            pFile = OsUtils::FOpen(dumpFilename, "w");
            if (NULL != pFile)
            {
                if (NULL != pInputData->pPDHwConfig)
                {
                    DumpPDAFSettings(OsUtils::FileNo(pFile), pInputData->pPDHwConfig);
                }
                OsUtils::FClose(pFile);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::UpdateCamifSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::UpdateCamifSettings(
    ISPInputData* pInputData)
{

    pInputData->pipelineIFEData.programCAMIF        = FALSE;
    pInputData->pipelineIFEData.numBatchedFrames    = m_usecaseNumBatchedFrames;
    pInputData->dualPDPipelineData.programCAMIFLite = FALSE;
    pInputData->dualPDPipelineData.numBatchedFrames = m_usecaseNumBatchedFrames;

    switch(m_titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            pInputData->pipelineIFEData.programCAMIF     = TRUE;
            pInputData->dualPDPipelineData.programCAMIFLite = TRUE;
            break;
        default:
            if ((TRUE == m_initialConfigPending) && (FALSE == IsPipelineStreamedOn()))
            {
                pInputData->pipelineIFEData.programCAMIF     = TRUE;
                pInputData->dualPDPipelineData.programCAMIFLite = TRUE;
            }
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::DumpIFEDebugConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::DumpIFEDebugConfig(
    INT             fd,
    IFEConfigInfo*  pIFEDebugData)
{
    BHistConfig*          pBHistConfig         =
        &(pIFEDebugData->frameConfig.AECStatsUpdateData.statsConfig.BHistConfig);
    BGBEConfig*           pHDRBEConfig         =
        &(pIFEDebugData->frameConfig.AECStatsUpdateData.statsConfig.BEConfig);
    HDRBHistConfig*       pHDRBHistConfig      =
        &(pIFEDebugData->frameConfig.AECStatsUpdateData.statsConfig.HDRBHistConfig);
    BGBEConfig*           pTintlessBGConfig    =
        &(pIFEDebugData->frameConfig.AECStatsUpdateData.statsConfig.TintlessBGConfig);
    BGBEConfig*           pAWBBGConfig         =
        &(pIFEDebugData->frameConfig.AWBStatsUpdateData.statsConfig.BGConfig);
    AFConfigParams*       pAFConfig            =
        &(pIFEDebugData->frameConfig.AFStatsUpdateData.statsConfig);
    BFInputConfigParams*  pBFInputConfigParams = &pAFConfig->BFStats.BFInputConfig;
    BFScaleConfigType*    pBFScaleConfigType   = &pAFConfig->BFStats.BFScaleConfig;
    IHistStatsConfig*     pIHistConfig         = &(pIFEDebugData->frameConfig.IHistStatsUpdateData.statsConfig);
    CropWindow            halCrop              = pIFEDebugData->HALTagsData.HALCrop;
    UINT32                numIOConfig          = pIFEDebugData->numIOConfig;
    IFEWMUpdate*          pWMUpdate            = NULL;

    IFE_SETTING_DUMP_LL(fd, pIFEDebugData->requestID);
    IFE_SETTING_DUMP(fd, pIFEDebugData->forceTriggerUpdate );
    // Selected Mode information for sensor
    CAMX_LOG_TO_FILE(fd, 0,
        "SensorMode: numPixelsPerLine: %d numLinesPerFrame: %d maxFps: %f Binning_H: %d Binning_V: %d WxH: %d X %d",
        m_pSensorModeData->numPixelsPerLine, m_pSensorModeData->numLinesPerFrame,
        m_pSensorModeData->maxFPS, m_pSensorModeData->binningTypeH, m_pSensorModeData->binningTypeV,
        m_pSensorModeData->resolution.outputWidth, m_pSensorModeData->resolution.outputHeight);
    CAMX_LOG_TO_FILE(fd, 0,
        "SensorMode: offset_x: %d offset_y: %d Crop [%d, %d, %d, %d] OutputPixelClk: %llu Active_wxh: [%d X %d]",
        m_pSensorModeData->offset.xStart, m_pSensorModeData->offset.yStart,
        m_pSensorModeData->cropInfo.firstPixel, m_pSensorModeData->cropInfo.firstLine,
        m_pSensorModeData->cropInfo.lastPixel, m_pSensorModeData->cropInfo.lastLine,
        m_pSensorModeData->outPixelClock, m_sensorActiveArrayWidth, m_sensorActiveArrayHeight);

    // Dump Tuning Mode Data
    IFE_SETTING_DUMP(fd, pIFEDebugData->tuningData.noOfSelectionParameter);
    for (UINT index = 0; index < pIFEDebugData->tuningData.noOfSelectionParameter; index++)
    {
        IFE_SETTING_DUMP(fd, pIFEDebugData->tuningData.TuningMode[index].mode);
        IFE_SETTING_DUMP(fd, pIFEDebugData->tuningData.TuningMode[index].subMode.value);
    }

    // Dump HAL Crop Data
    CAMX_LOG_TO_FILE(fd, 0, "HAL Crop [%d  %d  %d  %d]",
        halCrop.left, halCrop.top, halCrop.width, halCrop.height);

    // Dump BHIST Config
    CAMX_LOG_TO_FILE(fd, 0, "BHistConfig: Channel: %d ROI: [%d  %d  %d  %d] Uniform: %d",
        pBHistConfig->channel, pBHistConfig->ROI.left, pBHistConfig->ROI.top,
        pBHistConfig->ROI.width, pBHistConfig->ROI.height, pBHistConfig->uniform);

    // Dump HDRBE Config
    CAMX_LOG_TO_FILE(fd, 0,
        "HDRBEConfig: Hnum: %d Vnum: %d ROI: [%d  %d  %d  %d] outputBitDepth: %d OutputMode: %d",
        pHDRBEConfig->horizontalNum, pHDRBEConfig->verticalNum, pHDRBEConfig->ROI.left,
        pHDRBEConfig->ROI.top, pHDRBEConfig->ROI.width, pHDRBEConfig->ROI.height,
        pHDRBEConfig->outputBitDepth, pHDRBEConfig->outputMode);
    CAMX_LOG_TO_FILE(fd, 0,
        "HDRBEConfig: YStatsWeights[%f  %f  %f] greenType: %d channelGainThreshold[R GR B GB]: [%d  %d  %d  %d]",
        pHDRBEConfig->YStatsWeights[0], pHDRBEConfig->YStatsWeights[1], pHDRBEConfig->YStatsWeights[2],
        pHDRBEConfig->greenType, pHDRBEConfig->channelGainThreshold[ChannelIndexR],
        pHDRBEConfig->channelGainThreshold[ChannelIndexGR], pHDRBEConfig->channelGainThreshold[ChannelIndexB],
        pHDRBEConfig->channelGainThreshold[ChannelIndexGB] );

    // Dump HDR BHIST Config
    CAMX_LOG_TO_FILE(fd, 0, "HDRBHISTConfig: ROI: [%d  %d  %d  %d] greenChannelInput: %d inputFieldSelect: %d",
        pHDRBHistConfig->ROI.left, pHDRBHistConfig->ROI.top, pHDRBHistConfig->ROI.width, pHDRBHistConfig->ROI.height,
        pHDRBHistConfig->greenChannelInput, pHDRBHistConfig->inputFieldSelect);

    // Dump Tintless BG Config
    CAMX_LOG_TO_FILE(fd, 0,
        "Tintless BG Config: Hnum: %d Vnum: %d ROI: [%d  %d  %d  %d] outputBitDepth: %d OutputMode: %d",
        pTintlessBGConfig->horizontalNum, pTintlessBGConfig->verticalNum, pTintlessBGConfig->ROI.left,
        pTintlessBGConfig->ROI.top, pTintlessBGConfig->ROI.width, pTintlessBGConfig->ROI.height,
        pTintlessBGConfig->channelGainThreshold[ChannelIndexR],
        pTintlessBGConfig->channelGainThreshold[ChannelIndexGR],
        pTintlessBGConfig->channelGainThreshold[ChannelIndexB],
        pTintlessBGConfig->channelGainThreshold[ChannelIndexGB]);

    // Dump AWB BG Config
    CAMX_LOG_TO_FILE(fd, 0,
        "AWB BG Config: Hnum: %d Vnum: %d ROI: [%d  %d  %d  %d] outputBitDepth: %d OutputMode: %d",
        pAWBBGConfig->horizontalNum, pAWBBGConfig->verticalNum, pAWBBGConfig->ROI.left, pAWBBGConfig->ROI.top,
        pAWBBGConfig->ROI.width, pAWBBGConfig->ROI.height, pAWBBGConfig->outputBitDepth, pAWBBGConfig->outputMode);
    CAMX_LOG_TO_FILE(fd, 0,
        "AWB BG Config: YStatsWeights[%f  %f  %f] greenType: %d channelGainThreshold[R GR B GB]: [%d  %d  %d  %d]",
        pAWBBGConfig->YStatsWeights[0], pAWBBGConfig->YStatsWeights[1], pAWBBGConfig->YStatsWeights[2],
        pAWBBGConfig->greenType, pAWBBGConfig->channelGainThreshold[ChannelIndexR],
        pAWBBGConfig->channelGainThreshold[ChannelIndexGR], pAWBBGConfig->channelGainThreshold[ChannelIndexB],
        pAWBBGConfig->channelGainThreshold[ChannelIndexGB]);

    // Dump BF input Config
    IFE_SETTING_DUMP(fd, pBFInputConfigParams->isValid      );
    IFE_SETTING_DUMP(fd, pBFInputConfigParams->BFChannelSel );
    if (BFChannelSelectY == pBFInputConfigParams->BFChannelSel)
    {
        for (UINT index = 0; index < MaxYConfig; index++)
        {
            IFE_SETTING_DUMP_FLOAT(fd, pBFInputConfigParams->YAConfig[index]);
        }
    }

    // Dump BF Scale Config
    IFE_SETTING_DUMP(fd, pBFScaleConfigType->isValid       );
    IFE_SETTING_DUMP(fd, pBFScaleConfigType->BFScaleEnable );
    IFE_SETTING_DUMP(fd, pBFScaleConfigType->scaleM        );
    IFE_SETTING_DUMP(fd, pBFScaleConfigType->scaleN        );

    IFE_SETTING_DUMP(fd, pAFConfig->BFStats.BFStatsROIConfig.BFROIType              );
    IFE_SETTING_DUMP(fd, pAFConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension );

    for (UINT index = 0; index < pAFConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension; index++)
    {
        CAMX_LOG_TO_FILE(fd, 0, "BF Stats index: %d isValid: %d ROI[x, y, w, h]: [%d  %d  %d  %d]",
            index,
            pAFConfig->BFStats.BFStatsROIConfig.BFStatsROIDimension[index].isValid,
            pAFConfig->BFStats.BFStatsROIConfig.BFStatsROIDimension[index].ROI.left,
            pAFConfig->BFStats.BFStatsROIConfig.BFStatsROIDimension[index].ROI.top,
            pAFConfig->BFStats.BFStatsROIConfig.BFStatsROIDimension[index].ROI.width,
            pAFConfig->BFStats.BFStatsROIConfig.BFStatsROIDimension[index].ROI.height);
    }

    IFE_SETTING_DUMP(fd, pAFConfig->BFStats.BFGammaLUTConfig.isValid                );
    IFE_SETTING_DUMP(fd, pAFConfig->BFStats.BFGammaLUTConfig.numGammaLUT            );

    for (UINT index = 0; index < BFFilterTypeCount; index++)
    {
        IFE_SETTING_DUMP(fd, pAFConfig->BFStats.BFFilterConfig[index].isValid              );
        IFE_SETTING_DUMP(fd, pAFConfig->BFStats.BFFilterConfig[index].horizontalScaleEnable);
    }

    IFE_SETTING_DUMP(fd, pIHistConfig->ROI.width );
    IFE_SETTING_DUMP(fd, pIHistConfig->ROI.height);
    IFE_SETTING_DUMP(fd, pIHistConfig->channelYCC);

    // Dump IFE IO Buffer config
    for (UINT index = 0; index < numIOConfig; index++)
    {
        IFEConfigIOBufInfo*   pIOConfig = &(pIFEDebugData->ioConfig[index]);
        CAMX_LOG_TO_FILE(fd, 0, "I/O cfg portId=%d, hFence=%d, hMemHandle=%d ",
            pIOConfig->portId, pIOConfig->hFence, pIOConfig->hMemHandle);
    }

    // Dump IFE WM Config: BF25, DualPD and LCR
    if (CSLCameraTitanVersion::CSLTitan480 == m_titanVersion)
    {
        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            // Dump WM Update for LEFT IFE
            pWMUpdate = &pIFEDebugData->ISPData[LeftIFE].WMUpdate;

            for (UINT32 wmIndex = 0; wmIndex < pWMUpdate->numberOfWMUpdates; wmIndex++)
            {
                CAMX_LOG_TO_FILE(fd, 0, "Left IFE: WM PorID 0x%X [w x h][%d x %d] hInit %d mode %d virtFrameEn %d",
                    pWMUpdate->WMData[wmIndex].portID, pWMUpdate->WMData[wmIndex].width,
                    pWMUpdate->WMData[wmIndex].height, pWMUpdate->WMData[wmIndex].hInit,
                    pWMUpdate->WMData[wmIndex].mode, pWMUpdate->WMData[wmIndex].virtualFrameEn);
            }
            // Dump WM Update for RIGHT IFE
            pWMUpdate = &pIFEDebugData->ISPData[RightIFE].WMUpdate;

            for (UINT32 wmIndex = 0; wmIndex < pWMUpdate->numberOfWMUpdates; wmIndex++)
            {
                CAMX_LOG_TO_FILE(fd, 0, "Right IFE: WM PorID 0x%X [w x h][%d x %d] hInit %d mode %d virtFrameEn %d",
                    pWMUpdate->WMData[wmIndex].portID, pWMUpdate->WMData[wmIndex].width,
                    pWMUpdate->WMData[wmIndex].height, pWMUpdate->WMData[wmIndex].hInit,
                    pWMUpdate->WMData[wmIndex].mode, pWMUpdate->WMData[wmIndex].virtualFrameEn);
            }
        }
        else
        {
            // Dump WM Update for Common IFE
            pWMUpdate = &pIFEDebugData->ISPData[CommonIFE].WMUpdate;

            for (UINT32 wmIndex = 0; wmIndex < pWMUpdate->numberOfWMUpdates; wmIndex++)
            {
                CAMX_LOG_TO_FILE(fd, 0, "Common IFE: WM PorID 0x%X [w x h][%d x %d] hInit %d mode %d virtFrameEn %d",
                    pWMUpdate->WMData[wmIndex].portID, pWMUpdate->WMData[wmIndex].width,
                    pWMUpdate->WMData[wmIndex].height, pWMUpdate->WMData[wmIndex].hInit,
                    pWMUpdate->WMData[wmIndex].mode, pWMUpdate->WMData[wmIndex].virtualFrameEn);
            }
        }
    }

    if (TRUE == pIFEDebugData->frameConfig.pdHwConfig.enablePDHw)
    {
        // Dump DualPD/PDAF Config
        DumpPDAFSettings(fd, &pIFEDebugData->frameConfig.pdHwConfig);
    }

    // Dump Buffer Negotiation Data: Backward-Walk
    CAMX_LOG_TO_FILE(fd, 0, "Back-Walk Negotiation: Optimal %d x %d, Min %d x %d, Max %d x %d",
        m_inputBufferRequirement.optimalWidth, m_inputBufferRequirement.optimalHeight,
        m_inputBufferRequirement.minWidth, m_inputBufferRequirement.minHeight,
        m_inputBufferRequirement.maxWidth, m_inputBufferRequirement.maxHeight);

    // Dump Final Buffer Negotiation Data: Forward-Walk
    CAMX_LOG_TO_FILE(fd, 0, "Forward-walk Negotiation Data: ");
    for (UINT index = 0;
        (NULL != m_pBufferNegotiationData) && (index < m_pBufferNegotiationData->numOutputPortsNotified);
        index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &m_pBufferNegotiationData->pOutputPortNegotiationData[index];
        BufferProperties*          pOutputBufferProperties = pOutputPortNegotiationData->pFinalOutputBufferProperties;
        UINT                       outputPortIndex =
            m_pBufferNegotiationData->pOutputPortNegotiationData[index].outputPortIndex;
        UINT                       outputPortId = GetOutputPortId(outputPortIndex);

        CAMX_LOG_TO_FILE(fd, 0, "PortId: %d Optimal: %d X %d format: %d",
            outputPortId, pOutputBufferProperties->imageFormat.width, pOutputBufferProperties->imageFormat.height,
            pOutputBufferProperties->imageFormat.format);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::DumpDebugInternalNodeInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::DumpDebugInternalNodeInfo(
    UINT64  requestId,
    BOOL    isPerFrameDump)
{
    CHAR             dumpFilename[256];
    CamxDateTime     systemDateTime;
    FILE*            pIFEConfigFile = NULL;
    INT              ifeConfigFD    = -1;
    const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    BOOL dumpEnable = TRUE;
    if ((TRUE == isPerFrameDump) && (FALSE == pSettings->enableIFERegDump))
    {
        dumpEnable = FALSE;
    }

    if ((NULL != m_pIFEPipeline) && (TRUE == dumpEnable) && (NULL != m_pHangDumpBufferAddr))
    {
        FILE*                   pLeftFile       = NULL;
        FILE*                   pRightFile      = NULL;
        INT                     leftFD          = -1;
        INT                     rightFD         = -1;
        VOID*                   pDumpBufferAddr = NULL;
        CmdBuffer*              pRegBuffer      = NULL;

        if ((TRUE == pSettings->enableIFERegDump) && (TRUE == isPerFrameDump))
        {
            pRegBuffer      = CheckCmdBufferWithRequest(requestId, m_pRegDumpBufferManager);
            pDumpBufferAddr = pRegBuffer->GetHostAddr();
        }
        else
        {
            pDumpBufferAddr = m_pHangDumpBufferAddr;
        }

        OsUtils::GetDateTime(&systemDateTime);

        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                              "%s/%s_R%lluIFE_LeftRegDump_%04d%02d%02d_%02d%02d%02d.txt",
                              ConfigFileDirectory,
                              NodeIdentifierString(),
                              requestId,
                              systemDateTime.year + 1900,
                              systemDateTime.month + 1,
                              systemDateTime.dayOfMonth,
                              systemDateTime.hours,
                              systemDateTime.minutes,
                              systemDateTime.seconds);
            pLeftFile = OsUtils::FOpen(dumpFilename, "w");
            if (NULL != pLeftFile)
            {
                leftFD = OsUtils::FileNo(pLeftFile);
            }
            OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                              "%s/%s_R%lluIFE_RightRegDump_%04d%02d%02d_%02d%02d%02d.txt",
                              ConfigFileDirectory,
                              NodeIdentifierString(),
                              requestId,
                              systemDateTime.year + 1900,
                              systemDateTime.month + 1,
                              systemDateTime.dayOfMonth,
                              systemDateTime.hours,
                              systemDateTime.minutes,
                              systemDateTime.seconds);

            pRightFile = OsUtils::FOpen(dumpFilename, "w");
            if (NULL != pRightFile)
            {
                rightFD = OsUtils::FileNo(pRightFile);
            }
        }
        else
        {
            OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                              "%s/%s_R%lluIFE_RegDump_%04d%02d%02d_%02d%02d%02d.txt",
                              ConfigFileDirectory,
                              NodeIdentifierString(),
                              requestId,
                              systemDateTime.year + 1900,
                              systemDateTime.month + 1,
                              systemDateTime.dayOfMonth,
                              systemDateTime.hours,
                              systemDateTime.minutes,
                              systemDateTime.seconds);
            pLeftFile = OsUtils::FOpen(dumpFilename, "w");
            if (NULL != pLeftFile)
            {
                leftFD = OsUtils::FileNo(pLeftFile);
            }
        }
        m_pIFEPipeline->ParseHangRegDump(pDumpBufferAddr, leftFD, rightFD);
        if (NULL != pLeftFile)
        {
            OsUtils::FClose(pLeftFile);
        }

        if (NULL != pRightFile)
        {
            OsUtils::FClose(pRightFile);
        }
    }

    if (TRUE == dumpEnable)
    {
        UINT index = 0;
        do
        {
            // Dump only given request when per frame dump is enabled
            // Else dump all available request that are outstanding
            if (TRUE == isPerFrameDump)
            {
                index = GetIFEPerFrameDataIndex(requestId);
            }

            if (TRUE == m_IFEPerFrameData[index].isValid)
            {
                OsUtils::GetDateTime(&systemDateTime);

                if (IFEModuleMode::DualIFENormal == m_mode)
                {
                    CHAR  dumpStripeInputFilename[256];
                    FILE* pFile1 = NULL;
                    OsUtils::SNPrintF(dumpStripeInputFilename, sizeof(dumpStripeInputFilename),
                        "%s/%s_IFE_StripeInputdump_Reqid_%llu_%04d%02d%02d_%02d%02d%02d.txt",
                        ConfigFileDirectory,
                        NodeIdentifierString(),
                        m_IFEPerFrameData[index].requestID,
                        systemDateTime.year + 1900,
                        systemDateTime.month + 1,
                        systemDateTime.dayOfMonth,
                        systemDateTime.hours,
                        systemDateTime.minutes,
                        systemDateTime.seconds);
                    pFile1 = OsUtils::FOpen(dumpStripeInputFilename, "w");
                    if (NULL != pFile1)
                    {
                        DualIFEUtils::PrintDualIfeInput(OsUtils::FileNo(pFile1),
                            &m_IFEPerFrameData[index].dualIFEConfigData.stripingInput.stripingInput);
                        OsUtils::FClose(pFile1);
                    }

                    CHAR  dumpStripeOutputFilename[256];
                    FILE* pFile2 = NULL;
                    OsUtils::SNPrintF(dumpStripeOutputFilename, sizeof(dumpStripeOutputFilename),
                        "%s/%s_IFE_StripeOutputdump_Reqid_%llu_%04d%02d%02d_%02d%02d%02d.txt",
                        ConfigFileDirectory,
                        NodeIdentifierString(),
                        m_IFEPerFrameData[index].requestID,
                        systemDateTime.year + 1900,
                        systemDateTime.month + 1,
                        systemDateTime.dayOfMonth,
                        systemDateTime.hours,
                        systemDateTime.minutes,
                        systemDateTime.seconds);
                    pFile2 = OsUtils::FOpen(dumpStripeOutputFilename, "w");
                    if (NULL != pFile2)
                    {
                        DualIFEUtils::PrintDualIfeFrame(OsUtils::FileNo(pFile2),
                            &m_IFEPerFrameData[index].dualIFEConfigData.passOut);
                        OsUtils::FClose(pFile2);
                    }
                }

                OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                    "%s/%s_ConfigData_Reqid_%llu_%04d%02d%02d_%02d%02d%02d.txt",
                    ConfigFileDirectory,
                    NodeIdentifierString(),
                    m_IFEPerFrameData[index].requestID,
                    systemDateTime.year + 1900,
                    systemDateTime.month + 1,
                    systemDateTime.dayOfMonth,
                    systemDateTime.hours,
                    systemDateTime.minutes,
                    systemDateTime.seconds);
                pIFEConfigFile = OsUtils::FOpen(dumpFilename, "w");
                if (NULL != pIFEConfigFile)
                {
                    ifeConfigFD = OsUtils::FileNo(pIFEConfigFile);
                    /* Call config fill-up and then close the file */
                    DumpIFEDebugConfig(ifeConfigFD, &(m_IFEPerFrameData[index]));
                    /* Dump Done, Close the file */
                    OsUtils::FClose(pIFEConfigFile);
                }

                m_IFEPerFrameData[index].isValid = FALSE;
            }
            index++;

        } while ((index < IFERequestQueueDepth) && (FALSE == isPerFrameDump));
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CAMX_ASSERT(NULL != pExecuteProcessRequestData);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pNodeProcessRequestData);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pTuningModeData);


    CamxResult              result                   = CamxResultSuccess;
    BOOL                    hasExplicitDependencies  = TRUE;
    NodeProcessRequestData* pNodeRequestData         = pExecuteProcessRequestData->pNodeProcessRequestData;
    const StaticSettings*   pSettings                = HwEnvironment::GetInstance()->GetStaticSettings();
    BOOL                    isFSSnapshot             = FALSE;
    ISPIQTuningDataBuffer   IQOEMTriggerData;

    INT32 sequenceNumber = pNodeRequestData->processSequenceId;

    const UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(pNodeRequestData->pCaptureRequest->requestId);


    isFSSnapshot = IsFSSnapshot(pExecuteProcessRequestData, pNodeRequestData->pCaptureRequest->requestId);

    // For RDI only output case or for raw output only case, do not use 3A stats dependency check
    if ((FALSE == m_useStatsAlgoConfig) || (TRUE == m_RDIOnlyUseCase) ||
        (TRUE  == m_OEMIQSettingEnable) || (TRUE == m_OEMStatsConfig) ||
        // Treat FS snapshot as RDI only usecase i.e. don't set any dependencies
        (TRUE == isFSSnapshot))
    {
        hasExplicitDependencies = FALSE;
    }

    if (0 == sequenceNumber)
    {
        // For the very first request after the last flush
        if (FirstValidRequestId >= requestIdOffsetFromLastFlush)
        {
            FlushInfo flushInfo;
            GetFlushInfo(flushInfo);

            CAMX_LOG_INFO(CamxLogGroupISP, "hasFlushOccurred? = %d, reqId = %llu",
                          flushInfo.hasFlushOccurred,
                          pNodeRequestData->pCaptureRequest->requestId);

            if ((TRUE == flushInfo.hasFlushOccurred) && (CSLFlushAll == flushInfo.flushType))
            {
                result = ResubmitInitPacket(pNodeRequestData->pCaptureRequest->requestId);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to resubmit the init packet, reqId = %llu",
                                   pNodeRequestData->pCaptureRequest->requestId);
                }
            }
        }

        // If the sequence number is zero then it means we are not called from the DRQ, in which case we need to set our
        // dependencies.
        SetDependencies(pNodeRequestData, hasExplicitDependencies);

        // If no dependency, it should do process directly. Set sequneceNumber to 1 to do process directly
        // Or if no stats node, the first request will not be called.
        if (FALSE == Node::HasAnyDependency(pNodeRequestData->dependencyInfo))
        {
            sequenceNumber = 1;
        }
    }

    if (TRUE  == isFSSnapshot)
    {
        // Treat FS snapshot as RDI only usecase i.e. don't set any dependencies
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "FS: IFE:%d SeqNum: %d Snapshot Requested for ReqID:%llu",
            InstanceID(), sequenceNumber, pNodeRequestData->pCaptureRequest->requestId);
    }

    // IMPORTANT:
    // Ensure that no stateful (member) data for this node is modified before __ALL__ dependencies are met. Only
    // Member data that is NOT dependent on per-frame controls/request may be modified before dependencies are met.

    // We are able to process if the sequenceID == 1 (all dependencies are met)
    if ((1 == sequenceNumber) && (NULL  != m_pSensorModeData))
    {
        ISPInputData        moduleInput;
        UINT32              cameraId                = 0;
        UINT32              numberOfCamerasRunning  = 0;
        UINT32              currentCameraId         = 0;
        BOOL                isMultiCameraUsecase    = TRUE;
        BOOL                isMasterCamera          = TRUE;
        IFETuningMetadata*  pTuningMetadata         = NULL ;
        UINT                perFrameDataIndex       = GetIFEPerFrameDataIndex(pNodeRequestData->pCaptureRequest->requestId);
        LSCState            stateLSC;
        ISPStripeConfig*    pFrameConfig            = &m_IFEPerFrameData[perFrameDataIndex].frameConfig;

        if (CamxInvalidRequestId == m_IFELastAcceptedRequestId)
        {
            stateLSC = m_frameConfig.stateLSC;
        }
        else
        {
            stateLSC = m_IFEPerFrameData[GetIFEPerFrameDataIndex(m_IFELastAcceptedRequestId)].frameConfig.stateLSC;
        }

        Utils::Memset(&moduleInput, 0, sizeof(moduleInput));
        Utils::Memset(pFrameConfig, 0, sizeof(m_frameConfig));

        cameraId = GetPipeline()->GetCameraId();

        GetMultiCameraInfo(&isMultiCameraUsecase, &numberOfCamerasRunning, &currentCameraId, &isMasterCamera);

        if (TRUE == isMultiCameraUsecase)
        {
            cameraId = currentCameraId;
            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                             "instance=%p, cameraId = %d, currentCameraId = %d, isMasterCamera = %d,"
                             " isMultiCameraUsecase=%d, numberOfCamerasRunning=%u",
                             this, cameraId, currentCameraId, isMasterCamera, isMultiCameraUsecase, numberOfCamerasRunning);
        }

        // Assign Tuning-data pointer only if needed or available.
        pTuningMetadata = (TRUE == isMasterCamera) ? m_pTuningMetadata : NULL;

        moduleInput.pOTPData            = m_pOTPData;
        moduleInput.pIFEOutputPathInfo  = m_IFEOutputPathInfo;
        moduleInput.csidBinningInfo     = m_csidBinningInfo;

        // Since OEM IQ settings come from Inputpool, data always avail, grab unconditionally
        if (TRUE == m_OEMIQSettingEnable)
        {
            moduleInput.pOEMIQSetting = NULL;
            result = GetOEMIQSettings(&moduleInput.pOEMIQSetting);
        }

        if (TRUE == m_OEMStatsConfig && CamxResultSuccess == result)
        {
            result = GetOEMStatsConfig(pFrameConfig);
        }

        // If we actually have dependencies we can get data when the sequence number is 1
        // meaning we were called from the DRQ
        if ((CamxResultSuccess == result) && (TRUE == hasExplicitDependencies))
        {
            result = Get3AFrameConfig(&moduleInput, pFrameConfig, pNodeRequestData->pCaptureRequest->requestId);
        }

        if (CamxResultSuccess == result)
        {
            result = FetchCmdBuffers(pNodeRequestData->pCaptureRequest->requestId, FALSE);

            if (CamxResultSuccess == result)
            {
                moduleInput.pTuningDataManager  = GetTuningDataManager();

                // Setup the Input data for IQ Parameter
                m_IFELastAcceptedRequestId                  = pNodeRequestData->pCaptureRequest->requestId;
                moduleInput.frameNum                        = pNodeRequestData->pCaptureRequest->requestId;
                moduleInput.pAECUpdateData                  = &(pFrameConfig->AECUpdateData);
                moduleInput.pAECStatsUpdateData             = &(pFrameConfig->AECStatsUpdateData);
                moduleInput.pAWBUpdateData                  = &(pFrameConfig->AWBUpdateData);
                moduleInput.pAWBStatsUpdateData             = &(pFrameConfig->AWBStatsUpdateData);
                moduleInput.pAFUpdateData                   = &(pFrameConfig->AFUpdateData);
                moduleInput.pAFStatsUpdateData              = &(pFrameConfig->AFStatsUpdateData);
                moduleInput.pAFDStatsUpdateData             = &(pFrameConfig->AFDStatsUpdateData);
                moduleInput.pIHistStatsUpdateData           = &(pFrameConfig->IHistStatsUpdateData);
                moduleInput.pPDHwConfig                     = &(pFrameConfig->pdHwConfig);
                moduleInput.pCSStatsUpdateData              = &(pFrameConfig->CSStatsUpdateData);
                moduleInput.pStripeConfig                   = pFrameConfig;
                moduleInput.p32bitDMIBuffer                 = m_p32bitDMIBuffer;
                moduleInput.p64bitDMIBuffer                 = m_p64bitDMIBuffer;
                moduleInput.p32bitDMIBufferAddr             = m_p32bitDMIBufferAddr;
                moduleInput.p64bitDMIBufferAddr             = m_p64bitDMIBufferAddr;
                moduleInput.pHwContext                      = GetHwContext();
                moduleInput.pHALTagsData                    = &m_IFEPerFrameData[perFrameDataIndex].HALTagsData;
                moduleInput.disableManual3ACCM              = m_disableManual3ACCM;
                moduleInput.sensorID                        = cameraId;
                moduleInput.HVXData                         = m_HVXInputData.HVXConfiguration;
                moduleInput.sensorBitWidth                  = m_pSensorCaps->sensorConfigs[0].streamConfigs[0].bitWidth;
                moduleInput.sensorData.fullResolutionWidth  = m_ISPInputSensorData.fullResolutionWidth;
                moduleInput.sensorData.fullResolutionHeight = m_ISPInputSensorData.fullResolutionHeight;
                moduleInput.useHardcodedRegisterValues      = CheckToUseHardcodedRegValues(moduleInput.pHwContext);
                moduleInput.enableIFEDualStripeLog          = CheckToEnableDualIFEStripeInfo(moduleInput.pHwContext);

                moduleInput.pIFETuningMetadata              = pTuningMetadata;
                moduleInput.pTuningData                     = pExecuteProcessRequestData->pTuningModeData;
                moduleInput.tuningModeChanged               = ISPIQModule::IsTuningModeDataChanged(
                                                                  moduleInput.pTuningData,
                                                                  &m_tuningData);
                moduleInput.dumpRegConfig                   = static_cast<Titan17xContext*>(GetHwContext())->
                                                                  GetTitan17xSettingsManager()->
                                                                  GetTitan17xStaticSettings()->
                                                                  dumpIFERegConfigMask;
                moduleInput.regOffsetIndex                  = OffsetOfIFEIQModuleIndex;
                moduleInput.registerBETEn                   = FALSE;
                moduleInput.maxOutputWidthFD                = m_maxOutputWidthFD;
                moduleInput.titanVersion                    = m_titanVersion;
                moduleInput.maximumPipelineDelay            = GetMaximumPipelineDelay();
                moduleInput.isInitPacket                    = FALSE;
                moduleInput.pIFEStripeInterface             = m_pIFEStripeInterface;
                moduleInput.pFrameBased                     = &m_frameBased[0];
                moduleInput.pRDIStreams                     = &m_RDIStreams[0];
                moduleInput.IFEPixelRawPort                 = m_IFEPixelRawPort;

                if ((FirstValidRequestId >= requestIdOffsetFromLastFlush) ||
                    (IFEProfileIdOffline == m_instanceProperty.profileId))
                {
                    moduleInput.forceTriggerUpdate = TRUE;

                    FlushInfo flushInfo;
                    GetFlushInfo(flushInfo);

                    if (TRUE == flushInfo.hasFlushOccurred)
                    {
                        if (CSLFlushAll == flushInfo.flushType)
                        {
                            CAMX_LOG_INFO(CamxLogGroupISP,
                                          "Node::%s reqId = %llu, First request after flush. Resetting DMI bank to 1.",
                                          NodeIdentifierString(),
                                          pNodeRequestData->pCaptureRequest->requestId);
                            m_pIFEPipeline->UpdateDMIBankSelectValue(&moduleInput.bankUpdate, FALSE);
                        }
                        else
                        {
                            if (NULL != m_pFlushDumpBufferAddr)
                            {
                                m_pIFEPipeline->ParseFlushRegDump(m_pFlushDumpBufferAddr, &moduleInput.bankUpdate);
                            }
                            else
                            {
                                CAMX_LOG_WARN(CamxLogGroupISP, "m_pFlushDumpBufferAddr is NULL.");
                            }
                        }
                    }
                }

                // Update the Tintless Algo processing requirement
                moduleInput.skipTintlessProcessing = CanSkipAlgoProcessing(&moduleInput);

                // Cache tuning mode selector data for comparison for next frame, to help
                // optimize tuning data (tree) search in the IQ modules
                if (TRUE == moduleInput.tuningModeChanged)
                {
                    Utils::Memcpy(&m_tuningData, moduleInput.pTuningData, sizeof(ChiTuningModeParameter));
                    FillICAChromatixGridData(&m_tuningData);
                }

                moduleInput.pipelineIFEData.moduleMode = m_mode;
                if (TRUE == m_libInitialData.isSucceed)
                {
                    moduleInput.pLibInitialData = m_libInitialData.pLibData;
                }

                UpdateCamifSettings(&moduleInput);

                // Get HAL tags
                result = GetMetadataTags(&moduleInput);

                // Off-centered zoom is not supported in dual IFE mode.
                // Return error if IFE is running in dual IFE mode and HAL crop setting is not centered.
                if (IFEModuleMode::DualIFENormal == m_mode)
                {
                    const CropWindow& rOriginalHALCrop = moduleInput.originalHALCrop;

                    // The calculated sensor active array pixel width and height with assumed centered zoom
                    const INT32 expectedWidth  = rOriginalHALCrop.left * 2 + rOriginalHALCrop.width;
                    const INT32 expectedHeight = rOriginalHALCrop.top * 2 + rOriginalHALCrop.height;

                    if ((Utils::AbsoluteINT32(static_cast<INT32>(m_sensorActiveArrayWidth) - expectedWidth) > 2) ||
                        (Utils::AbsoluteINT32(static_cast<INT32>(m_sensorActiveArrayHeight) - expectedHeight) > 2))
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP, "Off-centered zoom is not supported in DualIFE mode: "
                                       "HAL crop: (%d, %d, %d, %d), expected: width=%d, height=%d, "
                                       "actual active pixel array: width=%u, height=%u",
                                       rOriginalHALCrop.left,
                                       rOriginalHALCrop.top,
                                       rOriginalHALCrop.width,
                                       rOriginalHALCrop.height,
                                       expectedWidth, expectedHeight,
                                       m_sensorActiveArrayWidth, m_sensorActiveArrayHeight);

                        result = CamxResultEInvalidArg;
                    }
                }

                if (CamxResultSuccess == result)
                {
                    BOOL initAll = FALSE;
                    if ((FALSE == m_useStatsAlgoConfig) && (FALSE == m_OEMIQSettingEnable) && (FALSE == m_OEMStatsConfig))
                    {
                        initAll = TRUE;
                    }
                    HardcodeSettings(&moduleInput, pFrameConfig, initAll);

                    // Restore LSC State
                    pFrameConfig->stateLSC = stateLSC;

                    HardcodeTintlessSettings(&moduleInput);

                    // Get optional OEM trigger data if available
                    if (NULL != moduleInput.pIFETuningMetadata)
                    {
                        IQOEMTriggerData.pBuffer    = moduleInput.pIFETuningMetadata->oemTuningData.IQOEMTuningData;
                        IQOEMTriggerData.size       = sizeof(moduleInput.pIFETuningMetadata->oemTuningData.IQOEMTuningData);
                    }
                    else
                    {
                        IQOEMTriggerData.pBuffer    = NULL;
                        IQOEMTriggerData.size       = 0;
                    }

                    // Call IQInterface to Set up the Trigger data
                    IQInterface::IQSetupTriggerData(&moduleInput, this, TRUE , &IQOEMTriggerData);

                    // Update Dual IFE IQ module config
                    if (IFEModuleMode::DualIFENormal == m_mode)
                    {
                        // Store bankSelect for interpolation to store mesh_table_l[bankSelect] and mesh_table_r[bankSelect]
                        pFrameConfig->pFrameLevelData   = &m_IFEPerFrameData[perFrameDataIndex].ISPFrameData;
                        moduleInput.pStripeConfig       = pFrameConfig;
                        moduleInput.pCalculatedData     = &m_IFEPerFrameData[perFrameDataIndex].ISPFramelevelData;
                        moduleInput.pCalculatedMetadata = &m_ISPMetadata;
                        if (ISPHwTitan480 == m_hwMask)
                        {
                            pFrameConfig->stateLSC.dependence40Data.bankSelect =
                                m_stripeConfigs[0].stateLSC.dependence40Data.bankSelect;
                        }
                        else
                        {
                            pFrameConfig->stateLSC.dependenceData.bankSelect =
                                m_stripeConfigs[0].stateLSC.dependenceData.bankSelect;
                        }
                        moduleInput.pStripingInput = &m_IFEPerFrameData[perFrameDataIndex].dualIFEConfigData.stripingInput;
                        result = PrepareStripingParameters(&moduleInput);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupISP, "PrepareStripingParameters failed");
                        }
                        m_stripeConfigs[0].stateLSC = pFrameConfig->stateLSC;
                        m_stripeConfigs[1].stateLSC = pFrameConfig->stateLSC;
                        if (CamxResultSuccess == result)
                        {
                            // Release stripes (if any)
                            // Note this will later be added to striping
                            // lib as a function so that SW doesn't care about internals of that lib!
                            if (IFEModuleMode::DualIFENormal == m_mode)
                            {
                                m_pIFEStripeInterface->ReleaseDualIFEPassOutput();
                            }
                            DualIFEUtils::UpdateDualIFEConfig(&moduleInput,
                                                              m_PDAFInfo,
                                                              m_stripeConfigs,
                                                              &m_dualIFESplitParams,
                                                              m_pPassOut,
                                                              m_pSensorModeData,
                                                              m_pIFEStripeInterface,
                                                              NodeIdentifierString());
                        }
                    }
                    else
                    {
                        // Each module has stripe specific configuration that needs to be updated before calling upon module
                        // Execute function. In single IFE case, there's only one stripe and hence we will use per frame
                        // stripeConfigs[0] to hold the input configuration
                        UpdateIQStateConfiguration(pFrameConfig, &m_stripeConfigs[0]);
                        moduleInput.pStripeConfig   = pFrameConfig;
                    }

                    if ((FALSE == isFSSnapshot) && (CamxResultSuccess == result))
                    {
                        // Execute IQ module configuration and updated module enable info
                        result = ProgramIQConfig(&moduleInput);
                    }
                }

                if (CamxResultSuccess == result && FALSE == m_RDIOnlyUseCase)
                {
                    if (FALSE == isFSSnapshot)
                    {
                        result = ProgramIQEnable(&moduleInput);
                    }
                    else
                    {
                        moduleInput.pCmdBuffer           = m_pCommonCmdBuffer;
                        moduleInput.pCalculatedData      = &m_IFEPerFrameData[perFrameDataIndex].ISPData[CommonIFE];
                        moduleInput.pCalculatedMetadata  = &m_ISPMetadata;
                    }
                }

                if (CamxResultSuccess == result)
                {
                    UpdateIFEDebugConfig(&moduleInput, pNodeRequestData->pCaptureRequest->requestId);
                }

                // Program output split info for dual IFE stripe
                if ((CamxResultSuccess == result) && (IFEModuleMode::DualIFENormal == m_mode))
                {
                    result = ProgramStripeConfig();
                    DualIFEUtils::ReleaseDualIfePassResult(m_pPassOut, m_pIFEStripeInterface);
                }

                // Update WM Config

                if (CamxResultSuccess == result)
                {
                    if (IFEModuleMode::DualIFENormal == m_mode)
                    {
                        result = m_pIFEPipeline->UpdateWMConfig(m_pLeftGenericBlobCmdBuffer,
                            &m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE]);
                        if (CamxResultSuccess == result)
                        {
                            result = m_pIFEPipeline->UpdateWMConfig(m_pRightGenericBlobCmdBuffer,
                                &m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE]);
                        }
                    }
                    else
                    {
                        result = m_pIFEPipeline->UpdateWMConfig(m_pGenericBlobCmdBuffer,
                            &m_IFEPerFrameData[perFrameDataIndex].ISPData[CommonIFE]);
                    }
                }

                // Post metadata from IQ modules to metadata
                if (CamxResultSuccess == result)
                {
                    if (FALSE == m_RDIOnlyUseCase)
                    {
                        result = PostMetadata(&moduleInput);
                    }
                    else
                    {
                        // For RDI only use-case, some of CTS test case expects tag in metadata.
                        result = PostMetadataRaw();
                    }
                }

                // Update tuning metadata if setting is enabled
                if ((NULL               != moduleInput.pIFETuningMetadata)             &&
                    (FALSE              == m_RDIOnlyUseCase)                           &&
                    (TRUE               == isMasterCamera)                             &&
                    (CamxResultSuccess  == result))
                {
                    // Only use debug data on the master camera
                    DumpTuningMetadata(&moduleInput);
                }
                else if (NULL != m_pTuningMetadata) // Print only if tuning-metadata is enable
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupDebugData,
                                     "Tuning-metadata:IFE: SKIP: reqID: %llu isMaster: %u RT: %u InstanceID: %u, isRDI: %u",
                                     moduleInput.frameNum,
                                     isMasterCamera,
                                     InstanceID(),
                                     m_RDIOnlyUseCase);
                }
            }
        }

        // Update bandwidth config if there is a change in enabled ports for this request
        if (CamxResultSuccess == result)
        {
            if (IFEGenericBlobTypeResourceBWConfig == m_pIFEPipeline->GetIFEBandWidthConfigurationVersion())
            {
                result = SetupResourceBWConfig(pExecuteProcessRequestData, pNodeRequestData->pCaptureRequest->requestId);
            }
            else if (IFEGenericBlobTypeResourceBWConfigV2 == m_pIFEPipeline->GetIFEBandWidthConfigurationVersion())
            {
                result = SetupResourceBWConfigV2(pExecuteProcessRequestData, pNodeRequestData->pCaptureRequest->requestId);
            }
        }

        if (CamxResultSuccess == result)
        {
            if (FALSE == IsPipelineStreamedOn())
            {
                if (TRUE == m_initialConfigPending)
                {
                    m_initialConfigPending = FALSE;

                    if (IFEModuleMode::DualIFENormal == m_mode)
                    {
                        result = AddCmdBufferReference(pNodeRequestData->pCaptureRequest->requestId,
                                                       CSLPacketOpcodesDualIFEInitialConfig,
                                                       FALSE);
                    }
                    else
                    {
                        result = AddCmdBufferReference(pNodeRequestData->pCaptureRequest->requestId,
                                                       CSLPacketOpcodesIFEInitialConfig,
                                                       FALSE);
                    }
                }
                else
                {
                    if (IFEModuleMode::DualIFENormal == m_mode)
                    {
                        result = AddCmdBufferReference(pNodeRequestData->pCaptureRequest->requestId,
                                                       CSLPacketOpcodesDualIFEUpdate,
                                                       FALSE);
                    }
                    else
                    {
                        result = AddCmdBufferReference(pNodeRequestData->pCaptureRequest->requestId,
                                                       CSLPacketOpcodesIFEUpdate,
                                                       FALSE);
                    }
                }
            }
            else
            {
                m_initialConfigPending = TRUE;
                if (IFEModuleMode::DualIFENormal == m_mode)
                {
                    result = AddCmdBufferReference(pNodeRequestData->pCaptureRequest->requestId,
                                                   CSLPacketOpcodesDualIFEUpdate,
                                                   FALSE);
                }
                else
                {
                    result = AddCmdBufferReference(pNodeRequestData->pCaptureRequest->requestId,
                                                   CSLPacketOpcodesIFEUpdate,
                                                   FALSE);
                }
            }
        }

        // Update the IO buffers to the request
        if (CamxResultSuccess == result)
        {
            result = ConfigBufferIO(pExecuteProcessRequestData);
        }

        // Commit DMI command buffers and do a CSL submit of the request
        if (CamxResultSuccess == result)
        {
            result = CommitAndSubmitPacket();
            if (CamxResultSuccess == result)
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "IFE:%d Submitted packets with requestId = %llu", InstanceID(),
                    pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId);
            }
            else if (CamxResultECancelledRequest == result)
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "IFE:%d Submit packet canceled for requestId = %llu as session is in"
                    " Flush state", InstanceID(),
                    pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId);
            }
        }
    }

    // Release stripes (if any)
    // Note this will later be added to striping lib as a function so that SW doesn't care about internals of that lib!
    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        m_pIFEStripeInterface->ReleaseDualIFEPassOutput();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupDeviceResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupDeviceResource()
{
    CamxResult          result                                  = CamxResultSuccess;
    SIZE_T              resourceSize                            = 0;
    VOID*               pInputResource                          = NULL;
    UINT                outputPortId[MaxDefinedIFEOutputPorts];
    UINT                totalOutputPort                         = 0;
    UINT                inputPortId[MaxDefinedIFEInputPorts];
    UINT                totalInputPort                          = 0;
    UINT                IFEInputResourceSize                    = 0;
    UINT                IFEAcquireInputVersion                  = 0;
    m_numResource                                               = 0;
    BOOL                inputMapped                             = FALSE;

    /// @todo (CAMX-1315)   Add logic for dual IFE and fall back on single IFE if dual fails.

    // Get Input Port List
    /// @todo (CAMX-1015) Get this only once in ProcessingNodeInitialize() and save it off in the IFENode class
    GetAllInputPortIds(&totalInputPort, &inputPortId[0]);

    // Get Output Port List
    GetAllOutputPortIds(&totalOutputPort, &outputPortId[0]);

    if (CamxResultSuccess == result)
    {
        if (totalOutputPort <= 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Error: no output port");
            result = CamxResultEInvalidArg;
        }
    }

    if (CamxResultSuccess == result)
    {
        // Set up pixel channel (including RAW) resource list
        pInputResource = NULL;
        resourceSize   = 0;

        UINT perChannelOutputPortId[MaxDefinedIFEOutputPorts];
        UINT perChannelFEOutputPortId[MaxDefinedIFEOutputPorts];
        UINT perChannelTotalOutputPort;
        UINT perChannelFETotalOutputPort;

        IFEAcquireInputVersion = m_pIFEPipeline->GetISPAcquireInputInfoVersion();
        if (ISPAcquireInputVersion2 == IFEAcquireInputVersion)
        {
            IFEInputResourceSize = m_pSensorModeData->streamConfigCount * (sizeof(ISPInResourceInfoVer2) +
                                   (totalOutputPort * sizeof(ISPOutResourceInfoVer2)));
            // In FS mode, IFE Bus Read is provided as an additional input resource
            if (TRUE == m_enableBusRead)
            {
                IFEInputResourceSize += sizeof(ISPInResourceInfoVer2);
            }
        }
        else
        {
            IFEInputResourceSize = m_pSensorModeData->streamConfigCount * (sizeof(ISPInResourceInfo) +
                                   (totalOutputPort * sizeof(ISPOutResourceInfo)));
            // In FS mode, IFE Bus Read is provided as an additional input resource
            if (TRUE == m_enableBusRead)
            {
                IFEInputResourceSize += sizeof(ISPInResourceInfo);
            }
        }

        m_IFEResourceSize    = IFEInputResourceSize + sizeof(ISPAcquireHWInfo);

        if (NULL == m_pIFEResourceInfo)
        {
            m_pIFEResourceInfo = static_cast<ISPAcquireHWInfo*>(CAMX_CALLOC(m_IFEResourceSize));
        }

        if (NULL != m_pIFEResourceInfo)
        {
            // For each port source type, setup the output ports.
            for (UINT streamIndex = 0; streamIndex < m_pSensorModeData->streamConfigCount; streamIndex++)
            {
                StreamType  streamType                  = m_pSensorModeData->streamConfig[streamIndex].type;
                const UINT  inputResourcePortSourceType = TranslateSensorStreamConfigTypeToPortSourceType(streamType);

                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Port source type = %u streamType=%u",
                    inputResourcePortSourceType, streamType);

                perChannelTotalOutputPort   = 0;
                perChannelFETotalOutputPort = 0;

                // Filter the output port that matches with the current input port source type
                for (UINT outputPortIndex = 0; outputPortIndex < totalOutputPort; outputPortIndex++)
                {
                    const UINT currentOutputPortSourceTypeId =
                        GetOutputPortSourceType(OutputPortIndex(outputPortId[outputPortIndex]));

                    // If input resource is of Pixel type, the output port source type can be either Undefined or Pixel.
                    if ((PortSrcTypePixel == inputResourcePortSourceType) &&
                        ((PortSrcTypeUndefined == currentOutputPortSourceTypeId) ||
                         (PortSrcTypePixel     == currentOutputPortSourceTypeId)))
                    {
                        // In FS mode configure only the RDI ports to Image stream
                        // Rest of the output ports should be configured to BusRead Input
                        if (TRUE == m_enableBusRead)
                        {
                            if ((IFEOutputPortRDI0 == outputPortId[outputPortIndex]) ||
                                (IFEOutputPortRDI1 == outputPortId[outputPortIndex]) ||
                                (IFEOutputPortRDI2 == outputPortId[outputPortIndex]) ||
                                (IFEOutputPortRDI3 == outputPortId[outputPortIndex]))
                            {
                                perChannelOutputPortId[perChannelTotalOutputPort] = outputPortId[outputPortIndex];
                                perChannelTotalOutputPort++;
                            }
                            else
                            {
                                perChannelFEOutputPortId[perChannelFETotalOutputPort] = outputPortId[outputPortIndex];
                                perChannelFETotalOutputPort++;
                            }
                        }
                        else
                        {
                            perChannelOutputPortId[perChannelTotalOutputPort] = outputPortId[outputPortIndex];
                            perChannelTotalOutputPort++;
                        }
                    }
                    else if (inputResourcePortSourceType == currentOutputPortSourceTypeId)
                    {
                        perChannelOutputPortId[perChannelTotalOutputPort] = outputPortId[outputPortIndex];
                        perChannelTotalOutputPort++;
                    }

                    if ((PortSrcTypePixel == inputResourcePortSourceType) &&
                        (TRUE == CheckIfPDAFType3Supported()))
                    {
                        // This is PDAF Type3 case
                        // In PDAF Type3, we dont have a separate PDAF Stream from sensor
                        // The PD Pixles will be present along with Image Pixels
                        // So,  to extract PD Pixels we have to link the RDI PDAF port and DUAL PD
                        // port to the pixel input.
                        if (((IFEOutputPortRDI0 == outputPortId[outputPortIndex]) ||
                            (IFEOutputPortRDI1 == outputPortId[outputPortIndex]) ||
                            (IFEOutputPortRDI2 == outputPortId[outputPortIndex]) ||
                            (IFEOutputPortRDI3 == outputPortId[outputPortIndex])) &&
                            (PortSrcTypePDAF == currentOutputPortSourceTypeId))
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Adding PDAF Type3 RDI Output port to Pixel Input");
                            perChannelOutputPortId[perChannelTotalOutputPort] = outputPortId[outputPortIndex];
                            perChannelTotalOutputPort++;
                        }
                        else if (IFEOutputPortStatsDualPD == outputPortId[outputPortIndex])
                        {

                            CAMX_LOG_VERBOSE(CamxLogGroupISP, "PDAF Type 3 Adding Dual PD Output port to Pixel Input");
                            perChannelOutputPortId[perChannelTotalOutputPort] = outputPortId[outputPortIndex];
                            perChannelTotalOutputPort++;
                        }
                    }
                }

                // Ignore for the sensor stream types that cannot be mapped to any IFE output port.
                if (perChannelTotalOutputPort > 0)
                {

                    Utils::Memset(&m_IFEInputResource, 0, sizeof(m_IFEInputResource));
                    Utils::Memset(&m_IFEOutputResource, 0, sizeof(m_IFEOutputResource));
                    inputMapped = FALSE;

                    result = SetupChannelResource(inputPortId[0],
                                                  perChannelTotalOutputPort,
                                                  &perChannelOutputPortId[0],
                                                  inputResourcePortSourceType,
                                                  streamIndex,
                                                  &inputMapped,
                                                  &m_IFEInputResource);

                    if ((CamxResultSuccess == result) && (TRUE == inputMapped))
                    {

                        if (IFEAcquireInputVersion == ISPAcquireInputVersion2)
                        {

                            pInputResource = reinterpret_cast<UINT8*>(&m_pIFEResourceInfo->data) + resourceSize;
                            result         = CopyResourcesToVer2(reinterpret_cast<ISPInResourceInfoVer2*>(pInputResource));
                            resourceSize  += sizeof(ISPInResourceInfoVer2) + (sizeof(ISPOutResourceInfoVer2) *
                                (perChannelTotalOutputPort - 1));
                        }
                        else
                        {
                            pInputResource = reinterpret_cast<UINT8*>(&m_pIFEResourceInfo->data) + resourceSize;
                            result         = CopyResourcesToVer1(reinterpret_cast<ISPInResourceInfo*>(pInputResource));
                            resourceSize  += sizeof(ISPInResourceInfo) + (sizeof(ISPOutResourceInfo) *
                                (perChannelTotalOutputPort - 1));
                        }

                        if (CamxResultSuccess == result)
                        {
                            m_numResource++;
                        }
                    }
                }

                if ((TRUE == m_enableBusRead)          &&
                    (perChannelFETotalOutputPort > 0) &&
                    (StreamType::IMAGE == streamType))
                {
                    CAMX_LOG_INFO(CamxLogGroupISP, "FS: perChannelTotalOutputPort:%d perChannelFETotalOutputPort:%d",
                                  perChannelTotalOutputPort, perChannelFETotalOutputPort);

                    Utils::Memset(&m_IFEInputResource, 0, sizeof(m_IFEInputResource));
                    Utils::Memset(&m_IFEOutputResource, 0, sizeof(m_IFEOutputResource));
                    inputMapped = FALSE;

                    result = SetupChannelResource(inputPortId[0],
                                                  perChannelFETotalOutputPort,
                                                  &perChannelFEOutputPortId[0],
                                                  inputResourcePortSourceType,
                                                  streamIndex,
                                                  &inputMapped,
                                                  &m_IFEInputResource);

                    if ((CamxResultSuccess == result) && (TRUE == inputMapped))
                    {
                        if (IFEAcquireInputVersion == ISPAcquireInputVersion2)
                        {
                            pInputResource = reinterpret_cast<UINT8*>(&m_pIFEResourceInfo->data) + resourceSize;
                            // Update the resource type as PHY0/1/2.. for the 1st time,
                            // once acquired the consecutive type should be through read path only.
                            reinterpret_cast<ISPInResourceInfoVer2*>(pInputResource)->resourceType = IFEInputBusRead;
                            result        = CopyResourcesToVer2(reinterpret_cast<ISPInResourceInfoVer2*>(pInputResource));
                            resourceSize += sizeof(ISPInResourceInfoVer2) + (sizeof(ISPOutResourceInfo) *
                                (perChannelFETotalOutputPort - 1));
                        }
                        else
                        {
                            pInputResource = reinterpret_cast<UINT8*>(&m_pIFEResourceInfo->data) + resourceSize;
                            reinterpret_cast<ISPInResourceInfo*>(pInputResource)->resourceType = IFEInputBusRead;
                            result        = CopyResourcesToVer1(reinterpret_cast<ISPInResourceInfo*>(pInputResource));
                            resourceSize += sizeof(ISPInResourceInfo) +
                                (sizeof(ISPOutResourceInfo) * (perChannelFETotalOutputPort - 1));
                        }

                        if (CamxResultSuccess == result)
                        {
                            m_numResource++;
                        }
                    }
                }
            }

            // Update the IFE resource structure so that KMD will decide which structure to use whicle acquiriing.
            m_pIFEResourceInfo->commonInfoVersion = m_pIFEPipeline->GetISPAcquireCommonInfoVersion();
            m_pIFEResourceInfo->commonInfoSize    = 0;
            m_pIFEResourceInfo->commonInfoOffset  = 0;
            m_pIFEResourceInfo->numInputs         = m_numResource;
            m_pIFEResourceInfo->inputInfoVersion  = IFEAcquireInputVersion;
            m_pIFEResourceInfo->inputInfoSize     = IFEInputResourceSize;
            m_pIFEResourceInfo->inputInfoOffset   = 0;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Calloc for m_pIFEResourceInfo failed!")
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CopyResourcesToVer1
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::CopyResourcesToVer1(
    ISPInResourceInfo* pInputResource)
{
    ISPOutResourceInfo* pOutputResource = NULL;
    CamxResult          result          = CamxResultSuccess;
    if (NULL != pInputResource)
    {
        pInputResource->resourceType = m_IFEInputResource.resourceType;
        pInputResource->laneType     = m_IFEInputResource.laneType;
        pInputResource->laneNum      = m_IFEInputResource.laneNum;
        pInputResource->laneConfig   = m_IFEInputResource.laneConfig;
        pInputResource->VC           = m_IFEInputResource.VC[0];
        pInputResource->DT           = m_IFEInputResource.DT[0];
        pInputResource->format       = m_IFEInputResource.format;
        pInputResource->testPattern  = m_IFEInputResource.testPattern;
        pInputResource->usageType    = m_IFEInputResource.usageType;
        pInputResource->leftStart    = m_IFEInputResource.leftStart;
        pInputResource->leftStop     = m_IFEInputResource.leftStop;
        pInputResource->leftWidth    = m_IFEInputResource.leftWidth;
        pInputResource->rightStart   = m_IFEInputResource.rightStart;
        pInputResource->rightStop    = m_IFEInputResource.rightStop;
        pInputResource->rightWidth   = m_IFEInputResource.rightWidth;
        pInputResource->lineStart    = m_IFEInputResource.lineStart;
        pInputResource->lineStop     = m_IFEInputResource.lineStop;
        pInputResource->height       = m_IFEInputResource.height;
        pInputResource->pixleClock   = m_IFEInputResource.pixleClock;
        pInputResource->batchSize    = m_IFEInputResource.batchSize;
        pInputResource->DSPMode      = m_IFEInputResource.DSPMode;
        pInputResource->HBICount     = m_IFEInputResource.HBICount;

        pInputResource->numberOutResource = m_IFEInputResource.numberOutResource;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "No Of Output Resources %d ", pInputResource->numberOutResource);

        pOutputResource = reinterpret_cast<ISPOutResourceInfo*>(&(pInputResource->pDataField));

        for (UINT32 i = 0; i < pInputResource->numberOutResource; i++)
        {
            pOutputResource->resourceType     = m_IFEOutputResource[i].resourceType;
            pOutputResource->format           = m_IFEOutputResource[i].format;
            pOutputResource->width            = m_IFEOutputResource[i].width;
            pOutputResource->height           = m_IFEOutputResource[i].height;
            pOutputResource->compositeGroupId = m_IFEOutputResource[i].compositeGroupId;
            pOutputResource->splitPoint       = m_IFEOutputResource[i].splitPoint;
            pOutputResource->secureMode       = m_IFEOutputResource[i].secureMode;
            pOutputResource++;

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Output resource type %d, Dimension [%d * %d] format %d ",
                pOutputResource->resourceType,
                pOutputResource->width,
                pOutputResource->height,
                pOutputResource->format);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Argument !!!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CopyResourcesToVer2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::CopyResourcesToVer2(
    ISPInResourceInfoVer2* pInputResource)
{
    ISPOutResourceInfoVer2* pOutputResource = NULL;
    CamxResult              result          = CamxResultSuccess;
    if (NULL != pInputResource)
    {
        pInputResource->resourceType = m_IFEInputResource.resourceType;
        pInputResource->laneType     = m_IFEInputResource.laneType;
        pInputResource->laneNum      = m_IFEInputResource.laneNum;
        pInputResource->laneConfig   = m_IFEInputResource.laneConfig;
        pInputResource->numValidVCDT = m_IFEInputResource.numValidVCDT;

        for (UINT32 i = 0; i < pInputResource->numValidVCDT; i++)
        {
            pInputResource->VC[i] = m_IFEInputResource.VC[i];
            pInputResource->DT[i] = m_IFEInputResource.DT[i];
        }

        pInputResource->format      = m_IFEInputResource.format;
        pInputResource->testPattern = m_IFEInputResource.testPattern;
        pInputResource->usageType   = m_IFEInputResource.usageType;
        pInputResource->leftStart   = m_IFEInputResource.leftStart;
        pInputResource->leftStop    = m_IFEInputResource.leftStop;
        pInputResource->leftWidth   = m_IFEInputResource.leftWidth;
        pInputResource->rightStart  = m_IFEInputResource.rightStart;
        pInputResource->rightStop   = m_IFEInputResource.rightStop;
        pInputResource->rightWidth  = m_IFEInputResource.rightWidth;
        pInputResource->lineStart   = m_IFEInputResource.lineStart;
        pInputResource->lineStop    = m_IFEInputResource.lineStop;
        pInputResource->height      = m_IFEInputResource.height;
        pInputResource->pixleClock  = m_IFEInputResource.pixleClock;
        pInputResource->batchSize   = m_IFEInputResource.batchSize;
        pInputResource->DSPMode     = m_IFEInputResource.DSPMode;
        pInputResource->HBICount    = m_IFEInputResource.HBICount;
        pInputResource->customNode  = m_IFEInputResource.customNode;

        pInputResource->numberOutResource = m_IFEInputResource.numberOutResource;
        pInputResource->offlineMode       = m_IFEInputResource.offlineMode;
        pInputResource->HorizontalBinning = m_IFEInputResource.horizontalBinning;
        pInputResource->QCFABinning       = m_IFEInputResource.QCFABinning;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "No Of Output Resources %d ", pInputResource->numberOutResource);

        pOutputResource = reinterpret_cast<ISPOutResourceInfoVer2*>(&(pInputResource->pDataField));
        for (UINT32 i = 0; i < pInputResource->numberOutResource; i++)
        {
            pOutputResource->resourceType    = m_IFEOutputResource[i].resourceType;
            pOutputResource->format          = m_IFEOutputResource[i].format;
            pOutputResource->width           = m_IFEOutputResource[i].width;
            pOutputResource->height          = m_IFEOutputResource[i].height;
            pOutputResource->compositeGroupId= m_IFEOutputResource[i].compositeGroupId;
            pOutputResource->splitPoint      = m_IFEOutputResource[i].splitPoint;
            pOutputResource->secureMode      = m_IFEOutputResource[i].secureMode;
            pOutputResource->wmMode          = m_IFEOutputResource[i].wmMode;
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Output resource type %d, Dimension [%d * %d] format %d ",
                pOutputResource->resourceType,
                pOutputResource->width,
                pOutputResource->height,
                pOutputResource->format);
            pOutputResource++;

        }

    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Argument !!!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculateCSIDClockRate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::CalculateCSIDClockRate(
    UINT64* pClockRate,
    UINT32  sensorBitWidth)
{
    CamxResult          result = CamxResultSuccess;
    UINT32              laneType;
    UINT32              laneCount;

    *pClockRate = 0;

    INT overrideCSIDClockMHz = static_cast<INT>(GetStaticSettings()->csidClockFrequencyMHz);
    if (0 > overrideCSIDClockMHz)
    {
        // If CSIDClockFrequencyMHz is set to 0xFFFFFFFF, this means to set CSID clock to its max turbo clock frequency.
        *pClockRate = HwEnvironment::GetInstance()->GetPlatformStaticCaps()->maxCSIDTURBOClock;
        CAMX_LOG_INFO(CamxLogGroupPower, "CSID Clock setting disabled by override %d. Setting CSIDClockHz = %llu",
                      overrideCSIDClockMHz, *pClockRate);
    }
    else
    {
        laneType            = m_pSensorModeData->is3Phase ? IFELaneTypeCPHY: IFELaneTypeDPHY;
        laneCount           = m_pSensorModeData->laneCount;

        CSLCameraTitanChipVersion titanChipVersion =
             static_cast <Titan17xContext *> (GetHwContext())->GetTitanChipVersion();

        switch (titanChipVersion)
        {
            case CSLCameraTitanChipVersion::CSLTitan150V100:
            case CSLCameraTitanChipVersion::CSLTitan150V110:
            case CSLCameraTitanChipVersion::CSLTitan170V100:
            case CSLCameraTitanChipVersion::CSLTitan170V110:
            case CSLCameraTitanChipVersion::CSLTitan170V120:
            case CSLCameraTitanChipVersion::CSLTitan175V100:
            case CSLCameraTitanChipVersion::CSLTitan175V101:
                if (IFELaneTypeCPHY == laneType && 3 == laneCount)
                {
                    *pClockRate         = 1.5 * (m_pSensorModeData->outPixelClock * sensorBitWidth / (laneCount * 16));
                }
                else if (IFELaneTypeCPHY == laneType)
                {
                    *pClockRate         = (m_pSensorModeData->outPixelClock * sensorBitWidth) / (laneCount * 16);
                }
                else if (IFELaneTypeDPHY == laneType)
                {
                    *pClockRate         = (m_pSensorModeData->outPixelClock * sensorBitWidth) / (laneCount * 8);
                }
                else
                {
                    result              = CamxResultEFailed;
                }
                break;

            case CSLCameraTitanChipVersion::CSLTitan170V200:
            case CSLCameraTitanChipVersion::CSLTitan175V120:
            case CSLCameraTitanChipVersion::CSLTitan175V130:
                if (IFELaneTypeCPHY == laneType)
                {
                    *pClockRate         = (m_pSensorModeData->outPixelClock * sensorBitWidth) / (laneCount * 32);
                }
                else if (IFELaneTypeDPHY == laneType)
                {
                    *pClockRate         = (m_pSensorModeData->outPixelClock * sensorBitWidth) / (laneCount * 8);
                }
                else
                {
                    result              = CamxResultEFailed;
                }
                break;

            default:
                result = CamxResultEUnsupported;
                CAMX_LOG_VERBOSE(CamxLogGroupPower, "Dynamic CSID clock rate not yet added for titan chip version %d",
                    titanChipVersion);
                break;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPower, "CSID clock rate %llu", *pClockRate);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupChannelResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupChannelResource(
    UINT                       inputPortId,
    UINT                       totalOutputPorts,
    const UINT*                pPortId,
    UINT                       portSourceTypeId,
    UINT                       streamIndex,
    BOOL*                      pInputMapped,
    IFEInputResourceInfo*      pInputResource)
{
    CamxResult             result          = CamxResultSuccess;
    IFEOutputResourceInfo* pOutputResource = NULL;
    BOOL                   inputMapped     = FALSE;

    /// @todo (CAMX-1315)   Add logic for dual IFE and fall back on single IFE if dual fails.
    CAMX_ASSERT(NULL != m_pSensorModeData);

    if (NULL == pInputResource)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to allocate ResourceInfo.");
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        pInputResource->numberOutResource = totalOutputPorts;

        if (IFEInputPortCustomHW == inputPortId)
        {
            pInputResource->customNode = 1;
        }

        // Setup input resource
        /// @todo (CAMX-1295) - Need to figure out the proper way to deal with TPG/SensorEmulation + RealDevice/Presil
        if ((CSLPresilEnabled == GetCSLMode())     ||
            (CSLPresilRUMIEnabled == GetCSLMode()) ||
            (TRUE == IsTPGMode()))
        {
            /// @todo (CAMX-1189) Move all the hardcode value to settings
            pInputResource->resourceType    = IFEInputTestGen;
            pInputResource->VC[0]           = 0x0;
            pInputResource->DT[0]           = 0x2B;
            pInputResource->numValidVCDT    = 1;
            pInputResource->laneNum         = 4;
            pInputResource->laneType        = IFELaneTypeDPHY;
            pInputResource->laneConfig      = 0x3210;

            m_CSIDecodeBitWidth             = TranslateCSIDataTypeToCSIDecodeFormat(static_cast<UINT8>(pInputResource->DT[0]));
            pInputResource->format          = TranslateFormatToISPImageFormat(CamX::Format::RawMIPI, m_CSIDecodeBitWidth);
            pInputResource->testPattern     = ISPPatternBayerRGRGRG;
            pInputResource->usageType       = ISPResourceUsageSingle;

            pInputResource->height          = m_pSensorModeData->resolution.outputHeight;
            pInputResource->lineStart       = 0;
            pInputResource->lineStop        = m_pSensorModeData->resolution.outputHeight - 1;
            pInputResource->batchSize       = 0;
            pInputResource->DSPMode         = m_HVXInputData.DSPMode;
            pInputResource->HBICount        = 64;
            inputMapped                     = TRUE;
        }
        else if (IFEProfileIdOffline == m_instanceProperty.profileId)
        {
            pInputResource->resourceType    = IFEInputBusRead;
            pInputResource->offlineMode     = TRUE;
            m_CSIDecodeBitWidth             = TranslateCSIDataTypeToCSIDecodeFormat(static_cast<UINT8>(pInputResource->DT[0]));
            pInputResource->format          = TranslateFormatToISPImageFormat(CamX::Format::RawMIPI, m_CSIDecodeBitWidth);
            inputMapped                     = TRUE;
        }
        else
        {
            // PHY/Lanes should not be configured for BusRead/Fetch path
            if (IFEInputBusRead != pInputResource->resourceType)
            {
                /// @todo (CAMX-795) Following field needs to from sensor usecase data
                switch (m_pSensorModeData->CSIPHYId)
                {
                    case 0:
                        pInputResource->resourceType = IFEInputPHY0;
                        break;
                    case 1:
                        pInputResource->resourceType = IFEInputPHY1;
                        break;
                    case 2:
                        pInputResource->resourceType = IFEInputPHY2;
                        break;
                    case 3:
                        pInputResource->resourceType = IFEInputPHY3;
                        break;
                    case 4:
                        pInputResource->resourceType = IFEInputPHY4;
                        break;
                    case 5:
                        pInputResource->resourceType = IFEInputPHY5;
                        break;
                    default:
                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "CSIPHY channel out of range value %d default to TestGen",
                                         m_pSensorModeData->CSIPHYId);
                        pInputResource->resourceType = IFEInputTestGen;
                        break;
                }


                pInputResource->laneType     = m_pSensorModeData->is3Phase ? IFELaneTypeCPHY : IFELaneTypeDPHY;
                pInputResource->laneNum      = m_pSensorModeData->laneCount;
                pInputResource->laneConfig   = m_pSensorCaps->CSILaneAssign;
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "is3Phase %d, laneType %d laneNum 0x%x laneAssign 0x%x",
                                    m_pSensorModeData->is3Phase,
                                    pInputResource->laneType,
                                    pInputResource->laneNum,
                                    m_pSensorCaps->CSILaneAssign);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "FS: ResourceType configured to BusRead");
            }


            // Get VC and DT information from IMAGE stream configuration
            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                             "Current sensor mode resolution output: width = %u, height = %u, streamConfigCount = %u",
                             m_pSensorModeData->resolution.outputWidth,
                             m_pSensorModeData->resolution.outputHeight,
                             m_pSensorModeData->streamConfigCount);
            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                             "streamConfig TYPE = %s, vc = 0x%02x, dt = 0x%02x, frameDim = (%d,%d,%d,%d)",
                             m_pSensorModeData->streamConfig[streamIndex].type == StreamType::IMAGE ? "IMAGE" :
                             m_pSensorModeData->streamConfig[streamIndex].type == StreamType::PDAF ? "PDAF" :
                             m_pSensorModeData->streamConfig[streamIndex].type == StreamType::HDR ? "HDR" :
                             m_pSensorModeData->streamConfig[streamIndex].type == StreamType::META ? "META" :
                             "OTHER",
                             m_pSensorModeData->streamConfig[streamIndex].vc,
                             m_pSensorModeData->streamConfig[streamIndex].dt,
                             m_pSensorModeData->streamConfig[streamIndex].frameDimension.xStart,
                             m_pSensorModeData->streamConfig[streamIndex].frameDimension.yStart,
                             m_pSensorModeData->streamConfig[streamIndex].frameDimension.width,
                             m_pSensorModeData->streamConfig[streamIndex].frameDimension.height);

            if ((StreamType::IMAGE == m_pSensorModeData->streamConfig[streamIndex].type) &&
                ((PortSrcTypePixel == portSourceTypeId) || (PortSrcTypeUndefined == portSourceTypeId)))
            {
                pInputResource->VC[0] = m_pSensorModeData->streamConfig[streamIndex].vc;
                pInputResource->DT[0] = m_pSensorModeData->streamConfig[streamIndex].dt;
                pInputResource->numValidVCDT = 1;

                m_CSIDecodeBitWidth    = TranslateCSIDataTypeToCSIDecodeFormat(static_cast<UINT8>(pInputResource->DT[0]));
                pInputResource->format = TranslateFormatToISPImageFormat(CamX::Format::RawMIPI, m_CSIDecodeBitWidth);
                inputMapped            = TRUE;

                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                 "StreamType::IMAGE setting VC / DT, vc = 0x%02x, dt = 0x%02x",
                                 pInputResource->VC,
                                 pInputResource->DT);
            }
            else if ((StreamType::PDAF   == m_pSensorModeData->streamConfig[streamIndex].type) &&
                    (PortSrcTypePDAF    == portSourceTypeId))
            {
                pInputResource->VC[0] = m_pSensorModeData->streamConfig[streamIndex].vc;
                pInputResource->DT[0] = m_pSensorModeData->streamConfig[streamIndex].dt;
                pInputResource->numValidVCDT = 1;

                const UINT bitWidth     = m_pSensorModeData->streamConfig[streamIndex].bitWidth;
                m_PDAFCSIDecodeFormat   = TranslateBitDepthToCSIDecodeFormat(bitWidth);
                pInputResource->format  = TranslateFormatToISPImageFormat(CamX::Format::RawMIPI, m_PDAFCSIDecodeFormat);
                inputMapped             = TRUE;

                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                 "StreamType::PDAF setting VC / DT, vc = 0x%02x, dt = 0x%02x, bit depth= %u",
                                 pInputResource->VC,
                                 pInputResource->DT,
                                 bitWidth);
            }
            else if ((StreamType::META == m_pSensorModeData->streamConfig[streamIndex].type) &&
                    (PortSrcTypeMeta == portSourceTypeId))
            {
                pInputResource->VC[0] = m_pSensorModeData->streamConfig[streamIndex].vc;
                pInputResource->DT[0] = m_pSensorModeData->streamConfig[streamIndex].dt;
                pInputResource->numValidVCDT = 1;

                const UINT bitWidth     = m_pSensorModeData->streamConfig[streamIndex].bitWidth;
                m_metaCSIDecodeFormat   = TranslateBitDepthToCSIDecodeFormat(bitWidth);
                pInputResource->format  = TranslateFormatToISPImageFormat(CamX::Format::RawMIPI, m_metaCSIDecodeFormat);
                inputMapped            = TRUE;

                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                 "StreamType::META setting VC / DT, vc = 0x%02x, dt = 0x%02x, bit depth= %u",
                                 pInputResource->VC,
                                 pInputResource->DT,
                                 bitWidth);
            }
            else if ((StreamType::HDR == m_pSensorModeData->streamConfig[streamIndex].type) &&
                    (PortSrcTypeHDR == portSourceTypeId))
            {
                pInputResource->VC[0] = m_pSensorModeData->streamConfig[streamIndex].vc;
                pInputResource->DT[0] = m_pSensorModeData->streamConfig[streamIndex].dt;
                pInputResource->numValidVCDT = 1;

                const UINT bitWidth     = m_pSensorModeData->streamConfig[streamIndex].bitWidth;
                m_HDRCSIDecodeFormat    = TranslateBitDepthToCSIDecodeFormat(bitWidth);
                pInputResource->format  = TranslateFormatToISPImageFormat(CamX::Format::RawMIPI, m_HDRCSIDecodeFormat);
                inputMapped             = TRUE;

                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                 "StreamType::HDR setting VC / DT, vc = 0x%02x, dt = 0x%02x, bit depth= %u",
                                 pInputResource->VC,
                                 pInputResource->DT,
                                 bitWidth);
            }

            if ((PortSrcTypePixel == portSourceTypeId) || (PortSrcTypeUndefined == portSourceTypeId))
            {
                m_CSIDecodeBitWidth    = TranslateCSIDataTypeToCSIDecodeFormat(static_cast<UINT8>(pInputResource->DT[0]));
                pInputResource->format = TranslateFormatToISPImageFormat(CamX::Format::RawMIPI, m_CSIDecodeBitWidth);
            }

            pInputResource->batchSize    = 0;
            pInputResource->DSPMode      = m_HVXInputData.DSPMode;
            pInputResource->HBICount     = 0;
            pInputResource->testPattern  = TranslateColorFilterPatternToISPPattern(m_pSensorCaps->colorFilterArrangement);

            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                             "InputResource: VC = %x, DT = %x, format = %d, testPattern = %d",
                             pInputResource->VC,
                             pInputResource->DT,
                             pInputResource->format,
                             pInputResource->testPattern);
        }

        if (NULL != pInputMapped)
        {
            *pInputMapped = inputMapped;
        }
        else
        {
            result = CamxResultEInvalidArg;
        }

        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            UINT32 sensorCropOutWidth = m_pSensorModeData->cropInfo.lastPixel -
                m_pSensorModeData->cropInfo.firstPixel + 1;
            UINT32 sensorFirstPixel   = m_pSensorModeData->cropInfo.firstPixel;

            pInputResource->usageType    = ISPResourceUsageDual;
            pInputResource->leftWidth    =
                m_dualIFESplitParams.splitPoint + m_dualIFESplitParams.rightPadding;

            pInputResource->leftStart    = sensorFirstPixel;
            if (1 <= (sensorFirstPixel + pInputResource->leftWidth))
            {
                pInputResource->leftStop = sensorFirstPixel + pInputResource->leftWidth - 1;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid value sensorFirstPixel %d leftWidth %d",
                    sensorFirstPixel, pInputResource->leftWidth);

                result = CamxResultEFailed;
            }
            pInputResource->rightWidth   =
                sensorCropOutWidth - m_dualIFESplitParams.splitPoint + m_dualIFESplitParams.leftPadding;
            pInputResource->rightStart   =
                sensorFirstPixel + m_dualIFESplitParams.splitPoint - m_dualIFESplitParams.leftPadding;
            pInputResource->rightStop    = sensorFirstPixel + sensorCropOutWidth - 1;
            pInputResource->height       = m_pSensorModeData->cropInfo.lastLine -
                m_pSensorModeData->cropInfo.firstLine + 1;
            pInputResource->lineStart    = m_pSensorModeData->cropInfo.firstLine;
            pInputResource->lineStop     = m_pSensorModeData->cropInfo.lastLine;

            CAMX_LOG_INFO(CamxLogGroupISP, "Left Start %d End %d Left Width %d",
                pInputResource->leftStart, pInputResource->leftStop, pInputResource->leftWidth);
            CAMX_LOG_INFO(CamxLogGroupISP, "Right Start %d End %d Left Width %d",
                pInputResource->rightStart, pInputResource->rightStop, pInputResource->rightWidth);
            CAMX_LOG_INFO(CamxLogGroupISP, "Start %d End %d Height %d",
                pInputResource->lineStart, pInputResource->lineStop, pInputResource->height);
        }
        else
        {
            // Single IFE configuration case
            pInputResource->usageType   = ISPResourceUsageSingle;
            pInputResource->leftStart   = m_pSensorModeData->cropInfo.firstPixel;
            pInputResource->leftStop    = m_pSensorModeData->cropInfo.lastPixel;
            pInputResource->lineStart   = m_pSensorModeData->cropInfo.firstLine;
            pInputResource->lineStop    = m_pSensorModeData->cropInfo.lastLine;


            pInputResource->leftWidth   = m_pSensorModeData->cropInfo.lastPixel -
                                            m_pSensorModeData->cropInfo.firstPixel + 1;

            pInputResource->height = m_pSensorModeData->cropInfo.lastLine -
                m_pSensorModeData->cropInfo.firstLine + 1;

            // Set CSID Binning
            if (TRUE == m_csidBinningInfo.isBinningEnabled)
            {
                if (CSIDBinningMode::HorizontalBinning == m_csidBinningInfo.binningMode)
                {
                    pInputResource->horizontalBinning = 1;
                }
                else if (CSIDBinningMode::QCFABinning == m_csidBinningInfo.binningMode)
                {
                    pInputResource->QCFABinning = 1;
                }
                CAMX_LOG_CONFIG(CamxLogGroupISP, "CSID: Enable horizontalBinning:%d, QCFABinning:%d",
                                pInputResource->horizontalBinning,
                                pInputResource->QCFABinning);
            }

            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                             "CSID crop leftStart = %u, leftStop = %u, lineStart = %u, lineStop = %u WxH = %dx%d ",
                             pInputResource->leftStart,
                             pInputResource->leftStop,
                             pInputResource->lineStart,
                             pInputResource->lineStop,
                             pInputResource->leftWidth,
                             pInputResource->height);

            // Override CSID crop setting if custom setting values are provided
            if (TRUE == EnableCSIDCropOverridingForSingleIFE())
            {
                // Override horizontal CSID crop setting
                pInputResource->leftStart    = m_instanceProperty.IFECSIDLeft;
                pInputResource->leftStop     = m_instanceProperty.IFECSIDLeft + m_instanceProperty.IFECSIDWidth - 1;
                pInputResource->leftWidth    = m_instanceProperty.IFECSIDWidth;

                // Override vertical CSID crop setting
                pInputResource->height       = m_instanceProperty.IFECSIDHeight;
                pInputResource->lineStart    = m_instanceProperty.IFECSIDTop;
                pInputResource->lineStop     = m_instanceProperty.IFECSIDTop + m_instanceProperty.IFECSIDHeight - 1;

                CAMX_LOG_INFO(CamxLogGroupISP,
                              "CSID crop override: leftStart = %u, leftStop = %u, lineStart = %u, lineStop = %u",
                              pInputResource->leftStart,
                              pInputResource->leftStop,
                              pInputResource->lineStart,
                              pInputResource->lineStop);
            }
        }
    }

    // Setup output resource
    if ((CamxResultSuccess == result) && (TRUE == inputMapped))
    {
        UINT32  ISPFormat;

        if (IFEProfileIdOffline == m_instanceProperty.profileId)
        {
            m_IFECoreConfig.inputMuxSelPP = 1;
        }

        for (UINT index = 0; index < totalOutputPorts; index++)
        {
            pOutputResource = &m_IFEOutputResource[index];

            /// @todo (CAMX-1015) Optimize by not calling this function(because it is searching and can be avoided
            UINT currentOutputPortIndex = OutputPortIndex(pPortId[index]);

            // Set width and height from the output port image format
            const ImageFormat* pCurrentOutputPortImageFormat = GetOutputPortImageFormat(currentOutputPortIndex);
            if (NULL != pCurrentOutputPortImageFormat)
            {
                pOutputResource->width  = pCurrentOutputPortImageFormat->width;
                pOutputResource->height = pCurrentOutputPortImageFormat->height;

                if ((CSLCameraTitanVersion::CSLTitan150 == m_titanVersion) &&
                    (TRUE == ImageFormatUtils::IsUBWC(static_cast<Format>(pCurrentOutputPortImageFormat->format))))
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Node::%s : portID = %u, Image format %d "
                       "not supported for titan version %x",
                        NodeIdentifierString(), pPortId[index],
                        pCurrentOutputPortImageFormat->format,
                        m_titanVersion);
                    result = CamxResultEUnsupported;
                    break;
                }
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Node::%s : portID = %u, Image format %d titan version %x",
                    NodeIdentifierString(), pPortId[index],
                    pCurrentOutputPortImageFormat->format,
                    m_titanVersion);

                if (IFEModuleMode::DualIFENormal == m_mode)
                {
                    CAMX_ASSERT(0 != m_pSensorModeData->resolution.outputWidth);
                    FLOAT temp = (static_cast<FLOAT>(pOutputResource->width) / m_pSensorModeData->resolution.outputWidth) *
                        m_dualIFESplitParams.splitPoint;
                    pOutputResource->splitPoint = CamX::Utils::RoundFLOAT(temp);
                }

                /// @todo (CAMX-1820) Need to move IFE path enable logic to executeProcessCaptureRequest to support
                /// enable/Disable IFE path per frame, based on conclusion of the above jira.

                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                    "Configure output portId = %u for port source type = %u",
                    pPortId[index],
                    portSourceTypeId);

                pOutputResource->secureMode = IsOutputPortSecure(pPortId[index]);

                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Output resource type %d, Dimension [%d * %d]",
                    pPortId[index],
                    pOutputResource->width,
                    pOutputResource->height);

                const ImageFormat*  pImageFormat = NULL;

                // Update width and height only for Full/FD/DS4/DS16 output ports.
                switch (pPortId[index])
                {
                    case IFEOutputPortFD:
                        pOutputResource->resourceType = IFEOutputFD;
                        pImageFormat = GetOutputPortImageFormat(OutputPortIndex(IFEOutputPortFD));
                        if (NULL != pImageFormat)
                        {
                            ISPFormat = TranslateFormatToISPImageFormat(pImageFormat->format, m_CSIDecodeBitWidth);
                            pOutputResource->format = ISPFormat;
                        }
                        break;

                    case IFEOutputPortFull:
                        pOutputResource->resourceType = IFEOutputFull;
                        pImageFormat = GetOutputPortImageFormat(OutputPortIndex(IFEOutputPortFull));
                        if (NULL != pImageFormat)
                        {
                            ISPFormat = TranslateFormatToISPImageFormat(pImageFormat->format, m_CSIDecodeBitWidth);
                            pOutputResource->format = ISPFormat;
                        }
                        break;

                    case IFEOutputPortDS4:
                        pOutputResource->resourceType      = IFEOutputDS4;
                        pOutputResource->format            = ISPFormatPD10;
                        m_IFECoreConfig.videoDS4R2PDEnable = 1;
                        break;

                    case IFEOutputPortDS16:
                        pOutputResource->resourceType       = IFEOutputDS16;
                        pOutputResource->format             = ISPFormatPD10;
                        m_IFECoreConfig.videoDS16R2PDEnable = 1;
                        break;

                    case IFEOutputPortDisplayFull:
                        pOutputResource->resourceType = IFEOutputDisplayFull;
                        pImageFormat = GetOutputPortImageFormat(OutputPortIndex(IFEOutputPortDisplayFull));
                        if (NULL != pImageFormat)
                        {
                            ISPFormat = TranslateFormatToISPImageFormat(pImageFormat->format, m_CSIDecodeBitWidth);
                            pOutputResource->format = ISPFormat;
                        }
                        break;

                    case IFEOutputPortDisplayDS4:
                        pOutputResource->resourceType        = IFEOutputDisplayDS4;
                        pOutputResource->format              = ISPFormatPD10;
                        m_IFECoreConfig.displayDS4R2PDEnable = 1;
                        break;

                    case IFEOutputPortDisplayDS16:
                        pOutputResource->resourceType         = IFEOutputDisplayDS16;
                        pOutputResource->format               = ISPFormatPD10;
                        m_IFECoreConfig.displayDS16R2PDEnable = 1;
                        break;

                    case IFEOutputPortCAMIFRaw:
                        pOutputResource->resourceType = IFEOutputRaw;
                        pOutputResource->format       = ISPFormatPlain1614;
                        m_IFEPixelRawPort             = IFEOutputPortCAMIFRaw;
                        break;

                    case IFEOutputPortLSCRaw:
                        pOutputResource->resourceType = IFEOutputRaw;
                        pOutputResource->format       = ISPFormatPlain1614;
                        m_IFEPixelRawPort             = IFEOutputPortLSCRaw;
                        break;

                    case IFEOutputPortGTMRaw:
                        pOutputResource->resourceType = IFEOutputRaw;
                        pOutputResource->format       = ISPFormatRaw14Private;
                        m_IFEPixelRawPort            = IFEOutputPortGTMRaw;
                        break;

                    case IFEOutputPortStatsHDRBE:
                        pOutputResource->resourceType = IFEOutputStatsHDRBE;
                        pOutputResource->format       = ISPFormatPlain64;
                        break;

                    case IFEOutputPortStatsHDRBHIST:
                        pOutputResource->resourceType = IFEOutputStatsHDRBHIST;
                        pOutputResource->format       = ISPFormatPlain64;
                        break;

                    case IFEOutputPortStatsBHIST:
                        pOutputResource->resourceType = IFEOutputStatsBHIST;
                        pOutputResource->format       = ISPFormatPlain64;
                        break;

                    case IFEOutputPortStatsAWBBG:
                        pOutputResource->resourceType = IFEOutputStatsAWBBG;
                        pOutputResource->format       = ISPFormatPlain64;
                        break;

                    case IFEOutputPortStatsBF:
                        pOutputResource->resourceType = IFEOutputStatsBF;
                        pOutputResource->format       = ISPFormatPlain64;
                        break;

                    case IFEOutputPortStatsTLBG:
                        pOutputResource->resourceType = IFEOutputStatsTLBG;
                        pOutputResource->format       = ISPFormatPlain64;
                        break;

                    case IFEOutputPortStatsDualPD:
                        pOutputResource->resourceType = IFEOutputDualPD;
                        result = SetRDIOutputPortFormat(pOutputResource,
                            pCurrentOutputPortImageFormat->format,
                            pPortId[index],
                            portSourceTypeId);
                        if (0 != GetStaticSettings()->enablePDLibTestMode)
                        {
                            CAMX_LOG_WARN(CamxLogGroupISP, "Enabling PD LIB test Mode"
                                                           "PD HW Data format will be set to PLAIN16");
                            pOutputResource->format = ISPFormatPlain1616;
                        }
                        break;

                    case IFEOutputPortLCR:
                        pOutputResource->resourceType = IFEOutputLCR;
                        pOutputResource->format       = ISPFormatPlain1616;
                        break;

                    case IFEOutputPortRDI0:
                        pOutputResource->resourceType = IFEOutputRDI0;
                        result = SetRDIOutputPortFormat(pOutputResource,
                            pCurrentOutputPortImageFormat->format,
                            pPortId[index],
                            portSourceTypeId);
                        break;

                    case IFEOutputPortRDI1:
                        pOutputResource->resourceType = IFEOutputRDI1;
                        result = SetRDIOutputPortFormat(pOutputResource,
                            pCurrentOutputPortImageFormat->format,
                            pPortId[index],
                            portSourceTypeId);
                        break;

                    case IFEOutputPortRDI2:
                        pOutputResource->resourceType = IFEOutputRDI2;
                        result = SetRDIOutputPortFormat(pOutputResource,
                            pCurrentOutputPortImageFormat->format,
                            pPortId[index],
                            portSourceTypeId);
                        break;

                    case IFEOutputPortRDI3:
                        pOutputResource->resourceType = IFEOutputRDI3;
                        result = SetRDIOutputPortFormat(pOutputResource,
                            pCurrentOutputPortImageFormat->format,
                            pPortId[index],
                            portSourceTypeId);
                        break;

                    case IFEOutputPortStatsIHIST:
                        pOutputResource->resourceType = IFEOutputStatsIHIST;
                        pOutputResource->format       = ISPFormatPlain1616;
                        break;

                    case IFEOutputPortStatsRS:
                        pOutputResource->resourceType = IFEOutputStatsRS;
                        pOutputResource->format       = ISPFormatPlain1616;
                        break;

                    case IFEOutputPortStatsCS:
                        pOutputResource->resourceType = IFEOutputStatsCS;
                        pOutputResource->format       = ISPFormatPlain64;
                        break;

                    case IFEOutputPortPDAF:
                        pOutputResource->resourceType = IFEOutputPDAF;
                        pOutputResource->format       = ISPFormatPlain1610;
                        break;

                    default:
                        m_IFEOutputPathInfo[index].path = FALSE;
                        result = CamxResultEInvalidArg;
                        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Port ID. %d", pPortId[index]);
                        break;
                }
            }

            // Enable IFE output buffer compositing
            Titan17xContext* pContext = static_cast<Titan17xContext *>(GetHwContext());
            BOOL enableGrouping = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableIFEOutputGrouping;

            if ((TRUE == enableGrouping) || (IFEModuleMode::DualIFENormal == m_mode))
            {
                // Group bus clients for composite handling
                switch (pPortId[index])
                {
                    case IFEOutputPortFD:
                        pOutputResource->compositeGroupId   = ISPOutputGroupId4;
                        break;

                    case IFEOutputPortFull:
                    case IFEOutputPortDS4:
                    case IFEOutputPortDS16:
                        pOutputResource->compositeGroupId   = ISPOutputGroupId0;
                        break;

                    case IFEOutputPortStatsHDRBHIST:
                    case IFEOutputPortStatsAWBBG:
                    case IFEOutputPortStatsHDRBE:
                    case IFEOutputPortStatsIHIST:
                    case IFEOutputPortStatsCS:
                    case IFEOutputPortStatsRS:
                    case IFEOutputPortStatsTLBG:
                    case IFEOutputPortStatsBHIST:
                        pOutputResource->compositeGroupId   = ISPOutputGroupId1;
                        break;

                    case IFEOutputPortStatsBF:
                        pOutputResource->compositeGroupId   = ISPOutputGroupId2;
                        break;

                    case IFEOutputPortCAMIFRaw:
                    case IFEOutputPortLSCRaw:
                    case IFEOutputPortGTMRaw:
                        pOutputResource->compositeGroupId   = ISPOutputGroupId5;
                        break;

                    case IFEOutputPortPDAF:
                        pOutputResource->compositeGroupId   = ISPOutputGroupId6;
                        break;

                    case IFEOutputPortDisplayFull:
                    case IFEOutputPortDisplayDS4:
                    case IFEOutputPortDisplayDS16:
                        /// @todo (CAMX-4027) Changes for new IFE display o/p. For now same as FullVidPort
                        pOutputResource->compositeGroupId   = ISPOutputGroupId3;
                        break;

                    default:
                        pOutputResource->compositeGroupId   = ISPOutputGroupIdNONE;
                        break;
                }
            }

            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::MapPortIdToChannelId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::MapPortIdToChannelId(
    UINT    portId,
    UINT32* pChannelId)
{
    /// @todo (CAMX-817) Need to discuss with CSI/Kernel team to avoid this run-time port id conversion
    CamxResult result = CamxResultSuccess;

    *pChannelId = 0;

    switch (portId)
    {
        case IFEOutputPortFull:
            *pChannelId = IFEOutputFull;
            break;

        case IFEOutputPortDS4:
            *pChannelId = IFEOutputDS4;
            break;

        case IFEOutputPortDS16:
            *pChannelId = IFEOutputDS16;
            break;

        case IFEOutputPortDisplayFull:
            *pChannelId = IFEOutputDisplayFull;
            break;

        case IFEOutputPortDisplayDS4:
            *pChannelId = IFEOutputDisplayDS4;
            break;

        case IFEOutputPortDisplayDS16:
            *pChannelId = IFEOutputDisplayDS16;
            break;

        case IFEOutputPortFD:
            *pChannelId = IFEOutputFD;
            break;

        case IFEOutputPortCAMIFRaw:
        case IFEOutputPortLSCRaw:
        case IFEOutputPortGTMRaw:
            *pChannelId = IFEOutputRaw;
            break;

        case IFEOutputPortStatsDualPD:
            *pChannelId = IFEOutputDualPD;
            break;

        case IFEOutputPortLCR:
            *pChannelId = IFEOutputLCR;
            break;

        case IFEOutputPortRDI0:
            *pChannelId = IFEOutputRDI0;
            break;

        case IFEOutputPortRDI1:
            *pChannelId = IFEOutputRDI1;
            break;

        case IFEOutputPortRDI2:
            *pChannelId = IFEOutputRDI2;
            break;

        case IFEOutputPortRDI3:
            *pChannelId = IFEOutputRDI3;
            break;

        case IFEOutputPortPDAF:
            *pChannelId = IFEOutputPDAF;
            break;

        case IFEOutputPortStatsRS:
            *pChannelId = IFEOutputStatsRS;
            break;

        case IFEOutputPortStatsCS:
            *pChannelId = IFEOutputStatsCS;
            break;

        case IFEOutputPortStatsIHIST:
            *pChannelId = IFEOutputStatsIHIST;
            break;

        case IFEOutputPortStatsBHIST:
            *pChannelId = IFEOutputStatsBHIST;
            break;

        case IFEOutputPortStatsHDRBE:
            *pChannelId = IFEOutputStatsHDRBE;
            break;

        case IFEOutputPortStatsHDRBHIST:
            *pChannelId = IFEOutputStatsHDRBHIST;
            break;

        case IFEOutputPortStatsTLBG:
            *pChannelId = IFEOutputStatsTLBG;
            break;

        case IFEOutputPortStatsAWBBG:
            *pChannelId = IFEOutputStatsAWBBG;
            break;

        case IFEOutputPortStatsBF:
            *pChannelId = IFEOutputStatsBF;
            break;

        default:
            result = CamxResultEInvalidArg;
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetPDAFInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::GetPDAFInformation()
{
    static const UINT PropertiesPDAF[] =
    {
        PropertyIDSensorPDAFInfo,    // 0
    };

    const SIZE_T numTags                         = CAMX_ARRAY_SIZE(PropertiesPDAF);
    VOID*        pPropertyDataPDAF[numTags]      = { 0 };
    UINT64       propertyDataPDAFOffset[numTags] = { 0 };


    GetDataList(PropertiesPDAF, pPropertyDataPDAF, propertyDataPDAFOffset, numTags);

    if (NULL != pPropertyDataPDAF[0])
    {
        Utils::Memcpy(&m_ISPInputSensorData.sensorPDAFInfo, pPropertyDataPDAF[0], sizeof(SensorPDAFInfo));
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFENode::IsPDHwEnabled()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::IsPDHwEnabled()
{
    BOOL isPDHwEnabled = FALSE;

    static const UINT GetProps[] =
    {
        PropertyIDUsecasePDHWEnableConditions
    };

    static const UINT       GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*                   pData[GetPropsLength]   = { 0 };
    UINT64                  offsets[GetPropsLength] = { 0 };
    PDHWEnableConditions*   pPDHWEnableConditions   = NULL;

    GetDataList(GetProps, pData, offsets, GetPropsLength);

    if (NULL != pData[0])
    {
        pPDHWEnableConditions = reinterpret_cast<PDHWEnableConditions*>(pData[0]);
        isPDHwEnabled =
            (pPDHWEnableConditions->isDualPDEnableConditionsMet || pPDHWEnableConditions->isSparsePDEnableConditionsMet);
    }

    return isPDHwEnabled;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::AcquireDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::AcquireDevice()
{
    /// @todo (CAMX-3118)   Add logic for dual IFE and fall back on single IFE if dual fails.
    CamxResult           result          = CamxResultSuccess;
    CSLDeviceResource    deviceResource;
    CSLDeviceAttribute   deviceAttribute = { 0 };

    if (IFEProfileIdOffline == m_instanceProperty.profileId)
    {
        deviceAttribute.attributeID              = CSLDeviceAttributeNonRealtimeOperation;
        deviceAttribute.pDeviceAttributeParam    = NULL;
        deviceAttribute.deviceAttributeParamSize = 0;
    }
    else
    {
        deviceAttribute.attributeID              = CSLDeviceAttributeRealtimeOperation;
        deviceAttribute.pDeviceAttributeParam    = NULL;
        deviceAttribute.deviceAttributeParamSize = 0;
    }

    if (CamxResultSuccess == result)
    {
        deviceResource.pDeviceResourceParam    = NULL;
        deviceResource.deviceResourceParamSize = 0;
        deviceResource.resourceID              = ISPResourceIdPort;

        result = CSLAcquireDevice(GetCSLSession(),
                                  &m_hDevice,
                                  DeviceIndices()[0],
                                  &deviceResource,
                                  0,
                                  &deviceAttribute,
                                  1,
                                  NodeIdentifierString());
        CAMX_LOG_INFO(CamxLogGroupCore, "Acquiring - IFEDevice : %s, Result: %d", NodeIdentifierString(), result);
        if (CamxResultSuccess == result)
        {
            SetDeviceAcquired(TRUE);
            AddCSLDeviceHandle(m_hDevice);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Acquire IFE Device Failed");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::UpdateIFECapabilityBasedOnCameraPlatform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::UpdateIFECapabilityBasedOnCameraPlatform()
{
    CamxResult result = CamxResultSuccess;
    m_titanVersion = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();

    switch(m_titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan170:
            m_hwMask            = ISPHwTitan170;
            m_maxOutputWidthFD  = IFEMaxOutputWidthFD;
            m_maxOutputHeightFD = IFEMaxOutputHeightFD;
            break;

        case CSLCameraTitanVersion::CSLTitan175:
        case CSLCameraTitanVersion::CSLTitan160:
            // Both CSLTitan175 and CSLTitan160 have the same IFE IQ h/w
            m_hwMask            = ISPHwTitan175;
            m_maxOutputWidthFD  = IFEMaxOutputWidthFD;
            m_maxOutputHeightFD = IFEMaxOutputHeightFD;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
            m_hwMask            = ISPHwTitan150;
            m_maxOutputWidthFD  = IFEMaxOutputWidthFDTalos;
            m_maxOutputHeightFD = IFEMaxOutputHeightFDTalos;
            break;

        case CSLCameraTitanVersion::CSLTitan480:
            m_hwMask            = ISPHwTitan480;
            m_maxOutputWidthFD  = IFEMaxOutputWidthFD;
            m_maxOutputHeightFD = IFEMaxOutputHeightFD;
            break;

        default:
            result = CamxResultEUnsupported;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported Titan Version = 0X%x",
                static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion());
            break;
    }

    m_pIFEPipeline->GetIFEDefaultConfig(&m_defaultConfig);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ConfigureIFECapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ConfigureIFECapability()
{
    CamxResult result = CamxResultSuccess;

    m_titanVersion = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();

    m_maxOutputWidthFD  = IFEMaxOutputWidthFD;
    // m_maxOutputHeightFD = IFEMaxOutputHeightFD;
    switch(m_titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan170:
            m_hwMask        = ISPHwTitan170;
            m_pIFEPipeline  = CAMX_NEW Titan170IFE;
            break;

        case CSLCameraTitanVersion::CSLTitan175:
        // case CSLCameraTitanVersion::CSLTitan160:
            // Both CSLTitan175 and CSLTitan160 have the same IFE IQ h/w
            m_hwMask        = ISPHwTitan175;
            m_pIFEPipeline  = CAMX_NEW Titan175IFE;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
            m_hwMask            = ISPHwTitan150;
            m_maxOutputWidthFD  = IFEMaxOutputWidthFDTalos;
            // m_maxOutputHeightFD = IFEMaxOutputHeightFDTalos;
            m_pIFEPipeline      = CAMX_NEW Titan150IFE;
            break;

        case CSLCameraTitanVersion::CSLTitan480:
            m_hwMask        = ISPHwTitan480;
            m_pIFEPipeline  = CAMX_NEW Titan480IFE;
            break;

        default:
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pIFEPipeline)
    {
        m_pIFEPipeline->SetTitanChipVersion(static_cast<Titan17xContext *>(GetHwContext())->GetTitanChipVersion());
        m_pIFEPipeline->FetchPipelineCapability(&m_capability);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported Titan Version = 0X%x, or Out of Memory",
            static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::HasOutputPortForIQModulePathConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::HasOutputPortForIQModulePathConfig(
    IFEPipelinePath pipelinePath
    ) const
{
    // Check if the matching output port for a given pipeline path exist in topology
    BOOL hasOutputPort = FALSE;

    switch (pipelinePath)
    {
        case IFEPipelinePath::VideoFullPath:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortFull].path;
            break;
        case IFEPipelinePath::FDPath:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortFD].path;
            break;
        case IFEPipelinePath::VideoDS4Path:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortDS4].path;
            break;
        case IFEPipelinePath::VideoDS16Path:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortDS16].path;
            break;
        case IFEPipelinePath::DisplayFullPath:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortDisplayFull].path;
            break;
        case IFEPipelinePath::DisplayDS4Path:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortDisplayDS4].path;
            break;
        case IFEPipelinePath::DisplayDS16Path:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortDisplayDS16].path;
            break;
        // If any of pixel raw output port is enabled, IFECrop module for this path needs to be enabled
        case IFEPipelinePath::PixelRawDumpPath:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortCAMIFRaw].path ||
                            m_IFEOutputPathInfo[IFEOutputPortLSCRaw].path   ||
                            m_IFEOutputPathInfo[IFEOutputPortGTMRaw].path;
            break;
        case IFEPipelinePath::DualPDPath:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortStatsDualPD].path;
            break;
        case IFEPipelinePath::LCRPath:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortLCR].path;
            break;
        case IFEPipelinePath::RDI0Path:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortRDI0].path;
            break;
        case IFEPipelinePath::RDI1Path:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortRDI1].path;
            break;
        case IFEPipelinePath::RDI2Path:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortRDI2].path;
            break;
        case IFEPipelinePath::RDI3Path:
            hasOutputPort = m_IFEOutputPathInfo[IFEOutputPortRDI3].path;
            break;
        case IFEPipelinePath::CommonPath:
            if (FALSE == m_RDIOnlyUseCase)
            {
                hasOutputPort = TRUE;
            }
            break;

        default:
            break;
    }

    return hasOutputPort;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CreateIFEIQModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::CreateIFEIQModules()
{
    CamxResult          result          = CamxResultSuccess;
    IFEIQModuleInfo*    pIQModule       = m_capability.pIFEIQModuleList;
    UINT                count           = 0;
    IFEModuleCreateData moduleInputData = {};
    UINT                totalOutputPort = 0;
    UINT                outputPortId[MaxDefinedIFEOutputPorts];
    UINT                cameraId        = GetPipeline()->GetCameraId();
    UINT32              sensorARMode    = GetSensorAspectRatioMode();

    // Get Output Port List
    GetAllOutputPortIds(&totalOutputPort, &outputPortId[0]);

    for (UINT32 index = 0; index < totalOutputPort; index++)
    {
        // Keep the record of which IFE output ports are actually connected/used in this pipeline/topology
        UINT IFEPathIndex = outputPortId[index];
        m_IFEOutputPathInfo[IFEPathIndex].path = TRUE;
    }

    m_numIFEIQModule = 0;
    CamX::Utils::Memset(m_stripeConfigs, 0, sizeof(m_stripeConfigs));
    moduleInputData.initializationData.pStripeConfig                     = &m_stripeConfigs[0];
    moduleInputData.titanVersion                                         = m_titanVersion;
    moduleInputData.initializationData.sensorID                          = cameraId;
    moduleInputData.initializationData.sensorData.sensorAspectRatioMode  = sensorARMode;

    for (count = 0; count < m_capability.numIFEIQModule; count++)
    {
        // Only create IQ modules if the module is installed and the output ports of the current path exists
        if (TRUE == IsIQModuleInstalled(&pIQModule[count]))
        {
            if (TRUE == HasOutputPortForIQModulePathConfig(pIQModule[count].IFEPath))
            {
                moduleInputData.pipelineData.IFEModuleType  = pIQModule[count].moduleType;
                moduleInputData.pipelineData.IFEPath        = pIQModule[count].IFEPath;

                if ((pIQModule[count].moduleType == ISPIQModuleType::IFEDUALPD)    ||
                    (pIQModule[count].moduleType == ISPIQModuleType::IFECAMIFLite) ||
                    (pIQModule[count].moduleType == ISPIQModuleType::IFECAMIFDualPD))
                {
                    const StaticSettings* pSettings = GetStaticSettings();
                    if (NULL != pSettings)
                    {
                        // To be update by IFE
                        if ((ISPHwTitan480 == m_hwMask) &&
                            (TRUE           == pSettings->pdafHWEnable) &&
                            (TRUE           == pSettings->disablePDAF))
                        {
                            CAMX_LOG_INFO(CamxLogGroupStats, " Skipping dualpd hw since it is disabled ");
                            continue;
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupStats, "Invalid settings");
                    }
                }

                if (pIQModule[count].moduleType == ISPIQModuleType::IFEHVX)
                {
                    if (NULL != m_pIFEHVXModule)
                    {
                        m_pIFEIQModule[m_numIFEIQModule] = m_pIFEHVXModule;
                        m_numIFEIQModule++;
                    }
                    continue;
                }

                result = pIQModule[count].IQCreate(&moduleInputData);

                if (CamxResultSuccess == result)
                {
                    m_pIFEIQModule[m_numIFEIQModule] = moduleInputData.pModule;

                    m_numIFEIQModule++;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, " Failed to Create IQ Module, count = %d moduleType %d ",
                        count, pIQModule[count].moduleType);
                    break;
                }
            }
            else
            {
                // Filter out for the IQ modules that do not have the IFE output ports defined in topology.
                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                 "IFEIQModuleItems[%u] installed, but skipping since the outport doesn't exist",
                                 count);
            }
        }
    }

    CamX::Utils::Memcpy(&m_stripeConfigs[1], &m_stripeConfigs[0], sizeof(m_stripeConfigs[0]));

    m_stripeConfigs[0].cropType = CropType::CropTypeFromLeft;
    m_stripeConfigs[1].cropType = CropType::CropTypeFromRight;

    m_stripeConfigs[0].stripeId = 0;
    m_stripeConfigs[1].stripeId = 1;

    m_stripeConfigs[0].pFrameLevelData = &m_ISPFrameData;
    m_stripeConfigs[1].pFrameLevelData = &m_ISPFrameData;

    // The clean-up for the error case happens outside this function

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CreateIFEStatsModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::CreateIFEStatsModules()
{
    CamxResult          result       = CamxResultSuccess;
    IFEStatsModuleInfo* pStatsModule = m_capability.pIFEStatsModuleList;
    UINT                count        = 0;
    UINT                i            = 0;
    UINT                outputPortId[MaxDefinedIFEOutputPorts];
    UINT                totalOutputPort = 0;
    IFEStatsModuleCreateData moduleInputData;

    m_numIFEStatsModule          = 0;
    moduleInputData.pipelineData = m_pipelineData;
    moduleInputData.titanVersion = m_titanVersion;

    GetAllOutputPortIds(&totalOutputPort, &outputPortId[0]);

    for (i = 0; i < totalOutputPort; i++)
    {
        if (outputPortId[i] >= IFEOutputPortStatsRS &&
            outputPortId[i] <= IFEOutputPortStatsAWBBG)
        {
            for (count = 0; count < m_capability.numIFEStatsModule; count++)
            {
                if ((outputPortId[i] == IFEStatsModuleOutputPorts[count]) &&
                    (TRUE == pStatsModule[count].installed ))
                {
                    result = pStatsModule[count].StatsCreate(&moduleInputData);

                    if (CamxResultSuccess == result)
                    {
                        m_pIFEStatsModule[m_numIFEStatsModule] = moduleInputData.pModule;
                        m_numIFEStatsModule++;
                    }
                    else
                    {
                        CAMX_ASSERT_ALWAYS_MESSAGE("%s: Failed to create Stats Module.  count %d", __FUNCTION__, count);

                        break;
                    }
                    break;
                }
            }
        }
    }

    // The clean-up for the error case happens outside this function

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CreateIFEHVXModules()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::CreateIFEHVXModules()
{
    IFEIQModuleInfo moduleInfo;
    IFEModuleCreateData moduleInputData = {0};

    Utils::Memset(&moduleInfo, 0, sizeof(moduleInfo));

    m_pIFEPipeline->GetISPIQModulesOfType(ISPIQModuleType::IFEHVX, &moduleInfo);

    if (TRUE == IsIQModuleInstalled(&moduleInfo))
    {
        moduleInputData.initializationData.pStripeConfig = &m_stripeConfigs[0];
        moduleInputData.pipelineData.IFEPath             = moduleInfo.IFEPath;
        moduleInputData.pHvxInitializeData               = &m_HVXInputData;
        moduleInputData.titanVersion                     = m_titanVersion;

        moduleInfo.IQCreate(&moduleInputData);

        m_pIFEHVXModule = moduleInputData.pModule;
    }

    return CamxResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetIFESWTMCModuleInstanceVersion()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SWTMCVersion IFENode::GetIFESWTMCModuleInstanceVersion()
{
    // If SWTMC IQ module is not installed, version is 1.0
    SWTMCVersion version = SWTMCVersion::TMC10;

    for (UINT i = 0; i < m_numIFEIQModule; i++)
    {
        if (ISPIQModuleType::SWTMC == m_pIFEIQModule[i]->GetIQType())
        {
            version = static_cast<SWTMCVersion>(m_pIFEIQModule[i]->GetVersion());
            break;
        }
    }

    return version;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::Cleanup()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::Cleanup()
{
    UINT        count  = 0;
    CamxResult  result = CamxResultSuccess;

    // De-allocate all of the IQ modules

    for (count = 0; count < m_numIFEIQModule; count++)
    {
        if (NULL != m_pIFEIQModule[count])
        {
            m_pIFEIQModule[count]->Destroy();
            m_pIFEIQModule[count] = NULL;
        }
    }

    m_numIFEIQModule = 0;

    for (count = 0; count < m_numIFEStatsModule; count++)
    {
        if (NULL != m_pIFEStatsModule[count])
        {
            m_pIFEStatsModule[count]->Destroy();
            m_pIFEStatsModule[count] = NULL;
        }
    }

    m_numIFEStatsModule = 0;

    if (NULL != m_pStripingInput)
    {
        CAMX_FREE(m_pStripingInput);
        m_pStripingInput = NULL;
    }

    if (NULL != m_pPassOut)
    {
        CAMX_FREE(m_pPassOut);
        m_pPassOut = NULL;
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

    if (NULL != m_pIFEPipeline)
    {
        CAMX_DELETE m_pIFEPipeline;
        m_pIFEPipeline = NULL;
    }

    PFStripeInterface  pStripeInterfaceAlgoEntry =
        reinterpret_cast<PFStripeInterface>(OsUtils::LibGetAddr(m_hHandle, "DestroyIFEStripeInterface"));
    if (NULL != pStripeInterfaceAlgoEntry)
    {
        pStripeInterfaceAlgoEntry(&m_pIFEStripeInterface);
    }

    if (NULL != m_hHandle)
    {
        CamX::OsUtils::LibUnmap(m_hHandle);
        m_hHandle = NULL;
    }

    DeallocateICAGridData();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PopulateGeneralTuningMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PopulateGeneralTuningMetadata(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    // Populate Sensor mode data
    pInputData->pIFETuningMetadata->sensorData.format           = m_pSensorModeData->format;
    pInputData->pIFETuningMetadata->sensorData.cropInfo         = m_pSensorModeData->cropInfo;
    pInputData->pIFETuningMetadata->sensorData.maxLineCount     = m_pSensorModeData->maxLineCount;
    pInputData->pIFETuningMetadata->sensorData.numLinesPerFrame = m_pSensorModeData->numLinesPerFrame;
    pInputData->pIFETuningMetadata->sensorData.numPixelsPerLine = m_pSensorModeData->numPixelsPerLine;
    pInputData->pIFETuningMetadata->sensorData.resolution       = m_pSensorModeData->resolution;
    pInputData->pIFETuningMetadata->sensorData.binningTypeH     = m_pSensorModeData->binningTypeH;
    pInputData->pIFETuningMetadata->sensorData.binningTypeV     = m_pSensorModeData->binningTypeV;

    if (m_pSensorModeData->binningTypeH != m_pSensorModeData->binningTypeV)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Vertical and Horizontal binning should be the same. (Vertical:%d, Horizontal:%d)",
            m_pSensorModeData->binningTypeH,  m_pSensorModeData->binningTypeV);
    }

    // Populate trigger data
    IFETuningTriggerData* pIFETuningTriggers    = &pInputData->pIFETuningMetadata->IFETuningTriggers;
    pIFETuningTriggers->AECexposureGainRatio    = pInputData->triggerData.AECexposureGainRatio;
    pIFETuningTriggers->AECexposureTime         = pInputData->triggerData.AECexposureTime;
    pIFETuningTriggers->AECSensitivity          = pInputData->triggerData.AECSensitivity;
    pIFETuningTriggers->AECGain                 = pInputData->triggerData.AECGain;
    pIFETuningTriggers->AECLuxIndex             = pInputData->triggerData.AECLuxIndex;
    pIFETuningTriggers->AWBleftGGainWB          = pInputData->triggerData.AWBleftGGainWB;
    pIFETuningTriggers->AWBleftBGainWB          = pInputData->triggerData.AWBleftBGainWB;
    pIFETuningTriggers->AWBleftRGainWB          = pInputData->triggerData.AWBleftRGainWB;
    pIFETuningTriggers->AWBColorTemperature     = pInputData->triggerData.AWBColorTemperature;
    pIFETuningTriggers->DRCGain                 = pInputData->triggerData.DRCGain;
    pIFETuningTriggers->DRCGainDark             = pInputData->triggerData.DRCGainDark;
    pIFETuningTriggers->lensPosition            = pInputData->triggerData.lensPosition;
    pIFETuningTriggers->lensZoom                = pInputData->triggerData.lensZoom;
    pIFETuningTriggers->postScaleRatio          = pInputData->triggerData.postScaleRatio;
    pIFETuningTriggers->preScaleRatio           = pInputData->triggerData.preScaleRatio;
    pIFETuningTriggers->sensorImageWidth        = pInputData->triggerData.sensorImageWidth;
    pIFETuningTriggers->sensorImageHeight       = pInputData->triggerData.sensorImageHeight;
    pIFETuningTriggers->CAMIFWidth              = pInputData->triggerData.CAMIFWidth;
    pIFETuningTriggers->CAMIFHeight             = pInputData->triggerData.CAMIFHeight;
    pIFETuningTriggers->numberOfLED             = pInputData->triggerData.numberOfLED;
    pIFETuningTriggers->LEDSensitivity          = static_cast<INT32>(pInputData->triggerData.LEDSensitivity);
    pIFETuningTriggers->bayerPattern            = pInputData->triggerData.bayerPattern;
    pIFETuningTriggers->sensorOffsetX           = pInputData->triggerData.sensorOffsetX;
    pIFETuningTriggers->sensorOffsetY           = pInputData->triggerData.sensorOffsetY;
    pIFETuningTriggers->blackLevelOffset        = pInputData->triggerData.blackLevelOffset;

    // Populate Sensor configuration data
    IFEBPSSensorConfigData*  pIFESensorConfig   = &pInputData->pIFETuningMetadata->IFESensorConfig;
    pIFESensorConfig->isBayer                   = pInputData->sensorData.isBayer;
    pIFESensorConfig->format                    = static_cast<UINT32>(pInputData->sensorData.format);
    pIFESensorConfig->digitalGain               = pInputData->sensorData.dGain;
    pIFESensorConfig->ZZHDRColorPattern         = pInputData->sensorData.ZZHDRColorPattern;
    pIFESensorConfig->ZZHDRFirstExposure        = pInputData->sensorData.ZZHDRFirstExposure;

    // Add common IFE tuning-data
    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIFETriggerModulesData,
                                            DebugDataTagType::TuningIFETriggerData,
                                            1,
                                            &pInputData->pIFETuningMetadata->IFETuningTriggers,
                                            sizeof(pInputData->pIFETuningMetadata->IFETuningTriggers));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    // Add Sensor tuning metadata tags
    UINT8 pixelFormat = static_cast<UINT8>(pInputData->pIFETuningMetadata->sensorData.format);
    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningSensorPixelFormat,
                                            DebugDataTagType::UInt8,
                                            1,
                                            &pixelFormat,
                                            sizeof(pixelFormat));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningSensorNumPixelsPerLine,
                                            DebugDataTagType::UInt32,
                                            1,
                                            &pInputData->pIFETuningMetadata->sensorData.numPixelsPerLine,
                                            sizeof(pInputData->pIFETuningMetadata->sensorData.numPixelsPerLine));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningSensorNumLinesPerFrame,
                                            DebugDataTagType::UInt32,
                                            1,
                                            &pInputData->pIFETuningMetadata->sensorData.numLinesPerFrame,
                                            sizeof(pInputData->pIFETuningMetadata->sensorData.numLinesPerFrame));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningSensorMaxLineCount,
                                            DebugDataTagType::UInt32,
                                            1,
                                            &pInputData->pIFETuningMetadata->sensorData.maxLineCount,
                                            sizeof(pInputData->pIFETuningMetadata->sensorData.maxLineCount));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningSensorResoultion,
                                            DebugDataTagType::TuningSensorResolution,
                                            1,
                                            &pInputData->pIFETuningMetadata->sensorData.resolution,
                                            sizeof(pInputData->pIFETuningMetadata->sensorData.resolution));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningSensorCropInfo,
                                            DebugDataTagType::TuningSensorCropInfo,
                                            1,
                                            &pInputData->pIFETuningMetadata->sensorData.cropInfo,
                                            sizeof(pInputData->pIFETuningMetadata->sensorData.cropInfo));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningBinningTypeH,
                                            DebugDataTagType::UInt32,
                                            1,
                                            &pInputData->pIFETuningMetadata->sensorData.binningTypeH,
                                            sizeof(pInputData->pIFETuningMetadata->sensorData.binningTypeH));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningBinningTypeV,
                                            DebugDataTagType::UInt32,
                                            1,
                                            &pInputData->pIFETuningMetadata->sensorData.binningTypeV,
                                            sizeof(pInputData->pIFETuningMetadata->sensorData.binningTypeV));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIFESensorConfigData,
                                            DebugDataTagType::TuningIFESensorConfig,
                                            1,
                                            &pInputData->pIFETuningMetadata->IFESensorConfig,
                                            sizeof(pInputData->pIFETuningMetadata->IFESensorConfig));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIFEOEMTuningData,
                                            DebugDataTagType::UInt32,
                                            CAMX_ARRAY_SIZE(pInputData->pIFETuningMetadata->oemTuningData.IQOEMTuningData),
                                            &pInputData->pIFETuningMetadata->oemTuningData.IQOEMTuningData,
                                            sizeof(pInputData->pIFETuningMetadata->oemTuningData.IQOEMTuningData));

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "Add Data Tag failed with error: %d.", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    // Add 3A tuning metadata tags
    // Copy the CHI AEC data locally to the expected debug data format which is byte-aligned and does not necessarily
    // contain all the fields contained in the CHI structure
    TuningAECFrameControl debugDataAEC;
    CAMX_STATIC_ASSERT(CAMX_ARRAY_SIZE(debugDataAEC.exposureInfo) <= CAMX_ARRAY_SIZE(pInputData->pAECUpdateData->exposureInfo));
    for (UINT32 i = 0; i < CAMX_ARRAY_SIZE(debugDataAEC.exposureInfo); i++)
    {
        debugDataAEC.exposureInfo[i].exposureTime       = pInputData->pAECUpdateData->exposureInfo[i].exposureTime;
        debugDataAEC.exposureInfo[i].linearGain         = pInputData->pAECUpdateData->exposureInfo[i].linearGain;
        debugDataAEC.exposureInfo[i].sensitivity        = pInputData->pAECUpdateData->exposureInfo[i].sensitivity;
        debugDataAEC.exposureInfo[i].deltaEVFromTarget  = pInputData->pAECUpdateData->exposureInfo[i].deltaEVFromTarget;
    }
    debugDataAEC.luxIndex       = pInputData->pAECUpdateData->luxIndex;
    debugDataAEC.flashInfo      = static_cast<UINT8>(pInputData->pAECUpdateData->flashInfo);
    debugDataAEC.preFlashState  = static_cast<UINT8>(pInputData->pAECUpdateData->preFlashState);

    CAMX_STATIC_ASSERT(CAMX_ARRAY_SIZE(debugDataAEC.LEDCurrents) <= CAMX_ARRAY_SIZE(pInputData->pAECUpdateData->LEDCurrents));
    for (UINT32 i = 0; i < CAMX_ARRAY_SIZE(debugDataAEC.LEDCurrents); i++)
    {
        debugDataAEC.LEDCurrents[i] = pInputData->pAECUpdateData->LEDCurrents[i];
    }
    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningAECData,
                                            DebugDataTagType::TuningAECFrameControl,
                                            1,
                                            &debugDataAEC,
                                            sizeof(debugDataAEC));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }


    // Copy the CHI AWB data locally to the expected debug data format which is byte-aligned
    TuningAWBFrameControl debugDataAWB;
    debugDataAWB.AWBGains[0]            = pInputData->pAWBUpdateData->AWBGains.rGain;
    debugDataAWB.AWBGains[1]            = pInputData->pAWBUpdateData->AWBGains.gGain;
    debugDataAWB.AWBGains[2]            = pInputData->pAWBUpdateData->AWBGains.bGain;
    debugDataAWB.colorTemperature       = pInputData->pAWBUpdateData->colorTemperature;
    debugDataAWB.isCCMOverrideEnabled   = (pInputData->pAWBUpdateData->AWBCCM[0].isCCMOverrideEnabled ? 1 : 0);

    CAMX_STATIC_ASSERT(
        CAMX_ARRAY_SIZE(debugDataAWB.CCMOffset) <= CAMX_ARRAY_SIZE(pInputData->pAWBUpdateData->AWBCCM[0].CCMOffset));
    for (UINT32 i = 0; i < CAMX_ARRAY_SIZE(debugDataAWB.CCMOffset); i++)
    {
        debugDataAWB.CCMOffset[i] = pInputData->pAWBUpdateData->AWBCCM[0].CCMOffset[i];
    }

    CAMX_STATIC_ASSERT(CAMX_ARRAY_ROWS(debugDataAWB.CCM) >= AWBNumCCMRows);
    CAMX_STATIC_ASSERT(CAMX_ARRAY_COLS(debugDataAWB.CCM) >= AWBNumCCMCols);
    for (UINT32 row = 0; row < AWBNumCCMRows; row++)
    {
        for (UINT32 col = 0; col < AWBNumCCMCols; col++)
        {
            debugDataAWB.CCM[row][col] = pInputData->pAWBUpdateData->AWBCCM[0].CCM[row][col];
        }
    }

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningAWBData,
                                            DebugDataTagType::TuningAWBFrameControl,
                                            1,
                                            &debugDataAWB,
                                            sizeof(debugDataAWB));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::DumpTuningMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::DumpTuningMetadata(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    static const UINT PropertiesTuning[] = { PropertyIDTuningDataIFE };
    VOID* pData[1] = { 0 };
    UINT length = CAMX_ARRAY_SIZE(PropertiesTuning);
    UINT64 propertyDataTuningOffset[1] = { 0 };

    GetDataList(PropertiesTuning, pData, propertyDataTuningOffset, length);

    DebugData* pDebugData = reinterpret_cast<DebugData*>(pData[0]);

    // Check if debug data buffer is available
    if (NULL == pDebugData || NULL == pDebugData->pData)
    {
        // Debug-data buffer not available is valid use case
        // Normal execution will continue without debug data
        CAMX_LOG_VERBOSE(CamxLogGroupDebugData,
                         "SKIP no debug-data available/needed ReqId: %llu instance: %u",
                         pInputData->frameNum,
                         InstanceID());
        return;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupDebugData, "Tuning-metadata:IFE: frameNum: %llu instance: %u, %p, size %zu",
                     pInputData->frameNum,
                     InstanceID(),
                     pDebugData->pData,
                     pDebugData->size);

    // Update the Debug data pool with the tuning data dump
    // Set the buffer pointer
    m_pDebugDataWriter->SetBufferPointer(static_cast<BYTE*>(pDebugData->pData),
                                         pDebugData->size);

    // Populate any metadata obtained direclty from base IFE node
    PopulateGeneralTuningMetadata(pInputData);

    m_pIFEPipeline->DumpTuningMetadata(pInputData, m_pDebugDataWriter);

    // Make a copy in main metadata pool
    UINT32              metaTag                 = 0;
    static const UINT   PropertiesDebugData[]   = { PropertyIDDebugDataAll };
    VOID*               pSrcData[1]             = { 0 };
    length                                      = CAMX_ARRAY_SIZE(PropertiesDebugData);
    propertyDataTuningOffset[0]                 = 0;
    GetDataList(PropertiesDebugData, pSrcData, propertyDataTuningOffset, length);

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.debugdata", "DebugDataAll", &metaTag);
    if (CamxResultSuccess == result)
    {
        const UINT  TuningVendorTag[]   = { metaTag };
        const VOID* pDstData[1]         = { pSrcData[0] };
        UINT        pDataCount[1]       = { sizeof(DebugData) };

        WriteDataList(TuningVendorTag, pDstData, pDataCount, 1);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupDebugData, "Fail to get DebugDataAll tag location");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::InitialCAMIFConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::InitialCAMIFConfig()
{
    CamxResult result        = CamxResultSuccess;
    UINT32     pathMask      = 0;
    BOOL*      pLeftPathEn   = m_CAMIFConfigInfo[0].enableCAMIFPath;
    BOOL*      pRightPathEn  = m_CAMIFConfigInfo[1].enableCAMIFPath;
    BOOL       IFEHWUsed[IFEHWTypeMax]  = { FALSE, FALSE };

    Utils::Memset(&m_CAMIFConfigInfo, 0, sizeof(m_CAMIFConfigInfo));

    for ( UINT32 i = 0; i < m_IFEAcquiredHWInfo.valid_acquired_hw; i++)
    {
        // Updating the paths enabled for regular IFE
        if (m_IFEAcquiredHWInfo.acquired_hw_id[i] & (IFE0HWMask | IFE1HWMask | IFE2HWMask))
        {
            pathMask = m_IFEAcquiredHWInfo.acquired_hw_path[i][0] & IFEPathMasks;

            m_CAMIFConfigInfo[0].IFEHWType    = IFEHWTypeNormal;

            // Set path enable by KMD HW return information
            pLeftPathEn[IFECAMIFPXLPath]    |= (pathMask & IFEPXLPathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFDualPDPath] |= (pathMask & IFEPPDPathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFLCRPath]    |= (pathMask & IFELCRPathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFRDI0Path]   |= (pathMask & IFERDI0PathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFRDI1Path]   |= (pathMask & IFERDI1PathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFRDI2Path]   |= (pathMask & IFERDI2PathMask) > 0 ? TRUE: FALSE;

            // Only dual IFE need acquired_hw_path[i][1], others [0]
            if (IFEModuleMode::DualIFENormal == m_mode)
            {
                pathMask = m_IFEAcquiredHWInfo.acquired_hw_path[i][1] & IFEPathMasks;

                m_CAMIFConfigInfo[1].IFEHWType    = IFEHWTypeNormal;

                pRightPathEn[IFECAMIFPXLPath]    |= (pathMask & IFEPXLPathMask) > 0 ? TRUE: FALSE;
                pRightPathEn[IFECAMIFDualPDPath] |= (pathMask & IFEPPDPathMask) > 0 ? TRUE: FALSE;
                pRightPathEn[IFECAMIFLCRPath]    |= (pathMask & IFELCRPathMask) > 0 ? TRUE: FALSE;
                pRightPathEn[IFECAMIFRDI0Path]   |= (pathMask & IFERDI0PathMask) > 0 ? TRUE: FALSE;
                pRightPathEn[IFECAMIFRDI1Path]   |= (pathMask & IFERDI1PathMask) > 0 ? TRUE: FALSE;
                pRightPathEn[IFECAMIFRDI2Path]   |= (pathMask & IFERDI2PathMask) > 0 ? TRUE: FALSE;

            }

            // Normal IFE HW was used
            IFEHWUsed[IFEHWTypeNormal] = TRUE;
        }

        // Updating the paths enabled for IFE lite
        if (m_IFEAcquiredHWInfo.acquired_hw_id[i] & (IFE0LiteHWMask | IFE1LiteHWMask | IFE2LiteHWMask) &&
            m_IFEAcquiredHWInfo.acquired_hw_path[i][0] & IFEPathMasks)
        {
            pathMask = m_IFEAcquiredHWInfo.acquired_hw_path[i][0] & IFEPathMasks;

            m_CAMIFConfigInfo[0].IFEHWType    = IFEHWTypeLite;

            pLeftPathEn[IFECAMIFPXLPath]    |= (pathMask & IFEPXLPathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFDualPDPath] |= (pathMask & IFEPPDPathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFLCRPath]    |= (pathMask & IFELCRPathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFRDI0Path]   |= (pathMask & IFERDI0PathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFRDI1Path]   |= (pathMask & IFERDI1PathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFRDI2Path]   |= (pathMask & IFERDI2PathMask) > 0 ? TRUE: FALSE;
            pLeftPathEn[IFECAMIFRDI3Path]   |= (pathMask & IFERDI3PathMask) > 0 ? TRUE: FALSE;

            // IFE Lite HW was used
            IFEHWUsed[IFEHWTypeLite] = TRUE;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE mode %d ,valid hw %d, id mask 0x%x,path mask 0x%x, 0x%x",
                        m_mode, i, m_IFEAcquiredHWInfo.acquired_hw_id[i],
                        m_IFEAcquiredHWInfo.acquired_hw_path[i][0],
                        m_IFEAcquiredHWInfo.acquired_hw_path[i][1]);
    }

    if ( TRUE == IFEHWUsed[IFEHWTypeNormal] && TRUE == IFEHWUsed[IFEHWTypeLite] )
    {
        // If we using IFE and IFE Lite in one camxifenode instance, we should have special way to
        // configure CAMIF, throw error since we do not support now
        CAMX_LOG_WARN(CamxLogGroupISP, "Enabling simultaneous IFE and IFE lite not supported")
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ProgramIQEnable()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ProgramIQEnable(
    ISPInputData* pInputData)
{
    CamxResult         result            = CamxResultSuccess;
    ISPInternalData*   pLeftISPData      = NULL;
    ISPInternalData*   pRightISPData     = NULL;
    ISPInternalData*   pCommonISPData    = NULL;
    UINT               perFrameDataindex = 0;

    // We Program IQ module during Init Packet submission and also during
    // ExectureProcessCaptureRequest (per-frame Dynamic Config) as well, and we
    // use two different data strucutres for two different instances (Init & EPR).
    // Hence we use below condition check to distinguish amongst data structures.
    if (pInputData->isInitPacket == FALSE)
    {
        perFrameDataindex = GetIFEPerFrameDataIndex(pInputData->frameNum);
        pLeftISPData      = &m_IFEPerFrameData[perFrameDataindex].ISPData[LeftIFE];
        pRightISPData     = &m_IFEPerFrameData[perFrameDataindex].ISPData[RightIFE];
        pCommonISPData    = &m_IFEPerFrameData[perFrameDataindex].ISPData[CommonIFE];
    }
    else
    {
        pLeftISPData     = &m_ISPData[LeftIFE];
        pRightISPData    = &m_ISPData[RightIFE];
        pCommonISPData   = &m_ISPData[CommonIFE];
    }

    if (IsSensorModeFormatYUV(m_ISPInputSensorData.format))
    {
        // Clearing out all of IQ blocks for Lens processing
        Utils::Memset(&pLeftISPData->moduleEnable.IQModules, 0, sizeof(pLeftISPData->moduleEnable.IQModules));
        Utils::Memset(&pRightISPData->moduleEnable.IQModules, 0, sizeof(pRightISPData->moduleEnable.IQModules));
        Utils::Memset(&pCommonISPData->moduleEnable.IQModules, 0, sizeof(pCommonISPData->moduleEnable.IQModules));

        pLeftISPData->moduleEnable.IQModules.chromaUpsampleEnable = 1;
        pRightISPData->moduleEnable.IQModules.chromaUpsampleEnable = 1;
        pCommonISPData->moduleEnable.IQModules.chromaUpsampleEnable = 1;
        // DEMUX should be always enable
        pLeftISPData->moduleEnable.IQModules.demuxEnable = 1;
        pRightISPData->moduleEnable.IQModules.demuxEnable = 1;
        pCommonISPData->moduleEnable.IQModules.demuxEnable = 1;

        // STATS path should be disable except iHIST
        m_IFEOutputPathInfo[IFEOutputPortStatsHDRBE].path = FALSE;
        m_IFEOutputPathInfo[IFEOutputPortStatsHDRBHIST].path = FALSE;
        m_IFEOutputPathInfo[IFEOutputPortStatsBF].path = FALSE;
        m_IFEOutputPathInfo[IFEOutputPortStatsAWBBG].path = FALSE;
        m_IFEOutputPathInfo[IFEOutputPortStatsBHIST].path = FALSE;
        m_IFEOutputPathInfo[IFEOutputPortStatsRS].path = FALSE;
        m_IFEOutputPathInfo[IFEOutputPortStatsCS].path = FALSE;
        m_IFEOutputPathInfo[IFEOutputPortStatsTLBG].path = FALSE;
    }

    // Update HVX tapout
    if (NULL != m_pIFEHVXModule)
    {
        pInputData->pCalculatedData->moduleEnable.DSPConfig.DSPSelect =
            (static_cast<IFEHVX*>(m_pIFEHVXModule))->GetHVXTapPoint();
    }
    pInputData->pCalculatedMetadata    = &m_ISPMetadata;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        // In dual IFE mode, each IFE will have its own enable bits with potentially different values, hence writing the enable
        // register to left/right (and not common) command buffers.
        CAMX_LOG_VERBOSE(CamxLogGroupApp, "Right command buffer");
        pInputData->pCmdBuffer      = m_pRightCmdBuffer;
        pInputData->pCalculatedData = pRightISPData;
        result = m_pIFEPipeline->ProgramIQModuleEnableConfig(pInputData,
                                                             pCommonISPData,
                                                             &m_IFEOutputPathInfo[0]);

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Left command buffer");
            pInputData->pCmdBuffer      = m_pLeftCmdBuffer;
            pInputData->pCalculatedData = pLeftISPData;
            result                      = m_pIFEPipeline->ProgramIQModuleEnableConfig(pInputData,
                                                                                      pCommonISPData,
                                                                                      &m_IFEOutputPathInfo[0]);

        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Main command buffer");
        pInputData->pCmdBuffer      = m_pCommonCmdBuffer;
        pInputData->pCalculatedData = pCommonISPData;
        result                      = m_pIFEPipeline->ProgramIQModuleEnableConfig(pInputData,
                                                                                  pCommonISPData,
                                                                                  &m_IFEOutputPathInfo[0]);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareHDRBEStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareHDRBEStatsMetadata(
    const ISPInputData*     pInputData,
    PropertyISPHDRBEStats*  pMetadata)
{
    UINT  perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);
    pMetadata->statsConfig  = pInputData->pCalculatedData->metadata.HDRBEStatsConfig;
    pMetadata->dualIFEMode  = FALSE;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pMetadata->dualIFEMode      = TRUE;
        pMetadata->stripeConfig[0]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.HDRBEStatsConfig;
        pMetadata->stripeConfig[1]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].metadata.HDRBEStatsConfig;

        /// @todo (CAMX-1293) Need to send frame level configuration as well. Manually create for now using two stripes.
        pMetadata->statsConfig.HDRBEConfig  = pInputData->pAECStatsUpdateData->statsConfig.BEConfig;
        pMetadata->statsConfig.isAdjusted   = FALSE;
        pMetadata->statsConfig.regionWidth  = pMetadata->stripeConfig[0].regionWidth;
        pMetadata->statsConfig.regionHeight = pMetadata->stripeConfig[0].regionHeight;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareBFStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareBFStatsMetadata(
    const ISPInputData* pInputData,
    PropertyISPBFStats* pMetadata)
{
    UINT  perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);
    pMetadata->statsConfig  = pInputData->pCalculatedData->metadata.BFStats;
    pMetadata->dualIFEMode  = FALSE;
    pMetadata->titanVersion = m_titanVersion;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pMetadata->dualIFEMode      = TRUE;
        pMetadata->stripeConfig[0]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.BFStats;
        pMetadata->stripeConfig[1]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].metadata.BFStats;

        /// @todo (CAMX-1293) Need to send frame level configuration as well. Manually create for now using two stripes.
        pMetadata->statsConfig.BFConfig     = pInputData->pAFStatsUpdateData->statsConfig;
        pMetadata->statsConfig.isAdjusted   = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareAWBBGStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareAWBBGStatsMetadata(
    const ISPInputData*     pInputData,
    PropertyISPAWBBGStats*  pMetadata)
{
    UINT  perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);
    pMetadata->statsConfig = pInputData->pCalculatedData->metadata.AWBBGStatsConfig;
    pMetadata->dualIFEMode = FALSE;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pMetadata->dualIFEMode      = TRUE;
        pMetadata->stripeConfig[0]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.AWBBGStatsConfig;
        pMetadata->stripeConfig[1]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].metadata.AWBBGStatsConfig;

        /// @todo (CAMX-1293) Need to send frame level configuration as well. Manually create for now using two stripes.
        pMetadata->statsConfig.AWBBGConfig  = pInputData->pAWBStatsUpdateData->statsConfig.BGConfig;
        pMetadata->statsConfig.isAdjusted   = FALSE;
        pMetadata->statsConfig.regionWidth  = pMetadata->stripeConfig[0].regionWidth;
        pMetadata->statsConfig.regionHeight = pMetadata->stripeConfig[0].regionHeight;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareRSStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareRSStatsMetadata(
    const ISPInputData* pInputData,
    PropertyISPRSStats* pMetadata)
{
    UINT  perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);
    pMetadata->statsConfig = pInputData->pCalculatedData->metadata.RSStats;
    pMetadata->dualIFEMode = FALSE;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pMetadata->dualIFEMode      = TRUE;
        pMetadata->stripeConfig[0]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.RSStats;
        pMetadata->stripeConfig[1]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].metadata.RSStats;

        /// @todo (CAMX-1293) Need to send frame level configuration as well. Manually create for now using two stripes.
        pMetadata->statsConfig.RSConfig     = pInputData->pAFDStatsUpdateData->statsConfig;
        pMetadata->statsConfig.isAdjusted   = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareCSStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareCSStatsMetadata(
    const ISPInputData* pInputData,
    PropertyISPCSStats* pMetadata)
{
    UINT  perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);
    pMetadata->statsConfig = pInputData->pCalculatedData->metadata.CSStats;
    pMetadata->dualIFEMode = FALSE;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pMetadata->dualIFEMode      = TRUE;
        pMetadata->stripeConfig[0]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.CSStats;
        pMetadata->stripeConfig[1]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].metadata.CSStats;

        /// @todo (CAMX-1293) Need to send frame level configuration as well. Manually create for now using two stripes.
        pMetadata->statsConfig.CSConfig = pInputData->pCSStatsUpdateData->statsConfig;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareBHistStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareBHistStatsMetadata(
    const ISPInputData*     pInputData,
    PropertyISPBHistStats*  pMetadata)
{
    UINT      perFrameDataIndex      = GetIFEPerFrameDataIndex(pInputData->frameNum);

    pMetadata->statsConfig           = pInputData->pCalculatedData->metadata.BHistStatsConfig;
    pMetadata->dualIFEMode           = FALSE;
    pMetadata->statsConfig.requestID = pInputData->frameNum;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pMetadata->dualIFEMode      = TRUE;
        pMetadata->stripeConfig[0]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.BHistStatsConfig;
        pMetadata->stripeConfig[1]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].metadata.BHistStatsConfig;

        /// @todo (CAMX-1293) Need to send frame level configuration as well. Manually create for now using two stripes.
        pMetadata->statsConfig.BHistConfig  = pInputData->pAECStatsUpdateData->statsConfig.BHistConfig;
        pMetadata->statsConfig.numBins      =
            m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.BHistStatsConfig.numBins;
        pMetadata->statsConfig.isAdjusted   = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareIHistStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareIHistStatsMetadata(
    const ISPInputData*     pInputData,
    PropertyISPIHistStats*  pMetadata)
{
    UINT  perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);
    pMetadata->statsConfig = pInputData->pCalculatedData->metadata.IHistStatsConfig;
    pMetadata->dualIFEMode = FALSE;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pMetadata->dualIFEMode      = TRUE;
        pMetadata->stripeConfig[0]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.IHistStatsConfig;
        pMetadata->stripeConfig[1]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].metadata.IHistStatsConfig;

        /// @todo (CAMX-1293) Need to send frame level configuration as well. Manually create for now using two stripes.
        pMetadata->statsConfig.IHistConfig  = pInputData->pIHistStatsUpdateData->statsConfig;
        pMetadata->statsConfig.numBins      =
            m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.IHistStatsConfig.numBins;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareHDRBHistStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareHDRBHistStatsMetadata(
    const ISPInputData*         pInputData,
    PropertyISPHDRBHistStats*   pMetadata)
{
    UINT    perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);

    pMetadata->statsConfig = pInputData->pCalculatedData->metadata.HDRBHistStatsConfig;
    pMetadata->dualIFEMode = FALSE;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pMetadata->dualIFEMode      = TRUE;
        pMetadata->stripeConfig[0]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.HDRBHistStatsConfig;
        pMetadata->stripeConfig[1]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].metadata.HDRBHistStatsConfig;

        /// @todo (CAMX-1293) Need to send frame level configuration as well. Manually create for now using two stripes.
        pMetadata->statsConfig.HDRBHistConfig   = pInputData->pAECStatsUpdateData->statsConfig.HDRBHistConfig;
        pMetadata->statsConfig.numBins          =
            m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.HDRBHistStatsConfig.numBins;
        pMetadata->statsConfig.isAdjusted       = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareTintlessBGStatsMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareTintlessBGStatsMetadata(
    const ISPInputData*     pInputData,
    PropertyISPTintlessBG*  pMetadata)
{
    UINT  perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);
    pMetadata->statsConfig = pInputData->pCalculatedData->metadata.tintlessBGStats;
    pMetadata->dualIFEMode = FALSE;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pMetadata->dualIFEMode      = TRUE;
        pMetadata->stripeConfig[0]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE].metadata.tintlessBGStats;
        pMetadata->stripeConfig[1]  = m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].metadata.tintlessBGStats;


        /// @todo (CAMX-1293) Need to send frame level configuration as well. Manually create for now using two stripes.
        pMetadata->statsConfig.tintlessBGConfig = pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig;
        pMetadata->statsConfig.isAdjusted       = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PostMetadataRaw
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::PostMetadataRaw()
{
    CamxResult  result                                  = CamxResultSuccess;
    UINT        dataIndex                               = 0;
    const VOID* pData[NumIFEMetadataRawOutputTags]      = { 0 };
    UINT        pDataCount[NumIFEMetadataRawOutputTags] = { 0 };

    // Neutral point default value set it to 0.
    Rational    neutralPoint[3]                         = { { 0 } };

    pDataCount[dataIndex] = 3;
    pData[dataIndex]      = &neutralPoint;
    dataIndex++;

    WriteDataList(IFEMetadataRawOutputTags, pData, pDataCount, NumIFEMetadataRawOutputTags);

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PostMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::PostMetadata(
    const ISPInputData* pInputData)
{
    CamxResult   result                             = CamxResultSuccess;
    UINT         dataIndex                          = 0;
    const VOID*  pData[IFETotalMetadataTags]        = { 0 };
    UINT         pDataCount[IFETotalMetadataTags]   = { 0 };
    UINT         pVendorTag[IFETotalMetadataTags]   = { 0 };

    // prepare IFE properties to publish
    PrepareIFEProperties(pInputData, pVendorTag, pData, pDataCount, &dataIndex);

    // prepare HAL metadata tags
    PrepareIFEHALTags(pInputData, pVendorTag, pData, pDataCount, &dataIndex);

    // Prepare IFE Vendor tags
    PrepareIFEVendorTags(pInputData, pVendorTag, pData, pDataCount, &dataIndex);

    // Post metadata tags
    CAMX_ASSERT(IFETotalMetadataTags >= dataIndex);
    if (IFETotalMetadataTags >= dataIndex)
    {
        WriteDataList(pVendorTag, pData, pDataCount, dataIndex);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "dataIndex (%d), has exceeded IFETotalMetadataTags (%d)!",
            dataIndex, IFETotalMetadataTags);
        result = CamxResultEOverflow;
    }

    if (TRUE == m_publishLDCGridData)
    {
        result = ResampleAndPublishICAGridPerCropWindow(pInputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareIFEProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareIFEProperties(
    const ISPInputData* pInputData,
    UINT*               pVendorTag,
    const VOID**        ppData,
    UINT*               pDataCount,
    UINT*               pDataIndex)
{
    IFECropInfo* pCropInfo;
    IFECropInfo* pAppliedCropInfo;
    UINT         dataIndex         = *pDataIndex;
    UINT         perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        // Write and publish the IFE residual crop info
        pCropInfo        = &m_IFEPerFrameData[perFrameDataIndex].ISPFramelevelData.metadata.cropInfo;

        // Write and publish the IFE applied crop info
        pAppliedCropInfo = &m_IFEPerFrameData[perFrameDataIndex].ISPFramelevelData.metadata.appliedCropInfo;
    }
    else
    {
        // Write and publish the IFE residual crop info
        pCropInfo        = &pInputData->pCalculatedData->metadata.cropInfo;

        // Write and publish the IFE applied crop info
        pAppliedCropInfo = &pInputData->pCalculatedData->metadata.appliedCropInfo;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                     "IFE:%d <crop> Full path: [%d,%d,%d,%d], Applied full path: [%d,%d,%d,%d] |"
                     " Display full path: [%d,%d,%d,%d], Applied display full path: [%d,%d,%d,%d]"
                     " req %llu",
                     InstanceID(),
                     pCropInfo->fullPath.left, pCropInfo->fullPath.top,
                     pCropInfo->fullPath.width, pCropInfo->fullPath.height,
                     pAppliedCropInfo->fullPath.left, pAppliedCropInfo->fullPath.top,
                     pAppliedCropInfo->fullPath.width, pAppliedCropInfo->fullPath.height,
                     pCropInfo->displayFullPath.left, pCropInfo->displayFullPath.top,
                     pCropInfo->displayFullPath.width, pCropInfo->displayFullPath.height,
                     pAppliedCropInfo->displayFullPath.left, pAppliedCropInfo->displayFullPath.top,
                     pAppliedCropInfo->displayFullPath.width, pAppliedCropInfo->displayFullPath.height,
                     pInputData->frameNum);

    pVendorTag[dataIndex]   = PropertyIDIFEDigitalZoom;
    pDataCount[dataIndex]   = sizeof(IFECropInfo);
    ppData[dataIndex++]     = pCropInfo;

    pVendorTag[dataIndex]   = PropertyIDIFEAppliedCrop;
    pDataCount[dataIndex]   = sizeof(IFECropInfo);
    ppData[dataIndex++]     = pAppliedCropInfo;

    // Write and publish internal metadata
    pVendorTag[dataIndex]   = PropertyIDIFEGammaOutput;
    pDataCount[dataIndex]   = sizeof(GammaInfo);
    ppData[dataIndex++]     = &m_IFEPerFrameData[perFrameDataIndex].ISPData[CommonIFE].gammaOutput;

    PropertyISPBFStats* pBFStatsProperty = &m_BFStatsMetadata;
    PrepareBFStatsMetadata(pInputData, pBFStatsProperty);
    pVendorTag[dataIndex]   = PropertyIDISPBFConfig;
    pDataCount[dataIndex]   = sizeof(PropertyISPBFStats);
    ppData[dataIndex++]     = pBFStatsProperty;

    PropertyISPIHistStats* pIHistStatsProperty = &m_IHistStatsMetadata;
    PrepareIHistStatsMetadata(pInputData, pIHistStatsProperty);
    pVendorTag[dataIndex]   = PropertyIDISPIHistConfig;
    pDataCount[dataIndex]   = sizeof(PropertyISPIHistStats);
    ppData[dataIndex++]     = pIHistStatsProperty;

    PropertyISPAWBBGStats* pAWBBGStatsProperty = &m_AWBBGStatsMetadata;
    PrepareAWBBGStatsMetadata(pInputData, pAWBBGStatsProperty);
    pVendorTag[dataIndex]   = PropertyIDISPAWBBGConfig;
    pDataCount[dataIndex]   = sizeof(PropertyISPAWBBGStats);
    ppData[dataIndex++]     = pAWBBGStatsProperty;

    PropertyISPHDRBEStats* pHDRBEStatsProperty = &m_HDRBEStatsMetadata;
    PrepareHDRBEStatsMetadata(pInputData, pHDRBEStatsProperty);
    pVendorTag[dataIndex]   = PropertyIDISPHDRBEConfig;
    pDataCount[dataIndex]   = sizeof(PropertyISPHDRBEStats);
    ppData[dataIndex++]     = pHDRBEStatsProperty;

    PropertyISPBHistStats* pBHistStatsProperty = &m_bhistStatsMetadata;
    PrepareBHistStatsMetadata(pInputData, pBHistStatsProperty);
    pVendorTag[dataIndex]   = PropertyIDISPBHistConfig;
    pDataCount[dataIndex]   = sizeof(PropertyISPBHistStats);
    ppData[dataIndex++]     = pBHistStatsProperty;

    PropertyISPHDRBHistStats* pHDRBHistStatsProperty = &m_HDRBHistStatsMetadata;
    PrepareHDRBHistStatsMetadata(pInputData, pHDRBHistStatsProperty);
    pVendorTag[dataIndex]   = PropertyIDISPHDRBHistConfig;
    pDataCount[dataIndex]   = sizeof(PropertyISPHDRBHistStats);
    ppData[dataIndex++]     = pHDRBHistStatsProperty;

    pVendorTag[dataIndex]   = PropertyIDIFEScaleOutput;
    pDataCount[dataIndex]   = sizeof(IFEScalerOutput);
    ppData[dataIndex++]     = &pInputData->pCalculatedData->scalerOutput[1];

    PropertyISPTintlessBG* pTintlessBGStatsProperty = &m_tintlessBGStatsMetadata;
    PrepareTintlessBGStatsMetadata(pInputData, pTintlessBGStatsProperty);
    pVendorTag[dataIndex]   = PropertyIDISPTintlessBGConfig;
    pDataCount[dataIndex]   = sizeof(PropertyISPTintlessBG);
    ppData[dataIndex++]     = pTintlessBGStatsProperty;

    PropertyISPRSStats* pRSStatsProperty = &m_RSStatsMetadata;
    PrepareRSStatsMetadata(pInputData, pRSStatsProperty);
    pVendorTag[dataIndex]   = PropertyIDISPRSConfig;
    pDataCount[dataIndex]   = sizeof(PropertyISPRSStats);
    ppData[dataIndex++]     = pRSStatsProperty;

    PropertyISPCSStats* pCSStatsProperty = &m_CSStatsMetadata;
    PrepareCSStatsMetadata(pInputData, pCSStatsProperty);
    pVendorTag[dataIndex]   = PropertyIDISPCSConfig;
    pDataCount[dataIndex]   = sizeof(PropertyISPCSStats);
    ppData[dataIndex++]     = pCSStatsProperty;

    /**
     * No matter the Single IFE/ Dual IFE, the pCalculatedData will always point to the m_ISPData[2]
     * Always update the percentageOfGTM with the latest update.
     */
    if (NULL != pInputData->triggerData.pADRCData)
    {
        pVendorTag[dataIndex]   = PropertyIDIFEADRCInfoOutput;
        pDataCount[dataIndex]   = sizeof(ADRCData);
        ppData[dataIndex++]     = pInputData->triggerData.pADRCData;
    }
    else
    {
        // Avoid blindly memsetting the whole struct since it is quite large
        m_ADRCData.version        = TMC10;
        m_ADRCData.enable         = FALSE;
        m_ADRCData.gtmEnable      = FALSE;
        m_ADRCData.ltmEnable      = FALSE;

        pVendorTag[dataIndex]   = PropertyIDIFEADRCInfoOutput;
        pDataCount[dataIndex]   = sizeof(ADRCData);
        ppData[dataIndex++]     = &m_ADRCData;
    }

    pVendorTag[dataIndex]   = PropertyIDIPEGamma15PreCalculation;
    pDataCount[dataIndex]   = sizeof(IPEGammaPreOutput);
    ppData[dataIndex++]     = &pInputData->pCalculatedData->IPEGamma15PreCalculationOutput;

    *pDataIndex = dataIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareIFEVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PrepareIFEVendorTags(
    const ISPInputData* pInputData,
    UINT*               pVendorTag,
    const VOID**        ppVendorTagData,
    UINT*               pVendorTagCount,
    UINT*               pDataIndex)
{
    UINT         dataIndex         = *pDataIndex;
    IFECropInfo* pCropInfo;
    IFECropInfo* pAppliedCropInfo;
    INT32        sensorWidth;
    INT32        sensorHeight;
    FLOAT        widthRatio;
    FLOAT        heightRatio;
    UINT         perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        // Write and publish the IFE residual crop info
        pCropInfo        = &m_IFEPerFrameData[perFrameDataIndex].ISPFramelevelData.metadata.cropInfo;

        // Write and publish the IFE applied crop info
        pAppliedCropInfo = &m_IFEPerFrameData[perFrameDataIndex].ISPFramelevelData.metadata.appliedCropInfo;
    }
    else
    {
        // Write and publish the IFE residual crop info
        pCropInfo        = &pInputData->pCalculatedData->metadata.cropInfo;

        // Write and publish the IFE applied crop info
        pAppliedCropInfo = &pInputData->pCalculatedData->metadata.appliedCropInfo;
    }

    sensorWidth = pInputData->sensorData.CAMIFCrop.lastPixel -
        pInputData->sensorData.CAMIFCrop.firstPixel + 1;
    sensorHeight = pInputData->sensorData.CAMIFCrop.lastLine -
        pInputData->sensorData.CAMIFCrop.firstLine + 1;

    // CSID crop override
    if (TRUE == EnableCSIDCropOverridingForSingleIFE())
    {
        sensorWidth  = m_instanceProperty.IFECSIDWidth;
        sensorHeight = m_instanceProperty.IFECSIDHeight;
    }

    // sensor width shift for YUV sensor
    if (TRUE == m_ISPInputSensorData.isYUV)
    {
        sensorWidth >>= 1;
    }

    // Prepare SensorIFEAppliedCrop
    CAMX_ASSERT((0 != m_sensorActiveArrayWidth) && (0 != m_sensorActiveArrayHeight));
    CAMX_ASSERT((0 != sensorWidth) && (0 != sensorHeight));

    widthRatio  = static_cast<FLOAT>(m_sensorActiveArrayWidth) / static_cast<FLOAT>(sensorWidth);
    heightRatio = static_cast<FLOAT>(m_sensorActiveArrayHeight) / static_cast<FLOAT>(sensorHeight);

    // Update the crop window for each stream with respect to active array
    m_modifiedCropWindow.fullPath.left          = Utils::RoundFLOAT(pAppliedCropInfo->fullPath.left   * widthRatio);
    m_modifiedCropWindow.fullPath.top           = Utils::RoundFLOAT(pAppliedCropInfo->fullPath.top    * heightRatio);
    m_modifiedCropWindow.fullPath.width         = Utils::RoundFLOAT(pAppliedCropInfo->fullPath.width  * widthRatio);
    m_modifiedCropWindow.fullPath.height        = Utils::RoundFLOAT(pAppliedCropInfo->fullPath.height * heightRatio);

    m_modifiedCropWindow.FDPath.left            = Utils::RoundFLOAT(pAppliedCropInfo->FDPath.left     * widthRatio);
    m_modifiedCropWindow.FDPath.top             = Utils::RoundFLOAT(pAppliedCropInfo->FDPath.top      * heightRatio);
    m_modifiedCropWindow.FDPath.width           = Utils::RoundFLOAT(pAppliedCropInfo->FDPath.width    * widthRatio);
    m_modifiedCropWindow.FDPath.height          = Utils::RoundFLOAT(pAppliedCropInfo->FDPath.height   * heightRatio);

    m_modifiedCropWindow.DS4Path.left           = Utils::RoundFLOAT(pAppliedCropInfo->DS4Path.left    * widthRatio);
    m_modifiedCropWindow.DS4Path.top            = Utils::RoundFLOAT(pAppliedCropInfo->DS4Path.top     * heightRatio);
    m_modifiedCropWindow.DS4Path.width          = Utils::RoundFLOAT(pAppliedCropInfo->DS4Path.width   * widthRatio);
    m_modifiedCropWindow.DS4Path.height         = Utils::RoundFLOAT(pAppliedCropInfo->DS4Path.height  * heightRatio);

    m_modifiedCropWindow.DS16Path.left          = Utils::RoundFLOAT(pAppliedCropInfo->DS16Path.left   * widthRatio);
    m_modifiedCropWindow.DS16Path.top           = Utils::RoundFLOAT(pAppliedCropInfo->DS16Path.top    * heightRatio);
    m_modifiedCropWindow.DS16Path.width         = Utils::RoundFLOAT(pAppliedCropInfo->DS16Path.width  * widthRatio);
    m_modifiedCropWindow.DS16Path.height        = Utils::RoundFLOAT(pAppliedCropInfo->DS16Path.height * heightRatio);

    m_modifiedCropWindow.displayFullPath.left   = Utils::RoundFLOAT(pAppliedCropInfo->displayFullPath.left   * widthRatio);
    m_modifiedCropWindow.displayFullPath.top    = Utils::RoundFLOAT(pAppliedCropInfo->displayFullPath.top    * heightRatio);
    m_modifiedCropWindow.displayFullPath.width  = Utils::RoundFLOAT(pAppliedCropInfo->displayFullPath.width  * widthRatio);
    m_modifiedCropWindow.displayFullPath.height = Utils::RoundFLOAT(pAppliedCropInfo->displayFullPath.height * heightRatio);

    m_modifiedCropWindow.displayDS4Path.left    = Utils::RoundFLOAT(pAppliedCropInfo->displayDS4Path.left    * widthRatio);
    m_modifiedCropWindow.displayDS4Path.top     = Utils::RoundFLOAT(pAppliedCropInfo->displayDS4Path.top     * heightRatio);
    m_modifiedCropWindow.displayDS4Path.width   = Utils::RoundFLOAT(pAppliedCropInfo->displayDS4Path.width   * widthRatio);
    m_modifiedCropWindow.displayDS4Path.height  = Utils::RoundFLOAT(pAppliedCropInfo->displayDS4Path.height  * heightRatio);

    m_modifiedCropWindow.displayDS16Path.left   = Utils::RoundFLOAT(pAppliedCropInfo->displayDS16Path.left   * widthRatio);
    m_modifiedCropWindow.displayDS16Path.top    = Utils::RoundFLOAT(pAppliedCropInfo->displayDS16Path.top    * heightRatio);
    m_modifiedCropWindow.displayDS16Path.width  = Utils::RoundFLOAT(pAppliedCropInfo->displayDS16Path.width  * widthRatio);
    m_modifiedCropWindow.displayDS16Path.height = Utils::RoundFLOAT(pAppliedCropInfo->displayDS16Path.height * heightRatio);

    if ((TRUE == m_publishLDCGridData) &&
        ((TRUE == m_ICAGridOut2InEnabled) || (TRUE == m_ICAGridIn2OutEnabled)))
    {
        if ((0 < sensorWidth) && (0 < sensorHeight))
        {
            // Fill LDC Frame zoom window as per grid defined domain
            FillLDCIFEOutZoomwindow(pAppliedCropInfo, pCropInfo, sensorWidth, sensorHeight);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid sensor width %d, height %d", sensorWidth, sensorHeight);
        }
    }

    // Post SensorIFEAppliedCrop, Index 0 maps to SensorIFEAppliedCrop, to change update IFEOutputVendorTags ordering
    pVendorTag[dataIndex]       = m_vendorTagArray[SensorIFEAppliedCropIndex];
    ppVendorTagData[dataIndex]  = { &m_modifiedCropWindow };
    pVendorTagCount[dataIndex]  = { sizeof(m_modifiedCropWindow) };
    dataIndex++;

    // Post ResidualCrop, Index 1 maps to ResidualCrop, to change update IFEOutputVendorTags ordering
    pVendorTag[dataIndex]       = m_vendorTagArray[ResidualCropIndex];
    ppVendorTagData[dataIndex]  = { pCropInfo };
    pVendorTagCount[dataIndex]  = { sizeof(IFECropInfo) };
    dataIndex++;

    // Post AppliedCrop, Index 2 maps to AppliedCrop, to change update IFEOutputVendorTags ordering
    pVendorTag[dataIndex]       = m_vendorTagArray[AppliedCropIndex];
    ppVendorTagData[dataIndex]  = { pAppliedCropInfo };
    pVendorTagCount[dataIndex]  = { sizeof(IFECropInfo) };
    dataIndex++;

    // Post to Gamma, Index 3 maps to Gamma Info, to change update IFEOutputVendorTags ordering
    pVendorTag[dataIndex]       = m_vendorTagArray[GammaInfoIndex];
    ppVendorTagData[dataIndex]  = { &m_IFEPerFrameData[perFrameDataIndex].ISPData[CommonIFE].gammaOutput };
    pVendorTagCount[dataIndex]  = { sizeof(GammaInfo) };
    dataIndex++;

    // check for MultiCameraId vendor tag and then read and post dual camera metadata if available
    if (0 != m_vendorTagArray[MultiCameraIdIndex])
    {
        const UINT multiCamIdTag[]  = { m_vendorTagArray[MultiCameraIdIndex] | InputMetadataSectionMask };
        VOID* pDataMultiCamId[]     = { 0 };
        MultiCameraIds* pMCCInfo    = NULL;
        UINT64 dataOffset[1]        = { 0 };
        if (CDKResultSuccess == GetDataList(multiCamIdTag, pDataMultiCamId, dataOffset, 1))
        {
            // Post MultiCameraId, Index 4 maps to MultiCameraId, to change update IFEOutputVendorTags ordering
            pVendorTag[dataIndex]       = m_vendorTagArray[MultiCameraIdIndex];
            ppVendorTagData[dataIndex]  = { pDataMultiCamId[0] };
            pVendorTagCount[dataIndex]  = { sizeof(MultiCameraIds) };
            dataIndex++;
            pMCCInfo = reinterpret_cast<MultiCameraIds*>(pDataMultiCamId[0]);
        }

        // Post MasterCamera, Index 5 maps to MasterCamera, to change update IFEOutputVendorTags ordering
        const UINT masterCameraTag[]        = { m_vendorTagArray[MasterCameraIndex] | InputMetadataSectionMask };
        VOID* pMasterCameraGetData[]        = { 0 };
        UINT64 masterCameraDataOffset[1]    = { 0 };
        GetDataList(masterCameraTag, pMasterCameraGetData, masterCameraDataOffset, 1);
        if (NULL != pMasterCameraGetData[0])
        {
            // Post MasterCamera, Index 5 maps to MasterCamera, to change update IFEOutputVendorTags ordering
            pVendorTag[dataIndex]       = m_vendorTagArray[MasterCameraIndex];
            ppVendorTagData[dataIndex]  = { pMasterCameraGetData[0] };
            pVendorTagCount[dataIndex]  = { 1 };
            dataIndex++;
        }
    }

    // Post crop_regions, Index 6 maps to crop_regions, to change update IFEOutputVendorTags ordering
    const UINT tagReadInput[] = { m_vendorTagArray[CropRegionsIndex] | InputMetadataSectionMask };
    VOID* pDataCropRegions[]  = { 0 };
    UINT64 dataOffset[1]      = { 0 };
    if (CDKResultSuccess == GetDataList(tagReadInput , pDataCropRegions, dataOffset, 1))
    {
        // Post crop_regions, Index 6 maps to crop_regions, to change update IFEOutputVendorTags ordering
        pVendorTag[dataIndex]       = m_vendorTagArray[CropRegionsIndex];
        ppVendorTagData[dataIndex]  = { pDataCropRegions[0] };
        pVendorTagCount[dataIndex]  = { sizeof(CaptureRequestCropRegions) };
        dataIndex++;
    }

    *pDataIndex = dataIndex;

    if (TRUE == GetStaticSettings()->enableStreamCropZoom)
    {
        // Enabling Port Specific metadata
        StreamCropInfo cropInfo;
        UINT           tagId;
        CamxResult     result = CamxResultSuccess;

        result = VendorTagManager::QueryVendorTagLocation("com.qti.camera.streamCropInfo",
                                                          "StreamCropInfo", &tagId);

        static const UINT DataTypeId[] = { tagId };
        const VOID*       pOutputData[CAMX_ARRAY_SIZE(DataTypeId)] = { &cropInfo };
        UINT              pDataSize[CAMX_ARRAY_SIZE(DataTypeId)]   = { sizeof(StreamCropInfo) };
        if (TRUE == m_IFEOutputPathInfo[IFEOutputPortFD].path)
        {
            // Updating crop with respect to active array in FOV
            cropInfo.fov.left   = m_modifiedCropWindow.FDPath.left;
            cropInfo.fov.top    = m_modifiedCropWindow.FDPath.top;
            cropInfo.fov.width  = m_modifiedCropWindow.FDPath.width;
            cropInfo.fov.height = m_modifiedCropWindow.FDPath.height;

            // Updating residual crop in crop fields
            cropInfo.crop.left   = pCropInfo->FDPath.left;
            cropInfo.crop.top    = pCropInfo->FDPath.top;
            cropInfo.crop.width  = pCropInfo->FDPath.width;
            cropInfo.crop.height = pCropInfo->FDPath.height;

            result = WritePSDataList(IFEOutputPortFD, DataTypeId, pOutputData, pDataSize,
                CAMX_ARRAY_SIZE(DataTypeId));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "WritePortDataList API failed for IFEOutputPortFD");
            }
        }
        if (TRUE == m_IFEOutputPathInfo[IFEOutputPortDisplayFull].path)
        {
            cropInfo.fov.left   = m_modifiedCropWindow.displayFullPath.left;
            cropInfo.fov.top    = m_modifiedCropWindow.displayFullPath.top;
            cropInfo.fov.width  = m_modifiedCropWindow.displayFullPath.width;
            cropInfo.fov.height = m_modifiedCropWindow.displayFullPath.height;

            cropInfo.crop.left   = pCropInfo->displayFullPath.left;
            cropInfo.crop.top    = pCropInfo->displayFullPath.top;
            cropInfo.crop.width  = pCropInfo->displayFullPath.width;
            cropInfo.crop.height = pCropInfo->displayFullPath.height;

            result = WritePSDataList(IFEOutputPortDisplayFull, DataTypeId, pOutputData, pDataSize,
                CAMX_ARRAY_SIZE(DataTypeId));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "WritePortDataList API failed for IFEOutputPortDisplayFull");
            }
        }
        if (TRUE == m_IFEOutputPathInfo[IFEOutputPortFull].path)
        {
            cropInfo.fov.left   = m_modifiedCropWindow.fullPath.left;
            cropInfo.fov.top    = m_modifiedCropWindow.fullPath.top;
            cropInfo.fov.width  = m_modifiedCropWindow.fullPath.width;
            cropInfo.fov.height = m_modifiedCropWindow.fullPath.height;

            cropInfo.crop.left   = pCropInfo->fullPath.left;
            cropInfo.crop.top    = pCropInfo->fullPath.top;
            cropInfo.crop.width  = pCropInfo->fullPath.width;
            cropInfo.crop.height = pCropInfo->fullPath.height;

            result = WritePSDataList(IFEOutputPortFull, DataTypeId, pOutputData, pDataSize,
                CAMX_ARRAY_SIZE(DataTypeId));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "WritePortDataList API failed for IFEOutputPortFull");
            }
        }
        cropInfo.fov.left   = m_modifiedCropWindow.fullPath.left;
        cropInfo.fov.top    = m_modifiedCropWindow.fullPath.top;
        cropInfo.fov.width  = m_modifiedCropWindow.fullPath.width;
        cropInfo.fov.height = m_modifiedCropWindow.fullPath.height;

        cropInfo.crop.left   = pCropInfo->fullPath.left;
        cropInfo.crop.top    = pCropInfo->fullPath.top;
        cropInfo.crop.width  = pCropInfo->fullPath.width;
        cropInfo.crop.height = pCropInfo->fullPath.height;

        result = WritePSDataList(IFEOutputPortFull, DataTypeId, pOutputData, pDataSize,
                                 CAMX_ARRAY_SIZE(DataTypeId));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "WritePortDataList API failed for IFEOutputPortFull");
        }

        PublishPSData(tagId, NULL);

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareIFEHALTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::PrepareIFEHALTags(
    const ISPInputData* pInputData,
    UINT*               pVendorTag,
    const VOID**        ppData,
    UINT*               pDataCount,
    UINT*               pDataIndex)
{
    CamxResult   result             = CamxResultSuccess;
    UINT         index              = *pDataIndex;
    FLOAT        widthRatio;
    FLOAT        heightRatio;
    INT32        sensorWidth;
    INT32        sensorHeight;
    UINT         perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);

    m_scalerCrop    = pInputData->pHALTagsData->HALCrop;


    sensorWidth = pInputData->sensorData.CAMIFCrop.lastPixel -
        pInputData->sensorData.CAMIFCrop.firstPixel + 1;
    sensorHeight = pInputData->sensorData.CAMIFCrop.lastLine -
        pInputData->sensorData.CAMIFCrop.firstLine + 1;

    // CSID crop override
    if (TRUE == EnableCSIDCropOverridingForSingleIFE())
    {
        sensorWidth  = m_instanceProperty.IFECSIDWidth;
        sensorHeight = m_instanceProperty.IFECSIDHeight;
    }

    if (TRUE == m_csidBinningInfo.isBinningEnabled)
    {
        sensorWidth  >>= 1;
        sensorHeight >>= 1;
    }

    // sensor width shift for YUV sensor
    if (TRUE == m_ISPInputSensorData.isYUV)
    {
        sensorWidth >>= 1;
    }

    CAMX_ASSERT((0 != m_sensorActiveArrayWidth) && (0 != m_sensorActiveArrayHeight));
    widthRatio  = static_cast<FLOAT>(m_sensorActiveArrayWidth) / static_cast<FLOAT>(sensorWidth);
    heightRatio = static_cast<FLOAT>(m_sensorActiveArrayHeight) / static_cast<FLOAT>(sensorHeight);

    m_scalerCrop.left   = Utils::RoundFLOAT(m_scalerCrop.left   * widthRatio);
    m_scalerCrop.top    = Utils::RoundFLOAT(m_scalerCrop.top    * heightRatio);
    m_scalerCrop.width  = Utils::RoundFLOAT(m_scalerCrop.width  * widthRatio);
    m_scalerCrop.height = Utils::RoundFLOAT(m_scalerCrop.height * heightRatio);

    pVendorTag[index]   = ScalerCropRegion;
    pDataCount[index]   = 4;
    ppData[index]       = &m_scalerCrop;
    index++;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pVendorTag[index]   = ShadingMode;
        pDataCount[index]   = 1;
        ppData[index]       = &m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].lensShadingInfo.shadingMode;
        index++;

        pVendorTag[index]   = StatisticsLensShadingMapMode;
        pDataCount[index]   = 1;
        ppData[index]       = &m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].lensShadingInfo.lensShadingMapMode;
        index++;

        pVendorTag[index]   = LensInfoShadingMapSize;
        pDataCount[index]   = 2;
        ppData[index]       = &m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].lensShadingInfo.lensShadingMapSize;
        index++;
    }
    else
    {
        pVendorTag[index]   = ShadingMode;
        pDataCount[index]   = 1;
        ppData[index]       = &pInputData->pCalculatedData->lensShadingInfo.shadingMode;
        index++;

        pVendorTag[index]   = StatisticsLensShadingMapMode;
        pDataCount[index]   = 1;
        ppData[index]       = &pInputData->pCalculatedData->lensShadingInfo.lensShadingMapMode;
        index++;

        pVendorTag[index]   = LensInfoShadingMapSize;
        pDataCount[index]   = 2;
        ppData[index]       = &pInputData->pCalculatedData->lensShadingInfo.lensShadingMapSize;
        index++;
    }

    if (StatisticsLensShadingMapModeOn == pInputData->pCalculatedData->lensShadingInfo.lensShadingMapMode)
    {
        pDataCount[index] = (TotalChannels * MESH_ROLLOFF_SIZE);
        pVendorTag[index] = StatisticsLensShadingMap;
        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            ppData[index] = m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].lensShadingInfo.lensShadingMap;
        }
        else
        {
            ppData[index] = pInputData->pCalculatedData->lensShadingInfo.lensShadingMap;
        }
        index++;
    }
    else
    {
        pVendorTag[index]  = StatisticsLensShadingMap;
        pDataCount[index]   = 1;
        ppData[index]       = NULL;
        index++;
    }

    for (UINT32 index = 0; index < ISPChannelMax; index++)
    {
        m_dynamicBlackLevel[index] =
            static_cast<FLOAT>(((pInputData->pCalculatedMetadata->BLSblackLevelOffset +
                                 pInputData->pCalculatedMetadata->linearizationAppliedBlackLevel[index]) >>
                                (IFEPipelineBitWidth - pInputData->sensorBitWidth)));
    }

    pVendorTag[index]   = SensorDynamicBlackLevel;
    pDataCount[index]   = 4;
    ppData[index]       = m_dynamicBlackLevel;
    index++;

    pVendorTag[index]   = SensorDynamicWhiteLevel;
    pDataCount[index]   = 1;
    ppData[index]       = &m_pSensorCaps->whiteLevel;
    index++;

    pVendorTag[index]   = SensorBlackLevelPattern;
    pDataCount[index]   = 4;
    ppData[index]       = &m_pSensorCaps->blackLevelPattern;
    index++;

    ComputeNeutralPoint(pInputData, &m_neutralPoint[0]);
    pVendorTag[index]   = SensorNeutralColorPoint;
    pDataCount[index]   = sizeof(m_neutralPoint) / sizeof(Rational);
    ppData[index]       = m_neutralPoint;
    index++;

    pVendorTag[index]   = BlackLevelLock;
    pDataCount[index]   = 1;
    ppData[index]       = &(m_IFEPerFrameData[perFrameDataIndex].ISPData[CommonIFE].blackLevelLock);
    index++;

    pVendorTag[index]   = ColorCorrectionGains;
    pDataCount[index]   = 4;
    ppData[index]       = &(m_IFEPerFrameData[perFrameDataIndex].ISPData[CommonIFE].colorCorrectionGains);
    index++;

    pVendorTag[index]   = ControlPostRawSensitivityBoost;
    pDataCount[index]   = 1;
    ppData[index]       = &(m_IFEPerFrameData[perFrameDataIndex].ISPData[CommonIFE].controlPostRawSensitivityBoost);
    index++;

    pVendorTag[index] = HotPixelMode;
    pDataCount[index] = 1;
    ppData[index]     = &(m_ISPMetadata.hotPixelMode);
    index++;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        pVendorTag[index]   = NoiseReductionMode;
        pDataCount[index]   = 1;
        ppData[index]       = &(m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE].noiseReductionMode);
        index++;
    }
    else
    {
        pVendorTag[index]   = NoiseReductionMode;
        pDataCount[index]   = 1;
        ppData[index]       = &pInputData->pCalculatedData->noiseReductionMode;
        index++;
    }

    pVendorTag[index]   = StatisticsHotPixelMapMode;
    pDataCount[index]   = 1;
    ppData[index]       = &pInputData->pHALTagsData->statisticsHotPixelMapMode;
    index++;

    pVendorTag[index]   = TonemapMode;
    pDataCount[index]   = 1;
    ppData[index]       = &pInputData->pHALTagsData->tonemapCurves.tonemapMode;
    index++;

    *pDataIndex = index;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ComputeNeutralPoint
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ComputeNeutralPoint(
    const ISPInputData* pInputData,
    Rational*           pNeutralPoint)
{
    FLOAT inverseWbGain[3]   = { 0 };
    INT32 neutralDenominator = (1 << 10);

    if (pInputData->pAWBUpdateData->AWBGains.rGain != 0 &&
        pInputData->pAWBUpdateData->AWBGains.gGain != 0 &&
        pInputData->pAWBUpdateData->AWBGains.bGain != 0)
    {
        inverseWbGain[0] = pInputData->pAWBUpdateData->AWBGains.gGain / pInputData->pAWBUpdateData->AWBGains.rGain;
        inverseWbGain[1] = pInputData->pAWBUpdateData->AWBGains.gGain;
        inverseWbGain[2] = pInputData->pAWBUpdateData->AWBGains.gGain / pInputData->pAWBUpdateData->AWBGains.bGain;

        pNeutralPoint[0].numerator   = CamX::Utils::FloatToQNumber(inverseWbGain[0], neutralDenominator);
        pNeutralPoint[1].numerator   = CamX::Utils::FloatToQNumber(inverseWbGain[1], neutralDenominator);
        pNeutralPoint[2].numerator   = CamX::Utils::FloatToQNumber(inverseWbGain[2], neutralDenominator);
    }
    else
    {
        pNeutralPoint[0].numerator   = neutralDenominator;
        pNeutralPoint[1].numerator   = neutralDenominator;
        pNeutralPoint[2].numerator   = neutralDenominator;

        CAMX_LOG_ERROR(CamxLogGroupISP, "Input WB gain is 0");
    }

    pNeutralPoint[0].denominator = neutralDenominator;
    pNeutralPoint[1].denominator = neutralDenominator;
    pNeutralPoint[2].denominator = neutralDenominator;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "WBGains: R:%f G:%f B:%f NP: %d/%d, %d/%d, %d/%d",
        pInputData->pAWBUpdateData->AWBGains.rGain, pInputData->pAWBUpdateData->AWBGains.gGain,
        pInputData->pAWBUpdateData->AWBGains.bGain,
        pNeutralPoint[0].numerator, pNeutralPoint[0].denominator,
        pNeutralPoint[1].numerator, pNeutralPoint[1].denominator,
        pNeutralPoint[2].numerator, pNeutralPoint[2].denominator);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFENode::PrepareStripingParameters()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult  result              = CamxResultSuccess;
    UINT        count               = 0;
    UINT        perFrameDataIndex   = 0;
    FLOAT       percentageOfGTM     = 0.0f;

    pInputData->IFEDynamicEnableMask        = GetStaticSettings()->IFEDynamicEnableMask;
    pInputData->isPrepareStripeInputContext = TRUE;

    // PrepareStriping is called during Init Packet submission and also during
    // ExectureProcessCaptureRequest (per-frame Dynamic Config) as well, and we
    // use two different data strucutres for two different instances (Init & EPR).
    // Hence we use below condition check to distinguish amongst data structures.
    if ((CamxInvalidRequestId == pInputData->frameNum) || (TRUE == pInputData->isInitPacket))
    {
        Utils::Memset(&m_ISPFramelevelData, 0, sizeof(m_ISPFramelevelData));
    }
    else
    {
        perFrameDataIndex = GetIFEPerFrameDataIndex(pInputData->frameNum);
        Utils::Memset(&m_IFEPerFrameData[perFrameDataIndex].ISPFramelevelData, 0, sizeof(ISPInternalData));
    }

    for (count = 0; count < m_numIFEIQModule; count++)
    {
        if (TRUE == IsADRCEnabled(pInputData, &percentageOfGTM))
        {
            // Update AEC Gain values for ADRC use cases, before GTM(includes) will be triggered by shortGain,
            // betweem GTM & LTM(includes) will be by shortGain*power(DRCGain, gtm_perc) and post LTM will be
            // by shortGain*DRCGain
            IQInterface::UpdateAECGain(m_pIFEIQModule[count]->GetIQType(), pInputData, percentageOfGTM);
        }

        result = m_pIFEIQModule[count]->PrepareStripingParameters(pInputData);
        if (result != CamxResultSuccess)
        {
            break;
        }
    }
    if (CamxResultSuccess == result)
    {
        for (count = 0; count < m_numIFEStatsModule; count++)
        {
            result = m_pIFEStatsModule[count]->PrepareStripingParameters(pInputData);
            if (result != CamxResultSuccess)
            {
                break;
            }
        }
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ProgramIQConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ProgramIQConfig(
    ISPInputData* pInputData)
{
    CamxResult              result              = CamxResultSuccess;
    UINT                    count               = 0;
    FLOAT                   percentageOfGTM     = 0.0f;
    ISPIQTuningDataBuffer   IQOEMTriggerData;
    UINT                    perFrameDataIndex   = 0;
    ISPInternalData*        pLeftISPData        = NULL;
    ISPInternalData*        pRightISPData       = NULL;
    ISPInternalData*        pCommonISPData      = NULL;
    BOOL                    adrcEnabled         = IsADRCEnabled(pInputData, &percentageOfGTM);

    // We Exectue IQ module during Init Packet submission and also during
    // ExectureProcessCaptureRequest (per-frame Dynamic Config) as well and we
    // use two different data strucutres for two different instances.
    // Hence we use below condition check to distinguish amongst data structures.
    if (pInputData->isInitPacket == FALSE)
    {
        perFrameDataIndex  = GetIFEPerFrameDataIndex(pInputData->frameNum);
        pLeftISPData       = &m_IFEPerFrameData[perFrameDataIndex].ISPData[LeftIFE];
        pRightISPData      = &m_IFEPerFrameData[perFrameDataIndex].ISPData[RightIFE];
        pCommonISPData     = &m_IFEPerFrameData[perFrameDataIndex].ISPData[CommonIFE];
        CamX::Utils::Memset(m_IFEPerFrameData[perFrameDataIndex].ISPData, 0,
            sizeof(m_IFEPerFrameData[perFrameDataIndex].ISPData));
        // Update ISPFrameData which is per frame Data structure updated during prepareStriping
        // And the same is needed during IQ Module Execute to know framelevel config.
        m_stripeConfigs[0].pFrameLevelData = &m_IFEPerFrameData[perFrameDataIndex].ISPFrameData;
        m_stripeConfigs[1].pFrameLevelData = &m_IFEPerFrameData[perFrameDataIndex].ISPFrameData;
    }
    else
    {
        pLeftISPData       = &m_ISPData[LeftIFE];
        pRightISPData      = &m_ISPData[RightIFE];
        pCommonISPData     = &m_ISPData[CommonIFE];
        CamX::Utils::Memset(m_ISPData, 0, sizeof(m_ISPData));
    }

    pInputData->isPrepareStripeInputContext = FALSE;

    pInputData->IFEDynamicEnableMask = GetStaticSettings()->IFEDynamicEnableMask;

    if (IFEModuleMode::DualIFENormal != m_mode)
    {
        if ((CamxInvalidRequestId == pInputData->frameNum) || (TRUE == pInputData->isInitPacket))
        {
            CamX::Utils::Memset(&m_ISPFramelevelData, 0, sizeof(m_ISPFramelevelData));
        }
        else
        {
            CamX::Utils::Memset(&m_IFEPerFrameData[perFrameDataIndex].ISPFramelevelData, 0, sizeof(ISPInternalData));
        }
    }

    // Get optional OEM trigger data if available
    if (NULL != pInputData->pIFETuningMetadata)
    {
        IQOEMTriggerData.pBuffer    = pInputData->pIFETuningMetadata->oemTuningData.IQOEMTuningData;
        IQOEMTriggerData.size       = sizeof(pInputData->pIFETuningMetadata->oemTuningData.IQOEMTuningData);
    }
    else
    {
        IQOEMTriggerData.pBuffer    = NULL;
        IQOEMTriggerData.size       = 0;
    }

    // Based on HW team recommendation always keep CGC ON
    if (TRUE == HwEnvironment::GetInstance()->IsHWBugWorkaroundEnabled(Titan17xWorkarounds::Titan17xWorkaroundsCDMDMICGCBug))
    {
        m_pIFEPipeline->FillCGCConfig(m_pCommonCmdBuffer);
    }

    // Update sensor bit width from sensor capability structure
    pInputData->sensorBitWidth = m_pSensorCaps->sensorConfigs[0].streamConfigs[0].bitWidth;

    if ((AdaptiveGTM == GetStaticSettings()->FDPreprocessing) || (GTM == GetStaticSettings()->FDPreprocessing))
    {
        // Get GTM percentage, DRC gain, DRC dark gain for FD preprocessing
        PropertyISPADRCParams adrcParams          = { 0 };
        UINT                  PropertyTag[]       = { PropertyIDIFEADRCParams };
        const UINT            propNum             = CAMX_ARRAY_SIZE(PropertyTag);
        const VOID*           pData[propNum]      = { &adrcParams };
        UINT                  pDataCount[propNum] = { sizeof(PropertyISPADRCParams) };

        adrcParams.bIsADRCEnabled = adrcEnabled;
        adrcParams.GTMPercentage  = percentageOfGTM;
        adrcParams.DRCGain        = pInputData->triggerData.DRCGain;
        adrcParams.DRCDarkGain    = pInputData->triggerData.DRCGainDark;

        WriteDataList(PropertyTag, pData, pDataCount, propNum);
    }

    if (m_PDAFInfo.enableSubsample)
    {
        m_stripeConfigs[0].CAMIFSubsampleInfo.enableCAMIFSubsample                    = m_PDAFInfo.enableSubsample;
        m_stripeConfigs[0].CAMIFSubsampleInfo.CAMIFSubSamplePattern.pixelSkipPattern  = m_PDAFInfo.pixelSkipPattern;
        m_stripeConfigs[0].CAMIFSubsampleInfo.CAMIFSubSamplePattern.lineSkipPattern   = m_PDAFInfo.lineSkipPattern;
        m_stripeConfigs[1].CAMIFSubsampleInfo.enableCAMIFSubsample                   = m_PDAFInfo.enableSubsample;
        m_stripeConfigs[1].CAMIFSubsampleInfo.CAMIFSubSamplePattern.pixelSkipPattern = m_PDAFInfo.pixelSkipPattern;
        m_stripeConfigs[1].CAMIFSubsampleInfo.CAMIFSubSamplePattern.lineSkipPattern  = m_PDAFInfo.lineSkipPattern;
    }

    m_stripeConfigs[0].pCSIDSubsampleInfo  = m_CSIDSubSampleInfo;
    m_stripeConfigs[1].pCSIDSubsampleInfo = m_CSIDSubSampleInfo;

    m_stripeConfigs[0].pCAMIFConfigInfo  = &m_CAMIFConfigInfo[0];
    m_stripeConfigs[1].pCAMIFConfigInfo = &m_CAMIFConfigInfo[1];

    pInputData->pCalculatedMetadata = &m_ISPMetadata;
    for (count = 0; count < m_numIFEIQModule; count++)
    {
        IQModuleDualIFEData dualIFEImpact       = { 0 };
        BOOL                dualIFESensitive    = FALSE;

        if (TRUE == adrcEnabled)
        {
            // Update AEC Gain values for ADRC use cases, before GTM(includes) will be triggered by shortGain,
            // betweem GTM & LTM(includes) will be by shortGain*power(DRCGain, gtm_perc) and post LTM will be
            // by shortGain*DRCGain
            IQInterface::UpdateAECGain(m_pIFEIQModule[count]->GetIQType(), pInputData, percentageOfGTM);
        }

        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            m_pIFEIQModule[count]->GetDualIFEData(&dualIFEImpact);
            dualIFESensitive = dualIFEImpact.dualIFESensitive;

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "m_type %d dualIFESensitive %d ",
                m_pIFEIQModule[count]->GetIQType(), dualIFESensitive);
            // If the module is dual-mode-sensitive, generate its command on left/right command buffer.
            if (FALSE == dualIFESensitive)
            {
                // An IQ module that is not sensitive to dual mode should not depend on anything left/right-specific.
                // It can only generate frame-level data that's common between left and right. Data that is needed
                // by left/right but generated by a common IQ is shared using a pointer to m_ISPFrameData. Only a
                // common IQ module may write into this; it can be read by all IQ.
                // If an IQ module depends on data generated by a dual-sensitive module, then it cannot be common.
                // For uniformity, another element is added to m_ISPData to track common data but this may be
                // refactored as more IQ's are enabled and there is more clarity as what exactly is written by them.
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Common command buffer");
                pInputData->pCalculatedData              = pCommonISPData;
                pInputData->pCmdBuffer                   = m_pCommonCmdBuffer;
                pInputData->pStripeConfig                = &m_stripeConfigs[0];
                result                                   = m_pIFEIQModule[count]->Execute(pInputData);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Right command buffer");
                pInputData->pCmdBuffer                   = m_pRightCmdBuffer;

                pInputData->pCalculatedData              = pRightISPData;
                pInputData->pStripeConfig                = &m_stripeConfigs[1];
                result                                   = m_pIFEIQModule[count]->Execute(pInputData);
                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Left command buffer");
                    pInputData->pCmdBuffer                   = m_pLeftCmdBuffer;
                    pInputData->pCalculatedData              = pLeftISPData;
                    pInputData->pStripeConfig                = &m_stripeConfigs[0];
                    result                                   = m_pIFEIQModule[count]->Execute(pInputData);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to Execute IQ Config for Right Stripe, count %d, IQType %d",
                        count, m_pIFEIQModule[count]->GetIQType());
                }
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to Execute IQ Config for Left Stripe, count %d, IQType %d",
                    count, m_pIFEIQModule[count]->GetIQType());
            }
        }
        else
        {
            // Program the common command buffer
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Main command buffer");
            pInputData->pCmdBuffer      = m_pCommonCmdBuffer;
            pInputData->pCalculatedData = pCommonISPData;
            pInputData->pStripeConfig   = &m_stripeConfigs[0];
            if (m_pIFEIQModule[count]->GetIQType() == ISPIQModuleType::IFEWB)
            {
                pInputData->pAECUpdateData->exposureInfo[0].sensitivity
                    = pInputData->pAECUpdateData->exposureInfo[0].sensitivity * pInputData->pAECUpdateData->predictiveGain;
                pInputData->pAECUpdateData->exposureInfo[0].linearGain
                    = pInputData->pAECUpdateData->exposureInfo[0].linearGain * pInputData->pAECUpdateData->predictiveGain;

                /// re applying triggerdata based on the above calculation
                Node* pBaseNode = this;
                IQInterface::IQSetupTriggerData(pInputData, pBaseNode, TRUE , &IQOEMTriggerData);
            }
            result                      = m_pIFEIQModule[count]->Execute(pInputData);
        }

        if (TRUE == adrcEnabled &&
                ISPIQModuleType::IFEGTM == m_pIFEIQModule[count]->GetIQType())
        {
            percentageOfGTM = pInputData->pCalculatedData->percentageOfGTM;
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "update the percentageOfGTM: %f", percentageOfGTM);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to Run IQ Config Type[%d]: %d",
                           count, m_pIFEIQModule[count]->GetIQType());
        }
    }

    for (count = 0; count < m_numIFEStatsModule; count++)
    {
        IQModuleDualIFEData dualIFEImpact       = { 0 };
        BOOL                dualIFESensitive    = FALSE;

        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            m_pIFEStatsModule[count]->GetDualIFEData(&dualIFEImpact);
            dualIFESensitive = dualIFEImpact.dualIFESensitive;

            if (FALSE == dualIFESensitive)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Common command buffer");
                pInputData->pCalculatedData = pCommonISPData;
                pInputData->pCmdBuffer      = m_pCommonCmdBuffer;
                result                      = m_pIFEStatsModule[count]->Execute(pInputData);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Right command buffer");
                pInputData->pCmdBuffer      = m_pRightCmdBuffer;
                pInputData->pCalculatedData = pRightISPData;
                pInputData->pStripeConfig   = &m_stripeConfigs[1];
                result                      = m_pIFEStatsModule[count]->Execute(pInputData);
                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Left command buffer");
                    pInputData->pCmdBuffer      = m_pLeftCmdBuffer;
                    pInputData->pCalculatedData = pLeftISPData;
                    pInputData->pStripeConfig   = &m_stripeConfigs[0];
                    result                      = m_pIFEStatsModule[count]->Execute(pInputData);
                }
                else
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Failed to Run Stats Config for Right Stripe, count %d", count);
                }
            }

            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Failed to Run Stats Config for Left Stripe, count %d", count);
            }
        }
        else
        {
            // Program the left IFE
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Main command buffer");
            pInputData->pCmdBuffer      = m_pCommonCmdBuffer;
            pInputData->pCalculatedData = pCommonISPData;
            pInputData->pStripeConfig   = &m_stripeConfigs[0];
            result                      = m_pIFEStatsModule[count]->Execute(pInputData);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to Run Stats Config Type[%d]: %d",
                           count, m_pIFEStatsModule[count]->GetStatsType());
        }
    }

    DumpIFESettings(pInputData);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetStaticMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::GetStaticMetadata()
{
    HwCameraInfo    cameraInfo;

    HwEnvironment::GetInstance()->GetCameraInfo(GetPipeline()->GetCameraId(), &cameraInfo);

    // Initialize default metadata
    m_HALTagsData.blackLevelLock                    = BlackLevelLockOff;
    m_HALTagsData.colorCorrectionMode               = ColorCorrectionModeFast;
    m_HALTagsData.controlAEMode                     = ControlAEModeOn;
    m_HALTagsData.controlAWBMode                    = ControlAWBModeAuto;
    m_HALTagsData.controlMode                       = ControlModeAuto;
    m_HALTagsData.controlPostRawSensitivityBoost    = cameraInfo.pPlatformCaps->minPostRawSensitivityBoost;
    m_HALTagsData.noiseReductionMode                = NoiseReductionModeFast;
    m_HALTagsData.shadingMode                       = ShadingModeFast;
    m_HALTagsData.statisticsHotPixelMapMode         = StatisticsHotPixelMapModeOff;
    m_HALTagsData.statisticsLensShadingMapMode      = StatisticsLensShadingMapModeOff;
    m_HALTagsData.tonemapCurves.tonemapMode         = TonemapModeFast;

    // Cache sensor capability information for this camera
    m_pSensorCaps = cameraInfo.pSensorCaps;

    m_sensorActiveArrayWidth  = m_pSensorCaps->activeArraySize.width;
    m_sensorActiveArrayHeight = m_pSensorCaps->activeArraySize.height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetMetadataTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::GetMetadataTags(
    ISPInputData* pModuleInput)
{
    CamxResult          result                          = CamxResultSuccess;
    VOID*               pTagsData[NumIFEMetadataTags]   = {0};
    CropWindow*         pCrop                           = NULL;
    CropWindow          halCrop                         = {0};
    INT32               sensorWidth                     = m_pSensorModeData->cropInfo.lastPixel -
                                                          m_pSensorModeData->cropInfo.firstPixel + 1;
    INT32               sensorHeight                    = m_pSensorModeData->cropInfo.lastLine -
                                                          m_pSensorModeData->cropInfo.firstLine + 1;
    UINT                dataIndex                       = 0;
    FLOAT               widthRatio;
    FLOAT               heightRatio;

    // CSID crop override
    if (TRUE == EnableCSIDCropOverridingForSingleIFE())
    {
        sensorWidth  = m_instanceProperty.IFECSIDWidth;
        sensorHeight = m_instanceProperty.IFECSIDHeight;
    }

    if (TRUE == m_csidBinningInfo.isBinningEnabled)
    {
        sensorWidth  >>= 1;
        sensorHeight >>= 1;
    }

    pModuleInput->pHALTagsData->noiseReductionMode = NoiseReductionModeFast;

    // sensor width shift for YUV sensor
    if (TRUE == m_ISPInputSensorData.isYUV)
    {
        sensorWidth >>= 1;
    }

    GetDataList(IFEMetadataTags, pTagsData, IFEMetadataTagReqOffset, NumIFEMetadataTags);

    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->blackLevelLock = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->colorCorrectionGains =
            *(static_cast<ColorCorrectionGain*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->colorCorrectionMode = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->colorCorrectionTransform =
            *(static_cast<ISPColorCorrectionTransform*>(pTagsData[dataIndex++]));
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
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->HALCrop = *(static_cast<CropWindow*>(pTagsData[dataIndex++]));
        pModuleInput->originalHALCrop       = pModuleInput->pHALTagsData->HALCrop;
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->controlAWBLock = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }
    if (NULL != pTagsData[dataIndex])
    {
        pModuleInput->pHALTagsData->controlAECLock = *(static_cast<UINT8*>(pTagsData[dataIndex++]));
    }

    // Map and update application crop window from sensor active pixel array domain to sensor output dimension domain
    halCrop = pModuleInput->pHALTagsData->HALCrop;
    pCrop = &(pModuleInput->pHALTagsData->HALCrop);

    if ((0 != m_sensorActiveArrayWidth) && (0 != m_sensorActiveArrayHeight) &&
        (0 != sensorWidth) && (0 != sensorHeight))
    {
        if ((TRUE == m_publishLDCGridData) && (TRUE == m_ICAGridOut2InEnabled))
        {
            if (0 == m_ICAGridDomain)
            {
                // input crop domain coversion when ldc grid defined on sensor active array
                ConvertIntoDistortedZoomWindow(pCrop, m_sensorActiveArrayWidth, m_sensorActiveArrayHeight);
            }
        }

        widthRatio  = static_cast<FLOAT>(sensorWidth) / static_cast<FLOAT>(m_sensorActiveArrayWidth);
        heightRatio = static_cast<FLOAT>(sensorHeight) / static_cast<FLOAT>(m_sensorActiveArrayHeight);

        pCrop->left   = Utils::RoundFLOAT(pCrop->left   * widthRatio);
        pCrop->top    = Utils::RoundFLOAT(pCrop->top    * heightRatio);
        pCrop->width  = Utils::RoundFLOAT(pCrop->width  * widthRatio);
        pCrop->height = Utils::RoundFLOAT(pCrop->height * heightRatio);

        CAMX_LOG_INFO(CamxLogGroupISP,
            "IFE:%d <crop> hal[%d,%d,%d,%d] Sensor:Act[%d,%d] Cur[%d,%d] Crop[%d,%d,%d,%d] req %llu",
            InstanceID(),
            halCrop.left, halCrop.top, halCrop.width, halCrop.height,
            m_sensorActiveArrayWidth, m_sensorActiveArrayHeight,
            sensorWidth, sensorHeight,
            pCrop->left, pCrop->top, pCrop->width, pCrop->height,
            pModuleInput->frameNum);

        if ((TRUE == m_publishLDCGridData) && (TRUE == m_ICAGridOut2InEnabled))
        {
            if (1 == m_ICAGridDomain)
            {
                // input crop domain conversion when ldc grid defined on IFE input
                ConvertIntoDistortedZoomWindow(pCrop, sensorWidth, sensorHeight);
            }
            else if (2 == m_ICAGridDomain)
            {
                // input crop domain conversion when ldc grid defined on IFE output
                FLOAT widthRatioToGrid  =
                    static_cast<FLOAT>(m_ifeOutputImageSize.widthPixels) / sensorWidth;
                FLOAT heightRatioToGrid =
                    static_cast<FLOAT>(m_ifeOutputImageSize.heightLines) / sensorHeight;

                pCrop->left   = Utils::RoundFLOAT(pCrop->left   * widthRatioToGrid);
                pCrop->top    = Utils::RoundFLOAT(pCrop->top    * heightRatioToGrid);
                pCrop->width  = Utils::RoundFLOAT(pCrop->width  * widthRatioToGrid);
                pCrop->height = Utils::RoundFLOAT(pCrop->height * heightRatioToGrid);

                ConvertIntoDistortedZoomWindow(pCrop, m_ifeOutputImageSize.widthPixels, m_ifeOutputImageSize.heightLines);

                pCrop->left   = Utils::RoundFLOAT(pCrop->left   / widthRatioToGrid);
                pCrop->top    = Utils::RoundFLOAT(pCrop->top    / heightRatioToGrid);
                pCrop->width  = Utils::RoundFLOAT(pCrop->width  / widthRatioToGrid);
                pCrop->height = Utils::RoundFLOAT(pCrop->height / heightRatioToGrid);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "IFE:%d Invalid sensor active array size : %dx%d, input %dx%d",
                       m_sensorActiveArrayWidth, m_sensorActiveArrayHeight,
                       sensorWidth, sensorHeight);
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
// IFENode::ProgramStripeConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ProgramStripeConfig()
{
    /// @todo (CAMX-1308)   The stripe info should come from the striping library. All the NOT_IMPLEMENTED sections below also
    /// fall in that category (other ports/formats).

    UINT32              outputPortId[MaxDefinedIFEOutputPorts];
    UINT32              totalOutputPorts    = 0;
    CamxResult          result              = CamxResultSuccess;
    IFEDualConfig*      pDualConfig         = NULL;
    IFEStripeConfig*    pStripes            = NULL;
    UINT32              leftIndex           = 0;
    UINT32              rightIndex          = 0;
    UINT32              numPorts            = 0;

    const UINT32 maxStripeConfigSize = DualIFEUtils::GetMaxStripeConfigSize(m_maxNumOfCSLIFEPortId);

    pDualConfig = reinterpret_cast<IFEDualConfig*>(
                    m_pDualIFEConfigCmdBuffer->BeginCommands(maxStripeConfigSize / sizeof(UINT32)));

    if (NULL != pDualConfig)
    {
        CamX::Utils::Memset(pDualConfig, 0, maxStripeConfigSize);

        // Ideally, we should only send whatever many port we program but for simplicity, just use max HW ports.
        pDualConfig->numPorts   = m_maxNumOfCSLIFEPortId;
        numPorts                = m_maxNumOfCSLIFEPortId;
        pStripes                = &pDualConfig->stripes[0];

        GetAllOutputPortIds(&totalOutputPorts, outputPortId);

        for (UINT outputPortIndex = 0; outputPortIndex < totalOutputPorts; outputPortIndex++)
        {
            const ImageFormat*  pImageFormat = GetOutputPortImageFormat(OutputPortIndex(outputPortId[outputPortIndex]));

            if (NULL != pImageFormat)
            {
                UINT32              portId;
                UINT32              statsPortId;
                UINT32              portIdx;
                UINT32              CSLPortID;
                UINT32              stripeWidth = 0;
                UINT32              stripeOffset = 0;

            // We need to convert from pixel to bytes here.
                switch (outputPortId[outputPortIndex])
                {
                    case IFEOutputPortGTMRaw:
                    case IFEOutputPortCAMIFRaw:
                    case IFEOutputPortLSCRaw:
                        portId = CSLIFEPortIndex(CSLIFEPortIdRawDump);
                        if (CamX::Format::RawPlain16 == pImageFormat->format)
                        {
                            // First stripe
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                            pStripes[leftIndex].offset = m_stripeConfigs[0].stream[PixelRawOutput].offset;

                            stripeWidth =
                                (ISPHwTitan480 == m_hwMask) ? m_stripeConfigs[0].stream[PixelRawOutput].width :
                                m_stripeConfigs[0].stream[PixelRawOutput].width * 2;
                            pStripes[leftIndex].width  = stripeWidth;
                            pStripes[leftIndex].portId = CSLIFEPortIdRawDump;
                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Full pixelraw port: left stripe offset: %d, width: %d",
                                pStripes[leftIndex].offset,
                                pStripes[leftIndex].width);
                            // Second stripe
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                            pStripes[rightIndex].offset =
                                (ISPHwTitan480 == m_hwMask) ? m_stripeConfigs[1].stream[PixelRawOutput].offset :
                                m_stripeConfigs[1].stream[PixelRawOutput].offset * 2;
                            stripeWidth =
                                (ISPHwTitan480 == m_hwMask) ? m_stripeConfigs[1].stream[PixelRawOutput].width :
                                m_stripeConfigs[1].stream[PixelRawOutput].width * 2;
                            pStripes[rightIndex].width  = stripeWidth;
                            pStripes[rightIndex].portId = CSLIFEPortIdRawDump;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Full pixelraw port: right stripe offset: %d, width: %d",
                                pStripes[rightIndex].offset,
                                pStripes[rightIndex].width);
                        }
                        break;
                    case IFEOutputPortLCR:
                        portId = CSLIFEPortIndex(CSLIFEPortIdLCR);
                        if (CamX::Format::RawPlain16 == pImageFormat->format)
                        {
                            // First stripe
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);

                            pStripes[leftIndex].offset = m_stripeConfigs[0].stream[LCROutput].offset;
                            pStripes[leftIndex].width  = m_stripeConfigs[0].stream[LCROutput].width;
                            pStripes[leftIndex].portId = CSLIFEPortIdLCR;
                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "LCR Out port: left stripe offset: %d, width: %d",
                                pStripes[leftIndex].offset,
                                pStripes[leftIndex].width);
                            // Second stripe
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);

                            pStripes[rightIndex].offset = m_stripeConfigs[0].stream[LCROutput].width;
                            pStripes[rightIndex].width  = m_stripeConfigs[1].stream[LCROutput].width;
                            pStripes[rightIndex].portId = CSLIFEPortIdLCR;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "LCR Out port: right stripe offset: %d, width: %d",
                                pStripes[rightIndex].offset,
                                pStripes[rightIndex].width);
                        }
                        else
                        {
                            CAMX_LOG_WARN(CamxLogGroupISP, "LCR Output needs to be in Plain16 Format!!!");
                        }
                        break;
                    case IFEOutputPortStatsDualPD:
                    case IFEOutputPortRDI0:
                    case IFEOutputPortRDI1:
                    case IFEOutputPortRDI2:
                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "RDI port %d is not striped; skipping",
                            outputPortId[outputPortIndex]);
                        break;
                    case IFEOutputPortFull:
                    case IFEOutputPortDisplayFull:
                        portId = CSLIFEPortIndex(CSLIFEPortIdFull);
                        CSLPortID = CSLIFEPortIdFull;
                        portIdx = FullOutput;
                        if (IFEOutputPortDisplayFull == outputPortId[outputPortIndex])
                        {
                            portId = CSLIFEPortIndex(CSLIFEPortIdDisplayFull);
                            CSLPortID = CSLIFEPortIdDisplayFull;
                            portIdx = DisplayFullOutput;
                        }
                        if ((CamX::Format::YUV420NV12 == pImageFormat->format) ||
                            (CamX::Format::YUV420NV21 == pImageFormat->format))
                        {
                            // First stripe
                            // For NV12, pixel width == byte width (for both Y and UV plane)
                            // Y plane
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                            pStripes[leftIndex].offset = m_stripeConfigs[0].stream[portIdx].offset;
                            pStripes[leftIndex].width  = m_stripeConfigs[0].stream[portIdx].width;
                            pStripes[leftIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full Y port: left stripe offset: %d, width: %d",
                                portIdx, pStripes[leftIndex].offset, pStripes[leftIndex].width);

                            // UV plane  (same split point and width)
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 1);
                            pStripes[leftIndex].offset = m_stripeConfigs[0].stream[portIdx].offset;
                            pStripes[leftIndex].width  = m_stripeConfigs[0].stream[portIdx].width;
                            pStripes[leftIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full UV port: left stripe offset: %d, width: %d",
                                portIdx, pStripes[leftIndex].offset, pStripes[leftIndex].width);

                            // Second stripe
                            // Y plane
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                            pStripes[rightIndex].offset = m_stripeConfigs[1].stream[portIdx].offset;
                            pStripes[rightIndex].width  = m_stripeConfigs[1].stream[portIdx].width;
                            pStripes[rightIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full Y port: right stripe offset: %d, width: %d",
                                portIdx, pStripes[rightIndex].offset, pStripes[rightIndex].width);

                            // UV plane  (same split point and width)
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 1);
                            pStripes[rightIndex].offset = m_stripeConfigs[1].stream[portIdx].offset;
                            pStripes[rightIndex].width  = m_stripeConfigs[1].stream[portIdx].width;
                            pStripes[rightIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full UV port: right stripe offset: %d, width: %d",
                                portIdx, pStripes[rightIndex].offset, pStripes[rightIndex].width);
                        }
                        else if (TRUE == ImageFormatUtils::IsUBWC(pImageFormat->format))
                        {
                            /// @note: H_INIT is currently supposed to be aligned with the split point; if this
                            ///        changes by striping library, then we need to calculate the start of the
                            ///        tile here and use partial writes to get correct HW behavior.
                            ///        This applies to both left and right stripes.
                            const UBWCTileInfo* pTileInfo = ImageFormatUtils::GetUBWCTileInfo(pImageFormat);

                            if (NULL != pTileInfo)
                            {
                                CAMX_ASSERT(CamxResultSuccess == result);
                                CAMX_ASSERT(0 != pTileInfo->BPPDenominator);
                                CAMX_ASSERT(0 != pTileInfo->widthPixels);

                                // First stripe
                                // For UBWC formats, offset indicates H_INIT value in pixels
                                // Y plane
                                UINT32 leftStripeWidthInPixel             =
                                    m_stripeConfigs[DualIFEStripeIdLeft].stream[portIdx].width;
                                UINT32 leftStripeWidthInPixelDenomAligned =
                                    Utils::AlignGeneric32(leftStripeWidthInPixel, pTileInfo->BPPDenominator);
                                UINT32 leftStripeWidthInPixelTileAligned  =
                                    Utils::AlignGeneric32(leftStripeWidthInPixel, pTileInfo->widthPixels);

                                UINT32 alignedWidth =
                                    (leftStripeWidthInPixelTileAligned * pTileInfo->BPPNumerator) / pTileInfo->BPPDenominator;

                                stripeWidth = (ISPHwTitan480 == m_hwMask) ? leftStripeWidthInPixel : alignedWidth;
                                // H_INIT for left stripe
                                leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                                pStripes[leftIndex].offset = 0;
                                pStripes[leftIndex].width  = stripeWidth;
                                pStripes[leftIndex].portId = CSLPortID;

                                // Calculate right partial write for Y
                                UINT32 partialLeft = 0;
                                UINT32 partialRight = ((leftStripeWidthInPixelTileAligned - leftStripeWidthInPixelDenomAligned)
                                    * pTileInfo->BPPNumerator) / pTileInfo->BPPDenominator;
                                UINT32 leftTileConfig =
                                    ((partialRight << IFERightPartialTileShift)    & IFERightPartialTileMask) |
                                    ((partialLeft  << IFELeftPartialTileShift)     & IFELeftPartialTileMask);

                                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                    "Idx:%u Full Y port (UBWC): left stripe offset: %d, width: %d, partial: %x",
                                    portIdx, pStripes[leftIndex].offset, pStripes[leftIndex].width, leftTileConfig);

                                // UV plane  (same split point and width)
                                // The same as Y plane
                                leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 1);
                                pStripes[leftIndex].offset = 0;
                                pStripes[leftIndex].width  = stripeWidth;
                                pStripes[leftIndex].portId = CSLPortID;

                                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                    "Idx:%u Full UV port (UBWC): left stripe offset: %d, width: %d, partial: %x",
                                    portIdx, pStripes[leftIndex].offset, pStripes[leftIndex].width, leftTileConfig);

                                // Second stripe
                                // Y plane
                                UINT32 rightStripeWidthInPixel =
                                    Utils::AlignGeneric32(m_stripeConfigs[1].stream[portIdx].width,
                                        pTileInfo->BPPDenominator);
                                UINT32 rightStripeDenomAlignedStartInPixel =
                                    Utils::AlignGeneric32(m_stripeConfigs[DualIFEStripeIdRight].stream[portIdx].offset,
                                        pTileInfo->BPPDenominator);
                                UINT32 rightStripeTileAlignedStartInPixel  =
                                    (rightStripeDenomAlignedStartInPixel / pTileInfo->widthPixels) * pTileInfo->widthPixels;
                                UINT32 rightStripeTileAlignedEndInPixel    =
                                    Utils::AlignGeneric32(rightStripeDenomAlignedStartInPixel + rightStripeWidthInPixel,
                                        pTileInfo->widthPixels);

                                alignedWidth =
                                    ((rightStripeTileAlignedEndInPixel - rightStripeTileAlignedStartInPixel)
                                        * pTileInfo->BPPNumerator)
                                    / pTileInfo->BPPDenominator;

                                stripeWidth = (ISPHwTitan480 == m_hwMask) ?
                                    m_stripeConfigs[DualIFEStripeIdRight].stream[portIdx].width : alignedWidth;
                                stripeOffset = (ISPHwTitan480 == m_hwMask) ?
                                    pStripes[leftIndex].width : rightStripeTileAlignedStartInPixel;
                                // H_INIT for right stripe: find the beginning of the tile where offset is.
                                rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                                pStripes[rightIndex].offset = stripeOffset;
                                pStripes[rightIndex].width  = stripeWidth;
                                pStripes[rightIndex].portId = CSLPortID;

                                // Calculate left and right partial write for Y
                                partialLeft = pTileInfo->widthBytes - partialRight;
                                partialRight =
                                    ((rightStripeTileAlignedEndInPixel -
                                    (rightStripeDenomAlignedStartInPixel + rightStripeWidthInPixel))
                                        * pTileInfo->BPPNumerator) / pTileInfo->BPPDenominator;
                                UINT32 rightTileConfig =
                                    ((partialRight << IFERightPartialTileShift)    & IFERightPartialTileMask) |
                                    ((partialLeft  << IFELeftPartialTileShift)     & IFELeftPartialTileMask);

                                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                    "Idx:%u Full Y port (UBWC): right stripe offset: %d, width: %d, partial: %x",
                                    portIdx,
                                    pStripes[rightIndex].offset,
                                    pStripes[rightIndex].width,
                                    rightTileConfig);

                                // UV plane  (same split point and width)
                                // The same as Y
                                rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 1);
                                pStripes[rightIndex].offset = stripeOffset;
                                pStripes[rightIndex].width  = stripeWidth;
                                pStripes[rightIndex].portId = CSLPortID;

                                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                    "Idx:%u Full UV port (UBWC): right stripe offset: %d, width: %d, partial: %x",
                                    portIdx,
                                    pStripes[rightIndex].offset,
                                    pStripes[rightIndex].width,
                                    rightTileConfig);

                                pStripes[GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0)].tileConfig =
                                    leftTileConfig;
                                pStripes[GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 1)].tileConfig =
                                    leftTileConfig;
                                pStripes[GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0)].tileConfig =
                                    rightTileConfig;
                                pStripes[GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 1)].tileConfig =
                                    rightTileConfig;
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupISP, "pTileInfo is NULL");
                                result = CamxResultEInvalidPointer;
                            }
                        }
                        else if (CamX::Format::P010 == pImageFormat->format)
                        {
                            // First stripe
                            // For P010, pixel width == 2*byte width (for both Y and UV plane)
                            // Y plane
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);

                            pStripes[leftIndex].offset = (ISPHwTitan480 == m_hwMask) ?
                                m_stripeConfigs[0].stream[portIdx].offset :
                                m_stripeConfigs[0].stream[portIdx].offset * 2;

                            stripeWidth = (ISPHwTitan480 == m_hwMask) ?
                                m_stripeConfigs[0].stream[portIdx].width :
                                m_stripeConfigs[0].stream[portIdx].width * 2;

                            pStripes[leftIndex].width  = stripeWidth;
                            pStripes[leftIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full Y port: left stripe offset: %d, width: %d",
                                portIdx, pStripes[leftIndex].offset, pStripes[leftIndex].width);

                            // UV plane  (same split point and width)
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 1);

                            pStripes[leftIndex].offset = (ISPHwTitan480 == m_hwMask) ?
                                m_stripeConfigs[0].stream[portIdx].offset :
                                m_stripeConfigs[0].stream[portIdx].offset * 2;

                            pStripes[leftIndex].width  = stripeWidth;
                            pStripes[leftIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full UV port: left stripe offset: %d, width: %d",
                                portIdx, pStripes[leftIndex].offset, pStripes[leftIndex].width);


                            // Second stripe
                            // Y plane
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);

                            pStripes[rightIndex].offset = (ISPHwTitan480 == m_hwMask) ?
                                m_stripeConfigs[1].stream[portIdx].offset :
                                m_stripeConfigs[1].stream[portIdx].offset * 2;

                            stripeWidth = (ISPHwTitan480 == m_hwMask) ?
                                m_stripeConfigs[1].stream[portIdx].width :
                                m_stripeConfigs[1].stream[portIdx].width * 2;

                            pStripes[rightIndex].width  = stripeWidth;
                            pStripes[rightIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full Y port: right stripe offset: %d, width: %d",
                                portIdx, pStripes[rightIndex].offset, pStripes[rightIndex].width);

                            // UV plane  (same split point and width)
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 1);

                            pStripes[rightIndex].offset = (ISPHwTitan480 == m_hwMask) ?
                                m_stripeConfigs[1].stream[portIdx].offset :
                                m_stripeConfigs[1].stream[portIdx].offset * 2;

                            pStripes[rightIndex].width  = stripeWidth;
                            pStripes[rightIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full UV port: right stripe offset: %d, width: %d",
                                portIdx, pStripes[rightIndex].offset, pStripes[rightIndex].width);
                        }
                        else if ((CamX::Format::YUV420NV12TP10 == pImageFormat->format) ||
                            (CamX::Format::YUV420NV21TP10 == pImageFormat->format))
                        {
                            // First stripe
                            // For TP10, pixel width == Ceil(width, 3) * 4 / 3(for both Y and UV plane)
                            // Y plane
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                            pStripes[leftIndex].offset = Utils::DivideAndCeil(m_stripeConfigs[0].stream[portIdx].offset, 3) * 4;
                            stripeWidth =
                                (ISPHwTitan480 == m_hwMask) ? m_stripeConfigs[0].stream[portIdx].width :
                                Utils::DivideAndCeil(m_stripeConfigs[0].stream[portIdx].width, 3) * 4;
                            pStripes[leftIndex].width  = stripeWidth;
                            pStripes[leftIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full Y port: left stripe offset: %d, width: %d",
                                portIdx, pStripes[leftIndex].offset, pStripes[leftIndex].width);

                            // UV plane  (same split point and width)
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 1);
                            pStripes[leftIndex].offset = Utils::DivideAndCeil(m_stripeConfigs[0].stream[portIdx].offset, 3) * 4;
                            pStripes[leftIndex].width  = stripeWidth;
                            pStripes[leftIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full UV port: left stripe offset: %d, width: %d",
                                portIdx, pStripes[leftIndex].offset, pStripes[leftIndex].width);

                            // Second stripe
                            // Y plane
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                            pStripes[rightIndex].offset =
                                Utils::DivideAndCeil(m_stripeConfigs[1].stream[portIdx].offset, 3) * 4;
                            stripeWidth =
                                (ISPHwTitan480 == m_hwMask) ? m_stripeConfigs[1].stream[portIdx].width :
                                Utils::DivideAndCeil(m_stripeConfigs[1].stream[portIdx].width, 3) * 4;
                            pStripes[rightIndex].width  = stripeWidth;
                            pStripes[rightIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full Y port: right stripe offset: %d, width: %d",
                                portIdx, pStripes[rightIndex].offset, pStripes[rightIndex].width);

                            // UV plane  (same split point and width)
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 1);
                            pStripes[rightIndex].offset =
                                Utils::DivideAndCeil(m_stripeConfigs[1].stream[portIdx].offset, 3) * 4;
                            pStripes[rightIndex].width  = stripeWidth;
                            pStripes[rightIndex].portId = CSLPortID;

                            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                "Idx:%u Full UV port: right stripe offset: %d, width: %d",
                                portIdx, pStripes[rightIndex].offset, pStripes[rightIndex].width);
                        }
                        else
                        {
                            CAMX_NOT_IMPLEMENTED();
                        }
                        break;

                    case IFEOutputPortFD:
                        portId = CSLIFEPortIndex(CSLIFEPortIdFD);
                        if (CamX::Format::Y8 == pImageFormat->format)
                        {
                            // First stripe
                            // For NV12, pixel width == byte width (for both Y and UV plane)
                            // Y plane
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                            pStripes[leftIndex].offset = m_stripeConfigs[DualIFEStripeIdLeft].stream[FDOutput].offset;
                            pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stream[FDOutput].width;
                            pStripes[leftIndex].portId = CSLIFEPortIdFD;

                            // Second stripe
                            // Y plane
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                            pStripes[rightIndex].offset = m_stripeConfigs[DualIFEStripeIdRight].stream[FDOutput].offset;
                            pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stream[FDOutput].width;
                            pStripes[rightIndex].portId = CSLIFEPortIdFD;
                            CAMX_LOG_VERBOSE(CamxLogGroupISP, "right FD width %d offset %d, left FD witdh %d offset %d",
                                pStripes[rightIndex].width, pStripes[rightIndex].offset,
                                pStripes[leftIndex].width, pStripes[leftIndex].offset);
                        }
                        else if ((CamX::Format::YUV420NV12 == pImageFormat->format) ||
                            (CamX::Format::YUV420NV21 == pImageFormat->format))
                        {
                            // First stripe
                            // For NV12, pixel width == byte width (for both Y and UV plane)
                            // Y plane
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                            pStripes[leftIndex].offset = m_stripeConfigs[DualIFEStripeIdLeft].stream[FDOutput].offset;
                            pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stream[FDOutput].width;
                            pStripes[leftIndex].portId = CSLIFEPortIdFD;

                            // UV plane  (same split point and width)
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 1);
                            pStripes[leftIndex].offset = m_stripeConfigs[DualIFEStripeIdLeft].stream[FDOutput].offset;
                            pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stream[FDOutput].width;
                            pStripes[leftIndex].portId = CSLIFEPortIdFD;

                            // Second stripe
                            // Y plane
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                            pStripes[rightIndex].offset = m_stripeConfigs[DualIFEStripeIdRight].stream[FDOutput].offset;
                            pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stream[FDOutput].width;
                            pStripes[rightIndex].portId = CSLIFEPortIdFD;
                            CAMX_LOG_VERBOSE(CamxLogGroupISP, "right FD width %d offset %d, left FD width %d offset %d",
                                pStripes[rightIndex].width, pStripes[rightIndex].offset,
                                pStripes[leftIndex].width, pStripes[leftIndex].offset);

                            // UV plane  (same split point and width)
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 1);
                            pStripes[rightIndex].offset = m_stripeConfigs[DualIFEStripeIdRight].stream[FDOutput].offset;
                            pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stream[FDOutput].width;
                            pStripes[rightIndex].portId = CSLIFEPortIdFD;
                        }
                        else
                        {
                            CAMX_NOT_IMPLEMENTED();
                        }
                        break;

                    case IFEOutputPortDS4:
                    case IFEOutputPortDisplayDS4:
                        CAMX_ASSERT(CamX::Format::PD10 == pImageFormat->format);
                        portId    = CSLIFEPortIndex(CSLIFEPortIdDownscaled4);
                        CSLPortID = CSLIFEPortIdDownscaled4;
                        portIdx   = DS4Output;
                        if (IFEOutputPortDisplayDS4 == outputPortId[outputPortIndex])
                        {
                            portId    = CSLIFEPortIndex(CSLIFEPortIdDisplayDownscaled4);
                            CSLPortID = CSLIFEPortIdDisplayDownscaled4;
                            portIdx   = DisplayDS4Output;
                        }
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = m_stripeConfigs[DualIFEStripeIdLeft].stream[portIdx].offset * 4;

                        if (ISPHwTitan480 == m_hwMask)
                        {
                            // For Titan480 the DS4 WM width needs to be configured as width / 2.
                            pStripes[leftIndex].offset = m_stripeConfigs[DualIFEStripeIdLeft].stream[portIdx].offset / 2;
                            pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stream[portIdx].width / 2;
                        }
                        else
                        {
                            pStripes[leftIndex].width =
                                Utils::AlignGeneric32(m_stripeConfigs[DualIFEStripeIdLeft].stream[portIdx].width * 4, 16);
                        }
                        pStripes[leftIndex].portId = CSLPortID;

                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Idx:%u left DS4 width %d offset %d",
                            portIdx, pStripes[leftIndex].width, pStripes[leftIndex].offset);
                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        pStripes[rightIndex].offset = m_stripeConfigs[DualIFEStripeIdRight].stream[portIdx].offset * 4;
                        if (ISPHwTitan480 == m_hwMask)
                        {
                            // For Titan480 the DS4 WM width needs to be configured as width / 2.
                            pStripes[rightIndex].offset = m_stripeConfigs[DualIFEStripeIdRight].stream[portIdx].offset / 2;
                            pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stream[portIdx].width / 2;
                        }
                        else
                        {
                            pStripes[rightIndex].width =
                                Utils::AlignGeneric32(m_stripeConfigs[DualIFEStripeIdRight].stream[portIdx].width * 4, 16);
                        }
                        pStripes[rightIndex].portId = CSLPortID;
                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Idx:%u right DS4 width %d offset %d",
                            portIdx, pStripes[rightIndex].width, pStripes[rightIndex].offset);
                        break;

                    case IFEOutputPortDS16:
                    case IFEOutputPortDisplayDS16:
                        CAMX_ASSERT(CamX::Format::PD10 == pImageFormat->format);
                        portId = CSLIFEPortIndex(CSLIFEPortIdDownscaled16);
                        CSLPortID = CSLIFEPortIdDownscaled16;
                        portIdx = DS16Output;
                        if (IFEOutputPortDisplayDS16 == outputPortId[outputPortIndex])
                        {
                            portId = CSLIFEPortIndex(CSLIFEPortIdDisplayDownscaled16);
                            CSLPortID = CSLIFEPortIdDisplayDownscaled16;
                            portIdx = DisplayDS16Output;
                        }
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = m_stripeConfigs[DualIFEStripeIdLeft].stream[portIdx].offset * 4;
                        if (ISPHwTitan480 == m_hwMask)
                        {
                            // For Titan480 the DS16 WM width needs to be configured as width / 2.
                            pStripes[leftIndex].offset = m_stripeConfigs[DualIFEStripeIdLeft].stream[portIdx].offset / 2;
                            pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stream[portIdx].width / 2;
                        }
                        else
                        {
                            pStripes[leftIndex].width =
                                Utils::AlignGeneric32(m_stripeConfigs[DualIFEStripeIdLeft].stream[portIdx].width * 4, 16);
                        }
                        pStripes[leftIndex].portId = CSLPortID;

                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Idx:%u left DS16 width %d offset %d",
                            portIdx, pStripes[leftIndex].width, pStripes[leftIndex].offset);

                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        pStripes[rightIndex].offset = m_stripeConfigs[DualIFEStripeIdRight].stream[portIdx].offset * 4;
                        if (ISPHwTitan480 == m_hwMask)
                        {
                            // For Titan480 the DS16 WM width needs to be configured as width / 2.
                            pStripes[rightIndex].offset = m_stripeConfigs[DualIFEStripeIdRight].stream[portIdx].offset / 2;
                            pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stream[portIdx].width / 2;
                        }
                        else
                        {
                            pStripes[rightIndex].width =
                                Utils::AlignGeneric32(m_stripeConfigs[DualIFEStripeIdRight].stream[portIdx].width * 4, 16);
                        }
                        pStripes[rightIndex].portId = CSLPortID;

                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Idx:%u right DS16 width %d offset %d",
                            portIdx, pStripes[rightIndex].width, pStripes[rightIndex].offset);
                        break;

                    case IFEOutputPortStatsAWBBG:
                        CAMX_ASSERT(CamX::Format::Blob == pImageFormat->format);
                        portId = CSLIFEPortIndex(CSLIFEPortIdStatAWBBG);
                        statsPortId = CSLIFEStatsPortIndex(CSLIFEPortIdStatAWBBG);
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = 0;
                        pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stats[statsPortId].width;
                        pStripes[leftIndex].portId = CSLIFEPortIdStatAWBBG;

                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        pStripes[rightIndex].offset = AWBBGStatsMaxWidth;
                        pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stats[statsPortId].width;
                        pStripes[rightIndex].portId = CSLIFEPortIdStatAWBBG;
                        break;
                    case IFEOutputPortStatsHDRBE:
                        CAMX_ASSERT(CamX::Format::Blob == pImageFormat->format);
                        portId = CSLIFEPortIndex(CSLIFEPortIdStatHDRBE);
                        statsPortId = CSLIFEStatsPortIndex(CSLIFEPortIdStatHDRBE);
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = 0;
                        pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stats[statsPortId].width;
                        pStripes[leftIndex].portId = CSLIFEPortIdStatHDRBE;

                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        pStripes[rightIndex].offset = HDRBEStatsMaxWidth;
                        pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stats[statsPortId].width;
                        pStripes[rightIndex].portId = CSLIFEPortIdStatHDRBE;
                        break;
                    case IFEOutputPortStatsBHIST:
                        CAMX_ASSERT(CamX::Format::Blob == pImageFormat->format);
                        portId = CSLIFEPortIndex(CSLIFEPortIdStatBHIST);
                        statsPortId = CSLIFEStatsPortIndex(CSLIFEPortIdStatBHIST);
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = 0;
                        pStripes[leftIndex].width  =
                            m_stripeConfigs[DualIFEStripeIdLeft].stats[statsPortId].width;
                        pStripes[leftIndex].portId = CSLIFEPortIdStatBHIST;

                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        pStripes[rightIndex].offset = BHistStatsWidth;
                        pStripes[rightIndex].width  =
                            m_stripeConfigs[DualIFEStripeIdRight].stats[statsPortId].width;
                        pStripes[rightIndex].portId = CSLIFEPortIdStatBHIST;
                        break;

                    case IFEOutputPortStatsHDRBHIST:
                        CAMX_ASSERT(CamX::Format::Blob == pImageFormat->format);
                        portId = CSLIFEPortIndex(CSLIFEPortIdStatHDRBHIST);
                        statsPortId = CSLIFEStatsPortIndex(CSLIFEPortIdStatHDRBHIST);
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = 0;
                        pStripes[leftIndex].width  =
                            m_stripeConfigs[DualIFEStripeIdLeft].stats[statsPortId].width;
                        pStripes[leftIndex].portId = CSLIFEPortIdStatHDRBHIST;

                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        pStripes[rightIndex].offset = HDRBHistStatsMaxWidth;
                        pStripes[rightIndex].width  =
                            m_stripeConfigs[DualIFEStripeIdRight].stats[statsPortId].width;
                        pStripes[rightIndex].portId = CSLIFEPortIdStatHDRBHIST;
                        break;

                    case IFEOutputPortStatsIHIST:
                        CAMX_ASSERT(CamX::Format::Blob == pImageFormat->format);
                        portId = CSLIFEPortIndex(CSLIFEPortIdStatIHIST);
                        statsPortId = CSLIFEStatsPortIndex(CSLIFEPortIdStatIHIST);
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = 0;
                        pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stats[statsPortId].width;
                        pStripes[leftIndex].portId = CSLIFEPortIdStatIHIST;

                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        pStripes[rightIndex].offset = IHistStatsWidth;
                        pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stats[statsPortId].width;
                        pStripes[rightIndex].portId = CSLIFEPortIdStatIHIST;
                        break;

                    case IFEOutputPortStatsRS:
                        CAMX_ASSERT(CamX::Format::Blob == pImageFormat->format);
                        portId = CSLIFEPortIndex(CSLIFEPortIdStatRS);
                        statsPortId = CSLIFEStatsPortIndex(CSLIFEPortIdStatRS);
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = 0;
                        pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stats[statsPortId].width;
                        pStripes[leftIndex].portId = CSLIFEPortIdStatRS;

                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        pStripes[rightIndex].offset = pStripes[leftIndex].width;
                        pStripes[rightIndex].width  = RSStatsWidth;
                        pStripes[rightIndex].portId = CSLIFEPortIdStatRS;
                        break;

                    case IFEOutputPortStatsCS:
                        CAMX_ASSERT(CamX::Format::Blob == pImageFormat->format);
                        portId = CSLIFEPortIndex(CSLIFEPortIdStatCS);
                        statsPortId = CSLIFEStatsPortIndex(CSLIFEPortIdStatCS);
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = 0;
                        pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stats[statsPortId].width;
                        pStripes[leftIndex].portId = CSLIFEPortIdStatCS;

                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        pStripes[rightIndex].offset = CSStatsWidth;
                        pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stats[statsPortId].width;
                        pStripes[rightIndex].portId = CSLIFEPortIdStatCS;
                        break;

                    case IFEOutputPortStatsBF:
                        CAMX_ASSERT((CamX::Format::Blob == pImageFormat->format) ||
                            (CamX::Format::RawPlain64 == pImageFormat->format));
                        portId = CSLIFEPortIndex(CSLIFEPortIdStatBF);
                        statsPortId = CSLIFEStatsPortIndex(CSLIFEPortIdStatBF);
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = 0;
                        pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stats[statsPortId].width;
                        pStripes[leftIndex].portId = CSLIFEPortIdStatBF;

                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        // To be update by IFE

                        // BF stats's index-based mode uses the same offset for dual IFE
                        pStripes[rightIndex].offset = (ISPHwTitan480 == m_hwMask) ? 0 : BFStatsMaxWidth;
                        pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stats[statsPortId].width;
                        pStripes[rightIndex].portId = CSLIFEPortIdStatBF;
                        break;

                    case IFEOutputPortStatsTLBG:
                        CAMX_ASSERT(CamX::Format::Blob == pImageFormat->format);
                        portId = CSLIFEPortIndex(CSLIFEPortIdStatTintlessBG);
                        statsPortId = CSLIFEStatsPortIndex(CSLIFEPortIdStatTintlessBG);
                        // First stripe
                        leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                        pStripes[leftIndex].offset = 0;
                        pStripes[leftIndex].width  = m_stripeConfigs[DualIFEStripeIdLeft].stats[statsPortId].width;
                        pStripes[leftIndex].portId = CSLIFEPortIdStatTintlessBG;

                        // Second stripe
                        rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                        pStripes[rightIndex].offset = TintlessBGStatsWidth;
                        pStripes[rightIndex].width  = m_stripeConfigs[DualIFEStripeIdRight].stats[statsPortId].width;
                        pStripes[rightIndex].portId = CSLIFEPortIdStatTintlessBG;
                        break;

                    case IFEOutputPortPDAF:
                        portId = CSLIFEPortIndex(CSLIFEPortIdPDAF);
                        if (CamX::Format::RawPlain16 == pImageFormat->format)
                        {
                            // First stripe
                            leftIndex = GetStripeConfigIndex(DualIFEStripeIdLeft, numPorts, portId, 0);
                            pStripes[leftIndex].offset = m_stripeConfigs[0].stream[PDAFRawOutput].offset;
                            pStripes[leftIndex].width  = m_stripeConfigs[0].stream[PDAFRawOutput].width * 2;
                            pStripes[leftIndex].portId = CSLIFEPortIdPDAF;

                            // Second stripe
                            rightIndex = GetStripeConfigIndex(DualIFEStripeIdRight, numPorts, portId, 0);
                            pStripes[rightIndex].offset = m_stripeConfigs[1].stream[PDAFRawOutput].offset * 2;
                            pStripes[rightIndex].width  = m_stripeConfigs[1].stream[PDAFRawOutput].width * 2;
                            pStripes[rightIndex].portId = CSLIFEPortIdPDAF;
                        }
                        break;

                    default:
                        CAMX_NOT_IMPLEMENTED();
                        break;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Output port is not defined");
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "pDualConfig is NULL");
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        result = m_pDualIFEConfigCmdBuffer->CommitCommands();
        CAMX_ASSERT(CamxResultSuccess == result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculateIQCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::CalculateIQCmdSize()
{
    UINT count = 0;

    for (count = 0; count < m_numIFEIQModule; count++)
    {
        IQModuleDualIFEData info = { 0 };
        m_pIFEIQModule[count]->GetDualIFEData(&info);

        m_totalIQCmdSizeDWord += m_pIFEIQModule[count]->GetIQCmdLength();

        m_pIFEIQModule[count]->Set32bitDMIBufferOffset(m_total32bitDMISizeDWord);
        m_total32bitDMISizeDWord += m_pIFEIQModule[count]->Get32bitDMILength();
        m_pIFEIQModule[count]->Set64bitDMIBufferOffset(m_total64bitDMISizeDWord);
        m_total64bitDMISizeDWord += m_pIFEIQModule[count]->Get64bitDMILength();

        if (TRUE == info.dualIFEDMI32Sensitive)
        {
            m_total32bitDMISizeDWord += m_pIFEIQModule[count]->Get32bitDMILength();
        }

        if (TRUE == info.dualIFEDMI64Sensitive)
        {
            m_total64bitDMISizeDWord += m_pIFEIQModule[count]->Get64bitDMILength();
        }
    }

    for (count = 0; count < m_numIFEStatsModule; count++)
    {
        IQModuleDualIFEData info = { 0 };
        m_pIFEStatsModule[count]->GetDualIFEData(&info);

        m_totalIQCmdSizeDWord += m_pIFEStatsModule[count]->GetIQCmdLength();

        m_pIFEStatsModule[count]->Set32bitDMIBufferOffset(m_total32bitDMISizeDWord);
        m_total32bitDMISizeDWord += m_pIFEStatsModule[count]->Get32bitDMILength();
        m_pIFEStatsModule[count]->Set64bitDMIBufferOffset(m_total64bitDMISizeDWord);
        m_total64bitDMISizeDWord += m_pIFEStatsModule[count]->Get64bitDMILength();

        if (TRUE == info.dualIFEDMI32Sensitive)
        {
            m_total32bitDMISizeDWord += m_pIFEStatsModule[count]->Get32bitDMILength();
        }

        if (TRUE == info.dualIFEDMI64Sensitive)
        {
            m_total64bitDMISizeDWord += m_pIFEStatsModule[count]->Get64bitDMILength();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::SetDependencies(
    NodeProcessRequestData* pNodeRequestData,
    BOOL                    hasExplicitDependencies)
{
    UINT32 count = 0;
    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(pNodeRequestData->pCaptureRequest->requestId);


    if (TRUE == hasExplicitDependencies)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE: ProcessRequest: Setting IQ dependencies for Req#%llu",
            pNodeRequestData->pCaptureRequest->requestId);

        if (TRUE == IsTagPresentInPublishList(PropertyIDAECFrameControl))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] = PropertyIDAECFrameControl;
        }
        if (TRUE == IsTagPresentInPublishList(PropertyIDAECStatsControl))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] = PropertyIDAECStatsControl;
        }
        if (TRUE == IsTagPresentInPublishList(PropertyIDAWBFrameControl))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] = PropertyIDAWBFrameControl;
        }
        if (TRUE == IsTagPresentInPublishList(PropertyIDAWBStatsControl))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] = PropertyIDAWBStatsControl;
        }
        if (TRUE == IsTagPresentInPublishList(PropertyIDAFDStatsControl))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] = PropertyIDAFDStatsControl;
        }
        if (TRUE == IsTagPresentInPublishList(PropertyIDPostSensorGainId))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] = PropertyIDPostSensorGainId;
        }
        if (TRUE == IsTagPresentInPublishList(PropertyIDParsedBHistStatsOutput) &&
            (TRUE == m_IFEOutputPathInfo[IFEOutputPortStatsBHIST].path))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] = PropertyIDParsedBHistStatsOutput;
        }

        // AF Stats is not required for Fast AEC,
        // so, skipping dependency check for AF when Fast AEC is enabled
        if (TRUE == IsTagPresentInPublishList(PropertyIDAFStatsControl) &&
            TRUE == IsTagPresentInPublishList(PropertyIDAFFrameControl))
        {
            // Check AF control
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++]  = PropertyIDAFStatsControl;
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++]  = PropertyIDAFFrameControl;

            if (TRUE == IsPDHwEnabled() && (TRUE == IsTagPresentInPublishList(PropertyIDPDHwConfig)))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++]  = PropertyIDPDHwConfig;
            }
        }

        if (TRUE == IsTagPresentInPublishList(PropertyIDParsedTintlessBGStatsOutput) &&
                (TRUE == m_IFEOutputPathInfo[IFEOutputPortStatsTLBG].path))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] =
                PropertyIDParsedTintlessBGStatsOutput;
        }

        if (TRUE == IsTagPresentInPublishList(PropertyIDParsedAWBBGStatsOutput) &&
           (TRUE == m_IFEOutputPathInfo[IFEOutputPortStatsAWBBG].path))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] =
                PropertyIDParsedTintlessBGStatsOutput;
        }

        if (TRUE == IsTagPresentInPublishList(PropertyIDSensorNumberOfLEDs))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] = PropertyIDSensorNumberOfLEDs;
        }

        if (count > 0)
        {
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
        }

    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE: Skip Setting Dependency for ReqID:%llu",
                      pNodeRequestData->pCaptureRequest->requestId);
    }

    // Set a dependency on the completion of the previous ExecuteProcessRequest() call
    // so that we can guarantee serialization of all ExecuteProcessRequest() calls for this node.
    // Needed since the ExecuteProcessRequest() implementation is not reentrant.
    // Remove when requirement CAMX-3030 is implemented.

    // Skip setting dependency for first request
    if (FirstValidRequestId < requestIdOffsetFromLastFlush)
    {
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count] = GetNodeCompleteProperty();
        // Always point to the previous request. Should NOT be tied to the pipeline delay!
        pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[count]     = 1;
        count++;
    }

    pNodeRequestData->dependencyInfo[0].propertyDependency.count                            = count;
    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
    pNodeRequestData->dependencyInfo[0].processSequenceId                                   = 1;
    pNodeRequestData->numDependencyLists                                                    = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CAMX_ASSERT(NULL != pBufferNegotiationData);

    CamxResult      result                          = CamxResultSuccess;
    UINT32          optimalInputWidth               = 0;
    UINT32          optimalInputHeight              = 0;
    UINT32          minInputWidth                   = 0;
    UINT32          minInputHeight                  = 0;
    UINT32          maxInputWidth                   = 0;
    UINT32          maxInputHeight                  = 0;
    UINT32          perOutputPortOptimalWidth       = 0;
    UINT32          perOutputPortOptimalHeight      = 0;
    UINT32          perOutputPortMinWidth           = 0;
    UINT32          perOutputPortMinHeight          = 0;
    UINT32          perOutputPortMaxWidth           = IFEMaxOutputWidthFull * 2;
    UINT32          perOutputPortMaxHeight          = IFEMaxOutputHeight;
    UINT32          totalInputPorts                 = 0;
    AlignmentInfo   alignmentLCM[FormatsMaxPlanes]  = { { 0 } };
    UINT32          inputPortId[MaxDefinedIFEInputPorts];
    UINT32          perOutputRDIPortWidth           = 0;
    UINT32          perOutputRDIPortHeight          = 0;
    FLOAT           optimalAspectRatio              = 0.0f;
    FLOAT           currentPortAspectRatio          = 0.0f;
    UINT32          sensorOutputWidth               = 0;
    UINT32          sensorOutputHeight              = 0;
    FLOAT           previousOptimalAspectRatio      = 0.0f;


    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    // Get Input Port List
    GetAllInputPortIds(&totalInputPorts, &inputPortId[0]);

    // The IFE node will have to loop through all the output ports which are connected to a child node or a HAL target.
    // The input buffer requirement will be the super resolution after looping through all the output ports.
    // The super resolution may have different aspect ratio compared to the original output port aspect ratio, but
    // this will be taken care of by the crop hardware associated with the output port.

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "No of Total output ports notified %d",
                   pBufferNegotiationData->numOutputPortsNotified);

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        UINT                       outputPortIndex            =
            pBufferNegotiationData->pOutputPortNegotiationData[index].outputPortIndex;
        UINT                       outputPortId               = GetOutputPortId(outputPortIndex);

        if ((FALSE == IsStatsOutputPort(outputPortId))           &&
            (TRUE ==  IsPixelOutputPortSourceType(outputPortId)) &&
            (IFEOutputPortPDAF != outputPortId))
        {
            perOutputPortOptimalWidth  = 0;
            perOutputPortOptimalHeight = 0;
            perOutputPortMinWidth      = 0;
            perOutputPortMinHeight     = 0;
            perOutputPortMaxWidth      = IFEMaxOutputWidthFull * 2 ;
            perOutputPortMaxHeight     = IFEMaxOutputHeight;

            // Nag: Check if we still need this
            // Get Max FD Output Width values based on the camera platform.
            UpdateIFECapabilityBasedOnCameraPlatform();

            // FD port has a different limit than the full port.
            if (IFEOutputPortFD == outputPortId)
            {
                perOutputPortMaxWidth = m_maxOutputWidthFD * 2;
            }

            if (TRUE == m_RDIOnlyUseCase)
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "Ignore single IFE output limitatation for RDI only usecase");
                perOutputPortMaxWidth  = IFEMaxOutputWidthRDIOnly;
                perOutputPortMaxHeight = IFEMaxOutputHeightRDIOnly;
            }

            Utils::Memset(&pOutputPortNegotiationData->outputBufferRequirementOptions, 0, sizeof(BufferRequirement));

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "No of Ouputput paths %d for output %dth path",
                           pOutputPortNegotiationData->numInputPortsNotification, index);

            FLOAT portAspectRatio           = 0.0f;
            FLOAT previousPortAspectRatio   = 0.0f;
            FLOAT subPortAspectRatio        = 0.0f;

            // Go through the requirements of the destination ports connected to a given output port of IFE
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

                // if the streams from different nodes connecting to same IFE port differ
                // adjust the aspect ratio to that of sensor active array to capture complete FOV

                // Current stream aspect ratio
                portAspectRatio   = static_cast<FLOAT>(perOutputPortOptimalWidth) / perOutputPortOptimalHeight;
                subPortAspectRatio = static_cast<FLOAT>(pInputPortRequirement->optimalWidth) /
                                                        pInputPortRequirement->optimalHeight;
                if (0 != inputIndex )
                {
                    subPortAspectRatio = static_cast<FLOAT>(pInputPortRequirement->optimalWidth) /
                                                            pInputPortRequirement->optimalHeight;
                    // Compare the current stream aspect ratio with all the previous streams aspect ratio
                    if (FALSE == Utils::FEqualCoarse(subPortAspectRatio, previousPortAspectRatio))
                    {
                        // Update only if the portAspectRatio is different from sensor active array aspect ratio
                        if (FALSE == Utils::FEqualCoarse(portAspectRatio, m_sensorActiveArrayAR))
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Adjust the aspect ratio for IFE port ID %d", outputPortId);
                            if (portAspectRatio > m_sensorActiveArrayAR)
                            {
                                perOutputPortOptimalHeight =
                                    Utils::EvenFloorUINT32(static_cast<UINT32>(perOutputPortOptimalWidth /
                                                                               m_sensorActiveArrayAR));
                            }
                            else
                            {
                                perOutputPortOptimalWidth =
                                    Utils::EvenFloorUINT32(static_cast<UINT32>(perOutputPortOptimalHeight *
                                                                               m_sensorActiveArrayAR));
                            }
                        }
                    }
                }

                // Update the current port AspectRatio to compare with the next stream
                previousPortAspectRatio   = static_cast<FLOAT>(perOutputPortOptimalWidth) / perOutputPortOptimalHeight;

                CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE port ID %d, port index %d, suboutput %d, Optimal[%d *%d],"
                                                "Min[%d * %d], max[%d * %d], suboutput Dimension [%d * %d]",
                               outputPortId, index, inputIndex,
                               perOutputPortOptimalWidth,
                               perOutputPortOptimalHeight,
                               perOutputPortMinWidth,
                               perOutputPortMinHeight,
                               perOutputPortMaxWidth,
                               perOutputPortMaxHeight,
                               pInputPortRequirement->optimalWidth,
                               pInputPortRequirement->optimalHeight);

                if ((IFEOutputPortRDI0 == outputPortId) ||
                    (IFEOutputPortRDI1 == outputPortId) ||
                    (IFEOutputPortRDI2 == outputPortId) ||
                    (IFEOutputPortRDI3 == outputPortId))
                {
                    perOutputRDIPortWidth  = perOutputPortOptimalWidth;
                    perOutputRDIPortHeight = perOutputPortOptimalHeight;
                }
            }

            // Ensure optimal dimensions are within min and max dimensions. There are chances that the optimal dimension goes
            // over the max. Correct for the same.
            UINT32 originalOptimalWidth = perOutputPortOptimalWidth;
            perOutputPortOptimalWidth  =
                Utils::ClampUINT32(perOutputPortOptimalWidth, perOutputPortMinWidth, perOutputPortMaxWidth);

            // Calculate OptimalHeight if witdh is clamped.
            if (originalOptimalWidth != 0)
            {
                perOutputPortOptimalHeight  =
                    Utils::RoundFLOAT(static_cast<FLOAT>(
                        (perOutputPortOptimalWidth * perOutputPortOptimalHeight) / originalOptimalWidth));
            }
            perOutputPortOptimalHeight =
                Utils::ClampUINT32(perOutputPortOptimalHeight, perOutputPortMinHeight, perOutputPortMaxHeight);

            // Store the buffer requirements for this output port which will be reused to set, during forward walk.
            // The values stored here could be final output dimensions unless it is overridden by forward walk.
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth  = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight = perOutputPortOptimalHeight;

            pOutputPortNegotiationData->outputBufferRequirementOptions.minWidth      = perOutputPortMinWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.minHeight     = perOutputPortMinHeight;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxWidth      = perOutputPortMaxWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxHeight     = perOutputPortMaxHeight;

            optimalInputWidth  = Utils::MaxUINT32(perOutputPortOptimalWidth,  optimalInputWidth);
            optimalInputHeight = Utils::MaxUINT32(perOutputPortOptimalHeight, optimalInputHeight);
            minInputWidth      = Utils::MaxUINT32(perOutputPortMinWidth,      minInputWidth);
            minInputHeight     = Utils::MaxUINT32(perOutputPortMinHeight,     minInputHeight);
            maxInputWidth      = Utils::MaxUINT32(perOutputPortMaxWidth,      maxInputWidth);
            maxInputHeight     = Utils::MaxUINT32(perOutputPortMaxHeight,     maxInputHeight);

            // if the streams from different nodes connecting to same IFE port differ
            // adjust the aspect ratio to that of sensor active array to capture complete FOV

            // Current stream aspect ratio
            optimalAspectRatio   = static_cast<FLOAT>(optimalInputWidth) / optimalInputHeight;
            currentPortAspectRatio = static_cast<FLOAT>(perOutputPortOptimalWidth) / perOutputPortOptimalHeight;

            if (0 != index)
            {
                currentPortAspectRatio = static_cast<FLOAT>(perOutputPortOptimalWidth) / perOutputPortOptimalHeight;
                // Compare the current stream aspect ratio with all the previous streams aspect ratio
                if (FALSE == Utils::FEqualCoarse(currentPortAspectRatio, previousOptimalAspectRatio))
                {
                    // Update only if the portAspectRatio is different from sensor active array aspect ratio
                    if (FALSE == Utils::FEqualCoarse(optimalAspectRatio, m_sensorActiveArrayAR))
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Adjust the aspect ratio for optimal Input wxh %d %d port wxh %d %d",
                        optimalInputWidth, optimalInputHeight, perOutputPortOptimalWidth, perOutputPortOptimalHeight);
                        if (optimalAspectRatio > m_sensorActiveArrayAR)
                        {
                            optimalInputHeight =
                                Utils::EvenFloorUINT32(static_cast<UINT32>(optimalInputWidth /
                                                                           m_sensorActiveArrayAR));
                        }
                        else
                        {
                            optimalInputWidth =
                                Utils::EvenFloorUINT32(static_cast<UINT32>(optimalInputHeight *
                                                                           m_sensorActiveArrayAR));
                        }
                    }
                }
            }

            // Update the current port AspectRatio to compare with the next stream
            previousOptimalAspectRatio   = static_cast<FLOAT>(optimalInputWidth) / optimalInputHeight;

        }
        else // Stats and DS ports do not take part in buffer negotiation
        {
            const ImageFormat* pImageFormat = GetOutputPortImageFormat(OutputPortIndex(outputPortId));

            if (NULL != pImageFormat)
            {
                pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth = pImageFormat->width;
                pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight = pImageFormat->height;
            }
        }

        // if the RDI buffer is the target buffer, it supersets all the dimensions
        if ((perOutputRDIPortWidth != 0) && (perOutputRDIPortHeight != 0))
        {
            if (TRUE == IsSinkPortWithBuffer(pOutputPortNegotiationData->outputPortIndex))
            {
                optimalInputWidth  = perOutputRDIPortWidth;
                optimalInputHeight = perOutputRDIPortHeight;
                minInputWidth      = perOutputRDIPortWidth;
                minInputHeight     = perOutputRDIPortHeight;
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Cap IFE dimension to RDI dimension %d x %d",
                                 optimalInputWidth, optimalInputHeight);
                break;
            }
        }
    }

    // update stride and scanline for output ports
    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
        {
            BufferRequirement* pInputPortRequirement = &pOutputPortNegotiationData->inputPortRequirement[inputIndex];
            for (UINT planeIdx = 0; planeIdx < FormatsMaxPlanes; planeIdx++)
            {
                alignmentLCM[planeIdx].strideAlignment =
                    Utils::CalculateLCM(
                        static_cast<INT32>(alignmentLCM[planeIdx].strideAlignment),
                        static_cast<INT32>(pInputPortRequirement->planeAlignment[planeIdx].strideAlignment));
                alignmentLCM[planeIdx].scanlineAlignment =
                    Utils::CalculateLCM(
                        static_cast<INT32>(alignmentLCM[planeIdx].scanlineAlignment),
                        static_cast<INT32>(pInputPortRequirement->planeAlignment[planeIdx].scanlineAlignment));
            }
        }
        Utils::Memcpy(&pOutputPortNegotiationData->outputBufferRequirementOptions.planeAlignment[0],
                      &alignmentLCM[0],
                      sizeof(AlignmentInfo) * FormatsMaxPlanes);

        Utils::Memset(&alignmentLCM[0], 0, sizeof(AlignmentInfo) * FormatsMaxPlanes);
    }

    if ((TRUE == IsTPGMode()) && (optimalInputWidth == 0) && (optimalInputHeight == 0))
    {
        optimalInputWidth   = m_testGenModeData.resolution.outputWidth;
        optimalInputHeight  = m_testGenModeData.resolution.outputHeight;
        CAMX_LOG_INFO(CamxLogGroupISP, "IFE: TPG MODE: Width x Height = %d x %d", optimalInputWidth, optimalInputHeight);
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
            CAMX_LOG_WARN(CamxLogGroupISP, "Min > Max, unable to use current format");
            result = CamxResultEFailed;
        }

        // Ensure optimal dimensions are within min and max dimensions. There are chances that the optimal dimension goes
        // over the max. Correct for the same.
        optimalInputWidth  = Utils::ClampUINT32(optimalInputWidth,  minInputWidth,  maxInputWidth);
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
            // If IPE is enabling SIMO and if one of the output is smaller than the other, then the scale capabilities (min,max)
            // needs to be adjusted after accounting for the scaling needed on the smaller output port.
            pInputBufferRequirement->minWidth      = minInputWidth;
            pInputBufferRequirement->minHeight     = minInputHeight;
            pInputBufferRequirement->maxWidth      = maxInputWidth;
            pInputBufferRequirement->maxHeight     = maxInputHeight;

            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                             "Buffer Negotiation dims, Port %d Optimal [%d x %d], Min [%d x %d], Max [%d x %d]\n",
                             inputPortId[input],
                             optimalInputWidth,
                             optimalInputHeight,
                             minInputWidth,
                             minInputHeight,
                             maxInputWidth,
                             maxInputHeight);
        }
    }

    if (CamxResultSuccess == result)
    {
        result = SetupICAGrid();
        m_inputBufferRequirement.optimalWidth  = optimalInputWidth;
        m_inputBufferRequirement.optimalHeight = optimalInputHeight;
        m_inputBufferRequirement.minWidth      = minInputWidth;
        m_inputBufferRequirement.minHeight     = minInputHeight;
        m_inputBufferRequirement.maxWidth      = maxInputWidth;
        m_inputBufferRequirement.maxHeight     = maxInputHeight;
        m_pBufferNegotiationData               = pBufferNegotiationData;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ExtractCAMIFDecimatedPattern
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::ExtractCAMIFDecimatedPattern(
    UINT32             horizontalOffset,
    UINT32             verticalOffset,
    PDLibBlockPattern* pBlockPattern)
{
    IFECAMIFPixelExtractionData newPdafPixelcoordinates[IFEMaxPdafPixelsPerBlock];
    IFECAMIFPixelExtractionData subPdafPixelcoordinates[IFEMaxPdafPixelsPerBlock];
    UINT32                      pdafPixelCount = 0;
    UINT16                      pixelSkip = 0;
    UINT16                      lineSkip  = 0;
    UINT32                      subBoxWidth = 0;
    UINT32                      subBoxHeight = 0;
    UINT32                      offsetSumX = 0;
    UINT32                      offsetSumY = 0;
    UINT32                      offsetBlockX = 0;
    UINT32                      offsetBlockY = 0;
    UINT32                      offsetExtractX = 0;
    UINT32                      offsetExtractY = 0;
    UINT32                      constX = 0;
    UINT32                      constY = 0;

    pdafPixelCount = m_ISPInputSensorData.sensorPDAFInfo.PDAFPixelCount;

    // CAMIF pixel and line skip operate on 32 pixels and 32 lines at a time
    // Hence 32 x 32 block is considered when subsampiling enabled

    constX = m_ISPInputSensorData.sensorPDAFInfo.PDAFBlockWidth / 16;
    constY = m_ISPInputSensorData.sensorPDAFInfo.PDAFBlockHeight / 16;

    for (UINT32 index = 0; index < pdafPixelCount; index++)
    {
        newPdafPixelcoordinates[index].xCordrinate =
            (m_ISPInputSensorData.sensorPDAFInfo.PDAFPixelCoords[index].PDXCoordinate) %
            m_ISPInputSensorData.sensorPDAFInfo.PDAFBlockWidth;
        newPdafPixelcoordinates[index].yCordrinate =
            (m_ISPInputSensorData.sensorPDAFInfo.PDAFPixelCoords[index].PDYCoordinate) %
            m_ISPInputSensorData.sensorPDAFInfo.PDAFBlockHeight;

        // Index of ecah 32x32 block
        newPdafPixelcoordinates[index].blockX = newPdafPixelcoordinates[index].xCordrinate / 16;
        newPdafPixelcoordinates[index].blockY = newPdafPixelcoordinates[index].yCordrinate / 16;

        // Mod by 32 to fit CAMIF subsampling pattern
        newPdafPixelcoordinates[index].xCordrinate = newPdafPixelcoordinates[index].xCordrinate % 16;
        newPdafPixelcoordinates[index].yCordrinate = newPdafPixelcoordinates[index].yCordrinate % 16;

        lineSkip |= ((static_cast<UINT16>(1)) << (15 - newPdafPixelcoordinates[index].yCordrinate));
        pixelSkip |= ((static_cast<UINT16>(1)) << (15 - newPdafPixelcoordinates[index].xCordrinate));
    }

    /* Sub Sampled Block Size*/
    subBoxWidth  = GetPixelsInSkipPattern(pixelSkip);
    subBoxHeight = GetPixelsInSkipPattern(lineSkip);

    for (UINT32 index = 0; index < pdafPixelCount; index++)
    {
        subPdafPixelcoordinates[index].xCordrinate =
            GetPixelsInSkipPattern(pixelSkip >> (15 - newPdafPixelcoordinates[index].xCordrinate)) - 1;
        subPdafPixelcoordinates[index].xCordrinate +=
            newPdafPixelcoordinates[index].blockX * subBoxWidth;
        subPdafPixelcoordinates[index].yCordrinate =
            GetPixelsInSkipPattern(lineSkip >> (15 - newPdafPixelcoordinates[index].yCordrinate)) - 1;
        subPdafPixelcoordinates[index].yCordrinate +=
            newPdafPixelcoordinates[index].blockY * subBoxHeight;
    }

    offsetBlockX = (horizontalOffset / 16) * subBoxWidth;
    offsetBlockY = (verticalOffset / 16) * subBoxHeight;

    if ((horizontalOffset % 16) > 0)
    {
        offsetExtractX = GetPixelsInSkipPattern(pixelSkip >> (16 - (horizontalOffset % 16)));
    }

    if ((verticalOffset % 16) > 0)
    {
        offsetExtractY = GetPixelsInSkipPattern(lineSkip >> (16 - (verticalOffset % 16)));
    }

    offsetSumX = offsetBlockX + offsetExtractX;
    offsetSumY = offsetBlockY + offsetExtractY;

    pBlockPattern->horizontalPDOffset = offsetSumX;
    pBlockPattern->verticalPDOffset   = offsetSumY;
    pBlockPattern->blockDimension.width = subBoxWidth * constX;
    pBlockPattern->blockDimension.height = subBoxHeight * constY;
    pBlockPattern->pixelCount = pdafPixelCount;

    for (UINT index = 0; index < pdafPixelCount; index++)
    {
        pBlockPattern->pixelCoordinate[index].x =
            (subPdafPixelcoordinates[index].xCordrinate + (1024 * subBoxWidth * constX) - offsetSumX) %
            (subBoxWidth * constX);
        pBlockPattern->pixelCoordinate[index].y =
            (subPdafPixelcoordinates[index].yCordrinate + (1024 * subBoxHeight * constY) - offsetSumY) %
            (subBoxHeight * constY);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFENode::PublishIFEOutputToUsecasePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PublishIFEOutputToUsecasePool(
    IFEOutputResolution* pIFEResolution)
{
    MetadataPool*           pPerUsecasePool = GetPerFramePool(PoolType::PerUsecase);
    MetadataSlot*           pPerUsecaseSlot = pPerUsecasePool->GetSlot(0);
    UsecasePropertyBlob*    pPerUsecaseBlob = NULL;
    CamxResult              result          = CamxResultSuccess;

    const UINT  IFEResolutionTag[]          = { PropertyIDUsecaseIFEOutputResolution };
    const VOID* pData[1]                    = { pIFEResolution };
    UINT        pDataCount[1]               = { sizeof(IFEOutputResolution) };

    result = WriteDataList(IFEResolutionTag, pData, pDataCount, 1);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupFD, "Falied to publish IFE Output uscasepool");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PreparePDAFInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PreparePDAFInformation()
{
    UINT16 pixelSkipPattern = 0;
    UINT16 lineSkipPattern = 0;
    UINT32 numberOfPixels = 0;
    UINT32 numberOfLines = 0;
    UINT32 pdafPixelCount = m_ISPInputSensorData.sensorPDAFInfo.PDAFPixelCount;
    PDLibDataBufferInfo PDAFBufferInfo;

    // This function is to post the IFE Output buffer characteristics to PDLIB
    // Based on the IFE Pixel and LIne skip pattern we will end up having PD Pixels
    // and Non PD Pixels in the output buffer. This funcation calculates the exact
    // coordinates of extracted PD Pixels with respect to the Output buffer
    Utils::Memset(&PDAFBufferInfo, 0, sizeof(PDLibDataBufferInfo));

    PDAFBufferInfo.imageOverlap     = 0;
    PDAFBufferInfo.bufferFormat     = PDLibBufferFormat::PDLibBufferFormatUnpacked16;
    PDAFBufferInfo.sensorType       = PDLibSensorType::PDLibSensorType3;
    PDAFBufferInfo.isp1BufferWidth  = m_PDAFInfo.bufferWidth;
    PDAFBufferInfo.isp1BufferStride = m_PDAFInfo.alignedBufferWidth * 2;
    PDAFBufferInfo.ispBufferHeight  = m_PDAFInfo.bufferHeight;

    ExtractCAMIFDecimatedPattern(m_ISPInputSensorData.sensorPDAFInfo.PDAFGlobaloffsetX,
                                 m_ISPInputSensorData.sensorPDAFInfo.PDAFGlobaloffsetY,
                                 &PDAFBufferInfo.isp1BlockPattern);
    // Publish IFE PDAF Buffer Information
    const UINT  outputTags[] = { PropertyIDUsecaseIFEPDAFInfo };
    const VOID* pOutputData[1] = { 0 };
    UINT        pDataCount[1] = { 0 };
    pDataCount[0] = sizeof(PDLibDataBufferInfo);
    pOutputData[0] = &PDAFBufferInfo;
    WriteDataList(outputTags, pOutputData, pDataCount, 1);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::FetchSensorInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::FetchSensorInfo()
{

    CamxResult result = CamxResultSuccess;

    // Update the sensor mode data from input metadata
    if (IFEProfileIdOffline == m_instanceProperty.profileId)
    {
        if (NULL == m_pSensorModeData)
        {
            UINT       metaTag   = 0;
            UINT       modeIndex = 0;

            result = VendorTagManager::QueryVendorTagLocation("com.qti.sensorbps", "mode_index", &metaTag);

            metaTag |= InputMetadataSectionMask;

            UINT              offlineSensorProperties[] = { PropertyIDUsecaseSensorModes, metaTag };
            static const UINT Length          = CAMX_ARRAY_SIZE(offlineSensorProperties);
            VOID*             pData[Length]   = { 0 };
            UINT64            offsets[Length] = { 0, 0 };

            GetDataList(offlineSensorProperties, pData, offsets, Length);

            // Read the sensor index info
            if (NULL != pData[1])
            {
                modeIndex = *reinterpret_cast<UINT*>(pData[1]);
                CAMX_LOG_INFO(CamxLogGroupISP, "Sensor mode index = %d, for offline IFE", modeIndex);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "Sensor mode index vendor tag not provided for offline IFE! Using default 0!");
            }

            // Read the sensor mode data based on index
            if (NULL != pData[0])
            {
                m_pSensorModeData = &(reinterpret_cast<UsecaseSensorModes*>(pData[0])->allModes[modeIndex]);
                GetSensorModeRes0Data(&m_pSensorModeRes0Data);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "Usecase pool did not contain sensor modes. Going to fault");
            }

            m_currentSensorModeSupportPDAF = FALSE;
            m_currentSensorModeSupportHDR  = FALSE;
            m_currentSensorModeSupportMeta = FALSE;
        }
    }
    else
    {
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

                GetSensorModeRes0Data(&m_pSensorModeRes0Data);

                if (NULL == m_pSensorModeData)
                {
                    result = CamxResultEInvalidState;
                    CAMX_LOG_ERROR(CamxLogGroupISP, "m_pSensorModeData is NULL.");
                }
                else if ((0 == m_pSensorModeData->resolution.outputWidth) || (0 == m_pSensorModeData->resolution.outputHeight))
                {
                    result = CamxResultEInvalidState;
                    CAMX_LOG_ERROR(CamxLogGroupISP,
                                   "Invalid sensor resolution, W:%d x H:%d!\n",
                                   m_pSensorModeData->resolution.outputWidth,
                                   m_pSensorModeData->resolution.outputHeight);
                }
            }

            m_currentSensorModeSupportPDAF = FindSensorStreamConfigIndex(StreamType::PDAF, NULL);
            m_currentSensorModeSupportHDR  = FindSensorStreamConfigIndex(StreamType::HDR, NULL);
            m_currentSensorModeSupportMeta = FindSensorStreamConfigIndex(StreamType::META, NULL);
        }

        // Update Sensor OTP data
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

        CAMX_LOG_INFO(CamxLogGroupISP, "m_currentSensorModeSupportPDAF = %u", m_currentSensorModeSupportPDAF);
        CAMX_LOG_INFO(CamxLogGroupISP, "m_currentSensorModeSupportHDR = %u", m_currentSensorModeSupportHDR);
        CAMX_LOG_INFO(CamxLogGroupISP, "m_currentSensorModeSupportMeta = %u", m_currentSensorModeSupportMeta);

        // Gets the PDAF Information from Sensor
        GetPDAFInformation();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    UINT32 finalSelectedOutputWidth     = 0;
    UINT32 finalSelectedOutputHeight    = 0;

    UINT32 fullPortOutputWidth          = 0;
    UINT32 fullPortOutputHeight         = 0;
    UINT32 displayFullPortOutputWidth   = 0;
    UINT32 displayFullPortOutputHeight  = 0;

    UINT16 pixelSkipPattern             = 0;
    UINT16 lineSkipPattern              = 0;
    UINT32 numberOfPixels               = 0;
    UINT32 numberOfLines                = 0;
    UINT32 pdafPixelCount               = 0;
    UINT32 residualWidth                = 0;
    UINT32 residualHeight               = 0;
    UINT16 residualWidthPattern         = 0;
    UINT16 residualHeightPattern        = 0;
    UINT16 offsetX                      = 0;
    UINT16 offsetY                      = 0;
    FLOAT  perOutputPortAspectRatio     = 0.0f;
    FLOAT  inputSensorAspectRatio       = 0.0f;
    FLOAT  curStreamAspectRatio         = 0.0f;
    UINT32 perOutputPortOptimalWidth    = 0;
    UINT32 perOutputPortOptimalHeight   = 0;
    UINT32 perOutputPortMinWidth        = 0;
    UINT32 perOutputPortMinHeight       = 0;
    UINT32 perOutputPortMaxWidth        = IFEMaxOutputWidthFull * 2;  // Since for DUAL IFE , Max width port width * 2
    UINT32 perOutputPortMaxHeight       = IFEMaxOutputHeight;
    UINT32 finalSelectedInputWidth      = 0;
    UINT32 finalSelectedInputHeight     = 0;

    InputPortNegotiationData* pInputPortNegotiationData = &pBufferNegotiationData->pInputPortNegotiationData[0];

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    // fetch the sensor mode data and PDAF info
    FetchSensorInfo();

    if (IFEProfileIdOffline == m_instanceProperty.profileId)
    {
        finalSelectedInputWidth  = pInputPortNegotiationData->pImageFormat->width;
        finalSelectedInputHeight = pInputPortNegotiationData->pImageFormat->height;

        if (((m_pSensorModeData->cropInfo.lastPixel - m_pSensorModeData->cropInfo.firstPixel + 1) != finalSelectedInputWidth) ||
            ((m_pSensorModeData->cropInfo.lastLine - m_pSensorModeData->cropInfo.firstLine + 1)    != finalSelectedInputHeight))
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Mismatch between input [%d * %d] and sensor dimensions[%d * %d]",
                          finalSelectedInputWidth,
                          finalSelectedInputHeight,
                          (m_pSensorModeData->cropInfo.lastPixel - m_pSensorModeData->cropInfo.firstPixel + 1),
                          (m_pSensorModeData->cropInfo.lastLine - m_pSensorModeData->cropInfo.firstLine + 1));
        }

        // If the sensor output is YUV, adjust the width
        if (Format::YUV422NV16 == pInputPortNegotiationData->pImageFormat->format)
        {
            finalSelectedInputWidth >>= 1;
        }
    }
    else
    {
        finalSelectedInputWidth = m_pSensorModeData->cropInfo.lastPixel - m_pSensorModeData->cropInfo.firstPixel + 1;
        finalSelectedInputHeight = m_pSensorModeData->cropInfo.lastLine - m_pSensorModeData->cropInfo.firstLine + 1;

        // CSID crop override
        if (TRUE == EnableCSIDCropOverridingForSingleIFE())
        {
            finalSelectedInputWidth  = m_instanceProperty.IFECSIDWidth;
            if (TRUE == IsSensorModeFormatYUV(m_pSensorModeData->format))
            {
                finalSelectedInputWidth >>= 1;
            }
            finalSelectedInputHeight = m_instanceProperty.IFECSIDHeight;

            CAMX_LOG_INFO(CamxLogGroupISP, "CSID crop override: finalSelectedInputWidth = %d, finalSelectedInputHeight = %d",
                          finalSelectedInputWidth, finalSelectedInputHeight);
        }
    }

    if (TRUE == m_csidBinningInfo.isBinningEnabled)
    {
        finalSelectedInputWidth     >>= 1;
        finalSelectedInputHeight    >>= 1;
    }

    CAMX_LOG_INFO(CamxLogGroupISP, "finalSelectedInputWidth = %d, finalSelectedInputHeight = %d",
                  finalSelectedInputWidth, finalSelectedInputHeight);

    // Calculate input aspect ratio
    if (0 != finalSelectedInputHeight)
    {
        inputSensorAspectRatio = static_cast<FLOAT>(finalSelectedInputWidth) /
            static_cast<FLOAT>(finalSelectedInputHeight);
    }

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        UINT                       outputPortIndex =
            pBufferNegotiationData->pOutputPortNegotiationData[index].outputPortIndex;
        UINT                       outputPortId = GetOutputPortId(outputPortIndex);
        FLOAT newOutputPortAspectRatio = 0.0f;

        if ((FALSE == IsStatsOutputPort(outputPortId)) &&
             (TRUE == IsPixelOutputPortSourceType(outputPortId)))
        {
            perOutputPortOptimalWidth  = 0;
            perOutputPortOptimalHeight = 0;
            perOutputPortMinWidth      = 0;
            perOutputPortMinHeight     = 0;
            perOutputPortMaxWidth      = IFEMaxOutputWidthFull * 2;
            perOutputPortMaxHeight     = IFEMaxOutputHeight;
            perOutputPortAspectRatio   = 0.0f;


            // FD port has a different limit than the full port.
            if (IFEOutputPortFD == outputPortId)
            {
                perOutputPortMaxWidth = IFEMaxOutputWidthFD * 2;
            }

            if (TRUE == m_RDIOnlyUseCase)
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "Ignore single IFE output limitatation for RDI only usecase");
                perOutputPortMaxWidth  = IFEMaxOutputWidthRDIOnly;
                perOutputPortMaxHeight = IFEMaxOutputHeightRDIOnly;
            }

            // Go through the requirements of the destination ports connected to a given output port of IFE
            for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
            {
                BufferRequirement* pInputPortRequirement = &pOutputPortNegotiationData->inputPortRequirement[inputIndex];

                // Optimal width per port is the super resolution of all the connected destination ports' optimal needs.
                perOutputPortOptimalWidth =
                    Utils::MaxUINT32(pInputPortRequirement->optimalWidth, perOutputPortOptimalWidth);
                perOutputPortOptimalHeight =
                    Utils::MaxUINT32(pInputPortRequirement->optimalHeight, perOutputPortOptimalHeight);

                if (0 != pInputPortRequirement->optimalHeight)
                {
                    curStreamAspectRatio = static_cast<FLOAT>(pInputPortRequirement->optimalWidth) /
                        static_cast<FLOAT>(pInputPortRequirement->optimalHeight);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupISP, "pInputPortRequirement->optimalHeightis 0");
                }
                // Get new OutputPortAspectRatio because perOutputPortOptimalWidth/Height may be modified,
                // this may not be standard ratio, just for further checking
                if (0 != perOutputPortOptimalHeight)
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
                    adjustAspectRatio = inputSensorAspectRatio > adjustAspectRatio ?
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
            perOutputPortOptimalWidth =
                Utils::ClampUINT32(perOutputPortOptimalWidth, perOutputPortMinWidth, perOutputPortMaxWidth);
            perOutputPortOptimalHeight =
                Utils::ClampUINT32(perOutputPortOptimalHeight, perOutputPortMinHeight, perOutputPortMaxHeight);

            // Store the buffer requirements for this output port which will be reused to set, during forward walk.
            // The values stored here could be final output dimensions unless it is overridden by forward walk.
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight = perOutputPortOptimalHeight;
        }
    }

    // Find Full port dimensions, for DS port dimension determination
    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        UINT                       outputPortId    = GetOutputPortId(pOutputPortData->outputPortIndex);

        if (IFEOutputPortFull == outputPortId)
        {
            fullPortOutputWidth  = pOutputPortData->outputBufferRequirementOptions.optimalWidth;
            fullPortOutputHeight = pOutputPortData->outputBufferRequirementOptions.optimalHeight;

            if (TRUE == pStaticSettings->capResolutionForSingleIFE)
            {
                if (fullPortOutputWidth > (IFEMaxOutputWidthFull))
                {
                    fullPortOutputWidth = IFEMaxOutputWidthFull;
                }
            }

            CAMX_LOG_VERBOSE(CamxLogGroupISP, " Found full port on index %d dims %dx%d",
                                 index, fullPortOutputWidth, fullPortOutputHeight);

            /// Clip if output greater than input since IFE cannot do upscale.
            if (fullPortOutputWidth > finalSelectedInputWidth)
            {
                fullPortOutputWidth = finalSelectedInputWidth;

                CAMX_LOG_INFO(CamxLogGroupISP, "outputPortId = %d, Output buffer clipped width = %d",
                    outputPortId, fullPortOutputWidth);
            }

            /// Clip if output greater than input, clamp since IFE cannot do upscale.
            if (fullPortOutputHeight > finalSelectedInputHeight)
            {
                fullPortOutputHeight = finalSelectedInputHeight;

                CAMX_LOG_INFO(CamxLogGroupISP, "outputPortId = %d, Output buffer clipped height = %d",
                    outputPortId, fullPortOutputHeight);
            }

            /// If downscale ratio is beyond ife limits, cap the output dimension.
            if ((finalSelectedInputWidth / fullPortOutputWidth) > IFEMaxDownscaleLimt)
            {
                fullPortOutputWidth = static_cast<UINT32>(finalSelectedInputWidth * IFEMaxDownscaleLimt);
                CAMX_LOG_ERROR(CamxLogGroupISP, "Scaleratio beyond limit, input height = %u, output height = %u",
                    finalSelectedInputWidth, fullPortOutputWidth);
            }
            if (finalSelectedInputHeight / fullPortOutputHeight > IFEMaxDownscaleLimt)
            {
                fullPortOutputHeight = static_cast<UINT32>(finalSelectedInputHeight * IFEMaxDownscaleLimt);
                CAMX_LOG_ERROR(CamxLogGroupISP, "Scaleratio beyond limit, input height = %u, output height = %u",
                    finalSelectedInputHeight, fullPortOutputHeight);
            }

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Found full port on index %d adj dims %dx%d",
                                 index, fullPortOutputWidth, fullPortOutputHeight);
        }

        if (IFEOutputPortDisplayFull == outputPortId)
        {
            displayFullPortOutputWidth  = pOutputPortData->outputBufferRequirementOptions.optimalWidth;
            displayFullPortOutputHeight = pOutputPortData->outputBufferRequirementOptions.optimalHeight;

            if (TRUE == pStaticSettings->capResolutionForSingleIFE)
            {
                if (displayFullPortOutputWidth > (IFEMaxOutputWidthFull))
                {
                    displayFullPortOutputWidth = IFEMaxOutputWidthFull;
                }
            }

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Found display full port on index %d dims %dx%d",
                                 index, displayFullPortOutputWidth, displayFullPortOutputHeight);

            /// Clip if output greater than input since IFE cannot do upscale.
            if (displayFullPortOutputWidth > finalSelectedInputWidth)
            {
                displayFullPortOutputWidth = finalSelectedInputWidth;

                CAMX_LOG_INFO(CamxLogGroupISP, "outputPortId = %d, Output buffer clipped width = %d",
                    outputPortId, displayFullPortOutputWidth);
            }

            /// Clip if output greater than input, clamp since IFE cannot do upscale.
            if (displayFullPortOutputHeight > finalSelectedInputHeight)
            {
                displayFullPortOutputHeight = finalSelectedInputHeight;

                CAMX_LOG_INFO(CamxLogGroupISP, "outputPortId = %d, Output buffer clipped height = %d",
                    outputPortId, displayFullPortOutputHeight);
            }

            /// If downscale ratio is beyond ife limits, cap the output dimension.
            if ((finalSelectedInputWidth / displayFullPortOutputWidth) > IFEMaxDownscaleLimt)
            {
                displayFullPortOutputWidth = static_cast<UINT32>(finalSelectedInputWidth * IFEMaxDownscaleLimt);
                CAMX_LOG_ERROR(CamxLogGroupISP, "Scaleratio beyond limit, input height = %u, output height = %u",
                    finalSelectedInputWidth, displayFullPortOutputWidth);
            }
            if (finalSelectedInputHeight / displayFullPortOutputHeight > IFEMaxDownscaleLimt)
            {
                displayFullPortOutputHeight = static_cast<UINT32>(finalSelectedInputHeight * IFEMaxDownscaleLimt);
                CAMX_LOG_ERROR(CamxLogGroupISP, "Scaleratio beyond limit, input height = %u, output height = %u",
                    finalSelectedInputHeight, displayFullPortOutputHeight);
            }

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Found display full port on index %d adj dims %dx%d",
                                 index, displayFullPortOutputWidth, displayFullPortOutputHeight);
        }
    }

    m_IFEOutputResolution.fullPortEnable              = (fullPortOutputWidth > 0 && fullPortOutputHeight > 0);
    m_IFEOutputResolution.fullPortDimension.width     = fullPortOutputWidth;
    m_IFEOutputResolution.fullPortDimension.height    = fullPortOutputHeight;
    m_IFEOutputResolution.displayPortEnable           = (displayFullPortOutputWidth > 0 && displayFullPortOutputHeight > 0);
    m_IFEOutputResolution.displayPortDimension.width  = displayFullPortOutputWidth;
    m_IFEOutputResolution.displayPortDimension.height = displayFullPortOutputHeight;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "fullPortDemension(%d x %d) displayPortDemension(%d x %d)",
        m_IFEOutputResolution.fullPortDimension.width,
        m_IFEOutputResolution.fullPortDimension.height,
        m_IFEOutputResolution.displayPortDimension.width,
        m_IFEOutputResolution.displayPortDimension.height);

    PublishIFEOutputToUsecasePool(&m_IFEOutputResolution);

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        BufferProperties*          pOutputBufferProperties    = pOutputPortNegotiationData->pFinalOutputBufferProperties;
        UINT                       outputPortId               = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);

        // Assume only singleIFE, cap output buffer to IFE limitation
        if (TRUE == pStaticSettings->capResolutionForSingleIFE)
        {
            if (pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth > (IFEMaxOutputWidthFull))
            {
                pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth = IFEMaxOutputWidthFull;
            }
        }
        if ((IFEOutputPortFD == outputPortId) &&
            (pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth > (m_maxOutputWidthFD)))
        {
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth = m_maxOutputWidthFD;
        }
        if ((IFEOutputPortFD == outputPortId) &&
            (pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight > (m_maxOutputHeightFD)))
        {
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight  = m_maxOutputHeightFD;
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
            (FALSE == IsDSOutputPort(outputPortId)) &&
            (TRUE  == IsPixelOutputPortSourceType(outputPortId)))
        {
            /// If downscale ratio is beyond ife limits, cap the output dimension.
            if (0 != finalSelectedOutputWidth)
            {
                if ((finalSelectedInputWidth / finalSelectedOutputWidth) > IFEMaxDownscaleLimt)
                {
                    finalSelectedOutputWidth = static_cast<UINT32>(finalSelectedInputWidth * IFEMaxDownscaleLimt);
                    CAMX_LOG_WARN(CamxLogGroupISP, "Scaleratio beyond limit, inp height = %d, out height = %d",
                        finalSelectedInputWidth, finalSelectedOutputWidth);
                }
            }
            if (0 != finalSelectedOutputHeight)
            {
                if (finalSelectedInputHeight / finalSelectedOutputHeight > IFEMaxDownscaleLimt)
                {
                    finalSelectedOutputHeight = static_cast<UINT32>(finalSelectedInputHeight * IFEMaxDownscaleLimt);
                    CAMX_LOG_WARN(CamxLogGroupISP, "Scaleratio beyond limit, inp height = %d, out height = %d",
                        finalSelectedInputHeight, finalSelectedOutputHeight);
                }
            }
        }
        UINT outputPortSourceTypeId = GetOutputPortSourceType(pOutputPortNegotiationData->outputPortIndex);
        BOOL shouldNegotiate        = TRUE;

        if (TRUE == IsSinkPortWithBuffer(pOutputPortNegotiationData->outputPortIndex))
        {
            auto* pOptions  = &pOutputPortNegotiationData->outputBufferRequirementOptions;
            shouldNegotiate = (TRUE == ((pOptions->maxWidth != pOptions->optimalWidth) ||
                                        (pOptions->minWidth != pOptions->optimalWidth)));
        }

        if (TRUE == shouldNegotiate)
        {
            switch (outputPortId)
            {
                case IFEOutputPortFull:
                case IFEOutputPortDisplayFull:
                case IFEOutputPortFD:
                    pOutputBufferProperties->imageFormat.width  = finalSelectedOutputWidth;
                    pOutputBufferProperties->imageFormat.height = finalSelectedOutputHeight;

                    m_ifeOutputImageSize.widthPixels = finalSelectedOutputWidth;
                    m_ifeOutputImageSize.heightLines = finalSelectedOutputHeight;

                    Utils::Memcpy(&pOutputBufferProperties->imageFormat.planeAlignment[0],
                                  &pOutputPortNegotiationData->outputBufferRequirementOptions.planeAlignment[0],
                                  sizeof(AlignmentInfo) * FormatsMaxPlanes);
                    break;

                case IFEOutputPortDS4:
                    pOutputBufferProperties->imageFormat.width =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(fullPortOutputWidth, DS4Factor) / DS4Factor);
                    pOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(fullPortOutputHeight, DS4Factor) / DS4Factor);
                    break;

                case IFEOutputPortDS16:
                    pOutputBufferProperties->imageFormat.width =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(fullPortOutputWidth, DS16Factor) / DS16Factor);
                    pOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(fullPortOutputHeight, DS16Factor) / DS16Factor);
                    break;

                case IFEOutputPortDisplayDS4:
                    pOutputBufferProperties->imageFormat.width =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(displayFullPortOutputWidth, DS4Factor) / DS4Factor);
                    pOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(displayFullPortOutputHeight, DS4Factor) / DS4Factor);
                    break;

                case IFEOutputPortDisplayDS16:
                    pOutputBufferProperties->imageFormat.width =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(displayFullPortOutputWidth, DS16Factor) / DS16Factor);
                    pOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(
                            Utils::AlignGeneric32(displayFullPortOutputHeight, DS16Factor) / DS16Factor);
                    break;

                /// @note * 2 below for stats is for handling dual IFE (conservatively). This can be optimized.
                case IFEOutputPortStatsIHIST:
                    pOutputBufferProperties->imageFormat.width  = IHistStatsWidth * 2;
                    pOutputBufferProperties->imageFormat.height = IHistStatsHeight;
                    break;

                case IFEOutputPortStatsHDRBE:
                    pOutputBufferProperties->imageFormat.width  = HDRBEStatsMaxWidth * 2;
                    pOutputBufferProperties->imageFormat.height = HDRBEStatsMaxHeight;
                    break;

                case IFEOutputPortStatsHDRBHIST:
                    pOutputBufferProperties->imageFormat.width  = HDRBHistStatsMaxWidth * 2;
                    pOutputBufferProperties->imageFormat.height = HDRBHistStatsMaxHeight;
                    break;

                case IFEOutputPortStatsAWBBG:
                    pOutputBufferProperties->imageFormat.width  = AWBBGStatsMaxWidth * 2;
                    pOutputBufferProperties->imageFormat.height = AWBBGStatsMaxHeight;
                    break;

                case IFEOutputPortStatsTLBG:
                    pOutputBufferProperties->imageFormat.width  = TintlessBGStatsWidth * 2;
                    pOutputBufferProperties->imageFormat.height = TintlessBGStatsHeight;
                    break;

                case IFEOutputPortStatsBHIST:
                    pOutputBufferProperties->imageFormat.width  = BHistStatsWidth * 2;
                    pOutputBufferProperties->imageFormat.height = 1;
                    break;

                case IFEOutputPortStatsRS:
                    pOutputBufferProperties->imageFormat.width  = RSStatsWidth * 2;
                    pOutputBufferProperties->imageFormat.height = RSStatsHeight;
                    break;

                case IFEOutputPortStatsCS:
                    pOutputBufferProperties->imageFormat.width  = CSStatsWidth * 2;
                    pOutputBufferProperties->imageFormat.height = CSStatsHeight;
                    break;

                case IFEOutputPortStatsBF:

                    if (m_hwMask == ISPHwTitan480)
                    {
                        pOutputBufferProperties->imageFormat.width  = BFStats25Width;
                        pOutputBufferProperties->imageFormat.height = BFStats25Height;
                        pOutputBufferProperties->imageFormat.format = Format::RawPlain64;
                    }
                    else
                    {
                        pOutputBufferProperties->imageFormat.width  = BFStatsMaxWidth * 2;
                        pOutputBufferProperties->imageFormat.height = BFStatsMaxHeight;
                    }
                    break;

                // For RDI output port cases, set the output buffer dimension to the IFE input buffer dimension.
                case IFEOutputPortRDI0:
                case IFEOutputPortRDI1:
                case IFEOutputPortRDI2:
                case IFEOutputPortRDI3:
                case IFEOutputPortStatsDualPD:
                    pOutputBufferProperties->imageFormat.width  = finalSelectedInputWidth;
                    pOutputBufferProperties->imageFormat.height = finalSelectedInputHeight;

                    if (IFEProfileIdOffline == m_instanceProperty.profileId)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported output path in Offline mode");
                    }
                    else
                    {
                        // Overwrite if the RDI port is associated with PDAF port source type.
                        if ((PortSrcTypePDAF == outputPortSourceTypeId) && (FALSE == GetStaticSettings()->disablePDAF))
                        {
                            UINT streamIndex;
                            if (TRUE == FindSensorStreamConfigIndex(StreamType::PDAF, &streamIndex))
                            {
                                UINT32 PDAFWidth = m_pSensorModeData->streamConfig[streamIndex].frameDimension.width;
                                UINT32 PDAFHeight = m_pSensorModeData->streamConfig[streamIndex].frameDimension.height;

                                // For type PDAF Type2 or 2PD SW-based type (i.e. dual PD) over RDI port case
                                if (Format::RawPlain16 == pOutputBufferProperties->imageFormat.format)
                                {

                                    // Read PDAFBufferFormat from sensor PDAF Info
                                    const PDBufferFormat sensorPDBufferFormat =
                                        m_ISPInputSensorData.sensorPDAFInfo.PDAFBufferFormat;

                                    // Check NativeBufferFormat if Sensor is PDAF Type2/DualPD
                                    if ((PDLibSensorType2 ==
                                        static_cast<PDLibSensorType>(m_ISPInputSensorData.sensorPDAFInfo.PDAFSensorType)) ||
                                        (PDLibSensorDualPD ==
                                        static_cast<PDLibSensorType>(m_ISPInputSensorData.sensorPDAFInfo.PDAFSensorType)) ||
                                        (PDLibSensorType1 ==
                                        static_cast<PDLibSensorType>(m_ISPInputSensorData.sensorPDAFInfo.PDAFSensorType)))
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
                                                        // Update the sensor stream type to distinguish RDI paths with that
                                                        // of Image.  Configure as frame based for Plain16, as PD width and
                                                        // height doesn't align with the SOF/SOL/Eol/EoF strobes
                                                        if (IFEOutputPortRDI0 == outputPortId)
                                                        {
                                                            m_frameBased[IFECSIDRDI0]           = TRUE;
                                                            m_RDIStreams[IFECSIDRDI0].width     = PDAFWidth;
                                                            m_RDIStreams[IFECSIDRDI0].height    = PDAFHeight;
                                                        }
                                                        else if (IFEOutputPortRDI1 == outputPortId)
                                                        {
                                                            m_frameBased[IFECSIDRDI1]           = TRUE;
                                                            m_RDIStreams[IFECSIDRDI1].width     = PDAFWidth;
                                                            m_RDIStreams[IFECSIDRDI1].height    = PDAFHeight;
                                                        }
                                                        else if (IFEOutputPortRDI2 == outputPortId)
                                                        {
                                                            m_frameBased[IFECSIDRDI2]           = TRUE;
                                                            m_RDIStreams[IFECSIDRDI2].width     = PDAFWidth;
                                                            m_RDIStreams[IFECSIDRDI2].height    = PDAFHeight;
                                                        }
                                                        else if (IFEOutputPortRDI3 == outputPortId)
                                                        {
                                                            m_frameBased[IFECSIDRDI3]           = TRUE;
                                                            m_RDIStreams[IFECSIDRDI3].width     = PDAFWidth;
                                                            m_RDIStreams[IFECSIDRDI3].height    = PDAFHeight;
                                                        }
                                                        else
                                                        {
                                                            CAMX_LOG_ERROR(CamxLogGroupISP,
                                                                           "Invalid RDI path %d", outputPortId);
                                                        }
                                                        break;
                                                    default:
                                                        pOutputBufferProperties->imageFormat.width  = PDAFWidth;
                                                        pOutputBufferProperties->imageFormat.height = PDAFHeight;
                                                        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                                                       "sensorPDBufferFormat %d, format %d",
                                                                       sensorPDBufferFormat,
                                                                       pOutputBufferProperties->imageFormat.format);

                                                        break;
                                                }
                                                break;

                                            default:
                                                CAMX_LOG_ERROR(CamxLogGroupISP, "Unsupported PDNativeBufferFormat = %d,"
                                                    "sensorPDBufferFormat %d",
                                                    sensorPDNativeBufferFormat,
                                                    sensorPDBufferFormat);
                                                break;
                                        }
                                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "PDNativeBufferFormat = %d, sensorPDBufferFormat %d",
                                            sensorPDNativeBufferFormat,
                                            sensorPDBufferFormat);
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

                                if ((ISPHwTitan480 == m_hwMask) && (TRUE == CheckIfPDAFType3Supported()))
                                {
                                    // We are in PDAF Type3 mode
                                    if (IFEOutputPortStatsDualPD == outputPortId)
                                    {
                                        // The Dual PD output can be maximum of Image Width * Image Height / 4
                                        // This is used for CSID Pixel Extraction + Sparse PD HW SAD Output
                                        pOutputBufferProperties->imageFormat.width  = finalSelectedInputWidth;
                                        pOutputBufferProperties->imageFormat.height = finalSelectedInputHeight / 4;
                                    }
                                    else
                                    {
                                        if (Format::RawPlain16 == pOutputBufferProperties->imageFormat.format)
                                        {
                                            IFECSIDExtractionInfo* pCSIDDropInfo = NULL;
                                            // This case is to handle extraction of PD Pixels through CSID
                                            // and dump the pixels through RDI
                                            CalculatePDAFBufferParams(&pOutputBufferProperties->imageFormat.width,
                                                &pOutputBufferProperties->imageFormat.height);
                                            // Need to align for 128 bytes --> 8 pixels(Each pixel is of 2 bytes)
                                            // This is due to HW limtation of RDI Port which can only output PLAIN128
                                            // format.
                                            pOutputBufferProperties->imageFormat.width =
                                                Utils::AlignGeneric32(pOutputBufferProperties->imageFormat.width, 8);
                                            m_PDAFInfo.alignedBufferWidth              =
                                                pOutputBufferProperties->imageFormat.width;
                                            if (IFEOutputPortRDI0 == outputPortId)
                                            {
                                                pCSIDDropInfo = &m_CSIDSubSampleInfo[IFECSIDRDI0];
                                            }
                                            else if (IFEOutputPortRDI1 == outputPortId)
                                            {
                                                pCSIDDropInfo = &m_CSIDSubSampleInfo[IFECSIDRDI1];
                                            }
                                            else if (IFEOutputPortRDI2 == outputPortId)
                                            {
                                                pCSIDDropInfo = &m_CSIDSubSampleInfo[IFECSIDRDI2];
                                            }
                                            else
                                            {
                                                pCSIDDropInfo = &m_CSIDSubSampleInfo[IFECSIDRDI3];
                                            }

                                            if (NULL != pCSIDDropInfo)
                                            {
                                                pCSIDDropInfo->enableCSIDSubsample = TRUE;
                                                // CSID has apixel and line drop pattern insteda of skip pattern
                                                // So we have to negate the skip pattern to specify which pixels to be dropped
                                                // In CSID 0 process the pixel and 1 means drop the pixel
                                                pCSIDDropInfo->CSIDSubSamplePattern.pixelSkipPattern =
                                                    ~m_PDAFInfo.pixelSkipPattern;
                                                pCSIDDropInfo->CSIDSubSamplePattern.lineSkipPattern  =
                                                    ~m_PDAFInfo.lineSkipPattern;
                                            }
                                            PreparePDAFInformation();
                                        }
                                        else
                                        {
                                            pOutputBufferProperties->imageFormat.width          = 0;
                                            pOutputBufferProperties->imageFormat.height         = 0;
                                            pOutputBufferProperties->immediateAllocImageBuffers = 0;
                                            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Format for PDAF Type3 %d",
                                                pOutputBufferProperties->imageFormat.format)
                                        }
                                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "PDAF Buffer Width %d Height %d",
                                            m_PDAFInfo.bufferWidth, m_PDAFInfo.bufferHeight);
                                    }
                                }
                                else
                                {
                                    pOutputBufferProperties->imageFormat.width          = 0;
                                    pOutputBufferProperties->imageFormat.height         = 0;
                                    pOutputBufferProperties->immediateAllocImageBuffers = 0;
                                    CAMX_LOG_INFO(CamxLogGroupISP, "PDAF stream not configured by sensor");
                                }
                            }
                        }
                        else if (PortSrcTypeMeta == outputPortSourceTypeId)
                        {
                            UINT streamIndex;
                            if (TRUE == FindSensorStreamConfigIndex(StreamType::META, &streamIndex))
                            {
                                UINT32 metaWidth = m_pSensorModeData->streamConfig[streamIndex].frameDimension.width;
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
                                else if (Format::RawPlain16 == pOutputBufferProperties->imageFormat.format)
                                {
                                    pOutputBufferProperties->imageFormat.width  = metaWidth;
                                    pOutputBufferProperties->imageFormat.height = metaHeight;
                                    if (IFEOutputPortRDI0 == outputPortId)
                                    {
                                        m_frameBased[IFECSIDRDI0]           = TRUE;
                                        m_RDIStreams[IFECSIDRDI0].width     = metaWidth;
                                        m_RDIStreams[IFECSIDRDI0].height    = metaHeight;
                                    }
                                    else if (IFEOutputPortRDI1 == outputPortId)
                                    {
                                        m_frameBased[IFECSIDRDI1]           = TRUE;
                                        m_RDIStreams[IFECSIDRDI1].width     = metaWidth;
                                        m_RDIStreams[IFECSIDRDI1].height    = metaHeight;
                                    }
                                    else if (IFEOutputPortRDI2 == outputPortId)
                                    {
                                        m_frameBased[IFECSIDRDI2]           = TRUE;
                                        m_RDIStreams[IFECSIDRDI2].width     = metaWidth;
                                        m_RDIStreams[IFECSIDRDI2].height    = metaHeight;
                                    }
                                    else if (IFEOutputPortRDI3 == outputPortId)
                                    {
                                        m_frameBased[IFECSIDRDI3]           = TRUE;
                                        m_RDIStreams[IFECSIDRDI3].width     = metaWidth;
                                        m_RDIStreams[IFECSIDRDI3].height    = metaHeight;
                                    }
                                    else
                                    {
                                        CAMX_LOG_ERROR(CamxLogGroupISP,
                                                       "Invalid RDI path %d", outputPortId);
                                    }


                                    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                                     "Meta buffer format = RawPlain16, width = %u, height = %u",
                                                     pOutputBufferProperties->imageFormat.width,
                                                     pOutputBufferProperties->imageFormat.height);
                                }
                                else if (Format::RawMIPI == pOutputBufferProperties->imageFormat.format)
                                {
                                    pOutputBufferProperties->imageFormat.width  = metaWidth;
                                    pOutputBufferProperties->imageFormat.height = metaHeight;

                                    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                                     "Meta buffer format = RawMIPI, width = %u, height = %u",
                                                     pOutputBufferProperties->imageFormat.width,
                                                     pOutputBufferProperties->imageFormat.height);
                                }
                                else
                                {
                                    // Unsupported Meta buffer format
                                    pOutputBufferProperties->imageFormat.width  = 0;
                                    pOutputBufferProperties->imageFormat.height = 0;

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
                        }
                        else if (PortSrcTypeHDR == outputPortSourceTypeId)
                        {
                            UINT streamIndex;
                            if (TRUE == FindSensorStreamConfigIndex(StreamType::HDR, &streamIndex))
                            {
                                UINT32 HDRWidth = m_pSensorModeData->streamConfig[streamIndex].frameDimension.width;
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
                    }
                    break;

                case IFEOutputPortCAMIFRaw:
                case IFEOutputPortLSCRaw:
                case IFEOutputPortGTMRaw:
                    pOutputBufferProperties->imageFormat.width  = finalSelectedInputWidth;
                    pOutputBufferProperties->imageFormat.height = finalSelectedInputHeight;
                    break;

                case IFEOutputPortLCR:
                    pOutputBufferProperties->imageFormat.width  = finalSelectedInputWidth;
                    pOutputBufferProperties->imageFormat.height = finalSelectedInputHeight / 4;
                    break;

                case IFEOutputPortPDAF:
                    // In Titan480 the PDAF Port is defeatured.
                    if (ISPHwTitan480 != m_hwMask)
                    {
                        CalculatePDAFBufferParams(&pOutputBufferProperties->imageFormat.width,
                            &pOutputBufferProperties->imageFormat.height);
                        PreparePDAFInformation();
                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "PDAF Buffer Width %d Height %d",
                            m_PDAFInfo.bufferWidth, m_PDAFInfo.bufferHeight);
                    }
                    break;

                default:
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Unhandled output portID");
                    break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculatePDAFBufferParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::CalculatePDAFBufferParams(
    UINT32* pBufferWidth,
    UINT32* pBufferHeight)
{
    UINT16 pixelSkipPattern      = 0;
    UINT16 lineSkipPattern       = 0;
    UINT32 numberOfPixels        = 0;
    UINT32 numberOfLines         = 0;
    UINT32 pdafPixelCount        = 0;
    UINT32 residualWidth         = 0;
    UINT32 residualHeight        = 0;
    UINT16 residualWidthPattern  = 0;
    UINT16 residualHeightPattern = 0;
    UINT16 offsetX               = 0;
    UINT16 offsetY               = 0;
    UINT32 inputWidth            = 0;
    UINT32 inputHeight           = 0;

    if ((NULL != pBufferWidth) && (NULL != pBufferHeight) && (NULL != m_pSensorModeData))
    {
        inputWidth  = m_pSensorModeData->cropInfo.lastPixel - m_pSensorModeData->cropInfo.firstPixel + 1;
        inputHeight = m_pSensorModeData->cropInfo.lastLine - m_pSensorModeData->cropInfo.firstLine + 1;

        // The below calculations are for calcualting the PDAF extracion Pattern
        // and the number of pixels and lines we are going to extract
        // Sensor Secifies the number of PD Pixels in each frame.
        // Based on the PD Pixel coordinates we generate a pattern to extract PD Pixels
        // and we calculate the required output buffer properties.
        pdafPixelCount = m_ISPInputSensorData.sensorPDAFInfo.PDAFPixelCount;

        // Generate the Pixel pattern based on the PD Pixelc oordinates
        // We genrate a 16 x 16 block pattenr based on the PD Pixel coordinates
        for (UINT coordinate = 0;
            (coordinate < pdafPixelCount) && (coordinate < IFEMaxPdafPixelsPerBlock);
            coordinate++)
        {
            pixelSkipPattern |=
                (1 << (m_ISPInputSensorData.sensorPDAFInfo.PDAFPixelCoords[coordinate].PDXCoordinate % 16));
            lineSkipPattern |=
                (1 << (m_ISPInputSensorData.sensorPDAFInfo.PDAFPixelCoords[coordinate].PDYCoordinate % 16));
        }

        // Find number of pixels
        numberOfPixels = GetPixelsInSkipPattern(pixelSkipPattern);
        numberOfLines  = GetPixelsInSkipPattern(lineSkipPattern);

        offsetX = m_pSensorModeData->cropInfo.firstPixel % 16;
        offsetY = m_pSensorModeData->cropInfo.firstLine % 16;

        // Adjust the Extraction Pattern based on the CAM IF Crop
        // For example if the the pattern is 1111 0000 1111 0000 and offset is 4
        // the adjusted pattern will become 0000 1111 0000 1111
        pixelSkipPattern = ((pixelSkipPattern << offsetX) | (pixelSkipPattern >> (16 - offsetX)));
        lineSkipPattern  = ((lineSkipPattern << offsetY) | (lineSkipPattern >> (16 - offsetY)));

        m_PDAFInfo.bufferWidth  = (inputWidth / 16) * numberOfPixels;
        m_PDAFInfo.bufferHeight = (inputHeight / 16) * numberOfLines;

        // Caclucalte the residual widtha nd height and adjust the buffer size based on the pattern
        residualWidth  = inputWidth % 16;
        residualHeight = inputHeight % 16;

        if (0 != residualWidth)
        {
            residualWidthPattern = ((static_cast<UINT16>(~0)) >> (16 - residualWidth));
        }

        if (0 != residualHeight)
        {
            residualHeightPattern = ((static_cast<UINT16>(~0)) >> (16 - residualHeight));
        }
        m_PDAFInfo.bufferWidth       += GetPixelsInSkipPattern(pixelSkipPattern & residualWidthPattern);
        m_PDAFInfo.bufferHeight      += GetPixelsInSkipPattern(lineSkipPattern & residualHeightPattern);
        m_PDAFInfo.alignedBufferWidth = m_PDAFInfo.bufferWidth;
        m_PDAFInfo.pixelSkipPattern   = pixelSkipPattern;
        m_PDAFInfo.lineSkipPattern    = lineSkipPattern;
        m_PDAFInfo.enableSubsample    = CheckIfPDAFType3Supported();
        *pBufferWidth                 = m_PDAFInfo.bufferWidth;
        *pBufferHeight                = m_PDAFInfo.bufferHeight;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::IsSensorModeFormatBayer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::IsSensorModeFormatBayer(
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
// IFENode::IsSensorModeFormatMono
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::IsSensorModeFormatMono(
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
/// IFENode::IsSensorModeFormatYUV
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::IsSensorModeFormatYUV(
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
/// IFENode::IsTPGMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::IsTPGMode()
{
    BOOL isTPG                        = FALSE;
    const StaticSettings*   pSettings = GetStaticSettings();
    if (NULL != pSettings)
    {
        isTPG = pSettings->enableTPG;
    }

    return isTPG;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::EvaluateDualIFEMode
IFEModuleMode DualIFEUtils::EvaluateDualIFEMode(
    ISPInputData* pISPInputdata,
    BOOL          isFS2Enable,
    BOOL          forceSingleIFEForPDAFType3)
{
    CAMX_UNREFERENCED_PARAM(pISPInputdata);

    const StaticSettings* pSettings         = HwEnvironment::GetInstance()->GetStaticSettings();
    IFEModuleMode         mode              = IFEModuleMode::SingleIFENormal;
    const PlatformStaticCaps*   pStaticCaps = HwEnvironment::GetInstance()->GetPlatformStaticCaps();

    if (TRUE == pSettings->enableDualIFE)
    {
        if (TRUE == pISPInputdata->RDIOnlyCase)
        {
            mode = IFEModuleMode::SingleIFENormal;
        }
        else if (TRUE == pISPInputdata->csidBinningInfo.isBinningEnabled)
        {
            mode = IFEModuleMode::SingleIFENormal;
        }
        else if (TRUE == pSettings->forceDualIFEOn)
        {
            mode = IFEModuleMode::DualIFENormal;
        }
        else
        {
            UINT64 IFEDualClockThreshod = static_cast<Titan17xContext*>(pISPInputdata->pHwContext)->
                GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->IFEDualClockThreshold;
            if (0xFFFFFFFF == IFEDualClockThreshod)
            {
                IFEDualClockThreshod = pStaticCaps->maxIFESVSClock;
            }
            if (pISPInputdata->isDualcamera)
            {
                if (pISPInputdata->minRequiredSingleIFEClock <= pStaticCaps->maxIFETURBOClock)
                {
                    mode = IFEModuleMode::SingleIFENormal;
                }
                else
                {
                    mode = IFEModuleMode::DualIFENormal;
                }
            }
            else if ((TRUE == isFS2Enable) || (TRUE == forceSingleIFEForPDAFType3))
            {
                // For FS2 mode only RDI0 is tied to IFE0 Hence only Single IFE mode is supported.
                // for PDAF Type3 only Single IFE mode is supported.
                mode = IFEModuleMode::SingleIFENormal;
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "Single IFE mode since FS2/PDAF Type3 enabled");
            }
            else if ((pISPInputdata->forceIFEflag == TRUE) &&
                (pISPInputdata->minRequiredSingleIFEClock <= pStaticCaps->maxIFETURBOClock))
            {
                mode = IFEModuleMode::SingleIFENormal;
            }
            else if (IFEProfileIdOffline == pISPInputdata->IFENodeInstanceProperty.profileId)
            {
                // Fetch engine is after CSID, This can be supported only by single IFE
                mode = IFEModuleMode::SingleIFENormal;
            }
            else if (pISPInputdata->minRequiredSingleIFEClock > IFEDualClockThreshod)
            {
                mode = IFEModuleMode::DualIFENormal;
            }
            else if ((pISPInputdata->HALData.stream[FDOutput].width > pISPInputdata->maxOutputWidthFD) ||
                     (pISPInputdata->HALData.stream[FullOutput].width > IFEMaxOutputWidthFull)         ||
                     (pISPInputdata->HALData.stream[DisplayFullOutput].width > IFEMaxOutputWidthFull))
            {
                CAMX_LOG_INFO(CamxLogGroupISP, "forcing dual IFE due to width of FD port %d Full port %d displayFull port %d",
                    pISPInputdata->HALData.stream[FDOutput].width,
                    pISPInputdata->HALData.stream[FullOutput].width,
                    pISPInputdata->HALData.stream[DisplayFullOutput].width);
                mode = IFEModuleMode::DualIFENormal;
            }
            else if (static_cast<INT32>(pISPInputdata->sensorData.sensorOut.width) > pStaticCaps->IFEMaxLineWidth)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "forcing dual IFE due to Input %d is greater than Line Buffer of IFE %d",
                              pISPInputdata->sensorData.sensorOut.width, pStaticCaps->IFEMaxLineWidth);

                mode = IFEModuleMode::DualIFENormal;
            }
        }
    }
    return mode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::FillBFStats23CfgFromOneStripe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DualIFEUtils::FillBFStats23CfgFromOneStripe(
    BFStatsConfigParams*       pBFStatsFromOneStripe,
    IFEStripeInterfaceOutput*  pStripeOut)
{
    BFStatsROIDimensionParams* pBFROIDimension = pBFStatsFromOneStripe->BFStatsROIConfig.BFStatsROIDimension;

    // Index 0 is start tag added for DMI transfer. This needs to be ignored.
    // This needs to be handled later in proper way.
    for (UINT32 index = 1; index <= BFMaxROIRegions; index++)
    {
        UINT64 bfROI = pStripeOut->BAFOut.BAFROIIndexLUT[index];

        // We don't get how many windows have been selected in each windows. So if bfROI is 0,
        // we break and use index as number of ROIs configured.
        if (0 == bfROI)
        {
            pBFStatsFromOneStripe->BFStatsROIConfig.numBFStatsROIDimension = index - 1;
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Reaching the end of BF ROI index, index = %d", index);
            break;
        }

        pBFROIDimension[index - 1].region      = static_cast<BFStatsRegionType>(bfROI >> DefaultDMISelShift);
        pBFROIDimension[index - 1].regionNum   = (bfROI >> DefaultDMIIndexShift)  & DefaultDMIIndexBits;
        pBFROIDimension[index - 1].ROI.left    = (bfROI >> DefaultDMILeftShift)   & DefaultDMILeftBits;
        pBFROIDimension[index - 1].ROI.top     = (bfROI >> DefaultDMITopShift)    & DefaultDMITopBits;
        pBFROIDimension[index - 1].ROI.width   = (bfROI >> DefaultDMIWidthShift)  & DefaultDMIWidthBits;
        pBFROIDimension[index - 1].ROI.height  = (bfROI >> DefaultDMIHeightShift) & DefaultDMIHeightBits;
        CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE> ROI[%d] region %d num %d left, top, width, height (%d, %d, %d, %d)",
            index - 1, pBFROIDimension[index - 1].region,
            pBFROIDimension[index - 1].regionNum,
            pBFROIDimension[index - 1].ROI.left,
            pBFROIDimension[index - 1].ROI.top,
            pBFROIDimension[index - 1].ROI.width,
            pBFROIDimension[index - 1].ROI.height);

        pBFStatsFromOneStripe->BFStatsROIConfig.numBFStatsROIDimension++;
    }

    if (TRUE == pStripeOut->BAFOut.mndsParam.enable)
    {
        pBFStatsFromOneStripe->BFScaleConfig.isValid                 = TRUE;
        pBFStatsFromOneStripe->BFScaleConfig.BFScaleEnable           = pStripeOut->BAFOut.mndsParam.enable;
        pBFStatsFromOneStripe->BFScaleConfig.interpolationResolution = pStripeOut->BAFOut.mndsParam.interpReso;
        pBFStatsFromOneStripe->BFScaleConfig.scaleM                  = pStripeOut->BAFOut.mndsParam.output;
        pBFStatsFromOneStripe->BFScaleConfig.scaleN                  = pStripeOut->BAFOut.mndsParam.input;
        pBFStatsFromOneStripe->BFScaleConfig.inputImageWidth         = pStripeOut->BAFOut.mndsParam.inputProcessedLength;
        pBFStatsFromOneStripe->BFScaleConfig.mnInit                  = pStripeOut->BAFOut.mndsParam.cntInit;
        pBFStatsFromOneStripe->BFScaleConfig.phaseInit               = pStripeOut->BAFOut.mndsParam.phaseInit;
        pBFStatsFromOneStripe->BFScaleConfig.phaseStep               = pStripeOut->BAFOut.mndsParam.phaseStep;
        pBFStatsFromOneStripe->BFScaleConfig.pixelOffset             = pStripeOut->BAFOut.mndsParam.pixelOffset;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::FillBFStats24CfgFromOneStripe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DualIFEUtils::FillBFStats24CfgFromOneStripe(
    BFStatsConfigParams*      pBFStatsFromOneStripe,
    IFEStripeInterfaceOutput* pStripeOut)
{
    BFStatsROIDimensionParams* pBFROIDimension = pBFStatsFromOneStripe->BFStatsROIConfig.BFStatsROIDimension;

    // Index 0 is start tag added for DMI transfer. This needs to be ignored.
    // This needs to be handled later in proper way.
    for (UINT32 index = 1; index <= BFMaxROIRegions; index++)
    {
        UINT64 bfROI = pStripeOut->BAFOutv24.BAFROIIndexLUT[index];

        // We don't get how many windows have been selected in each windows. So if bfROI is 0,
        // we break and use index as number of ROIs configured.
        if (0 == bfROI)
        {
            pBFStatsFromOneStripe->BFStatsROIConfig.numBFStatsROIDimension = index - 1;
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Reaching the end of BF ROI index, index = %d", index);
            break;
        }

        pBFROIDimension[index - 1].region     = static_cast<BFStatsRegionType>(bfROI >> DefaultDMISelShift);
        pBFROIDimension[index - 1].regionNum  = (bfROI >> DefaultDMIIndexShift)  & DefaultDMIIndexBits;
        pBFROIDimension[index - 1].ROI.left   = (bfROI >> DefaultDMILeftShift)   & DefaultDMILeftBits;
        pBFROIDimension[index - 1].ROI.top    = (bfROI >> DefaultDMITopShift)    & DefaultDMITopBits;
        pBFROIDimension[index - 1].ROI.width  = (bfROI >> DefaultDMIWidthShift)  & DefaultDMIWidthBits;
        pBFROIDimension[index - 1].ROI.height = (bfROI >> DefaultDMIHeightShift) & DefaultDMIHeightBits;
        CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE> ROI[%d] region %d num %d left, top, width, height (%d, %d, %d, %d)",
            index - 1, pBFROIDimension[index - 1].region,
            pBFROIDimension[index - 1].regionNum,
            pBFROIDimension[index - 1].ROI.left,
            pBFROIDimension[index - 1].ROI.top,
            pBFROIDimension[index - 1].ROI.width,
            pBFROIDimension[index - 1].ROI.height);

        pBFStatsFromOneStripe->BFStatsROIConfig.numBFStatsROIDimension++;
    }

    if (TRUE == pStripeOut->BAFOutv24.mndsParam.enable)
    {
        pBFStatsFromOneStripe->BFScaleConfig.isValid                 = TRUE;
        pBFStatsFromOneStripe->BFScaleConfig.BFScaleEnable           = pStripeOut->BAFOutv24.mndsParam.enable;
        pBFStatsFromOneStripe->BFScaleConfig.interpolationResolution = pStripeOut->BAFOutv24.mndsParam.interpReso;
        pBFStatsFromOneStripe->BFScaleConfig.scaleM                  = pStripeOut->BAFOutv24.mndsParam.output;
        pBFStatsFromOneStripe->BFScaleConfig.scaleN                  = pStripeOut->BAFOutv24.mndsParam.input;
        pBFStatsFromOneStripe->BFScaleConfig.inputImageWidth         = pStripeOut->BAFOutv24.mndsParam.inputProcessedLength;
        pBFStatsFromOneStripe->BFScaleConfig.mnInit                  = pStripeOut->BAFOutv24.mndsParam.cntInit;
        pBFStatsFromOneStripe->BFScaleConfig.phaseInit               = pStripeOut->BAFOutv24.mndsParam.phaseInit;
        pBFStatsFromOneStripe->BFScaleConfig.phaseStep               = pStripeOut->BAFOutv24.mndsParam.phaseStep;
        pBFStatsFromOneStripe->BFScaleConfig.pixelOffset             = pStripeOut->BAFOutv24.mndsParam.pixelOffset;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::FillBFStats25CfgFromOneStripe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DualIFEUtils::FillBFStats25CfgFromOneStripe(
    BFStatsConfigParams*      pBFStatsFromOneStripe,
    IFEStripeInterfaceOutput* pStripeOut,
    const UINT32              totalROICountFromFrameLevel)
{
    BFStatsROIDimensionParams* pBFROIDimension = pBFStatsFromOneStripe->BFStatsROIConfig.BFStatsROIDimension;

    UINT32 mergeNeededRegionCount = 0;

    for (UINT32 index = 0; index < BFMaxROIRegions; index++)
    {
        // In BF v2.5 BAFOutv25.BAFROIIndexLUT[] is an array of two UINT64
        const UINT64*   pBFROI          = pStripeOut->BAFOutv25.BAFROIIndexLUT[index];
        const UINT64    lower64bitBFROI = pBFROI[0]; // Lower 64-bit of 128-bit
        const UINT64    upper64BitBFROI = pBFROI[1]; // Upper 64-bit of 128-bit

        // Note that BF v2.5 there is no frame-tag, hence "index-1" is not needed any more
        pBFROIDimension[index].region     = static_cast<BFStatsRegionType>(
                                                (upper64BitBFROI >> DefaultBF25DMIUpper64bitSelShift) & 0x1);
        pBFROIDimension[index].regionNum  = (lower64bitBFROI >> DefaultBF25DMIIndexShift) & DefaultBF25DMIIndexBits;
        pBFROIDimension[index].ROI.left   = (lower64bitBFROI >> DefaultDMILeftShift)      & DefaultDMILeftBits;
        pBFROIDimension[index].ROI.top    = (lower64bitBFROI >> DefaultDMITopShift)       & DefaultDMITopBits;
        pBFROIDimension[index].ROI.width  = (lower64bitBFROI >> DefaultDMIWidthShift)     & DefaultDMIWidthBits;
        pBFROIDimension[index].ROI.height = (lower64bitBFROI >> DefaultDMIHeightShift)    & DefaultDMIHeightBits;
        // New addtion in BF v2.5
        pBFROIDimension[index].outputID   = (upper64BitBFROI & DefaultBF25DMIUpper64bitOutputIdBits) |
                                            ((lower64bitBFROI >> DefaultBF25DMIOutputIdShift) &
                                            DefaultBF25DMILower64bitOutputIdBits);
        pBFROIDimension[index].needMerge  = (upper64BitBFROI >> DefaultBF25DMIUpper64bitMergeShift) & 0x1;

        const BOOL isEndOfBuffer = (upper64BitBFROI >> DefaultBF25DMIUpper64bitEndOfBufferShift) & 0x1;

        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "<DualIFE> BF25 ROI[%d] region %d num %d left, top, width, height (%d, %d, %d, %d), "
                         "oid=%u, merge=%u, eob=%u",
                         index,
                         pBFROIDimension[index].region,
                         pBFROIDimension[index].regionNum,
                         pBFROIDimension[index].ROI.left,
                         pBFROIDimension[index].ROI.top,
                         pBFROIDimension[index].ROI.width,
                         pBFROIDimension[index].ROI.height,
                         pBFROIDimension[index].outputID,
                         pBFROIDimension[index].needMerge,
                         isEndOfBuffer);

        pBFStatsFromOneStripe->BFStatsROIConfig.numBFStatsROIDimension++;

        // If needMerge bit is set, then set the output ID after all the non-merge bit set region ID are allocated.
        if (TRUE == pBFROIDimension[index].needMerge)
        {
            pBFROIDimension[index].outputID = totalROICountFromFrameLevel + mergeNeededRegionCount;
            mergeNeededRegionCount++;
        }
        else
        {
            pBFROIDimension[index].outputID = pBFROIDimension[index].regionNum;
        }

        // In case if we need to check the calculated output ID.
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Updated BF25: index = %d, regionID=%d, OID=%d, merge=%d, eob=%d",
                         index,
                         pBFROIDimension[index].regionNum,
                         pBFROIDimension[index].outputID,
                         pBFROIDimension[index].needMerge, // merge bit
                         isEndOfBuffer);

        if (TRUE == isEndOfBuffer)
        {
            // In BF v2.5 (index + 1) is the total number of ROIs
            pBFStatsFromOneStripe->BFStatsROIConfig.numBFStatsROIDimension = (index + 1);
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Reaching the end of BF ROI index, index = %d", index);
            break;
        }
    }

    if (TRUE == pStripeOut->BAFOutv25.mndsParam.enable)
    {
        BFScaleConfigType* pBFScaleConfig = &pBFStatsFromOneStripe->BFScaleConfig;

        pBFScaleConfig->isValid                 = TRUE;
        pBFScaleConfig->BFScaleEnable           = pStripeOut->BAFOutv25.mndsParam.enable;
        pBFScaleConfig->interpolationResolution = pStripeOut->BAFOutv25.mndsParam.interpReso;
        pBFScaleConfig->scaleM                  = pStripeOut->BAFOutv25.mndsParam.output;
        pBFScaleConfig->scaleN                  = pStripeOut->BAFOutv25.mndsParam.input;
        pBFScaleConfig->inputImageWidth         = pStripeOut->BAFOutv25.mndsParam.inputProcessedLength;
        pBFScaleConfig->mnInit                  = pStripeOut->BAFOutv25.mndsParam.cntInit;
        pBFScaleConfig->phaseInit               = pStripeOut->BAFOutv25.mndsParam.phaseInit;
        pBFScaleConfig->phaseStep               = pStripeOut->BAFOutv25.mndsParam.phaseStep;
        pBFScaleConfig->pixelOffset             = pStripeOut->BAFOutv25.mndsParam.pixelOffset;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::FillCfgFromOneStripe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DualIFEUtils::FillCfgFromOneStripe(
    ISPInputData*       pISPInputdata,
    IFEStripeInterfaceOutput*    pStripeOut,
    ISPStripeConfig*    pStripeConfig)
{
    CAMX_ASSERT(NULL != pISPInputdata);
    CAMX_ASSERT(NULL != pStripeOut);
    CAMX_ASSERT(NULL != pStripeConfig);

    if (1 == pStripeOut->edgeStripeLT)
    {
        CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE>/<scaler> left stripe");
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE>/<scaler> right stripe");
    }

    CAMX_LOG_INFO(CamxLogGroupISP, "<scaler> full %d FD %d DS4 %d DS16 %d",
        pStripeOut->outRange_full[1]  - pStripeOut->outRange_full[0]  + 1,
        pStripeOut->outRange_fd[1]    - pStripeOut->outRange_fd[0]    + 1,
        pStripeOut->outRange_1to4[1]  - pStripeOut->outRange_1to4[0]  + 1,
        pStripeOut->outRange_1to16[1] - pStripeOut->outRange_1to16[0] + 1);
    CAMX_LOG_INFO(CamxLogGroupISP, "<scaler> disp %d DS4 %d DS16 %d",
        pStripeOut->outRange_disp[1]  - pStripeOut->outRange_disp[0]  + 1,
        pStripeOut->outRange_1to4_disp[1]  - pStripeOut->outRange_1to4_disp[0]  + 1,
        pStripeOut->outRange_1to16_disp[1] - pStripeOut->outRange_1to16_disp[0] + 1);
    CAMX_LOG_INFO(CamxLogGroupISP, "<scaler> post crop (enable, input, firstpixel, last pixel)"
        "full (%d %d %d %d) FD (%d %d %d %d) DS4 (%d %d %d %d) DS16 (%d %d %d %d) luma phase Init %lu chroma phase init %lu",
        pStripeOut->outCropVideoFullLuma.enable, pStripeOut->outCropVideoFullLuma.inDim,
        pStripeOut->outCropVideoFullLuma.firstOut, pStripeOut->outCropVideoFullLuma.lastOut,
        pStripeOut->outCropFDLuma.enable, pStripeOut->outCropFDLuma.inDim,
        pStripeOut->outCropFDLuma.firstOut, pStripeOut->outCropFDLuma.lastOut,
        pStripeOut->outCropVideoDS4Luma.enable, pStripeOut->outCropVideoDS4Luma.inDim,
        pStripeOut->outCropVideoDS4Luma.firstOut, pStripeOut->outCropVideoDS4Luma.lastOut,
        pStripeOut->outCropVideoDS16Luma.enable, pStripeOut->outCropVideoDS16Luma.inDim,
        pStripeOut->outCropVideoDS16Luma.firstOut, pStripeOut->outCropVideoDS16Luma.lastOut,
        pStripeOut->preDSXphaseInitLuma, pStripeOut->preDSXphaseInitChroma);
    CAMX_LOG_INFO(CamxLogGroupISP, "<scaler> Disp post crop (enable, input, firstpixel, last pixel)"
        "disp (%d %d %d %d) DS4 (%d %d %d %d) DS16 (%d %d %d %d)",
        pStripeOut->outCropDispFullLuma.enable, pStripeOut->outCropDispFullLuma.inDim,
        pStripeOut->outCropDispFullLuma.firstOut, pStripeOut->outCropDispFullLuma.lastOut,
        pStripeOut->outCropDispDS4Luma.enable, pStripeOut->outCropDispDS4Luma.inDim,
        pStripeOut->outCropDispDS4Luma.firstOut, pStripeOut->outCropDispDS4Luma.lastOut,
        pStripeOut->outCropDispDS16Luma.enable, pStripeOut->outCropDispDS16Luma.inDim,
        pStripeOut->outCropDispDS16Luma.firstOut, pStripeOut->outCropDispDS16Luma.lastOut);
    CAMX_LOG_INFO(CamxLogGroupISP, "<scaler>pre crop (enable, input, firstpixel, last pixel)"
        "DS4 (%d %d %d %d) DS16 (%d %d %d %d)",
        pStripeOut->preDS4CropVideoDS4Luma.enable, pStripeOut->preDS4CropVideoDS4Luma.inDim,
        pStripeOut->preDS4CropVideoDS4Luma.firstOut, pStripeOut->preDS4CropVideoDS4Luma.lastOut,
        pStripeOut->preDS4CropVideoDS16Luma.enable, pStripeOut->preDS4CropVideoDS16Luma.inDim,
        pStripeOut->preDS4CropVideoDS16Luma.firstOut, pStripeOut->preDS4CropVideoDS16Luma.lastOut);
    CAMX_LOG_INFO(CamxLogGroupISP, "<scaler> Disp pre crop (enable, input, firstpixel, last pixel)"
        "DS4 (%d %d %d %d) DS16 (%d %d %d %d)",
        pStripeOut->preDS4CropDispDS4Luma.enable, pStripeOut->preDS4CropDispDS4Luma.inDim,
        pStripeOut->preDS4CropDispDS4Luma.firstOut, pStripeOut->preDS4CropDispDS4Luma.lastOut,
        pStripeOut->preDS4CropDispDS16Luma.enable, pStripeOut->preDS4CropDispDS16Luma.inDim,
        pStripeOut->preDS4CropDispDS16Luma.firstOut, pStripeOut->preDS4CropDispDS16Luma.lastOut);
    CAMX_LOG_INFO(CamxLogGroupISP, "<scaler> Disp C pre crop (enable, input, firstpixel, last pixel)"
        "DS4 (%d %d %d %d) DS16 (%d %d %d %d)",
        pStripeOut->preDS4CropDispDS4Chroma.enable, pStripeOut->preDS4CropDispDS4Chroma.inDim,
        pStripeOut->preDS4CropDispDS4Chroma.firstOut, pStripeOut->preDS4CropDispDS4Chroma.lastOut,
        pStripeOut->preDS4CropDispDS16Chroma.enable, pStripeOut->preDS4CropDispDS16Chroma.inDim,
        pStripeOut->preDS4CropDispDS16Chroma.firstOut, pStripeOut->preDS4CropDispDS16Chroma.lastOut);
    CAMX_LOG_INFO(CamxLogGroupISP, "<scaler> mnds full in %d out %d offset %d cnt %d mn %d phase %d",
        pStripeOut->mndsConfigVideoFullLuma.input, pStripeOut->mndsConfigVideoFullLuma.output,
        pStripeOut->mndsConfigVideoFullLuma.pixelOffset,
        pStripeOut->mndsConfigVideoFullLuma.cntInit,
        pStripeOut->mndsConfigVideoFullLuma.phaseInit,
        pStripeOut->mndsConfigVideoFullLuma.phaseStep);
    CAMX_LOG_INFO(CamxLogGroupISP, "<scaler> mnds disp in %d out %d offset %d cnt %d mn %d phase %d",
        pStripeOut->mndsConfigDispFullLuma.input, pStripeOut->mndsConfigDispFullLuma.output,
        pStripeOut->mndsConfigDispFullLuma.pixelOffset,
        pStripeOut->mndsConfigDispFullLuma.cntInit,
        pStripeOut->mndsConfigDispFullLuma.phaseInit,
        pStripeOut->mndsConfigDispFullLuma.phaseStep);

    // For Kona
    CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE>/<scaler> MNDS21 Full: in %d out %d offset %d phaseInit %d phaseStep %d",
        pStripeOut->mndsConfigVideoFullLumav21.input, pStripeOut->mndsConfigVideoFullLumav21.output,
        pStripeOut->mndsConfigVideoFullLumav21.pixelOffset,
        pStripeOut->mndsConfigVideoFullLumav21.phaseInit,
        pStripeOut->mndsConfigVideoFullLumav21.phaseStep);
    CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE>/<scaler> MNDS21 FD: in %d out %d offset %d phaseInit %d phaseStep %d",
        pStripeOut->mndsConfigFDLumav21.input, pStripeOut->mndsConfigFDLumav21.output,
        pStripeOut->mndsConfigFDLumav21.pixelOffset,
        pStripeOut->mndsConfigFDLumav21.phaseInit,
        pStripeOut->mndsConfigFDLumav21.phaseStep);
    CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE>/<scaler> MNDS21 Disp: in %d out %d offset %d phaseInit %d phaseStep %d",
        pStripeOut->mndsConfigDispFullLumav21.input, pStripeOut->mndsConfigDispFullLumav21.output,
        pStripeOut->mndsConfigDispFullLumav21.pixelOffset,
        pStripeOut->mndsConfigDispFullLumav21.phaseInit,
        pStripeOut->mndsConfigDispFullLumav21.phaseStep);
    CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE>/<scaler> MNDS21: Full MNDS Y: PreCrop: enable=%u inDim=%u firstOut=%u lastOut=%d",
        pStripeOut->preMndsCropVideoFullLuma.enable,
        pStripeOut->preMndsCropVideoFullLuma.inDim,
        pStripeOut->preMndsCropVideoFullLuma.firstOut,
        pStripeOut->preMndsCropVideoFullLuma.lastOut);
    CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE>/<scaler> MNDS21: Full MNDS C: PreCrop: enable=%u inDim=%u firstOut=%u lastOut=%d",
        pStripeOut->preMndsCropVideoFullChroma.enable,
        pStripeOut->preMndsCropVideoFullChroma.inDim,
        pStripeOut->preMndsCropVideoFullChroma.firstOut,
        pStripeOut->preMndsCropVideoFullChroma.lastOut);
    CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE>/<scaler> MNDS21: Disp MNDS Y: PreCrop: enable=%u inDim=%u firstOut=%u lastOut=%d",
        pStripeOut->preMndsCropDispFullLuma.enable,
        pStripeOut->preMndsCropDispFullLuma.inDim,
        pStripeOut->preMndsCropDispFullLuma.firstOut,
        pStripeOut->preMndsCropDispFullLuma.lastOut);
    CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE>/<scaler> MNDS21: Disp MNDS C: PreCrop: enable=%u inDim=%u firstOut=%u lastOut=%d",
        pStripeOut->preMndsCropDispFullChroma.enable,
        pStripeOut->preMndsCropDispFullChroma.inDim,
        pStripeOut->preMndsCropDispFullChroma.firstOut,
        pStripeOut->preMndsCropDispFullChroma.lastOut);

    // LSC
    if (TRUE == pISPInputdata->pStripingInput->enableBits.rolloff)
    {
        if (CSLCameraTitanVersion::CSLTitan480 == pISPInputdata->titanVersion)
        {
            pStripeConfig->stateLSC.dependence40Data.stripeOut.lx_start =
                pStripeOut->rolloffOutStripe.gridIndex;
            pStripeConfig->stateLSC.dependence40Data.stripeOut.bx_start =
                pStripeOut->rolloffOutStripe.subgridIndex;
            pStripeConfig->stateLSC.dependence40Data.stripeOut.bx_d1    =
                pStripeOut->rolloffOutStripe.pixelIndexWithinSubgrid;
            CAMX_LOG_INFO(CamxLogGroupISP,
                "Fetching: lx_start = %d, bx_d1 = %d, bx_start = %d\n",
                pStripeOut->rolloffOutStripe.gridIndex,
                pStripeOut->rolloffOutStripe.pixelIndexWithinSubgrid,
                pStripeOut->rolloffOutStripe.subgridIndex);
        }
        else
        {
            pStripeConfig->stateLSC.dependenceData.stripeOut.lx_start =
                pStripeOut->rolloffOutStripe.gridIndex;
            pStripeConfig->stateLSC.dependenceData.stripeOut.bx_start =
                pStripeOut->rolloffOutStripe.subgridIndex;
            pStripeConfig->stateLSC.dependenceData.stripeOut.bx_d1    =
                pStripeOut->rolloffOutStripe.pixelIndexWithinSubgrid;
            CAMX_LOG_INFO(CamxLogGroupISP,
                "Fetching: lx_start = %d, bx_d1 = %d, bx_start = %d\n",
                pStripeOut->rolloffOutStripe.gridIndex,
                pStripeOut->rolloffOutStripe.pixelIndexWithinSubgrid,
                pStripeOut->rolloffOutStripe.subgridIndex);
        }
    }

    // AWB BG
    if (TRUE == pStripeOut->BGAWBOut.BGEnabled)
    {
        pStripeConfig->AWBStatsUpdateData.statsConfig                           =
            pISPInputdata->pAWBStatsUpdateData->statsConfig;
        pStripeConfig->AWBStatsUpdateData.statsConfig.BGConfig.horizontalNum    =
            pStripeOut->BGAWBOut.BGRgnNumStripeHor + 1;
        pStripeConfig->AWBStatsUpdateData.statsConfig.BGConfig.ROI.left         =
            pStripeOut->BGAWBOut.BGROIHorizOffset;
        pStripeConfig->AWBStatsUpdateData.statsConfig.BGConfig.ROI.width        =
            (pStripeOut->BGAWBOut.BGRgnNumStripeHor + 1) *
            (pISPInputdata->pStripingInput->stripingInput.BGAWBInput.BGRgnWidth + 1);
    }

    // BG Tintless
    if (TRUE == pStripeOut->BGTintlessOut.BGEnabled)
    {
        pStripeConfig->AECStatsUpdateData.statsConfig.TintlessBGConfig          =
            pISPInputdata->pAECStatsUpdateData->statsConfig.TintlessBGConfig;
        pStripeConfig->AECStatsUpdateData.statsConfig.TintlessBGConfig.horizontalNum    =
            pStripeOut->BGTintlessOut.BGRgnNumStripeHor + 1;
        pStripeConfig->AECStatsUpdateData.statsConfig.TintlessBGConfig.ROI.left         =
            pStripeOut->BGTintlessOut.BGROIHorizOffset;
        pStripeConfig->AECStatsUpdateData.statsConfig.TintlessBGConfig.ROI.width        =
            (pStripeOut->BGTintlessOut.BGRgnNumStripeHor + 1) *
            (pISPInputdata->pStripingInput->stripingInput.BGTintlessInput.BGRgnWidth + 1);
    }

    // HDR BE
    if (TRUE == pStripeOut->beOut.BEEnable)
    {
        pStripeConfig->AECStatsUpdateData.statsConfig.BEConfig                      =
            pISPInputdata->pAECStatsUpdateData->statsConfig.BEConfig;
        pStripeConfig->AECStatsUpdateData.statsConfig.BEConfig.horizontalNum        =
            pStripeOut->beOut.BERgnHorNum + 1;
        pStripeConfig->AECStatsUpdateData.statsConfig.BEConfig.ROI.left             =
            pStripeOut->beOut.BEROIHorOffset;
        pStripeConfig->AECStatsUpdateData.statsConfig.BEConfig.ROI.width            =
            (pStripeOut->beOut.BERgnHorNum + 1) *
            (pISPInputdata->pStripingInput->stripingInput.BEInput.BERgnWidth + 1);
        pStripeConfig->AECStatsUpdateData.statsConfig.BEConfig.isStripeValid        = TRUE;
    }

    // BAF
    pStripeConfig->AFStatsUpdateData.statsConfig = pISPInputdata->pAFStatsUpdateData->statsConfig;
    BFStatsROIDimensionParams* pBFROIDimension =
        pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension;
    pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension = 0;
    Utils::Memset(pBFROIDimension, 0, sizeof(BFStatsROIDimensionParams));

    CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE> CropType %d enable %d",
            pISPInputdata->pStripeConfig->cropType,
            pStripeOut->BAFOut.enable);

    if (TRUE == pStripeOut->BAFOut.enable)
    {
        // Pass the BF stats structure from the current stripe configure structure (either left or right stripe in dual IFE).
        FillBFStats23CfgFromOneStripe(&(pStripeConfig->AFStatsUpdateData.statsConfig.BFStats), pStripeOut);
    }

    // check if BF stats enable for Talos
    if (TRUE == pStripeOut->BAFOutv24.enable)
    {
        // Pass the BF stats structure from the current stripe configure structure (either left or right stripe in dual IFE).
        FillBFStats24CfgFromOneStripe(&(pStripeConfig->AFStatsUpdateData.statsConfig.BFStats), pStripeOut);
    }

    // check if BF stats enable for Titan480
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "pStripeOut->BAFOutv25.enable = %d, pISPInputdata->pAFStatsUpdateData->statsConfig = %d",
                     pStripeOut->BAFOutv25.enable,
                     pISPInputdata->pAFStatsUpdateData->statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension);

    if (TRUE == pStripeOut->BAFOutv25.enable)
    {
        // Pass the BF stats structure from the current stripe configure structure (either left or right stripe in dual IFE).
        FillBFStats25CfgFromOneStripe(&(pStripeConfig->AFStatsUpdateData.statsConfig.BFStats),
            pStripeOut,
            pISPInputdata->pAFStatsUpdateData->statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension);
    }

    // IHist
    if (TRUE == pStripeOut->iHistOut.enable)
    {
        pStripeConfig->IHistStatsUpdateData.statsConfig             =
            pISPInputdata->pIHistStatsUpdateData->statsConfig;
        pStripeConfig->IHistStatsUpdateData.statsConfig.ROI.left    = pStripeOut->iHistOut.histRgnHorOffset;
        pStripeConfig->IHistStatsUpdateData.statsConfig.ROI.width   =
            (pStripeOut->iHistOut.histRgnHorNum + 1) * DefaultIHistStatsRegionWidth;
    }

    // HDR BHist
    if (TRUE == pStripeOut->hdrBhistOut.bihistEnabled)
    {
        pStripeConfig->AECStatsUpdateData.statsConfig.HDRBHistConfig            =
            pISPInputdata->pAECStatsUpdateData->statsConfig.HDRBHistConfig;
        pStripeConfig->AECStatsUpdateData.statsConfig.HDRBHistConfig.ROI.left   =
            pStripeOut->hdrBhistOut.bihistROIHorOffset;
        pStripeConfig->AECStatsUpdateData.statsConfig.HDRBHistConfig.ROI.width  =
            (pStripeOut->hdrBhistOut.bihistRgnHorNum + 1) * DefaultHDRBHistStatsRegionWidth;
    }

    // RSCS
    if (TRUE == pStripeOut->rscsOut.RSEnable)
    {
        pStripeConfig->AFDStatsUpdateData.statsConfig               = pISPInputdata->pAFDStatsUpdateData->statsConfig;
        pStripeConfig->AFDStatsUpdateData.statsConfig.statsHNum     = pStripeOut->rscsOut.RSRgnHorNum + 1;
    }

    if (TRUE == pStripeOut->rscsOut.CSEnable)
    {
        pStripeConfig->CSStatsUpdateData.statsConfig                = pISPInputdata->pCSStatsUpdateData->statsConfig;
        pStripeConfig->CSStatsUpdateData.statsConfig.statsHOffset   = pStripeOut->rscsOut.CSRgnHorOffset;
        pStripeConfig->CSStatsUpdateData.statsConfig.statsHNum      = pStripeOut->rscsOut.CSRgnHorNum + 1;
    }

    pStripeConfig->CAMIFCrop.firstPixel = pStripeOut->fetchFirstPixel;
    pStripeConfig->CAMIFCrop.lastPixel  = pStripeOut->fetchLastPixel;

    // BHist
    if (TRUE == pStripeOut->bHistOut.bihistEnabled)
    {
        pStripeConfig->AECStatsUpdateData.statsConfig.BHistConfig           =
            pISPInputdata->pAECStatsUpdateData->statsConfig.BHistConfig;
        pStripeConfig->AECStatsUpdateData.statsConfig.BHistConfig.ROI.left  =
            pStripeOut->bHistOut.bihistROIHorOffset;
        pStripeConfig->AECStatsUpdateData.statsConfig.BHistConfig.ROI.width =
            (pStripeOut->bHistOut.bihistRgnHorNum + 1) * DefaultBHistStatsRegionWidth;
    }

    UINT32  fullWidth;
    UINT32  FDWidth;
    UINT32  DS4Width;
    UINT32  DS16Width;

    // Display Full/DS4/DS16 ports support
    UINT32  dispWidth;
    UINT32  DS4DisplayWidth;
    UINT32  DS16DisplayWidth;

    // Extract output ranges
    fullWidth   = pStripeOut->outRange_full[1] - pStripeOut->outRange_full[0] + 1;
    FDWidth     = pStripeOut->outRange_fd[1] - pStripeOut->outRange_fd[0] + 1;
    DS4Width    = pStripeOut->outRange_1to4[1] - pStripeOut->outRange_1to4[0] + 1;
    DS16Width   = pStripeOut->outRange_1to16[1] - pStripeOut->outRange_1to16[0] + 1;

    // Display Full/DS4/DS16 ports support
    dispWidth        = pStripeOut->outRange_disp[1]       - pStripeOut->outRange_disp[0] + 1;
    DS4DisplayWidth  = pStripeOut->outRange_1to4_disp[1]  - pStripeOut->outRange_1to4_disp[0] + 1;
    DS16DisplayWidth = pStripeOut->outRange_1to16_disp[1] - pStripeOut->outRange_1to16_disp[0] + 1;

    pStripeConfig->stream[FDOutput].width     = FDWidth;
    pStripeConfig->stream[FDOutput].height    =
        Utils::EvenCeilingUINT32(pISPInputdata->HALData.stream[FDOutput].height);
    pStripeConfig->stream[FDOutput].offset    = pStripeOut->outRange_fd[0];
    pStripeConfig->stream[FullOutput].width   = fullWidth;
    pStripeConfig->stream[FullOutput].height  =
        Utils::EvenCeilingUINT32(pISPInputdata->HALData.stream[FullOutput].height);
    pStripeConfig->stream[FullOutput].offset  = pStripeOut->outRange_full[0];
    pStripeConfig->stream[DS4Output].width    = DS4Width;
    pStripeConfig->stream[DS4Output].height   =
        Utils::EvenCeilingUINT32(pISPInputdata->HALData.stream[DS4Output].height);
    pStripeConfig->stream[DS4Output].offset   = pStripeOut->outRange_1to4[0];
    pStripeConfig->stream[DS16Output].width   = DS16Width;
    pStripeConfig->stream[DS16Output].height  =
        Utils::EvenCeilingUINT32(pISPInputdata->HALData.stream[DS16Output].height);
    pStripeConfig->stream[DS16Output].offset  = pStripeOut->outRange_1to16[0];

    // Display Full/DS4/DS16 ports support
    pStripeConfig->stream[DisplayFullOutput].width   = dispWidth;
    pStripeConfig->stream[DisplayFullOutput].height  =
        Utils::EvenCeilingUINT32(pISPInputdata->HALData.stream[DisplayFullOutput].height);
    pStripeConfig->stream[DisplayFullOutput].offset  = pStripeOut->outRange_disp[0];

    pStripeConfig->stream[DisplayDS4Output].width    = DS4DisplayWidth;
    pStripeConfig->stream[DisplayDS4Output].height   =
        Utils::EvenCeilingUINT32(pISPInputdata->HALData.stream[DisplayDS4Output].height);
    pStripeConfig->stream[DisplayDS4Output].offset   = pStripeOut->outRange_1to4_disp[0];

    pStripeConfig->stream[DisplayDS16Output].width   = DS16DisplayWidth;
    pStripeConfig->stream[DisplayDS16Output].height  =
        Utils::EvenCeilingUINT32(pISPInputdata->HALData.stream[DisplayDS16Output].height);
    pStripeConfig->stream[DisplayDS16Output].offset  = pStripeOut->outRange_1to16_disp[0];

    // Update LCR Stripe Offsets
    pStripeConfig->stream[LCROutput].width  =
        pStripeOut->lcrOutput.lastPixel - pStripeOut->lcrOutput.firstPixel + 1;
    pStripeConfig->stream[LCROutput].height =
        Utils::EvenCeilingUINT32(pISPInputdata->HALData.stream[LCROutput].height);
    pStripeConfig->stream[LCROutput].offset = pStripeOut->lcrOutput.firstPixel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::FetchCfgWithStripeOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DualIFEUtils::FetchCfgWithStripeOutput(
    ISPInputData*       pISPInputdata,
    IFEStripingPassOutput*      pPassOut,
    ISPStripeConfig*    pStripeConfigs)
{
    uint16_t stripeNum = 0;

    CAMX_UNREFERENCED_PARAM(pISPInputdata);

    CAMX_ASSERT((NULL != pPassOut));

    IFEStripeInterfaceOutput* pStripe1 = pPassOut->pStripeOutput[0];

    CAMX_ASSERT(NULL != pStripe1);

    if (1 == pStripe1->edgeStripeLT) // left stripe
    {
        stripeNum = 0;
    }
    else if (1 == pStripe1->edgeStripeRB)// right stripe
    {
        stripeNum = 1;
    }
    pStripeConfigs[stripeNum].pStripeOutput = pStripe1;
    pStripeConfigs[stripeNum].stripeId = stripeNum;
    /* Reset Left & Right Stripe config for BE stats ROI */
    pStripeConfigs[0].AECStatsUpdateData.statsConfig.BEConfig.isStripeValid = FALSE;
    pStripeConfigs[1].AECStatsUpdateData.statsConfig.BEConfig.isStripeValid = FALSE;
    FillCfgFromOneStripe(pISPInputdata, pStripe1, &pStripeConfigs[stripeNum]);

    IFEStripeInterfaceOutput* pStripe2 = pPassOut->pStripeOutput[1];


    CAMX_ASSERT(NULL != pStripe2);

    if (1 == pStripe2->edgeStripeLT) // left stripe
    {
        stripeNum = 0;
    }
    else if (1 == pStripe2->edgeStripeRB)// right stripe
    {
        stripeNum = 1;
    }
    pStripeConfigs[stripeNum].pStripeOutput = pStripe2;
    pStripeConfigs[stripeNum].stripeId = stripeNum;
    FillCfgFromOneStripe(pISPInputdata, pStripe2, &pStripeConfigs[stripeNum]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::ComputeSplitParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DualIFEUtils::ComputeSplitParams(
    ISPInputData*          pISPInputdata,
    DualIFESplitParams*    pSplitParams,
    const SensorMode*      pSensorMode,
    IFEStripeInterface*    pStripingInterface)
{
    CamxResult            result         = CamxResultSuccess;
    const StaticSettings* pSettings      = HwEnvironment::GetInstance()->GetStaticSettings();
    UINT32                sensorOutWidth = 0;

    CAMX_ASSERT(NULL != pISPInputdata);
    CAMX_ASSERT(NULL != pSplitParams);
    CAMX_ASSERT(TRUE == pSettings->enableDualIFE);

    sensorOutWidth = pSensorMode->cropInfo.lastPixel -
                     pSensorMode->cropInfo.firstPixel + 1;
    result = UpdateStripingInput(pISPInputdata);
    IFEStripeInterfaceInput* pInput = &pISPInputdata->pStripingInput->stripingInput;
    if (CamxResultSuccess == result)
    {
        result = pStripingInterface->DeriveFetchRange(pInput);
    }
    if (CamxResultSuccess == result)
    {
        // calculate spliting point and left/right padding
        pSplitParams->splitPoint = sensorOutWidth / 2;

        if ((pSplitParams->splitPoint > static_cast<UINT32>(pInput->fetchLeftStripeEnd)) ||
            (pSplitParams->splitPoint < static_cast<UINT32>(pInput->fetchRightStripeStart)))
        {
            pSplitParams->splitPoint = (pInput->fetchRightStripeStart + pInput->fetchLeftStripeEnd + 1) / 2;
        }

        pSplitParams->leftPadding  = pSplitParams->splitPoint - pInput->fetchRightStripeStart;
        pSplitParams->rightPadding = pInput->fetchLeftStripeEnd - pSplitParams->splitPoint + 1;
        CAMX_LOG_INFO(CamxLogGroupISP,
                         "<DualIFE> leftStripeEnd = %d, rightStripeStart = %d",
                         pInput->fetchLeftStripeEnd,
                         pInput->fetchRightStripeStart);

        CAMX_LOG_INFO(CamxLogGroupISP, "<DualIFE> splitPoint = %d, leftPadding = %d, rightPadding = %d",
            pSplitParams->splitPoint, pSplitParams->leftPadding, pSplitParams->rightPadding);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::ReleaseDualIfePassResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DualIFEUtils::ReleaseDualIfePassResult(
    IFEStripingPassOutput*  pPassOut,
    IFEStripeInterface*     pIFEStripeInterface)
{
    if (pPassOut == NULL)
    {
        return;
    }

    pIFEStripeInterface->ReleaseDualIFEPassOutput();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::PrintDualIfeInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DualIFEUtils::PrintDualIfeInput(
    INT   fd,
    const IFEStripeInterfaceInput* pStripingInput)
{
    STRIPE_FIELD_PRINT(fd, pStripingInput->tiering);
    STRIPE_FIELD_PRINT(fd, pStripingInput->striping);
    STRIPE_FIELD_PRINT(fd, pStripingInput->inputFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->inputWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->inputHeight);
    STRIPE_FIELD_PRINT(fd, pStripingInput->useZoomSettingFromExternal);
    STRIPE_FIELD_PRINT(fd, pStripingInput->roiMNDSoutfull.startX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->roiMNDSoutfull.endX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->roiMNDSoutfd.startX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->roiMNDSoutfd.endX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->roiMNDSoutdisp.startX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->roiMNDSoutdisp.endX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->zoomWindow.zoomEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->zoomWindow.startX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->zoomWindow.startY);
    STRIPE_FIELD_PRINT(fd, pStripingInput->zoomWindow.width);
    STRIPE_FIELD_PRINT(fd, pStripingInput->zoomWindow.height);
    STRIPE_FIELD_PRINT(fd, pStripingInput->videofulloutFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->videofulloutWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->videofulloutHeight);
    STRIPE_FIELD_PRINT(fd, pStripingInput->videofullDSXoutWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->videofullDSXoutHeight);
    STRIPE_FIELD_PRINT(fd, pStripingInput->videofullDS4outFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->videofullDS16outFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->fdOutFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->fdOutWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->fdOutHeight);
    STRIPE_FIELD_PRINT(fd, pStripingInput->dispFulloutFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->dispFulloutWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->dispFulloutHeight);
    STRIPE_FIELD_PRINT(fd, pStripingInput->dispfullDS4outFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->dispfullDS16outFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->rawOutFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->PDAFOutFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->pedestalEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->rolloffEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BAFEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGTintlessEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGAWBEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BEEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->PDAFEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->HDREnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BPCEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->ABFEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->tappingPointBE);
    STRIPE_FIELD_PRINT(fd, pStripingInput->tapoffPointHVX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->kernelSizeHVX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->fetchLeftStripeEnd);
    STRIPE_FIELD_PRINT(fd, pStripingInput->fetchRightStripeStart);

    STRIPE_FIELD_PRINT(fd, pStripingInput->HDRInput.HDRZrecFirstRBExp);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaFD.input);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaFD.output);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaFD.roundingOptionHor);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaFD.roundingOptionVer);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaFD.input);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaFD.output);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaFD.roundingOptionHor);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaFD.roundingOptionVer);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaFull.input);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaFull.output);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaFull.roundingOptionHor);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaFull.roundingOptionVer);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaFull.input);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaFull.output);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaFull.roundingOptionHor);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaFull.roundingOptionVer);

    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS21InputLumaFull.input);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS21InputLumaFull.output);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS21InputLumaFull.roundingOptionHor);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS21InputLumaFull.roundingOptionVer);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS21InputChromaFull.input);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS21InputChromaFull.output);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS21InputChromaFull.roundingOptionHor);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS21InputChromaFull.roundingOptionVer);

    STRIPE_FIELD_PRINT(fd, pStripingInput->DSXInputVideoFullv10.chromaInputWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->DSXInputVideoFullv10.chromaOutWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->DSXInputVideoFullv10.chromaScaleRatioX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->DSXInputVideoFullv10.chromaStratLocationX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->DSXInputVideoFullv10.lumaInputWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->DSXInputVideoFullv10.lumaOutWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->DSXInputVideoFullv10.lumaScaleRatioX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->DSXInputVideoFullv10.lumaStartLocationX);

    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaDisp.input);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaDisp.output);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaDisp.roundingOptionHor);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputLumaDisp.roundingOptionVer);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaDisp.input);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaDisp.output);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaDisp.roundingOptionHor);
    STRIPE_FIELD_PRINT(fd, pStripingInput->MNDS16InputChromaDisp.roundingOptionVer);
    STRIPE_FIELD_PRINT(fd, pStripingInput->PDPCInputV11.PDAFGlobalOffsetX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->PDPCInputV11.PDAFEndX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->PDPCInputV11.PDAFzzHDRFirstRBExp);

    for (UINT32 idx = 0; idx < 64; idx++ )
    {
        STRIPE_FIELD_PRINT(fd, pStripingInput->PDPCInputV11.PDAFPDMask[idx]);
    }

    STRIPE_FIELD_PRINT(fd, pStripingInput->PDPCInputV30.enable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->PDPCInputV30.PDAFGlobalOffsetX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->PDPCInputV30.PDAFPDPCEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->PDPCInputV30.PDAFTableoffsetX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->PDPCInputV30.PDAFEndX);
    STRIPE_FIELD_PRINT(fd, pStripingInput->PDPCInputV30.PDAFzzHDRFirstRBExp);

    STRIPE_FIELD_PRINT(fd, pStripingInput->pedestalParam.enable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->pedestalParam.blockWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->pedestalParam.meshGridBwidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->pedestalParam.lxStart);
    STRIPE_FIELD_PRINT(fd, pStripingInput->pedestalParam.bxStart);
    STRIPE_FIELD_PRINT(fd, pStripingInput->pedestalParam.bx_d1_l);
    STRIPE_FIELD_PRINT(fd, pStripingInput->rollOffParam.enable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->rollOffParam.blockWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->rollOffParam.meshGridBwidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->rollOffParam.lxStart);
    STRIPE_FIELD_PRINT(fd, pStripingInput->rollOffParam.bxStart);
    STRIPE_FIELD_PRINT(fd, pStripingInput->rollOffParam.bx_d1_l);
    STRIPE_FIELD_PRINT(fd, pStripingInput->rollOffParam.numMeshgainHoriz);
    STRIPE_FIELD_PRINT(fd, pStripingInput->HDRBhistInput.bihistEnabled);
    STRIPE_FIELD_PRINT(fd, pStripingInput->HDRBhistInput.bihistROIHorOffset);
    STRIPE_FIELD_PRINT(fd, pStripingInput->HDRBhistInput.bihistRgnHorNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->bHistInput.bihistEnabled);
    STRIPE_FIELD_PRINT(fd, pStripingInput->bHistInput.bihistROIHorOffset);
    STRIPE_FIELD_PRINT(fd, pStripingInput->bHistInput.bihistRgnHorNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->iHistInput.enable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->iHistInput.histRgnHorOffset);
    STRIPE_FIELD_PRINT(fd, pStripingInput->iHistInput.histRgnHorNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGTintlessInput.BGEnabled);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGTintlessInput.BGRgnVertNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGTintlessInput.BGRgnHorizNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGTintlessInput.BGRegionSampling);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGTintlessInput.BGRgnWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGTintlessInput.BGROIHorizOffset);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGTintlessInput.BGSatOutputEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGTintlessInput.BGYOutputEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGAWBInput.BGEnabled);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGAWBInput.BGRgnVertNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGAWBInput.BGRgnHorizNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGAWBInput.BGRegionSampling);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGAWBInput.BGRgnWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGAWBInput.BGROIHorizOffset);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGAWBInput.BGSatOutputEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BGAWBInput.BGYOutputEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BEInput.BEEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BEInput.BEzzHDRFirstRBExp);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BEInput.BEROIHorizOffset);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BEInput.BERgnWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BEInput.BERgnHorizNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BEInput.BERgnVertNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BEInput.BESatOutputEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BEInput.BEYOutputEnable);
    for (UINT32 idx = 0; idx < STRIPE_BF_STATS_RGNCOUNT_V23; idx++ )
    {
        STRIPE_FIELD_PRINT_LL(fd, pStripingInput->BAFInput.BAFROIIndexLUT[idx]);
    }
    STRIPE_FIELD_PRINT(fd, pStripingInput->BAFInput.BAFHorizScalerEn);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BAFInput.BAF_fir_h1_en);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BAFInput.BAF_iir_h1_en);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BAFInput.mndsParam.roundingOptionHor);
    STRIPE_FIELD_PRINT(fd, pStripingInput->BAFInput.mndsParam.roundingOptionVer);
    STRIPE_FIELD_PRINT(fd, pStripingInput->RSCSInput.RSEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->RSCSInput.CSEnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->RSCSInput.RSRgnHorOffset);
    STRIPE_FIELD_PRINT(fd, pStripingInput->RSCSInput.RSRgnWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->RSCSInput.RSRgnHorNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->RSCSInput.RSRgnVerNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->RSCSInput.CSRgnHorOffset);
    STRIPE_FIELD_PRINT(fd, pStripingInput->RSCSInput.CSRgnWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->RSCSInput.CSRgnHorNum);
    STRIPE_FIELD_PRINT(fd, pStripingInput->RSCSInput.CSRgnVerNum);

    STRIPE_FIELD_PRINT(fd, pStripingInput->LCREnable);
    STRIPE_FIELD_PRINT(fd, pStripingInput->LCROutFormat);
    STRIPE_FIELD_PRINT(fd, pStripingInput->LCROutHeight);
    STRIPE_FIELD_PRINT(fd, pStripingInput->LCROutWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->LCRInput.blockWidth);
    STRIPE_FIELD_PRINT(fd, pStripingInput->LCRInput.firstPDCol);
    STRIPE_FIELD_PRINT(fd, pStripingInput->LCRInput.firstPixel);
    STRIPE_FIELD_PRINT(fd, pStripingInput->LCRInput.lastPixel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::PrintDualIfeOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DualIFEUtils::PrintDualIfeOutput(
    INT   fd,
    const IFEStripeInterfaceOutput* pStripingOutput)
{
    STRIPE_FIELD_PRINT(fd, pStripingOutput->edgeStripeLT);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->edgeStripeRB);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->fetchFirstPixel);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->fetchLastPixel);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->hvxTapoffFirstPixel);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->hvxTapoffLastPixel);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_fd[0]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_fd[1]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_full[0]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_full[1]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_1to4[0]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_1to4[1]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_1to16[0]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_1to16[1]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_disp[0]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_disp[1]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_1to4_disp[0]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_1to4_disp[1]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_1to16_disp[0]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_1to16_disp[1]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_raw[0]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_raw[1]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_pdaf[0]);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->outRange_pdaf[1]);

    // crop amounts
    PRINT_CROP_1D(fd, pStripingOutput->outCropFDLuma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropFDChroma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropVideoFullLuma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropVideoFullChroma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropVideoDS4Luma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropVideoDS4Chroma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropVideoDS16Luma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropVideoDS16Chroma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropRaw);
    PRINT_CROP_1D(fd, pStripingOutput->preDS4CropVideoDS4Luma);
    PRINT_CROP_1D(fd, pStripingOutput->preDS4CropVideoDS4Chroma);
    PRINT_CROP_1D(fd, pStripingOutput->preDS4CropVideoDS16Luma);
    PRINT_CROP_1D(fd, pStripingOutput->preDS4CropVideoDS16Chroma);
    STRIPE_FIELD_PRINT_LL(fd, pStripingOutput->preDSXphaseInitLuma);
    STRIPE_FIELD_PRINT_LL(fd, pStripingOutput->preDSXphaseInitChroma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropDispFullLuma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropDispFullChroma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropDispDS4Luma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropDispDS4Chroma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropDispDS16Luma);
    PRINT_CROP_1D(fd, pStripingOutput->outCropDispDS16Chroma);
    PRINT_CROP_1D(fd, pStripingOutput->preDS4CropDispDS4Luma);
    PRINT_CROP_1D(fd, pStripingOutput->preDS4CropDispDS4Chroma);
    PRINT_CROP_1D(fd, pStripingOutput->preDS4CropDispDS16Luma);
    PRINT_CROP_1D(fd, pStripingOutput->preDS4CropDispDS16Chroma);

    // Kona scaler
    PRINT_MNDS_V20(fd, pStripingOutput->mndsConfigFDLumav21);
    PRINT_MNDS_V20(fd, pStripingOutput->mndsConfigFDChromav21);
    PRINT_MNDS_V20(fd, pStripingOutput->mndsConfigVideoFullLumav21);
    PRINT_MNDS_V20(fd, pStripingOutput->mndsConfigVideoFullChromav21);
    PRINT_MNDS_V20(fd, pStripingOutput->mndsConfigDispFullLumav21);
    PRINT_MNDS_V20(fd, pStripingOutput->mndsConfigDispFullChromav21);

    //// scalers
    PRINT_MNDS_V16(fd, pStripingOutput->mndsConfigFDLuma);
    PRINT_MNDS_V16(fd, pStripingOutput->mndsConfigFDChroma);
    PRINT_MNDS_V16(fd, pStripingOutput->mndsConfigVideoFullLuma);
    PRINT_MNDS_V16(fd, pStripingOutput->mndsConfigVideoFullChroma);
    PRINT_MNDS_V16(fd, pStripingOutput->bafDownscaler);
    PRINT_MNDS_V16(fd, pStripingOutput->mndsConfigDispFullLuma);
    PRINT_MNDS_V16(fd, pStripingOutput->mndsConfigDispFullChroma);

    STRIPE_FIELD_PRINT(fd, pStripingOutput->rolloffOutStripe.gridIndex);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->rolloffOutStripe.subgridIndex);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->rolloffOutStripe.pixelIndexWithinSubgrid);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->pedestalOutStripe.gridIndex);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->pedestalOutStripe.subgridIndex);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->pedestalOutStripe.pixelIndexWithinSubgrid);

    PRINT_WE_STRIPE(fd, pStripingOutput->BGTintlessWriteEngineStripeParam);
    PRINT_WE_STRIPE(fd, pStripingOutput->BGAWBWriteEngineStripeParam);
    PRINT_WE_STRIPE(fd, pStripingOutput->BEWriteEngineStripeParam);
    PRINT_WE_STRIPE(fd, pStripingOutput->BAFWriteEngineStripeParam);
    PRINT_WE_STRIPE(fd, pStripingOutput->rowSumWriteEngineStripeParam);
    PRINT_WE_STRIPE(fd, pStripingOutput->colSumWriteEngineStripeParam);

    STRIPE_FIELD_PRINT(fd, pStripingOutput->lcrOutput.enable);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->lcrOutput.firstPixel);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->lcrOutput.lastPixel);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->lcrOutput.stripeCut);

    STRIPE_FIELD_PRINT(fd, pStripingOutput->PDPCOutv30.PDAFEndX);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->PDPCOutv30.PDAFGlobalOffset);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->PDPCOutv30.PDAFPDPCEnable);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->PDPCOutv30.PDAFTableOffset);
    STRIPE_FIELD_PRINT(fd, pStripingOutput->PDPCOutv30.PDAFzzHDRFirstRBExp);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::PrintDualIfeFrame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DualIFEUtils::PrintDualIfeFrame(
    INT   fd,
    const IFEStripingPassOutput* pIFEStripingPassOutput)
{
    for (UINT i = 0; i < 2; i++)
    {
        CAMX_LOG_INFO(CamxLogGroupISP, "stripe %d output start", i);
        PrintDualIfeOutput(fd, pIFEStripingPassOutput->pStripeOutput[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::UpdateDualIFEConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DualIFEUtils::UpdateDualIFEConfig(
    ISPInputData*            pISPInputdata,
    IFEPDAFInfo              PDAFInfo,
    ISPStripeConfig*         pStripeConfig,
    DualIFESplitParams*      pSplitParams,
    IFEStripingPassOutput*   pPassOut,
    const SensorMode*        pSensorMode,
    IFEStripeInterface*      pStripingInterface,
    const CHAR*              pNodeString)
{
    CamxResult            result                           = CamxResultSuccess;
    const StaticSettings* pSettings                        = HwEnvironment::GetInstance()->GetStaticSettings();
    UINT32                splitPoint                       = pSplitParams->splitPoint;
    UINT32                rightPadding                     = pSplitParams->rightPadding;
    UINT32                leftPadding                      = pSplitParams->leftPadding;
    FLOAT                 scaleratio                       = 1.0f;
    FLOAT                 splitRatio[IFEMaxNonCommonPaths] = { 0.0f };
    UINT32                fullAlign                        = 2;
    BOOL                  midNextAlignemnt                 = TRUE;
    UINT32                rightStripeFirstPixel            = 0;
    UINT32                sensorOutHeight                  = 0;
    UINT32                sensorOutWidth                   = 0;

    sensorOutHeight = pSensorMode->cropInfo.lastLine - pSensorMode->cropInfo.firstLine + 1;
    sensorOutWidth  = pSensorMode->cropInfo.lastPixel - pSensorMode->cropInfo.firstPixel + 1;

    UINT32 leftWidth    = splitPoint + rightPadding;
    UINT32 rightWidth   = sensorOutWidth - splitPoint + leftPadding;
    FLOAT inputSplitRatio = static_cast<FLOAT>(pSplitParams->splitPoint) / sensorOutWidth;

    // Update stats module output data
    pStripeConfig[0].AECUpdateData = *pISPInputdata->pAECUpdateData;
    pStripeConfig[1].AECUpdateData = *pISPInputdata->pAECUpdateData;
    pStripeConfig[0].AWBUpdateData = *pISPInputdata->pAWBUpdateData;
    pStripeConfig[1].AWBUpdateData = *pISPInputdata->pAWBUpdateData;

    result = UpdateStripingInput(pISPInputdata);
    if (CamxResultSuccess == result)
    {

        if (TRUE == pISPInputdata->enableIFEDualStripeLog)
        {
            CHAR  dumpFilename[256];
            FILE* pFile = NULL;
            OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                "%s/%s_IFE_StripeInputdump_%llu.txt", ConfigFileDirectory,
                pNodeString, pISPInputdata->frameNum);
            pFile = OsUtils::FOpen(dumpFilename, "w");
            if (NULL != pFile)
            {
                PrintDualIfeInput(OsUtils::FileNo(pFile), &pISPInputdata->pStripingInput->stripingInput);
                OsUtils::FClose(pFile);
            }
        }

        IFEStripeInterfaceInput* pInput = &pISPInputdata->pStripingInput->stripingInput;
        // Use striping library to get the per stripe configuration

        pStripingInterface->DeriveStriping(pInput, pPassOut);

        if (TRUE == pISPInputdata->enableIFEDualStripeLog)
        {
            CHAR  dumpFilename[256];
            FILE* pFile = NULL;
            OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                "%s/%s_IFE_StripeOutputdump_%llu.txt", ConfigFileDirectory,
                pNodeString, pISPInputdata->frameNum);
            pFile = OsUtils::FOpen(dumpFilename, "w");
            if (NULL != pFile)
            {
                PrintDualIfeFrame(OsUtils::FileNo(pFile), pPassOut);
                OsUtils::FClose(pFile);
            }
        }

        if (CamxResultSuccess == result)
        {
            DualIFEUtils::FetchCfgWithStripeOutput(pISPInputdata, pPassOut, pStripeConfig);
            if (pISPInputdata->HALData.stream[PixelRawOutput].width > 0)
            {
                pStripeConfig[0].stream[PixelRawOutput].width     = pSplitParams->splitPoint;
                pStripeConfig[0].stream[PixelRawOutput].height    =
                    Utils::EvenCeilingUINT32(pISPInputdata->HALData.stream[PixelRawOutput].height);
                pStripeConfig[0].stream[PixelRawOutput].offset    = 0;

                pStripeConfig[1].stream[PixelRawOutput].width     = pSplitParams->splitPoint;
                pStripeConfig[1].stream[PixelRawOutput].height    =
                    Utils::EvenCeilingUINT32(pISPInputdata->HALData.stream[PixelRawOutput].height);
                pStripeConfig[1].stream[PixelRawOutput].offset    = sensorOutWidth -
                    pSplitParams->splitPoint;

                if (PDAFInfo.enableSubsample)
                {
                    UINT32 leftSplitWidth   = ((pSplitParams->splitPoint - 1) | 0xF) + 1;
                    UINT32 rightStripeStart = splitPoint - leftPadding;

                    pStripeConfig[0].CAMIFSubsampleInfo.PDAFCAMIFCrop.firstPixel = 0;
                    pStripeConfig[0].CAMIFSubsampleInfo.PDAFCAMIFCrop.lastPixel  = leftSplitWidth - 1;
                    pStripeConfig[0].CAMIFSubsampleInfo.PDAFCAMIFCrop.firstLine  = 0;
                    pStripeConfig[0].CAMIFSubsampleInfo.PDAFCAMIFCrop.lastLine   =
                        sensorOutHeight - 1;
                    pStripeConfig[0].stream[PDAFRawOutput].offset                = 0;
                    pStripeConfig[0].stream[PDAFRawOutput].width                 =
                        (leftSplitWidth / 16) * GetPixelsInSkipPattern(PDAFInfo.pixelSkipPattern);
                    pStripeConfig[0].stream[PDAFRawOutput].height                =
                        PDAFInfo.bufferHeight;

                    pStripeConfig[1].CAMIFSubsampleInfo.PDAFCAMIFCrop.firstPixel = leftSplitWidth - rightStripeStart;
                    pStripeConfig[1].CAMIFSubsampleInfo.PDAFCAMIFCrop.lastPixel  = rightWidth - 1;
                    pStripeConfig[1].CAMIFSubsampleInfo.PDAFCAMIFCrop.firstLine  = 0;
                    pStripeConfig[1].CAMIFSubsampleInfo.PDAFCAMIFCrop.lastLine   =
                        sensorOutHeight - 1;
                    pStripeConfig[1].stream[PDAFRawOutput].offset                =
                        pStripeConfig[0].stream[PDAFRawOutput].width;
                    pStripeConfig[1].stream[PDAFRawOutput].width                 =
                        PDAFInfo.bufferWidth - pStripeConfig[0].stream[PDAFRawOutput].width;
                    pStripeConfig[1].stream[PDAFRawOutput].height                =
                        PDAFInfo.bufferHeight;
                }
            }
        }
    }
    if (CamxResultSuccess == result)
    {
        pStripeConfig[0].CAMIFCrop  = {0, 0, (leftWidth - 1), (sensorOutHeight - 1) };
        pStripeConfig[1].CAMIFCrop  =
        {
            sensorOutWidth - rightWidth,
            0,
            (sensorOutWidth - 1),
            (sensorOutHeight - 1)
        };

        UINT32 rightMinusLeftPadding = (rightPadding > leftPadding) ? (rightPadding - leftPadding) : 0;
        pStripeConfig[0].stateHVX.overlap = rightMinusLeftPadding + 1;
        pStripeConfig[1].stateHVX.overlap = rightMinusLeftPadding + 1;
        pStripeConfig[0].stateHVX.rightImageOffset = rightPadding;
        pStripeConfig[1].stateHVX.rightImageOffset = rightPadding;


        if (TRUE == pISPInputdata->HVXData.DSEnabled)
        {
            scaleratio = static_cast<FLOAT>(pISPInputdata->HVXData.HVXOut.width) /
                static_cast<FLOAT>(pISPInputdata->sensorData.sensorOut.width);
        }
        splitPoint   = static_cast<UINT32>(scaleratio * pSplitParams->splitPoint);
        rightPadding = static_cast<UINT32>(scaleratio * pSplitParams->rightPadding);
        leftPadding  = static_cast<UINT32>(scaleratio * pSplitParams->leftPadding);
        pStripeConfig[0].stateHVX.inputWidth  = leftWidth;
        pStripeConfig[1].stateHVX.inputWidth  = rightWidth;
        pStripeConfig[0].stateHVX.inputHeight = sensorOutHeight;
        pStripeConfig[1].stateHVX.inputHeight = sensorOutHeight;

        pStripeConfig[0].stateHVX.hvxOutDimension.width = static_cast<UINT32>(scaleratio * leftWidth);
        pStripeConfig[1].stateHVX.hvxOutDimension.width = static_cast<UINT32>(scaleratio * rightWidth);
        pStripeConfig[0].stateHVX.hvxOutDimension.height = pISPInputdata->HVXData.HVXOut.height;
        pStripeConfig[1].stateHVX.hvxOutDimension.height = pISPInputdata->HVXData.HVXOut.height;

        if ((1 <= pStripeConfig[0].stateHVX.hvxOutDimension.width) && (1 <= pISPInputdata->HVXData.HVXOut.height))
        {
            pStripeConfig[0].stateHVX.cropWindow =
            {
                0,
                0,
                (pStripeConfig[0].stateHVX.hvxOutDimension.width - 1),
                (pISPInputdata->HVXData.HVXOut.height - 1)
            };
        }

        if ((1 <= pISPInputdata->HVXData.HVXOut.width) && (1 <= pISPInputdata->HVXData.HVXOut.height))
        {
            UINT32 HVXOutDimensionWidthDiff =
                (pStripeConfig[0].stateHVX.hvxOutDimension.width > pStripeConfig[1].stateHVX.hvxOutDimension.width) ?
                (pStripeConfig[0].stateHVX.hvxOutDimension.width - pStripeConfig[1].stateHVX.hvxOutDimension.width) : 0;

            pStripeConfig[1].stateHVX.cropWindow =
            {
                HVXOutDimensionWidthDiff,
                0,
                (pISPInputdata->HVXData.HVXOut.width - 1),
                (pISPInputdata->HVXData.HVXOut.height - 1)
            };
        }

        if (TRUE == pISPInputdata->HVXData.DSEnabled)
        {
            rightStripeFirstPixel = pStripeConfig[1].stateHVX.cropWindow.firstPixel;
        }
        else
        {
            rightStripeFirstPixel = pStripeConfig[1].CAMIFCrop.firstPixel;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                            "Stripinglib stripe (LEFT) output dim (offset, w, h):"
                            "full(%d,%d,%d), fd(%d,%d,%d), ds4(%d,%d,%d), ds16(%d,%d,%d) pixelRaw(%d,%d,%d)"
                            "disp(%d,%d,%d), ds4(%d,%d,%d), ds16(%d,%d,%d)",
                            pStripeConfig[0].stream[FullOutput].offset,
                            pStripeConfig[0].stream[FullOutput].width,
                            pStripeConfig[0].stream[FullOutput].height,
                            pStripeConfig[0].stream[FDOutput].offset,
                            pStripeConfig[0].stream[FDOutput].width,
                            pStripeConfig[0].stream[FDOutput].height,
                            pStripeConfig[0].stream[DS4Output].offset,
                            pStripeConfig[0].stream[DS4Output].width,
                            pStripeConfig[0].stream[DS4Output].height,
                            pStripeConfig[0].stream[DS16Output].offset,
                            pStripeConfig[0].stream[DS16Output].width,
                            pStripeConfig[0].stream[DS16Output].height,
                            pStripeConfig[0].stream[PixelRawOutput].offset,
                            pStripeConfig[0].stream[PixelRawOutput].width,
                            pStripeConfig[0].stream[PixelRawOutput].height,
                            pStripeConfig[0].stream[DisplayFullOutput].offset,
                            pStripeConfig[0].stream[DisplayFullOutput].width,
                            pStripeConfig[0].stream[DisplayFullOutput].height,
                            pStripeConfig[0].stream[DisplayDS4Output].offset,
                            pStripeConfig[0].stream[DisplayDS4Output].width,
                            pStripeConfig[0].stream[DisplayDS4Output].height,
                            pStripeConfig[0].stream[DisplayDS16Output].offset,
                            pStripeConfig[0].stream[DisplayDS16Output].width,
                            pStripeConfig[0].stream[DisplayDS16Output].height);

        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                            "Stripinglib stripe (RIGHT) output dim (offset, w, h):"
                            "full(%d,%d,%d), fd(%d,%d,%d), ds4(%d,%d,%d), ds16(%d,%d,%d) pixelRaw(%d,%d,%d)"
                            "disp(%d,%d,%d), ds4(%d,%d,%d), ds16(%d,%d,%d)",
                            pStripeConfig[1].stream[FullOutput].offset,
                            pStripeConfig[1].stream[FullOutput].width,
                            pStripeConfig[1].stream[FullOutput].height,
                            pStripeConfig[1].stream[FDOutput].offset,
                            pStripeConfig[1].stream[FDOutput].width,
                            pStripeConfig[1].stream[FDOutput].height,
                            pStripeConfig[1].stream[DS4Output].offset,
                            pStripeConfig[1].stream[DS4Output].width,
                            pStripeConfig[1].stream[DS4Output].height,
                            pStripeConfig[1].stream[DS16Output].offset,
                            pStripeConfig[1].stream[DS16Output].width,
                            pStripeConfig[1].stream[DS16Output].height,
                            pStripeConfig[1].stream[PixelRawOutput].offset,
                            pStripeConfig[1].stream[PixelRawOutput].width,
                            pStripeConfig[1].stream[PixelRawOutput].height,
                            pStripeConfig[1].stream[DisplayFullOutput].offset,
                            pStripeConfig[1].stream[DisplayFullOutput].width,
                            pStripeConfig[1].stream[DisplayFullOutput].height,
                            pStripeConfig[1].stream[DisplayDS4Output].offset,
                            pStripeConfig[1].stream[DisplayDS4Output].width,
                            pStripeConfig[1].stream[DisplayDS4Output].height,
                            pStripeConfig[1].stream[DisplayDS16Output].offset,
                            pStripeConfig[1].stream[DisplayDS16Output].width,
                            pStripeConfig[1].stream[DisplayDS16Output].height);

        CAMX_ASSERT(0 != pISPInputdata->sensorData.sensorOut.width);
        splitRatio[FullOutput] = (static_cast<FLOAT>(pSplitParams->splitPoint) / sensorOutWidth);
        CropWindow halCrop = pISPInputdata->pHALTagsData->HALCrop;

        splitRatio[FullOutput]                      = static_cast<FLOAT>(pStripeConfig[0].stream[FullOutput].width) /
            pISPInputdata->HALData.stream[FullOutput].width;
        pStripeConfig[0].HALCrop[FullOutput].height = halCrop.height;
        pStripeConfig[0].HALCrop[FullOutput].top    = halCrop.top;
        pStripeConfig[0].HALCrop[FullOutput].left   = halCrop.left;
        pStripeConfig[0].HALCrop[FullOutput].width  = static_cast<UINT32>(inputSplitRatio * halCrop.width);

        splitRatio[FDOutput]                        = static_cast<FLOAT>(pStripeConfig[0].stream[FDOutput].width) /
            pISPInputdata->HALData.stream[FDOutput].width;
        pStripeConfig[0].HALCrop[FDOutput].height = halCrop.height;
        pStripeConfig[0].HALCrop[FDOutput].top    = halCrop.top;
        pStripeConfig[0].HALCrop[FDOutput].left   = halCrop.left;
        pStripeConfig[0].HALCrop[FDOutput].width  = static_cast<UINT32>(inputSplitRatio * halCrop.width);

        splitRatio[DS4Output]                       = static_cast<FLOAT>(pStripeConfig[0].stream[DS4Output].width) /
            pISPInputdata->HALData.stream[DS4Output].width;
        pStripeConfig[0].HALCrop[DS4Output].height = halCrop.height;
        pStripeConfig[0].HALCrop[DS4Output].top    = halCrop.top;
        pStripeConfig[0].HALCrop[DS4Output].left   = halCrop.left;
        pStripeConfig[0].HALCrop[DS4Output].width  = static_cast<UINT32>(inputSplitRatio * halCrop.width);

        splitRatio[DS16Output]                      = static_cast<FLOAT>(pStripeConfig[0].stream[DS16Output].width) /
            pISPInputdata->HALData.stream[DS16Output].width;
        pStripeConfig[0].HALCrop[DS16Output].height = halCrop.height;
        pStripeConfig[0].HALCrop[DS16Output].top    = halCrop.top;
        pStripeConfig[0].HALCrop[DS16Output].left   = halCrop.left;
        pStripeConfig[0].HALCrop[DS16Output].width  = static_cast<UINT32>(inputSplitRatio * halCrop.width);

        if (pISPInputdata->HALData.stream[PixelRawOutput].width > 0)
        {
            pStripeConfig[0].HALCrop[PixelRawOutput].height = pISPInputdata->sensorData.sensorOut.height;
            pStripeConfig[0].HALCrop[PixelRawOutput].top    = 0;
            pStripeConfig[0].HALCrop[PixelRawOutput].left   = 0;
            pStripeConfig[0].HALCrop[PixelRawOutput].width  = pStripeConfig[0].stream[PixelRawOutput].width;
        }

        splitRatio[DisplayFullOutput]                      =
            static_cast<FLOAT>(pStripeConfig[0].stream[DisplayFullOutput].width) /
            pISPInputdata->HALData.stream[DisplayFullOutput].width;
        pStripeConfig[0].HALCrop[DisplayFullOutput].height = halCrop.height;
        pStripeConfig[0].HALCrop[DisplayFullOutput].top    = halCrop.top;
        pStripeConfig[0].HALCrop[DisplayFullOutput].left   = halCrop.left;
        pStripeConfig[0].HALCrop[DisplayFullOutput].width  = static_cast<UINT32>(inputSplitRatio * halCrop.width);

        splitRatio[DisplayDS4Output]                      =
            static_cast<FLOAT>(pStripeConfig[0].stream[DisplayDS4Output].width) /
            pISPInputdata->HALData.stream[DisplayDS4Output].width;
        pStripeConfig[0].HALCrop[DisplayDS4Output].height = halCrop.height;
        pStripeConfig[0].HALCrop[DisplayDS4Output].top    = halCrop.top;
        pStripeConfig[0].HALCrop[DisplayDS4Output].left   = halCrop.left;
        pStripeConfig[0].HALCrop[DisplayDS4Output].width  = static_cast<UINT32>(inputSplitRatio * halCrop.width);

        splitRatio[DisplayDS16Output]                      =
            static_cast<FLOAT>(pStripeConfig[0].stream[DisplayDS16Output].width) /
            pISPInputdata->HALData.stream[DisplayDS16Output].width;
        pStripeConfig[0].HALCrop[DisplayDS16Output].height = halCrop.height;
        pStripeConfig[0].HALCrop[DisplayDS16Output].top    = halCrop.top;
        pStripeConfig[0].HALCrop[DisplayDS16Output].left   = halCrop.left;
        pStripeConfig[0].HALCrop[DisplayDS16Output].width  = static_cast<UINT32>(inputSplitRatio * halCrop.width);

        // HAL crop should be releative to CAMIF crop
        pStripeConfig[1].HALCrop[FDOutput].height   = halCrop.height;
        pStripeConfig[1].HALCrop[FDOutput].top      = halCrop.top;
        pStripeConfig[1].HALCrop[FDOutput].left     = pSplitParams->leftPadding;
        pStripeConfig[1].HALCrop[FDOutput].width    = halCrop.width - pStripeConfig[0].HALCrop[FDOutput].width;

        pStripeConfig[1].HALCrop[FullOutput].height = halCrop.height;
        pStripeConfig[1].HALCrop[FullOutput].top    = halCrop.top;
        pStripeConfig[1].HALCrop[FullOutput].left   = pSplitParams->leftPadding;
        pStripeConfig[1].HALCrop[FullOutput].width  = halCrop.width - pStripeConfig[0].HALCrop[FullOutput].width;

        pStripeConfig[1].HALCrop[DS4Output].height  = halCrop.height;
        pStripeConfig[1].HALCrop[DS4Output].top     = halCrop.top;
        pStripeConfig[1].HALCrop[DS4Output].left    = pSplitParams->leftPadding;
        pStripeConfig[1].HALCrop[DS4Output].width   = halCrop.width - pStripeConfig[0].HALCrop[DS4Output].width;

        pStripeConfig[1].HALCrop[DS16Output].height = halCrop.height;
        pStripeConfig[1].HALCrop[DS16Output].top    = halCrop.top;
        pStripeConfig[1].HALCrop[DS16Output].left   = pSplitParams->leftPadding;
        pStripeConfig[1].HALCrop[DS16Output].width  = halCrop.width - pStripeConfig[0].HALCrop[DS16Output].width;

        if (pISPInputdata->HALData.stream[PixelRawOutput].width > 0)
        {
            pStripeConfig[1].HALCrop[PixelRawOutput].height = sensorOutHeight;
            pStripeConfig[1].HALCrop[PixelRawOutput].top    = 0;
            pStripeConfig[1].HALCrop[PixelRawOutput].left   = pSplitParams->leftPadding;
            pStripeConfig[1].HALCrop[PixelRawOutput].width  = pStripeConfig[1].stream[PixelRawOutput].width;
        }

        pStripeConfig[1].HALCrop[DisplayFullOutput].height = halCrop.height;
        pStripeConfig[1].HALCrop[DisplayFullOutput].top    = halCrop.top;
        pStripeConfig[1].HALCrop[DisplayFullOutput].left   = pSplitParams->leftPadding;
        pStripeConfig[1].HALCrop[DisplayFullOutput].width  = halCrop.width - pStripeConfig[0].HALCrop[DisplayFullOutput].width;

        pStripeConfig[1].HALCrop[DisplayDS4Output].height  = halCrop.height;
        pStripeConfig[1].HALCrop[DisplayDS4Output].top     = halCrop.top;
        pStripeConfig[1].HALCrop[DisplayDS4Output].left    = pSplitParams->leftPadding;
        pStripeConfig[1].HALCrop[DisplayDS4Output].width   = halCrop.width - pStripeConfig[0].HALCrop[DisplayDS4Output].width;

        pStripeConfig[1].HALCrop[DisplayDS16Output].height = halCrop.height;
        pStripeConfig[1].HALCrop[DisplayDS16Output].top    = halCrop.top;
        pStripeConfig[1].HALCrop[DisplayDS16Output].left   = pSplitParams->leftPadding;
        pStripeConfig[1].HALCrop[DisplayDS16Output].width  = halCrop.width - pStripeConfig[0].HALCrop[DisplayDS16Output].width;

        CAMX_LOG_INFO(CamxLogGroupISP,
                            "<dualIFE> Overwritten stripe (LEFT) output dim (offset, w, h):"
                            "full(%d,%d,%d), fd(%d,%d,%d), ds4(%d,%d,%d), ds16(%d,%d,%d)"
                            "disp(%d,%d,%d), ds4(%d,%d,%d), ds16(%d,%d,%d)",
                            pStripeConfig[0].stream[FullOutput].offset,
                            pStripeConfig[0].stream[FullOutput].width,
                            pStripeConfig[0].stream[FullOutput].height,
                            pStripeConfig[0].stream[FDOutput].offset,
                            pStripeConfig[0].stream[FDOutput].width,
                            pStripeConfig[0].stream[FDOutput].height,
                            pStripeConfig[0].stream[DS4Output].offset,
                            pStripeConfig[0].stream[DS4Output].width,
                            pStripeConfig[0].stream[DS4Output].height,
                            pStripeConfig[0].stream[DS16Output].offset,
                            pStripeConfig[0].stream[DS16Output].width,
                            pStripeConfig[0].stream[DS16Output].height,
                            pStripeConfig[0].stream[DisplayFullOutput].offset,
                            pStripeConfig[0].stream[DisplayFullOutput].width,
                            pStripeConfig[0].stream[DisplayFullOutput].height,
                            pStripeConfig[0].stream[DisplayDS4Output].offset,
                            pStripeConfig[0].stream[DisplayDS4Output].width,
                            pStripeConfig[0].stream[DisplayDS4Output].height,
                            pStripeConfig[0].stream[DisplayDS16Output].offset,
                            pStripeConfig[0].stream[DisplayDS16Output].width,
                            pStripeConfig[0].stream[DisplayDS16Output].height);

        CAMX_LOG_INFO(CamxLogGroupISP,
                            "Overwritten stripe (RIGHT) output dim (offset, w, h):"
                            "full(%d,%d,%d), fd(%d,%d,%d), ds4(%d,%d,%d), ds16(%d,%d,%d)"
                            "disp(%d,%d,%d), ds4(%d,%d,%d), ds16(%d,%d,%d)",
                            pStripeConfig[1].stream[FullOutput].offset,
                            pStripeConfig[1].stream[FullOutput].width,
                            pStripeConfig[1].stream[FullOutput].height,
                            pStripeConfig[1].stream[FDOutput].offset,
                            pStripeConfig[1].stream[FDOutput].width,
                            pStripeConfig[1].stream[FDOutput].height,
                            pStripeConfig[1].stream[DS4Output].offset,
                            pStripeConfig[1].stream[DS4Output].width,
                            pStripeConfig[1].stream[DS4Output].height,
                            pStripeConfig[1].stream[DS16Output].offset,
                            pStripeConfig[1].stream[DS16Output].width,
                            pStripeConfig[1].stream[DS16Output].height,
                            pStripeConfig[1].stream[DisplayFullOutput].offset,
                            pStripeConfig[1].stream[DisplayFullOutput].width,
                            pStripeConfig[1].stream[DisplayFullOutput].height,
                            pStripeConfig[1].stream[DisplayDS4Output].offset,
                            pStripeConfig[1].stream[DisplayDS4Output].width,
                            pStripeConfig[1].stream[DisplayDS4Output].height,
                            pStripeConfig[1].stream[DisplayDS16Output].offset,
                            pStripeConfig[1].stream[DisplayDS16Output].width,
                            pStripeConfig[1].stream[DisplayDS16Output].height);

        // Update left and right stripe width for each of the stats module
        pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatHDRBE)].width       =
            pStripeConfig[0].AECStatsUpdateData.statsConfig.BEConfig.horizontalNum *
            pISPInputdata->pAECStatsUpdateData->statsConfig.BEConfig.verticalNum * HDRBEStatsOutputSizePerRegion;
        pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatHDRBE)].width       =
            pStripeConfig[1].AECStatsUpdateData.statsConfig.BEConfig.horizontalNum *
            pISPInputdata->pAECStatsUpdateData->statsConfig.BEConfig.verticalNum * HDRBEStatsOutputSizePerRegion;

        pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatTintlessBG)].width  =
            pStripeConfig[0].AECStatsUpdateData.statsConfig.TintlessBGConfig.horizontalNum *
            pISPInputdata->pAECStatsUpdateData->statsConfig.TintlessBGConfig.verticalNum * TintlessStatsOutputSizePerRegion;
        pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatTintlessBG)].width  =
            pStripeConfig[1].AECStatsUpdateData.statsConfig.TintlessBGConfig.horizontalNum *
            pISPInputdata->pAECStatsUpdateData->statsConfig.TintlessBGConfig.verticalNum * TintlessStatsOutputSizePerRegion;

        pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatAWBBG)].width       =
            pStripeConfig[0].AWBStatsUpdateData.statsConfig.BGConfig.horizontalNum *
            pISPInputdata->pAWBStatsUpdateData->statsConfig.BGConfig.verticalNum * AWBBGStatsOutputSizePerRegion;
        pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatAWBBG)].width       =
            pStripeConfig[1].AWBStatsUpdateData.statsConfig.BGConfig.horizontalNum *
            pISPInputdata->pAWBStatsUpdateData->statsConfig.BGConfig.verticalNum * AWBBGStatsOutputSizePerRegion;

        if (0 == pStripeConfig[0].AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension)
        {
            pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBF)].width =
                (1 + BFStatsNumOfFramgTagEntry) * BFStatsOutputSizePerRegion;
        }
        else
        {
            pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBF)].width =
                (pStripeConfig[0].AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension
                    + BFStatsNumOfFramgTagEntry) * BFStatsOutputSizePerRegion;
        }

        if (0 == pStripeConfig[1].AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension)
        {
            pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBF)].width =
                (1 + BFStatsNumOfFramgTagEntry) * BFStatsOutputSizePerRegion;
        }
        else
        {
            pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBF)].width =
                (pStripeConfig[1].AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension
                    + BFStatsNumOfFramgTagEntry) * BFStatsOutputSizePerRegion;
        }


        CAMX_LOG_INFO(CamxLogGroupISP,
                            "<dualIFE> Overwritten stripe (LEFT) ouput dim (offset, w, h):"
                            "full(%d,%d,%d), fd(%d,%d,%d), ds4(%d,%d,%d), ds16(%d,%d,%d) BF(%d %d %d noROI %d)",
                            pStripeConfig[0].stream[FullOutput].offset,
                            pStripeConfig[0].stream[FullOutput].width,
                            pStripeConfig[0].stream[FullOutput].height,
                            pStripeConfig[0].stream[FDOutput].offset,
                            pStripeConfig[0].stream[FDOutput].width,
                            pStripeConfig[0].stream[FDOutput].height,
                            pStripeConfig[0].stream[DS4Output].offset,
                            pStripeConfig[0].stream[DS4Output].width,
                            pStripeConfig[0].stream[DS4Output].height,
                            pStripeConfig[0].stream[DS16Output].offset,
                            pStripeConfig[0].stream[DS16Output].width,
                            pStripeConfig[0].stream[DS16Output].height,
                            pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBF)].width,
                            pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBF)].height,
                            pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBF)].offset,
                            pStripeConfig[0].AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension);

        CAMX_LOG_INFO(CamxLogGroupISP,
                            "Overwritten stripe (RIGHT) ouput dim (offset, w, h):"
                            "full(%d,%d,%d), fd(%d,%d,%d), ds4(%d,%d,%d), ds16(%d,%d,%d) BF(%d %d %d noROI %d)",
                            pStripeConfig[1].stream[FullOutput].offset,
                            pStripeConfig[1].stream[FullOutput].width,
                            pStripeConfig[1].stream[FullOutput].height,
                            pStripeConfig[1].stream[FDOutput].offset,
                            pStripeConfig[1].stream[FDOutput].width,
                            pStripeConfig[1].stream[FDOutput].height,
                            pStripeConfig[1].stream[DS4Output].offset,
                            pStripeConfig[1].stream[DS4Output].width,
                            pStripeConfig[1].stream[DS4Output].height,
                            pStripeConfig[1].stream[DS16Output].offset,
                            pStripeConfig[1].stream[DS16Output].width,
                            pStripeConfig[1].stream[DS16Output].height,
                            pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBF)].width,
                            pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBF)].height,
                            pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBF)].offset,
                            pStripeConfig[1].AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension);

        pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBHIST)].width       = BHistStatsWidth;
        pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatBHIST)].width       = BHistStatsWidth;

        pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatHDRBHIST)].width    = HDRBHistStatsMaxWidth;
        pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatHDRBHIST)].width    = HDRBHistStatsMaxWidth;

        pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatIHIST)].width       = IHistStatsWidth;
        pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatIHIST)].width       = IHistStatsWidth;

        pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatRS)].width = RSStatsWidth;
        pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatRS)].width = RSStatsWidth;

        pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatCS)].width = CSStatsWidth;
        pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatCS)].width = CSStatsWidth;

        pStripeConfig[0].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatTintlessBG)].width = TintlessBGStatsWidth;
        pStripeConfig[1].stats[CSLIFEStatsPortIndex(CSLIFEPortIdStatTintlessBG)].width = TintlessBGStatsWidth;

    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DualIFEUtils::UpdateStripingInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DualIFEUtils::UpdateStripingInput(
    ISPInputData* pISPInputdata)
{
    IFEStripeInterfaceInput* pInput = &pISPInputdata->pStripingInput->stripingInput;
    CamxResult              result = CamxResultSuccess;

    switch (pISPInputdata->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
            pInput->tiering = TITAN_150_V1;
            break;
        case CSLCameraTitanVersion::CSLTitan170:
            pInput->tiering = TITAN_170_V1;
            break;
        case CSLCameraTitanVersion::CSLTitan175:
        case CSLCameraTitanVersion::CSLTitan160:
            // Both CSLTitan175 and CSLTitan160 have the same IFE IQ h/w
            pInput->tiering = TITAN_175_V1;
            break;
        case CSLCameraTitanVersion::CSLTitan480:
            pInput->tiering = TITAN_480_V1;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "Invalid Camera Titan Version",
                           pISPInputdata->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    pInput->striping            = 1;
    pInput->inputFormat         = TranslateInputFormatToStripingLib(pISPInputdata->sensorData.format,
        pISPInputdata->sensorBitWidth);
    pInput->inputWidth          = static_cast<int16_t>(pISPInputdata->pStripeConfig->CAMIFCrop.lastPixel -
        pISPInputdata->pStripeConfig->CAMIFCrop.firstPixel + 1);
    pInput->inputHeight         = static_cast<int16_t>(pISPInputdata->pStripeConfig->CAMIFCrop.lastLine -
        pISPInputdata->pStripeConfig->CAMIFCrop.firstLine + 1);

#if DUALIFE_STRIPINGLIB_ZOOMS
    pInput->useZoomSettingFromExternal = FALSE;
    pInput->zoomWindow.startX = static_cast<int16_t>(pISPInputdata->pHALTagsData->HALCrop.left);
    pInput->zoomWindow.start_y = static_cast<int16_t>(pISPInputdata->pHALTagsData->HALCrop.top);
    pInput->zoomWindow.width   = static_cast<int16_t>(pISPInputdata->pHALTagsData->HALCrop.width);
    pInput->zoomWindow.height  = static_cast<int16_t>(pISPInputdata->pHALTagsData->HALCrop.height);
#else
    pInput->useZoomSettingFromExternal  = TRUE;
    if (pISPInputdata->pCalculatedData->fullOutCrop.lastPixel == 0)
    {
        pISPInputdata->pCalculatedData->fullOutCrop.firstPixel                   = 0;
        pISPInputdata->pCalculatedData->fullOutCrop.lastPixel                    = pInput->inputWidth - 1;
        pISPInputdata->HALData.format[FullOutput]                                = Format::YUV420NV12;
        pISPInputdata->HALData.stream[FullOutput].width                          = pInput->inputWidth;
        pISPInputdata->HALData.stream[FullOutput].height                         = pInput->inputHeight;
        pISPInputdata->HALData.format[DS4Output ]                                = Format::PD10;
        pISPInputdata->HALData.format[DS16Output ]                               = Format::PD10;
        pISPInputdata->pCalculatedData->scalerOutput[FullOutput].dimension.width = pInput->inputWidth;
        pISPInputdata->pCalculatedData->scalerOutput[FullOutput].input.width     = pInput->inputWidth;
        pInput->videofulloutFormat                                                   = static_cast<int16_t>(Format::YUV420NV12);
        pInput->videofulloutWidth                                                    = pInput->inputWidth;
        pInput->videofulloutHeight                                                   = pInput->inputHeight;
        CAMX_LOG_INFO(CamxLogGroupISP, "<dualIFE> full out not enabled: override with default");
    }
    pInput->roiMNDSoutfull.startX       = static_cast<uint16_t>(pISPInputdata->pCalculatedData->fullOutCrop.firstPixel);
    pInput->roiMNDSoutfull.endX         = static_cast<uint16_t>(pISPInputdata->pCalculatedData->fullOutCrop.lastPixel);
    pInput->roiMNDSoutfd.startX         = static_cast<uint16_t>(pISPInputdata->pCalculatedData->fdOutCrop.firstPixel);
    pInput->roiMNDSoutfd.endX           = static_cast<uint16_t>(pISPInputdata->pCalculatedData->fdOutCrop.lastPixel);
    pInput->roiMNDSoutdisp.startX       = static_cast<uint16_t>(pISPInputdata->pCalculatedData->dispOutCrop.firstPixel);
    pInput->roiMNDSoutdisp.endX         = static_cast<uint16_t>(pISPInputdata->pCalculatedData->dispOutCrop.lastPixel);
    CAMX_LOG_INFO(CamxLogGroupISP, "<Scaler> full startX %d endX %d fd startX %d endX %d disp startX %d endX %d",
        pISPInputdata->pCalculatedData->fullOutCrop.firstPixel, pISPInputdata->pCalculatedData->fullOutCrop.lastPixel,
        pISPInputdata->pCalculatedData->fdOutCrop.firstPixel, pISPInputdata->pCalculatedData->fdOutCrop.lastPixel,
        pISPInputdata->pCalculatedData->dispOutCrop.firstPixel, pISPInputdata->pCalculatedData->dispOutCrop.lastPixel);

#endif // DUALIFE_STRIPINGLIB_ZOOMS

    if (TRUE == pISPInputdata->pIFEOutputPathInfo[IFEOutputPortFull].path)
    {
        pInput->videofulloutFormat = TranslateOutputFormatToStripingLib(pISPInputdata->HALData.format[FullOutput]);
        pInput->videofulloutWidth  = static_cast<int16_t>(pISPInputdata->HALData.stream[FullOutput].width);
        pInput->videofulloutHeight = static_cast<int16_t>(pISPInputdata->HALData.stream[FullOutput].height);
    }

    if (TRUE == pISPInputdata->pIFEOutputPathInfo[IFEOutputPortDS4].path)
    {
        pInput->videofullDS4outFormat = TranslateOutputFormatToStripingLib(pISPInputdata->HALData.format[DS4Output]);
        pInput->videofullDSXoutWidth  = static_cast<int16_t>(pISPInputdata->HALData.stream[DS4Output].width);
        pInput->videofullDSXoutHeight = static_cast<int16_t>(pISPInputdata->HALData.stream[DS4Output].height);
    }

    if (TRUE == pISPInputdata->pIFEOutputPathInfo[IFEOutputPortDS16].path)
    {
        pInput->videofullDS16outFormat = TranslateOutputFormatToStripingLib(pISPInputdata->HALData.format[DS16Output]);
    }

    // Display Full/DS4/DS16 ports support
    if (TRUE == pISPInputdata->pIFEOutputPathInfo[IFEOutputPortDisplayFull].path)
    {
        pInput->dispFulloutFormat = TranslateOutputFormatToStripingLib(pISPInputdata->HALData.format[DisplayFullOutput]);
        pInput->dispFulloutWidth  = static_cast<int16_t>(pISPInputdata->HALData.stream[DisplayFullOutput].width);
        pInput->dispFulloutHeight = static_cast<int16_t>(pISPInputdata->HALData.stream[DisplayFullOutput].height);
    }

    if (TRUE == pISPInputdata->pIFEOutputPathInfo[IFEOutputPortDisplayDS4].path)
    {
        pInput->dispfullDS4outFormat = TranslateOutputFormatToStripingLib(pISPInputdata->HALData.format[DisplayDS4Output]);
    }

    if (TRUE == pISPInputdata->pIFEOutputPathInfo[IFEOutputPortDisplayDS16].path)
    {
        pInput->dispfullDS16outFormat = TranslateOutputFormatToStripingLib(pISPInputdata->HALData.format[DisplayDS16Output]);
    }

    pInput->fdOutFormat      = TranslateOutputFormatToStripingLib(pISPInputdata->HALData.format[FDOutput]);
    pInput->fdOutWidth       = static_cast<int16_t>(pISPInputdata->HALData.stream[FDOutput].width);
    pInput->fdOutHeight      = static_cast<int16_t>(pISPInputdata->HALData.stream[FDOutput].height);
    pInput->HDRInput            = pISPInputdata->pStripingInput->stripingInput.HDRInput;

    pInput->MNDS16InputLumaFD.roundingOptionHor = 0;
    pInput->MNDS16InputLumaFD.roundingOptionHor = 0;
    pInput->MNDS16InputChromaFD.roundingOptionHor = 0;
    pInput->MNDS16InputChromaFD.roundingOptionVer = 0;

    pInput->MNDS16InputLumaFD.output =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[FDOutput].dimension.width);
    pInput->MNDS16InputLumaFD.input  =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[FDOutput].input.width);
    pInput->MNDS16InputChromaFD.output = pInput->MNDS16InputLumaFD.output / 2;
    pInput->MNDS16InputChromaFD.input  = pInput->MNDS16InputLumaFD.input;

    pInput->MNDS16InputLumaFull.roundingOptionHor    = 0;
    pInput->MNDS16InputLumaFull.roundingOptionVer    = 0;
    pInput->MNDS16InputChromaFull.roundingOptionHor    = 0;
    pInput->MNDS16InputChromaFull.roundingOptionVer    = 0;

    pInput->MNDS16InputLumaFull.output =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[FullOutput].dimension.width);
    pInput->MNDS16InputLumaFull.input  =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[FullOutput].input.width);
    pInput->MNDS16InputChromaFull.output = pInput->MNDS16InputLumaFull.output / 2;
    pInput->MNDS16InputChromaFull.input  = pInput->MNDS16InputLumaFull.input;

    // Kona support:
    // Fill video full MNDS v2.1 parameters
    pInput->MNDS21InputLumaFull.roundingOptionHor = 0;
    pInput->MNDS21InputLumaFull.roundingOptionVer = 0;
    pInput->MNDS21InputChromaFull.roundingOptionHor = 0;
    pInput->MNDS21InputChromaFull.roundingOptionVer = 0;

    pInput->MNDS21InputLumaFull.output =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[FullOutput].dimension.width);
    pInput->MNDS21InputLumaFull.input =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[FullOutput].input.width);
    // For Chroma, the output is the half of the Luma (Y)
    pInput->MNDS21InputChromaFull.output = pInput->MNDS21InputLumaFull.output / 2;
    pInput->MNDS21InputChromaFull.input  = pInput->MNDS21InputLumaFull.input;

    // Kona Display path:
    // Fill display full MNDS v2.1 parameters
    pInput->MNDS21InputLumaDisp.roundingOptionHor = 0;
    pInput->MNDS21InputLumaDisp.roundingOptionVer = 0;
    pInput->MNDS21InputChromaDisp.roundingOptionHor = 0;
    pInput->MNDS21InputChromaDisp.roundingOptionVer = 0;

    pInput->MNDS21InputLumaDisp.output =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[DisplayFullOutput].dimension.width);
    pInput->MNDS21InputLumaDisp.input  =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[DisplayFullOutput].input.width);
    // For Chroma, the output is the half of the Luma (Y)
    pInput->MNDS21InputChromaDisp.output = pInput->MNDS21InputLumaDisp.output / 2;
    pInput->MNDS21InputChromaDisp.input  = pInput->MNDS21InputLumaDisp.input;

    pInput->MNDS21InputLumaFD.roundingOptionHor = 0;
    pInput->MNDS21InputLumaFD.roundingOptionVer = 0;
    pInput->MNDS21InputChromaFD.roundingOptionHor = 0;
    pInput->MNDS21InputChromaFD.roundingOptionVer = 0;

    pInput->MNDS21InputLumaFD.output =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[FDOutput].dimension.width);
    pInput->MNDS21InputLumaFD.input =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[FDOutput].input.width);
    // For Chroma, the output is the half of the Luma (Y)
    pInput->MNDS21InputChromaFD.output = pInput->MNDS21InputLumaFD.output / 2;
    pInput->MNDS21InputChromaFD.input  = pInput->MNDS21InputLumaFD.input;

    // Display Full/DS4/DS16 ports support
    pInput->MNDS16InputLumaDisp.roundingOptionHor    = 0;
    pInput->MNDS16InputLumaDisp.roundingOptionVer    = 0;
    pInput->MNDS16InputChromaDisp.roundingOptionHor    = 0;
    pInput->MNDS16InputChromaDisp.roundingOptionVer    = 0;

    pInput->MNDS16InputLumaDisp.output =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[DisplayFullOutput].dimension.width);
    pInput->MNDS16InputLumaDisp.input  =
        static_cast<uint16_t>(pISPInputdata->pCalculatedData->scalerOutput[DisplayFullOutput].input.width);
    pInput->MNDS16InputChromaDisp.output = pInput->MNDS16InputLumaDisp.output / 2;
    pInput->MNDS16InputChromaDisp.input  = pInput->MNDS16InputLumaDisp.input;

    pInput->PDPCInputV11             = pISPInputdata->pStripingInput->stripingInput.PDPCInputV11;
    pInput->pedestalEnable     = static_cast<int16_t>(pISPInputdata->pStripingInput->enableBits.pedestal);

    pInput->rolloffEnable      = static_cast<int16_t>(pISPInputdata->pStripingInput->enableBits.rolloff);
    pInput->BAFEnable          = static_cast<int16_t>(pISPInputdata->pStripingInput->enableBits.BAF);
    pInput->BGTintlessEnable  = static_cast<int16_t>(pISPInputdata->pStripingInput->enableBits.BGTintless);
    pInput->BGAWBEnable       = static_cast<int16_t>(pISPInputdata->pStripingInput->enableBits.BGAWB);
    pInput->BEEnable           = static_cast<int16_t>(pISPInputdata->pStripingInput->enableBits.BE);
    pInput->PDAFEnable         = static_cast<int16_t>(pISPInputdata->pStripingInput->enableBits.PDAF);
    pInput->HDREnable          = static_cast<int16_t>(pISPInputdata->pStripingInput->enableBits.HDR);
    pInput->BPCEnable          = static_cast<int16_t>(pISPInputdata->pStripingInput->enableBits.BPC);
    pInput->ABFEnable          = static_cast<int16_t>(pISPInputdata->pStripingInput->enableBits.ABF);
    pInput->tappingPointBE     = pISPInputdata->pStripingInput->stripingInput.tappingPointBE;
    pInput->tapoffPointHVX     = pISPInputdata->pStripingInput->stripingInput.tapoffPointHVX;
    pInput->kernelSizeHVX      = pISPInputdata->pStripingInput->stripingInput.kernelSizeHVX;
    pInput->pedestalParam       = pISPInputdata->pStripingInput->stripingInput.pedestalParam;
    pInput->rollOffParam        = pISPInputdata->pStripingInput->stripingInput.rollOffParam;
    pInput->HDRBhistInput      = pISPInputdata->pStripingInput->stripingInput.HDRBhistInput;
    pInput->iHistInput         = pISPInputdata->pStripingInput->stripingInput.iHistInput;
    pInput->bHistInput         = pISPInputdata->pStripingInput->stripingInput.bHistInput;
    pInput->BGTintlessInput   = pISPInputdata->pStripingInput->stripingInput.BGTintlessInput;
    pInput->BGAWBInput        = pISPInputdata->pStripingInput->stripingInput.BGAWBInput;
    pInput->BEInput            = pISPInputdata->pStripingInput->stripingInput.BEInput;
    pInput->RSCSInput          = pISPInputdata->pStripingInput->stripingInput.RSCSInput;

    if (CSLCameraTitanVersion::CSLTitan480 == pISPInputdata->titanVersion)
    {
        Utils::Memcpy(&pInput->BAFInputv25, &pISPInputdata->pStripingInput->stripingInput.BAFInputv25,
            sizeof(IFEStripeBayerFocusv25InputParam));
    }
    else if (CSLCameraTitanVersion::CSLTitan150 == pISPInputdata->titanVersion)
    {
        Utils::Memcpy(&pInput->BAFInputv24, &pISPInputdata->pStripingInput->stripingInput.BAFInputv24,
            sizeof(IFEStripeBayerFocusv24InputParam));
    }
    else
    {
        Utils::Memcpy(&pInput->BAFInput, &pISPInputdata->pStripingInput->stripingInput.BAFInput,
            sizeof(IFEStripeBayerFocusv23InputParam));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DualIFEUtils::TranslateOutputFormatToStripingLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int16_t DualIFEUtils::TranslateOutputFormatToStripingLib(
    Format srcFormat)
{
    int16_t format = IMAGE_FORMAT_INVALID;
    switch (srcFormat)
    {
        case Format::UBWCTP10:
            format = IMAGE_FORMAT_UBWC_TP_10;
            break;
        case Format::PD10:
            format = IMAGE_FORMAT_PD_10;
            break;
        case Format::YUV420NV12:
        case Format::YUV420NV21:
         /* configure Y8 as NV12 and if needed ignore chroma */
        case Format::Y8:
            format = IMAGE_FORMAT_LINEAR_NV12;
            break;
        case Format::UBWCNV12:
        case Format::UBWCNV12FLEX:
            format = IMAGE_FORMAT_UBWC_NV_12;
            break;
        case Format::UBWCNV124R:
            format = IMAGE_FORMAT_UBWC_NV12_4R;
            break;
        case Format::P010:
            format = IMAGE_FORMAT_LINEAR_P010;
            break;
        case Format::YUV420NV12TP10:
        case Format::YUV420NV21TP10:
            format = IMAGE_FORMAT_LINEAR_TP_10;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Missing format translation to striping lib format: %d!", srcFormat);
            break;
    }
    return format;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DualIFEUtils::TranslateInputFormatToStripingLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int16_t DualIFEUtils::TranslateInputFormatToStripingLib(
    PixelFormat srcFormat,
    UINT32 CSIDBitWidth)
{
    int16_t format = IMAGE_FORMAT_INVALID;
    switch (srcFormat)
    {
        case PixelFormat::BayerBGGR:
        case PixelFormat::BayerGBRG:
        case PixelFormat::BayerGRBG:
        case PixelFormat::BayerRGGB:
            switch (CSIDBitWidth)
            {
                case 14:
                    format = IMAGE_FORMAT_MIPI_14;
                    break;
                case 12:
                    format = IMAGE_FORMAT_MIPI_12;
                    break;
                case 10:
                    format = IMAGE_FORMAT_MIPI_10;
                    break;
                case 8:
                    format = IMAGE_FORMAT_MIPI_8;
                    break;
                default:
                    CAMX_ASSERT_ALWAYS_MESSAGE("Invalid CSID format bitwidth; defaulting to 10 bit: %d!", CSIDBitWidth);
                    format = IMAGE_FORMAT_MIPI_10;
                    break;
            }
            break;
        case PixelFormat::YUVFormatUYVY:
        case PixelFormat::YUVFormatYUYV:
            switch (CSIDBitWidth)
            {
                case 10:
                    format = IMAGE_FORMAT_YUV422_10;
                    break;
                case 8:
                    format = IMAGE_FORMAT_YUV422_8;
                    break;
                default:
                    CAMX_ASSERT_ALWAYS_MESSAGE("Invalid CSID format bitwidth; defaulting to 10 bit: %d!", CSIDBitWidth);
                    format = IMAGE_FORMAT_YUV422_10;
                    break;
            }
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Missing format translation to striping lib format: %d!", srcFormat);
            break;
    }
    return format;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::HardcodeSettingsSetDefaultBFFilterInputConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::HardcodeSettingsSetDefaultBFFilterInputConfig(
    AFStatsControl* pAFConfigOutput)
{
    AFConfigParams* pStatsConfig = NULL;

    pStatsConfig = &pAFConfigOutput->statsConfig;

    // 1. H1 Config
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].horizontalScaleEnable = FALSE;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].isValid               = TRUE;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].shiftBits             = -3;

    // 1a. H1 FIR
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.enable               = TRUE;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.numOfFIRCoefficients = 13;

    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[0]  = -1;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[1]  = -2;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[2]  = -1;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[3]  = 1;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[4]  = 5;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[5]  = 8;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[6]  = 10;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[7]  = 8;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[8]  = 5;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[9]  = 1;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[10] = -1;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[11] = -2;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFIRFilterConfig.FIRFilterCoefficients[12] = -1;

    // 1b. H1 IIR
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.enable = TRUE;

    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.b10 = 0.092346f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.b11 = 0.000000f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.b12 = -0.092346f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.a11 = 1.712158f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.a12 = -0.815308f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.b20 = 0.112976f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.b21 = 0.000000f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.b22 = -0.112976f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.a21 = 1.869690f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFIIRFilterConfig.a22 = -0.898743f;

    // 1c. H1 Coring
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.threshold = (1 << 16);
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.gain      = 16;

    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[0]  = 0;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[1]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[2]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[3]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[4]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[5]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[6]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[7]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[8]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[9]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[10] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[11] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[12] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[13] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[14] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[15] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.core[16] = 16;

    // 2. H2 Config
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].horizontalScaleEnable = TRUE;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].isValid               = TRUE;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].shiftBits             = 3;

    // 2a. H2 FIR
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFIRFilterConfig.enable = FALSE;

    // 2b. H2 IIR
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.enable = TRUE;

    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.b10 = 0.078064f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.b11 = 0.000000f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.b12 = -0.078064f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.a11 = 1.735413f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.a12 = -0.843811f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.b20 = 0.257202f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.b21 = 0.000000f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.b22 = -0.257202f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.a21 = 1.477051f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFIIRFilterConfig.a22 = -0.760071f;

    // 2c. H2 Coring
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.threshold = (1 << 16);
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.gain      = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[0]   = 0;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[1]   = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[2]   = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[3]   = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[4]   = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[5]   = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[6]   = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[7]   = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[8]   = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[9]   = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[10]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[11]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[12]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[13]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[14]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[15]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal2].BFFilterCoringConfig.core[16]  = 16;

    // 3. Vertical Config
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].isValid   = TRUE;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].shiftBits = 0;

    // 3a. Vertical FIR
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFIRFilterConfig.enable               = TRUE;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFIRFilterConfig.numOfFIRCoefficients = 5;

    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFIRFilterConfig.FIRFilterCoefficients[0] = 1;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFIRFilterConfig.FIRFilterCoefficients[1] = 1;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFIRFilterConfig.FIRFilterCoefficients[2] = 1;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFIRFilterConfig.FIRFilterCoefficients[3] = 1;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFIRFilterConfig.FIRFilterCoefficients[4] = 1;

    // 3b. H1 IIR
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.enable = TRUE;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.b10    = 0.894897f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.b11    = -1.789673f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.b12    = 0.894897f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.a11    = 1.778625f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.a12    = -0.800781f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.b20    = 0.112976f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.b21    = 0.000000f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.b22    = -0.112976f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.a21    = 1.869690f;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFIIRFilterConfig.a22    = -0.898743f;

    // 3c. H1 Coring
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.threshold = (1 << 16);
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.gain      = 16;

    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[0]  = 0;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[1]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[2]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[3]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[4]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[5]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[6]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[7]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[8]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[9]  = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[10] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[11] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[12] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[13] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[14] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[15] = 16;
    pStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.core[16] = 16;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::HardcodeSettingsSetDefaultBFROIInputConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::HardcodeSettingsSetDefaultBFROIInputConfig(
    AFStatsControl* pAFConfigOutput,
    UINT32          CAMIFWidth,
    UINT32          CAMIFHeight)
{
    UINT32                     horizontalOffset = 0;
    UINT32                     verticalOffset   = 0;
    UINT32                     ROIWidth         = 0;
    UINT32                     ROIHeight        = 0;
    UINT32                     horizontalNum    = 0;
    UINT32                     verticalNum      = 0;
    UINT32                     indexX           = 0;
    UINT32                     indexY           = 0;
    BFStatsROIDimensionParams* pROI             = NULL;
    BFStatsROIConfigType*      pROIConfig       = NULL;

    pROIConfig = &pAFConfigOutput->statsConfig.BFStats.BFStatsROIConfig;

    Utils::Memset(pROIConfig, 0, sizeof(BFStatsROIConfigType));
    pROIConfig->numBFStatsROIDimension = 0;

    // Configure ROI in 5 x 5 grids
    horizontalNum = 5;
    verticalNum   = 5;

    ROIWidth  = Utils::FloorUINT32(2, (((25 * CAMIFWidth) / horizontalNum) / 100));
    ROIHeight = Utils::FloorUINT32(2, (((25 * CAMIFHeight) / verticalNum) / 100));

    horizontalOffset = (CAMIFWidth - (horizontalNum * ROIWidth)) >> 1;
    verticalOffset   = (CAMIFHeight - (verticalNum * ROIHeight)) >> 1;

    for (indexY = 0; indexY < verticalNum; indexY++)
    {
        for (indexX = 0; indexX < horizontalNum; indexX++)
        {
            pROI = &pROIConfig->BFStatsROIDimension[pROIConfig->numBFStatsROIDimension];

            pROI->region     = BFStatsPrimaryRegion;
            pROI->ROI.left   = Utils::FloorUINT32(2, (horizontalOffset + (indexX * ROIWidth)));
            pROI->ROI.top    = Utils::FloorUINT32(2, (verticalOffset + (indexY * ROIHeight)));
            pROI->ROI.width  = ROIWidth - 1;
            pROI->ROI.height = ROIHeight - 1;
            pROI->regionNum  = pROIConfig->numBFStatsROIDimension++;
            pROI->isValid    = TRUE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::HardcodeSettingsBFStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IFENode::HardcodeSettingsBFStats(
    ISPStripeConfig* pStripeConfig)
{
    const UINT32    CAMIFWidth      = pStripeConfig->CAMIFCrop.lastPixel - pStripeConfig->CAMIFCrop.firstPixel + 1;
    const UINT32    CAMIFHeight     = pStripeConfig->CAMIFCrop.lastLine - pStripeConfig->CAMIFCrop.firstLine + 1;
    AFStatsControl* pAFUpdateData   = &pStripeConfig->AFStatsUpdateData;

    pAFUpdateData->statsConfig.mask = BFStats;

    HardcodeSettingsSetDefaultBFFilterInputConfig(pAFUpdateData);
    HardcodeSettingsSetDefaultBFROIInputConfig(pAFUpdateData, CAMIFWidth, CAMIFHeight);

    pAFUpdateData->statsConfig.BFStats.BFStatsROIConfig.BFROIType     = BFStatsCustomROI;
    pAFUpdateData->statsConfig.BFStats.BFScaleConfig.BFScaleEnable    = FALSE;
    pAFUpdateData->statsConfig.BFStats.BFScaleConfig.isValid          = FALSE;
    pAFUpdateData->statsConfig.BFStats.BFScaleConfig.scaleM           = 1;
    pAFUpdateData->statsConfig.BFStats.BFScaleConfig.scaleN           = 2;
    pAFUpdateData->statsConfig.BFStats.BFInputConfig.BFChannelSel     = BFChannelSelectG;
    pAFUpdateData->statsConfig.BFStats.BFInputConfig.BFInputGSel      = BFInputSelectGr;
    pAFUpdateData->statsConfig.BFStats.BFInputConfig.isValid          = TRUE;
    pAFUpdateData->statsConfig.BFStats.BFInputConfig.YAConfig[0]      = 0;
    pAFUpdateData->statsConfig.BFStats.BFInputConfig.YAConfig[1]      = 0;
    pAFUpdateData->statsConfig.BFStats.BFInputConfig.YAConfig[2]      = 0;
    pAFUpdateData->statsConfig.BFStats.BFGammaLUTConfig.isValid       = FALSE;
    pAFUpdateData->statsConfig.BFStats.BFGammaLUTConfig.numGammaLUT   = DefaultBFGammaEntries;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupHFRInitialConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupHFRInitialConfig()
{
    CamxResult              result                  = CamxResultSuccess;
    UINT                    totalOutputPorts        = 0;
    SIZE_T                  resourceHFRConfigSize   = 0;
    IFEResourceHFRConfig*   pResourceHFRConfig      = NULL;
    UINT                    outputPortId[MaxDefinedIFEOutputPorts];
    UINT32                  channelId;

    // Get Output Port List
    GetAllOutputPortIds(&totalOutputPorts, &outputPortId[0]);
    if ((totalOutputPorts == 0) || (totalOutputPorts > MaxDefinedIFEOutputPorts))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Error: invalid number of output ports %d", totalOutputPorts);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        resourceHFRConfigSize   = sizeof(IFEResourceHFRConfig) +
                                  (sizeof(IFEPortHFRConfig) * (totalOutputPorts - 1 - m_disabledOutputPorts));
        pResourceHFRConfig      = static_cast<IFEResourceHFRConfig*>(CAMX_CALLOC(resourceHFRConfigSize));

        if (NULL == pResourceHFRConfig)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to allocate ResourceHFRInfo.");
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        pResourceHFRConfig->numPorts = totalOutputPorts - m_disabledOutputPorts;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "totalOutputPorts=%d, m_usecaseNumBatchedFrames=%d, m_disabledOutputPorts = %d",
                         totalOutputPorts, m_usecaseNumBatchedFrames, m_disabledOutputPorts);

        UINT portHFRConfigIndex = 0;
        for (UINT portIndex = 0; portIndex < totalOutputPorts; portIndex++)
        {
            UINT currentInternalOutputPortIndex = OutputPortIndex(outputPortId[portIndex]);
            if (TRUE == m_isDisabledOutputPort[currentInternalOutputPortIndex])
            {
                CAMX_LOG_INFO(CamxLogGroupISP,
                              "Skipping for output portId = %u because the sensor does not support port source type = %u",
                              outputPortId[portIndex],
                              GetOutputPortSourceType(currentInternalOutputPortIndex));
                continue;
            }

            result = MapPortIdToChannelId(outputPortId[portIndex], &channelId);

            if (CamxResultSuccess == result)
            {
                IFEPortHFRConfig* pPortHFRConfig = &pResourceHFRConfig->portHFRConfig[portHFRConfigIndex];

                pPortHFRConfig->portResourceId = channelId;

                if (m_usecaseNumBatchedFrames > 1)
                {
                    if (TRUE == IsBatchEnabledPort(outputPortId[portIndex]))
                    {
                        pPortHFRConfig->framedropPattern  = 1;
                        pPortHFRConfig->framedropPeriod   = 0;
                        pPortHFRConfig->subsamplePattern  = 1 << (m_usecaseNumBatchedFrames - 1);
                        pPortHFRConfig->subsamplePeriod   = (m_usecaseNumBatchedFrames - 1);
                    }
                    else
                    {
                        pPortHFRConfig->framedropPattern  = 1;
                        pPortHFRConfig->framedropPeriod   = (m_usecaseNumBatchedFrames - 1);
                        pPortHFRConfig->subsamplePattern  = 1;
                        pPortHFRConfig->subsamplePeriod   = 0;
                    }
                }
                else
                {
                    pPortHFRConfig->framedropPattern  = 1;
                    pPortHFRConfig->framedropPeriod   = 0;
                    pPortHFRConfig->subsamplePattern  = 1;
                    pPortHFRConfig->subsamplePeriod   = 0;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Error: invalid Port Id %d", outputPortId[portIndex]);
                break;
            }
            portHFRConfigIndex++;
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteGenericBlobData(m_pGenericBlobCmdBuffer,
                                                         IFEGenericBlobTypeHFRConfig,
                                                         static_cast<UINT32>(resourceHFRConfigSize),
                                                         reinterpret_cast<BYTE*>(pResourceHFRConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Error: Writing Blob data size=%d, result=%d",
                               static_cast<UINT32>(resourceHFRConfigSize), result);
            }
        }
    }

    if (NULL != pResourceHFRConfig)
    {
        CAMX_FREE(pResourceHFRConfig);
        pResourceHFRConfig = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupUBWCInitialConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupUBWCInitialConfig()
{
    CamxResult                result                 = CamxResultSuccess;
    UINT                      totalOutputPorts       = 0;
    SIZE_T                    resourceUBWCConfigSize = 0;
    CSLResourceUBWCConfigV2*  pResourceUBWCConfig    = NULL;
    UINT                      outputPortId[MaxDefinedIFEOutputPorts];
    UINT32                    channelId;
    UINT                      totalUBWCOutputPorts   = 0;
    UINT                      outputUBWCPortId[MaxDefinedIFEOutputPorts];
    UINT32                    packerCfg;

    // Get Output Port List
    GetAllOutputPortIds(&totalOutputPorts, &outputPortId[0]);
    if ((totalOutputPorts == 0) || (totalOutputPorts > MaxDefinedIFEOutputPorts))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Error: invalid number of output ports %d", totalOutputPorts);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        for (UINT portIndex = 0; portIndex < totalOutputPorts; portIndex++)
        {
            UINT currentOutputPortIndex = OutputPortIndex(outputPortId[portIndex]);

            if (TRUE == m_isDisabledOutputPort[portIndex])
            {
                CAMX_LOG_VERBOSE(CamxLogGroupISP,
                                 "Skipping for output portId = %u because the sensor does not support port source type = %u",
                                 GetOutputPortId(portIndex),
                                 GetOutputPortSourceType(portIndex));
                continue;
            }
            const ImageFormat* pCurrentOutputPortImageFormat = GetOutputPortImageFormat(currentOutputPortIndex);
            if ((NULL != pCurrentOutputPortImageFormat) &&
                (TRUE == ImageFormatUtils::IsUBWC(pCurrentOutputPortImageFormat->format)))
            {
                outputUBWCPortId[totalUBWCOutputPorts] = outputPortId[portIndex];
                totalUBWCOutputPorts++;
            }
        }

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "number of UBWC Supported output ports %d", totalUBWCOutputPorts);

        if ((CamxResultSuccess == result) && (totalUBWCOutputPorts > 0))
        {
            resourceUBWCConfigSize = sizeof(CSLResourceUBWCConfigV2) +
                (sizeof(CSLPortUBWCConfigV2) * (CSLMaxNumPlanes - 1) * (totalUBWCOutputPorts - 1));
            pResourceUBWCConfig = static_cast<CSLResourceUBWCConfigV2*>(CAMX_CALLOC(resourceUBWCConfigSize));

            if (NULL == pResourceUBWCConfig)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to allocate ResourceUBWCInfo.");
                result = CamxResultENoMemory;
            }
        }

        if ((CamxResultSuccess == result) && (totalUBWCOutputPorts > 0))
        {
            pResourceUBWCConfig->numPorts       = totalUBWCOutputPorts;
            pResourceUBWCConfig->UBWCAPIVersion = IFESupportedUBWCVersions4; ///< which supports both UBWC 2.0 ,3.0, 4.0
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "totalOutputPorts=%d, m_usecaseNumBatchedFrames=%d, m_disabledOutputPorts = %d",
                             totalOutputPorts, m_usecaseNumBatchedFrames, m_disabledOutputPorts);

            UINT portUBWCConfigIndex = 0;
            for (UINT portIndex = 0; portIndex < totalUBWCOutputPorts; portIndex++)
            {
                if (TRUE == m_isDisabledOutputPort[portIndex])
                {
                    CAMX_LOG_INFO(CamxLogGroupISP,
                                  "Skipping for output portId = %u because the sensor does not support port source type = %u",
                                  GetOutputPortId(portIndex),
                                  GetOutputPortSourceType(portIndex));
                    continue;
                }

                UINT currentOutputPortIndex = OutputPortIndex(outputUBWCPortId[portIndex]);
                // Set image format from the output port index
                const ImageFormat* pCurrentOutputPortImageFormat = GetOutputPortImageFormat(currentOutputPortIndex);

                if ((NULL != pCurrentOutputPortImageFormat) &&
                    (TRUE == ImageFormatUtils::IsUBWC(pCurrentOutputPortImageFormat->format)))
                {
                    result = MapPortIdToChannelId(outputUBWCPortId[portIndex], &channelId);

                    if (CamxResultSuccess == result)
                    {
                        UINT numPlanes = ImageFormatUtils::GetNumberOfPlanes(pCurrentOutputPortImageFormat);
                        for (UINT planeIndex = 0; planeIndex < numPlanes; planeIndex++)
                        {
                            UBWCPartialTileInfo   UBWCPartialTileParam;
                            CSLPortUBWCConfigV2*    pPortUBWCConfig =
                                &pResourceUBWCConfig->portUBWCConfig[portUBWCConfigIndex][planeIndex];

                            pPortUBWCConfig->portResourceId = channelId;
                            pPortUBWCConfig->metadataStride =
                                pCurrentOutputPortImageFormat->formatParams.yuvFormat[planeIndex].metadataStride;
                            pPortUBWCConfig->metadataSize   =
                                pCurrentOutputPortImageFormat->formatParams.yuvFormat[planeIndex].metadataSize;


                            const struct UBWCTileInfo* pTileInfo =
                                ImageFormatUtils::GetUBWCTileInfo(pCurrentOutputPortImageFormat);

                            if (NULL != pTileInfo)
                            {
                                // partial tile information calculated assuming this full frame processing
                                // dual ife this will be overwritten by ife node.
                                // hInit & vInit are 0, as we are programming entire frame
                                ImageFormatUtils::GetUBWCPartialTileInfo(pTileInfo,
                                                                         &UBWCPartialTileParam,
                                                                         0,
                                                                         pCurrentOutputPortImageFormat->width);

                                pPortUBWCConfig->tileConfig         |=
                                    (UBWCPartialTileParam.partialTileBytesLeft & 0x3F) << 16;
                                pPortUBWCConfig->tileConfig         |=
                                    (UBWCPartialTileParam.partialTileBytesRight & 0x3F) << 23;

                                pPortUBWCConfig->modeConfig          =
                                    m_pIFEPipeline->GetUBWCModeConfig(pCurrentOutputPortImageFormat, planeIndex);
                                pPortUBWCConfig->modeConfig1         =
                                    ImageFormatUtils::GetUBWCModeConfig1(pCurrentOutputPortImageFormat);
                                pPortUBWCConfig->hInitialVal         = UBWCPartialTileParam.horizontalInit;
                                pPortUBWCConfig->vInitialVal         = UBWCPartialTileParam.verticalInit;
                                pPortUBWCConfig->metadataOffset      = 0;
                                pPortUBWCConfig->staticControl       = 0;
                                pPortUBWCConfig->control2            =
                                    ImageFormatUtils::GetUBWCModeConfig1(pCurrentOutputPortImageFormat);
                                pPortUBWCConfig->statsControl2       = 0x0;
                                pPortUBWCConfig->lossyThreshold0     =
                                    (static_cast<Titan17xContext *>(GetHwContext()))->GetUBWCLossyThreshold0(
                                    pCurrentOutputPortImageFormat->ubwcVerInfo.version,
                                    LossyPathIFE,
                                    pCurrentOutputPortImageFormat);
                                pPortUBWCConfig->lossyThreshold1     =
                                    (static_cast<Titan17xContext *>(GetHwContext()))->GetUBWCLossyThreshold1(
                                    pCurrentOutputPortImageFormat->ubwcVerInfo.version,
                                    LossyPathIFE,
                                    pCurrentOutputPortImageFormat);
                                pPortUBWCConfig->offsetVarianceLossy = UBWCv4OffsetsVarLossy;
                                pPortUBWCConfig->bandwidthLimit      =
                                    (static_cast<Titan17xContext *>(GetHwContext()))->GetUBWCBandwidthLimit(
                                        pCurrentOutputPortImageFormat->ubwcVerInfo.version,
                                        UBWCLossyPath::LossyPathIFE,
                                        planeIndex,
                                        pCurrentOutputPortImageFormat);
                                pPortUBWCConfig->bandwidthLimit      = (pPortUBWCConfig->bandwidthLimit << 1);
                                pPortUBWCConfig->bandwidthLimit     |= 1;
                                result = m_pIFEPipeline->GetUBWCPackerConfig(pCurrentOutputPortImageFormat, &packerCfg);
                                pPortUBWCConfig->packerConfig = packerCfg;
                                if (CamxResultSuccess != result)
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupISP, "packer config invalid %d for port %d  format %d",
                                        pPortUBWCConfig->packerConfig,
                                        outputUBWCPortId[portIndex],
                                        pCurrentOutputPortImageFormat->format);
                                    result = CamxResultEUnsupported;
                                    break;
                                }

                                CAMX_LOG_INFO(CamxLogGroupISP, "modeConfig %d,mode1 %d,tile %d,hinit %d,vinit %d,metaoff %d"
                                              "staticctrl %d,ctrl %d,statsctrl %d,lossyThresh0 %d,lossythresh1 %d,offvar %d"
                                              "version %d, lossy %d",
                                              pPortUBWCConfig->modeConfig,
                                              pPortUBWCConfig->modeConfig1,
                                              pPortUBWCConfig->tileConfig,
                                              pPortUBWCConfig->hInitialVal,
                                              pPortUBWCConfig->vInitialVal,
                                              pPortUBWCConfig->metadataOffset,
                                              pPortUBWCConfig->staticControl,
                                              pPortUBWCConfig->control2,
                                              pPortUBWCConfig->statsControl2,
                                              pPortUBWCConfig->lossyThreshold0,
                                              pPortUBWCConfig->lossyThreshold1,
                                              pPortUBWCConfig->offsetVarianceLossy,
                                              pCurrentOutputPortImageFormat->ubwcVerInfo.version,
                                              pCurrentOutputPortImageFormat->ubwcVerInfo.lossy);
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupUtils, "pTileInfo is NULL");
                                result = CamxResultEFailed;
                                break;
                            }
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP, "Error: invalid Port Id %d", outputUBWCPortId[portIndex]);
                        break;
                    }
                    portUBWCConfigIndex++;
                }
            }

            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteGenericBlobData(m_pGenericBlobCmdBuffer,
                                                             IFEGenericBlobTypeUBWCConfigV2,
                                                             static_cast<UINT32>(resourceUBWCConfigSize),
                                                             reinterpret_cast<BYTE*>(pResourceUBWCConfig));
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Error: Writing Blob data size=%d, result=%d",
                                   static_cast<UINT32>(resourceUBWCConfigSize), result);
                }
            }
        }
    }

    if (NULL != pResourceUBWCConfig)
    {
        CAMX_FREE(pResourceUBWCConfig);
        pResourceUBWCConfig = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupBusReadInitialConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupBusReadInitialConfig()
{
    CamxResult            result            = CamxResultSuccess;
    SIZE_T                busReadConfigSize = 0;
    IFEBusReadConfig*     pBusReadConfig    = NULL;

    busReadConfigSize = sizeof(IFEBusReadConfig);
    pBusReadConfig    = static_cast<IFEBusReadConfig*>(CAMX_CALLOC(busReadConfigSize));

    if (NULL == pBusReadConfig)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to allocate BusReadConfig.");
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        pBusReadConfig->HBICount = m_pSensorModeData->minHorizontalBlanking;
        if (BusReadMinimumHBI > pBusReadConfig->HBICount)
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "Invalid BusRead HBI. Clamp to minimum required");
            pBusReadConfig->HBICount = BusReadMinimumHBI;
        }

        // A value of VBI will be counted to VBI*8 number of lines in h/w after fetching each frame
        pBusReadConfig->VBICount = (m_pSensorModeData->minVerticalBlanking >> 3);
        if (BusReadMinimumVBI > pBusReadConfig->VBICount)
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "Invalid BusRead VBI. Clamp to minimum required");
            pBusReadConfig->VBICount = BusReadMinimumVBI;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "BusRead minHBI=%u minVBI=%u",
            pBusReadConfig->HBICount, pBusReadConfig->VBICount);

        pBusReadConfig->lineSyncEnable = 0;
        pBusReadConfig->FSMode         = 1;
        pBusReadConfig->syncEnable     = 0;
        pBusReadConfig->goCmdSelect    = 1;
        pBusReadConfig->clientEnable   = 1;
        pBusReadConfig->unpackMode     = TranslateFormatToISPImageFormat(CamX::Format::RawMIPI, m_CSIDecodeBitWidth);;
        pBusReadConfig->latencyBufSize = 0x1000;

        result = PacketBuilder::WriteGenericBlobData(m_pGenericBlobCmdBuffer,
                                                     IFEGenericBlobTypeBusReadConfig,
                                                     static_cast<UINT32>(busReadConfigSize),
                                                     reinterpret_cast<BYTE*>(pBusReadConfig));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Error: Writing BusRead Blob data size=%d, result=%d",
                static_cast<UINT32>(busReadConfigSize), result);
        }
    }

    if (NULL != pBusReadConfig)
    {
        CAMX_FREE(pBusReadConfig);
        pBusReadConfig = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetIFEInputWidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFENode::GetIFEInputWidth(
    IFESplitID IFEIndex)
{
    CAMX_ASSERT(IFESplitID::Right >= IFEIndex);

    UINT32 width = 0;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        if (IFESplitID::Left == IFEIndex)
        {
            width = m_dualIFESplitParams.splitPoint + m_dualIFESplitParams.rightPadding;
        }
        else
        {
            width = m_pSensorModeData->resolution.outputWidth - m_dualIFESplitParams.splitPoint +
                    m_dualIFESplitParams.leftPadding;
        }
    }
    else
    {
        width = m_pSensorModeData->resolution.outputWidth;
    }

    return width;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculatePixelClockRate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 IFENode::CalculatePixelClockRate(
    UINT32  inputWidth)
{
    UINT64  pixClockHz = 0;
    UINT    pixPerClk  = HwEnvironment::GetInstance()->GetPlatformStaticCaps()->IFEPixelsPerClock;

    CAMX_ASSERT((0 != m_pSensorModeData->resolution.outputWidth) &&
                (0 != m_pSensorModeData->maxFPS) &&
                (0 != m_pSensorModeData->numLinesPerFrame));

    CheckForRDIOnly();

    if (TRUE == m_csidBinningInfo.isBinningEnabled)
    {
        inputWidth >>= 1;
    }

    if (FALSE == m_RDIOnlyUseCase)
    {
        UINT32 minIFEhbi;

        if (TRUE == m_HVXInputData.enableHVX)
        {
            // With HVX enabled, might require different min HBI.
            minIFEhbi = HwEnvironment::GetInstance()->GetPlatformStaticCaps()->IFEDefaultMinHBIWithHVX;
        }
        else
        {
            minIFEhbi = IFEDefaultMinHBI;
        }

        pixClockHz = m_pSensorModeData->outPixelClock;

        if (0 != m_pSensorModeData->minHorizontalBlanking)
        {
            pixClockHz = (Utils::ByteAlign32((inputWidth / pixPerClk + minIFEhbi), 32) * m_pSensorModeData->outPixelClock) /
                         (m_pSensorModeData->resolution.outputWidth + m_pSensorModeData->minHorizontalBlanking);
        }
        else
        {
            // When minHorizontalBlanking is not populated, fallback to provided frameLineLength
            UINT64 theoreticalLineLength = static_cast<UINT64>(m_pSensorModeData->outPixelClock /
                (m_pSensorModeData->maxFPS * m_pSensorModeData->numLinesPerFrame));

            CAMX_LOG_INFO(CamxLogGroupPower,
                             "theoreticalLineLength=%llu maxFPS=%lf numPixelsPerLine=%u numLinesPerFrame=%u",
                             theoreticalLineLength, m_pSensorModeData->maxFPS,
                             m_pSensorModeData->numPixelsPerLine, m_pSensorModeData->numLinesPerFrame);

            pixClockHz = (Utils::ByteAlign32((inputWidth / pixPerClk + minIFEhbi), 32) * m_pSensorModeData->outPixelClock) /
                         (Utils::MinUINT64(theoreticalLineLength, Utils::MaxUINT32(m_pSensorModeData->resolution.outputWidth,
                                                                                   m_pSensorModeData->numPixelsPerLine)));
        }

        const UINT32 statsFlushingCycles  = HwEnvironment::GetInstance()->GetPlatformStaticCaps()->IFEStatsFlushingCycles;
        const UINT32 imageFlushingVBLines = HwEnvironment::GetInstance()->GetPlatformStaticCaps()->IFEImageFlushingVBI;

        UINT32 minIFEvbi   = HwEnvironment::GetInstance()->GetPlatformStaticCaps()->IFEDefaultMinVBI;
        FLOAT lineDuration = CalculateSensorLineDuration(pixClockHz);

        if ((0 < pixClockHz) && (0 < lineDuration))
        {
            // Calculation of minimum vertical blanking lines needs to consider the IFE clock as duration to flushing
            // the stats can be changed.
            minIFEvbi   =  Utils::MaxUINT32(Utils::Ceiling(statsFlushingCycles * (1000000.0f / pixClockHz) / lineDuration) +
                                                imageFlushingVBLines,
                                                minIFEvbi);
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupPower, "Invalid line duration or pixClockHz!");
        }

        CAMX_LOG_INFO(CamxLogGroupPower,
                         "inputWidth=%u minVerticalBlanking=%u minHorizontalBlanking=%u "
                         "pixClockHz=%llu minHBI=%u minVBI=%u Sensor Op %llu sensorOutputWidth=%u",
                         inputWidth, m_pSensorModeData->minVerticalBlanking, m_pSensorModeData->minHorizontalBlanking,
                         pixClockHz, minIFEhbi, minIFEvbi, m_pSensorModeData->outPixelClock,
                         m_pSensorModeData->resolution.outputWidth);

        if ((0 != m_pSensorModeData->minVerticalBlanking) &&
            (minIFEvbi > m_pSensorModeData->minVerticalBlanking))
        {
            pixClockHz = (pixClockHz * minIFEvbi) / m_pSensorModeData->minVerticalBlanking;
            CAMX_LOG_INFO(CamxLogGroupPower, "pixClockHz=%llu", pixClockHz);
        }
    }

    if (TRUE == m_enableBusRead)
    {
        // In FS mode increase IFE Clock frequency e.g. in frame based mode with sensor running at 120FPS
        // i.e. writing @appr. 8.2ms/frame, IFE only has a reduced time of ~24ms to process the frame.
        // Hence the frequency needs to be increased by nearly 33%.
        // Computations based on system team recommendations
        FLOAT IOMargin   = 1.1f;
        FLOAT offlineFPS = (m_pSensorModeData->maxFPS * m_FPS) / (m_pSensorModeData->maxFPS - m_FPS);
        pixClockHz      *= ((offlineFPS / m_FPS) * IOMargin);

        CAMX_LOG_INFO(CamxLogGroupISP, "FS: MaxFPS: %f VidFPS: %d OfflineFPS: %f Increase pixClockHz to %llu",
            m_pSensorModeData->maxFPS, m_FPS, offlineFPS, pixClockHz);
    }

    return pixClockHz;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetPixelClockRate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 IFENode::GetPixelClockRate(
    IFESplitID IFEIndex)
{
    CAMX_ASSERT(IFESplitID::Right >= IFEIndex);

    UINT64 pixClockHz = 0;
    INT32  height     = 0;

    INT overridePixClockMHz = static_cast<INT>(GetStaticSettings()->ifeClockFrequencyMHz);
    if (0 > overridePixClockMHz)
    {
        // If ifeClockFrequencyMHz is set to 0xFFFFFFFF, this means to set IFE clock to its max turbo clock frequency.
        pixClockHz = HwEnvironment::GetInstance()->GetPlatformStaticCaps()->maxIFETURBOClock;
        m_IFEThroughPut[static_cast<UINT>(IFEIndex)] = 1.0f;
        CAMX_LOG_INFO(CamxLogGroupPower, "Clock setting disabled by override %d. Setting pixClockHz = %llu",
                      overridePixClockMHz, pixClockHz);
    }
    else
    {
        if (0 == m_ifeClockRate[static_cast<UINT>(IFEIndex)])
        {
            if (0 != overridePixClockMHz)
            {
                pixClockHz = overridePixClockMHz * 1000000; // Convert to Hz
                CAMX_LOG_INFO(CamxLogGroupPower, "Using override clock rate %llu Hz", pixClockHz);
            }
            else
            {
                UINT32 inputWidth = GetIFEInputWidth(IFEIndex);

                // Force a minimum clock rate to meet HW constraints on 845
                pixClockHz = Utils::MaxUINT64(HwEnvironment::GetInstance()->GetPlatformStaticCaps()->minIFEHWClkRate,
                                              CalculatePixelClockRate(inputWidth));

                CAMX_LOG_INFO(CamxLogGroupPower, "Using clock rate %llu Hz for inputWidth: %d", pixClockHz, inputWidth);
            }

            m_ifeClockRate[static_cast<UINT>(IFEIndex)] = pixClockHz;

            height = m_pSensorModeData->cropInfo.lastLine - m_pSensorModeData->cropInfo.firstLine + 1;

            if (height > 0)
            {
                const FLOAT IFEactualVbi                     = m_pSensorModeData->minVerticalBlanking * pixClockHz /
                                                               static_cast<FLOAT>(m_pSensorModeData->outPixelClock);
                const UINT32 IFEMinRequiredVbi               = 32;
                m_IFEThroughPut[static_cast<UINT>(IFEIndex)] = static_cast<FLOAT>(IFEMinRequiredVbi) / IFEactualVbi;
                CAMX_LOG_INFO(CamxLogGroupISP, "IFE Index %d Throughtput %f  IFE actual VBI %f",
                              IFEIndex, m_IFEThroughPut[static_cast<UINT>(IFEIndex)], IFEactualVbi);
            }

        }
        else
        {
            pixClockHz = m_ifeClockRate[static_cast<UINT>(IFEIndex)];
        }
    }

    return pixClockHz;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupHVXInitialConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupHVXInitialConfig(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_pIFEHVXModule)
    {
        result = (static_cast<IFEHVX*>(m_pIFEHVXModule))->InitConfiguration(pInputData);
        if (CamxResultSuccess == result)
        {
            m_HVXInputData.DSPMode  = DSPModeRoundTrip;
            pInputData->HVXData     = m_HVXInputData.HVXConfiguration;
        }
        else if (CamxResultEUnsupported == result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "DSP operaion is not supported");
            m_HVXInputData.enableHVX = FALSE;
            result                   = CamxResultSuccess;
        }
        else
        {
            m_HVXInputData.enableHVX    = FALSE;
            CAMX_LOG_ERROR(CamxLogGroupISP, "result %d ", result);
        }
    }

    CAMX_LOG_INFO(CamxLogGroupISP, "DSenable  %d ", m_HVXInputData.HVXConfiguration.DSEnabled);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PrepareStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::PrepareStreamOn()
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != m_pIFEHVXModule) && (TRUE == m_HVXInputData.enableHVX))
    {
        result = (static_cast<IFEHVX*>(m_pIFEHVXModule))->PrepareStreamOn();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::OnStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::OnStreamOff(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(modeBitmask);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Prepare stream off ");

    if ((NULL != m_pIFEHVXModule) && (TRUE == m_HVXInputData.enableHVX))
    {
        result = (static_cast<IFEHVX*>(m_pIFEHVXModule))->OnStreamOff(modeBitmask);
    }

    m_initialConfigPending = TRUE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetFPS()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::GetFPS()
{
    CamxResult        result                 = CamxResultSuccess;
    static const UINT UsecasePropertiesIFE[] = { PropertyIDUsecaseFPS };
    const UINT        length                 = CAMX_ARRAY_SIZE(UsecasePropertiesIFE);
    VOID*             pData[length]          = { 0 };

    UINT64            usecasePropertyDataIFEOffset[length] = { 0 };

    GetDataList(UsecasePropertiesIFE, pData, usecasePropertyDataIFEOffset, length);

    if (NULL != pData[0])
    {
        m_FPS = *reinterpret_cast<UINT*>(pData[0]);
    }
    else
    {
        m_FPS = 30;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculateRDIClockRate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 IFENode::CalculateRDIClockRate()
{
    UINT64 clockRate = CalculatePixelClockRate(m_pSensorModeData->resolution.outputWidth);

    // We'll write out at 2 bytes per pixel
    clockRate = clockRate >> 1;

    // Force minimum clock rate (dependent on HW version)
    clockRate = Utils::MaxUINT64(HwEnvironment::GetInstance()->GetPlatformStaticCaps()->minIFEHWClkRate, clockRate);

    CAMX_LOG_VERBOSE(CamxLogGroupPower, "RDI clock=%llu", clockRate);
    return clockRate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculateSensorLineDuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT IFENode::CalculateSensorLineDuration(
    UINT64 ifeClockRate)
{
    FLOAT lineDuration             = 1.0f;
    FLOAT lineLength               = 0.0f;
    FLOAT sensorHeightwithBlanking = 0.0f;

    if (0 < m_pSensorModeData->minVerticalBlanking)
    {
        sensorHeightwithBlanking = static_cast<FLOAT>(m_pSensorModeData->resolution.outputHeight +
            m_pSensorModeData->minVerticalBlanking);

        if ((FALSE == Utils::FEqual(m_pSensorModeData->maxFPS, 0.0f)) &&
            (FALSE == Utils::FEqual(sensorHeightwithBlanking, 0.0f)))
        {
            // Convert line duration to MicroSeconds - So --> 1.0f x 1000000.0f
            lineDuration = (1000000.0f / m_pSensorModeData->maxFPS) / sensorHeightwithBlanking;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "lineDuration=%f maxFPS %f  outputHeight %d minVerticalBlanking %d ",
                                        lineDuration,
                                        m_pSensorModeData->maxFPS,
                                        m_pSensorModeData->resolution.outputHeight,
                                        m_pSensorModeData->minVerticalBlanking);

    }
    else if (0 < ifeClockRate)
    {
        if (0 != m_pSensorModeData->minHorizontalBlanking)
        {
            lineLength = static_cast<FLOAT>(m_pSensorModeData->resolution.outputWidth) +
                         static_cast<FLOAT>(m_pSensorModeData->minHorizontalBlanking);
        }
        else
        {
            lineLength = static_cast<FLOAT>(
                m_pSensorModeData->outPixelClock / (m_pSensorModeData->maxFPS * m_pSensorModeData->numLinesPerFrame));
        }

        lineDuration = lineLength * 1000000.0f / ifeClockRate;

        // IFE pixels clock can be lower due to  IFE ppc efficiency. When calculate the actual line duration, we need to remove
        // the IFE ppc efficiency.

        lineDuration /= HwEnvironment::GetInstance()->GetPlatformStaticCaps()->IFEPixelsPerClock;

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "lineDuration=%f clockRate=%llu lineLength=%f ",
            lineDuration, ifeClockRate, lineLength);

    }

    return lineDuration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::IsRDIBWPath
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::IsRDIBWPath(
    IFEBWPathGrouping  path)
{
    BOOL bIsRDIPath;

    switch (path)
    {
        case DataPathRDI0 :
        case DataPathRDI1 :
        case DataPathRDI2 :
        case DataPathRDI3 :
            bIsRDIPath = TRUE;
            break;
        default :
            bIsRDIPath = FALSE;
            break;
    }

    return bIsRDIPath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::InitializeAllBWPaths
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::InitializeAllBWPaths(
    UINT32  numFIFEs,
    UINT64  camnocSplitBWbytes,
    UINT64  externalSplitBWbytes)
{
    for (UINT i = 0; i < numFIFEs; i++)
    {
        for (UINT index = 0; index < DataPathMax; index++)
        {
            CSLAXIperPathBWVote* pBWPathPerVote = &m_BWResourceConfigV2.outputPathBWInfo[(i * DataPathMax) + index];

            pBWPathPerVote->usageData = (0 == i) ? IFEUsageLeftPixelPath : IFEUsageRighftPixelPath;

            if (TRUE == IsRDIBWPath(static_cast<IFEBWPathGrouping>(index)))
            {
                if (0 == i)
                {
                    pBWPathPerVote->usageData = IFEUsageRDIPath;
                }
                else
                {
                    // RDI vote is for only one HW.Make it invalid for other VFE
                    pBWPathPerVote->usageData = IFEUsageInvalid;
                }
            }

            pBWPathPerVote->transactionType = CSLAXITransactionWrite;
            pBWPathPerVote->pathDataType    = index;
            pBWPathPerVote->camnocBW        = camnocSplitBWbytes;
            pBWPathPerVote->mnocABBW        = externalSplitBWbytes;
            pBWPathPerVote->mnocIBBW        = camnocSplitBWbytes;
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetOverrideBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::GetOverrideBandwidth(
    INT                  overrideCamnocMBytes,
    INT                  overrideExternalMBytes)
{
    UINT       totalOutputPorts                 = 0;
    UINT32     outputPortId[MaxDefinedIFEOutputPorts];

    GetAllOutputPortIds(&totalOutputPorts, outputPortId);

    if (0 < overrideCamnocMBytes)
    {
        overrideCamnocMBytes = overrideExternalMBytes;
    }
    else if (0 < overrideExternalMBytes)
    {
        overrideExternalMBytes = overrideCamnocMBytes;
    }

    CheckForRDIOnly();
    UINT   numActivePorts = 0;
    if (FALSE == m_RDIOnlyUseCase)
    {
        numActivePorts++;
        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            numActivePorts++;
        }
    }

    for (UINT outputPortIndex = 0; outputPortIndex < totalOutputPorts; outputPortIndex++)
    {
        switch (outputPortId[outputPortIndex])
        {
            case IFEOutputPortRDI0:
            case IFEOutputPortRDI1:
            case IFEOutputPortRDI2:
            case IFEOutputPortRDI3:
                numActivePorts++;
                break;

            default:
                break;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPower, "totalOutputPorts=%d numActivePorts=%d", totalOutputPorts, numActivePorts);

    UINT64 camnocSplitBWbytes   = static_cast<UINT64>(overrideCamnocMBytes) * 1000000 / numActivePorts;
    UINT64 externalSplitBWbytes = static_cast<UINT64>(overrideExternalMBytes) * 1000000 / numActivePorts;

    if (FALSE == m_RDIOnlyUseCase)
    {
        m_BWResourceConfig.leftPixelVote.camnocBWbytes    = camnocSplitBWbytes;
        m_BWResourceConfig.leftPixelVote.externalBWbytes  = externalSplitBWbytes;
        CAMX_LOG_INFO(CamxLogGroupPower, "BW leftPixelVote.camnocBWbytes=%llu", camnocSplitBWbytes);
        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            m_BWResourceConfig.rightPixelVote.camnocBWbytes   = camnocSplitBWbytes;
            m_BWResourceConfig.rightPixelVote.externalBWbytes = externalSplitBWbytes;
            CAMX_LOG_INFO(CamxLogGroupPower, "BW rightPixelVote.camnocBWbytes=%llu", camnocSplitBWbytes);
        }
    }

    for (UINT outputPortIndex = 0; outputPortIndex < totalOutputPorts; outputPortIndex++)
    {
        switch (outputPortId[outputPortIndex])
        {
            case IFEOutputPortRDI0:
            case IFEOutputPortRDI1:
            case IFEOutputPortRDI2:
            case IFEOutputPortRDI3:
            {
                UINT32 idx = outputPortId[outputPortIndex] - IFEOutputPortRDI0;
                m_BWResourceConfig.rdiVote[idx].camnocBWbytes   = camnocSplitBWbytes;
                m_BWResourceConfig.rdiVote[idx].externalBWbytes = externalSplitBWbytes;
                CAMX_LOG_INFO(CamxLogGroupPower, "BW RDI idx=%d camnocBWbytes=%llu", idx, camnocSplitBWbytes);
                break;
            }

            default:
                break;
        }
    }

    UINT       numIFEs = 1;

    if (IFEModuleMode::DualIFENormal == m_mode)
    {
        numIFEs = 2;
    }

    InitializeAllBWPaths(numIFEs, camnocSplitBWbytes, externalSplitBWbytes);
    m_BWResourceConfigV2.numPaths = numIFEs * DataPathMax;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculatePixelPortLineBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 IFENode::CalculatePixelPortLineBandwidth(
    UINT32 outputWidthInBytes,
    UINT   IFEIndex)
{
    UINT64 camnocBWbytes = static_cast<UINT64>( (outputWidthInBytes * 1000000.f) / m_sensorLineTimeMSecs[IFEIndex]);

    return camnocBWbytes;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetBWPathGrouping
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBWPathGrouping IFENode::GetBWPathGrouping(
    UINT32 outputPortID)
{
    IFEBWPathGrouping groupValue;

    switch (outputPortID)
    {
        case IFEOutputPortFull:
            groupValue = DataPathVideo;
            break;

        case IFEOutputPortDisplayFull:
            groupValue = DataPathDisplay;
            break;

        case IFEOutputPortStatsHDRBE:
        case IFEOutputPortStatsHDRBHIST:
        case IFEOutputPortStatsTLBG:
        case IFEOutputPortStatsBF:
        case IFEOutputPortStatsAWBBG:
        case IFEOutputPortStatsBHIST:
        case IFEOutputPortStatsRS:
        case IFEOutputPortStatsCS:
        case IFEOutputPortStatsIHIST:
            groupValue = DataPathStats;
            break;

        case IFEOutputPortDS4:
        case IFEOutputPortDisplayDS4:
        case IFEOutputPortDS16:
        case IFEOutputPortDisplayDS16:
        case IFEOutputPortFD:
        case IFEOutputPortLCR:
            groupValue = DataPathLinear;
            break;

        case IFEOutputPortPDAF:
        case IFEOutputPortStatsDualPD:
            groupValue = DataPathPDAF;
            break;

        case IFEOutputPortCAMIFRaw:
        case IFEOutputPortLSCRaw:
        case IFEOutputPortGTMRaw:
            groupValue = DataPathPixelRaw;
            break;

        case IFEOutputPortRDI0:
            groupValue = DataPathRDI0;
            break;

        case IFEOutputPortRDI1:
            groupValue = DataPathRDI1;
            break;

        case IFEOutputPortRDI2:
            groupValue = DataPathRDI2;
            break;

        case IFEOutputPortRDI3:
            groupValue = DataPathRDI3;
            break;

        default:
            groupValue = DataPathLinear;
            break;
    }

    return groupValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::NeedsActiveIFEABVote
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::NeedsActiveIFEABVote()
{
    BOOL IFEABVote = FALSE;

    CSLCameraTitanChipVersion titanChipVersion = static_cast<Titan17xContext *>(GetHwContext())->GetTitanChipVersion();

    switch (titanChipVersion)
    {
        case CSLCameraTitanChipVersion::CSLTitan175V120:
        case CSLCameraTitanChipVersion::CSLTitan175V130:
            IFEABVote = TRUE;
            break;

        default:
            IFEABVote = FALSE;
            break;
    }

    return IFEABVote;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetUBWCCompressionRatio
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT IFENode::GetUBWCCompressionRatio(
    UINT   srcWidth,
    UINT   srcHeight,
    const ImageFormat* pImageFormat)
{
    FLOAT compressionRatio = 1.0f;

    if (UBWCVersion::UBWCVersion2 == pImageFormat->ubwcVerInfo.version)
    {
        // Setting UBWC 2.0 Compression ratio
        if (ImageFormatUtils::IsUHDResolution(srcWidth, srcHeight))
        {
            if ((TRUE == ImageFormatUtils::Is10BitFormat(pImageFormat->format)))
            {
                compressionRatio = IFEUBWCvr2CompressionRatio10BitUHD;
            }
            else
            {
                compressionRatio = IFEUBWCvr2CompressionRatio8BitUHD;
            }
        }
        else
        {
            // Setting UBWC 2.0 Compression ratio for NON- UHD resolution
            if ((TRUE == ImageFormatUtils::Is10BitFormat(pImageFormat->format)))
            {
                compressionRatio = IFEUBWCvr2CompressionRatio10Bit;

            }
            else
            {
                compressionRatio = IFEUBWCvr2CompressionRatio8Bit;

            }
        }

    }
    else if (UBWCVersion::UBWCVersion3 == pImageFormat->ubwcVerInfo.version)
    {
        // Setting UBWC 3.0 Compression ratio
        if (ImageFormatUtils::IsUHDResolution(srcWidth, srcHeight))
        {
            if ((TRUE == ImageFormatUtils::Is10BitFormat(pImageFormat->format)))
            {
                if (TRUE == pImageFormat->ubwcVerInfo.lossy)
                {
                    compressionRatio = IFEUBWCvr3CompressionRatio10BitUHDLossy;
                }
                else
                {
                    compressionRatio = IFEUBWCvr3CompressionRatio10BitUHDLossless;
                }
            }
            else
            {
                if (TRUE == pImageFormat->ubwcVerInfo.lossy)
                {
                    compressionRatio = IFEUBWCvr3CompressionRatio8BitUHDLossy;
                }
                else
                {
                    compressionRatio = IFEUBWCvr3CompressionRatio8BitUHDLossless;
                }
            }
        }
        else
        {
            // Setting UBWC 3.0 Compression ratio for NON- UHD resolution
            if ((TRUE == ImageFormatUtils::Is10BitFormat(pImageFormat->format)))
            {
                if (TRUE == pImageFormat->ubwcVerInfo.lossy)
                {
                    compressionRatio = IFEUBWCvr3CompressionRatio10BitLossy;
                }
                else
                {
                    compressionRatio = IFEUBWCvr3CompressionRatio10BitLossless;
                }
            }
            else
            {
                if (TRUE == pImageFormat->ubwcVerInfo.lossy)
                {
                    compressionRatio = IFEUBWCvr3CompressionRatio8BitLossy;
                }
                else
                {
                    compressionRatio = IFEUBWCvr3CompressionRatio8BitLossless;
                }
            }
        }
    }
    else
    {
        // Setting UBWC 4.0 Compression ratio
        if (ImageFormatUtils::IsUHDResolution(srcWidth, srcHeight))
        {
            if ((TRUE == ImageFormatUtils::Is10BitFormat(pImageFormat->format)))
            {
                if (TRUE == pImageFormat->ubwcVerInfo.lossy)
                {
                    compressionRatio = IFEUBWCvr4CompressionRatio10BitUHDLossy;
                }
                else
                {
                    compressionRatio = IFEUBWCvr4CompressionRatio10BitUHDLossless;
                }
            }
            else
            {
                if (TRUE == pImageFormat->ubwcVerInfo.lossy)
                {
                    compressionRatio = IFEUBWCvr4CompressionRatio8BitUHDLossy;
                }
                else
                {
                    compressionRatio = IFEUBWCvr4CompressionRatio8BitUHDLossless;
                }
            }
        }
        else
        {
            // Setting UBWC 4.0 Compression ratio for NON- UHD resolution
            if ((TRUE == ImageFormatUtils::Is10BitFormat(pImageFormat->format)))
            {
                if (TRUE == pImageFormat->ubwcVerInfo.lossy)
                {
                    compressionRatio = IFEUBWCvr4CompressionRatio10BitLossy;
                }
                else
                {
                    compressionRatio = IFEUBWCvr4CompressionRatio10BitLossless;
                }
            }
            else
            {
                if (TRUE == pImageFormat->ubwcVerInfo.lossy)
                {
                    compressionRatio = IFEUBWCvr4CompressionRatio8BitLossy;
                }
                else
                {
                    compressionRatio = IFEUBWCvr4CompressionRatio8BitLossless;
                }
            }
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPower,
                     "IFE srcWidth: %d srcHeight: %d ubwcVersion: %d ubwcLossy: %d, compressionRatio %f",
                     srcWidth, srcHeight, pImageFormat->ubwcVerInfo.version, pImageFormat->ubwcVerInfo.lossy,
                     compressionRatio);

    return compressionRatio;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculateBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::CalculateBandwidth(
    ExecuteProcessRequestData* pExecuteProcessRequestData,
    UINT64                     requestId)
{
    CAMX_UNREFERENCED_PARAM(requestId);

    CamxResult result                 = CamxResultSuccess;
    INT        overrideCamnocMBytes   = static_cast<INT>(GetStaticSettings()->ifeCamnocBandwidthMBytes);
    INT        overrideExternalMBytes = static_cast<INT>(GetStaticSettings()->ifeExternalBandwidthMBytes);
    UINT       perFrameDataIndex      = GetIFEPerFrameDataIndex(requestId);

    if ((0 > overrideCamnocMBytes) || (0 > overrideExternalMBytes))
    {
        CAMX_LOG_INFO(CamxLogGroupPower, "BW setting disabled by override camnoc=%d external=%d",
                      overrideCamnocMBytes, overrideExternalMBytes);
        result = CamxResultEDisabled;
    }
    else if ((0 < overrideCamnocMBytes) || (0 < overrideExternalMBytes))
    {
        GetOverrideBandwidth(overrideCamnocMBytes, overrideExternalMBytes);
    }
    else
    {
        // Overrides are disabled. Calculate based on HW characteristics.

        UINT       ISPDataIndexBase                 = 2; // Default to single IFE base
        UINT       totalOutputPorts                 = 0;
        UINT32     outputPortId[MaxDefinedIFEOutputPorts];
        UINT       numIFEs                          = 1;

        GetAllOutputPortIds(&totalOutputPorts, outputPortId);

        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            numIFEs          = 2;
            ISPDataIndexBase = 0;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "m_mode %d numIFEs %d ", m_mode, numIFEs);

        PerRequestActivePorts*  pPerRequestPorts;
        UINT                    numOutputPorts   = 0;
        if (NULL == pExecuteProcessRequestData)
        {
            // Calculating for all output ports
            numOutputPorts = totalOutputPorts;
        }
        else
        {
            // Calculating for output ports that are enabled for this request
            pPerRequestPorts = pExecuteProcessRequestData->pEnabledPortsInfo;

            CAMX_ASSERT(NULL != pPerRequestPorts);

            numOutputPorts = pPerRequestPorts->numOutputPorts;
        }


        UINT32 sensorVBI             = m_pSensorModeData->numLinesPerFrame - m_pSensorModeData->resolution.outputHeight;
        FLOAT  bandwidthMultiplierAB = (m_pSensorModeData->resolution.outputHeight + sensorVBI) /
            static_cast<FLOAT>(m_pSensorModeData->resolution.outputHeight);

        // For HFR use cases the IFE display and FD ports should be configured to run at lower FPS
        UINT32 previewFPS = m_FPS / m_usecaseNumBatchedFrames;

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "sensorVBI=%u Sensor FLL:%u Height:%u bwMultiplier:%f m_FPS:%u PreviewFPS:%u "
            "batchSize=%u",
            sensorVBI, m_pSensorModeData->numLinesPerFrame,
            m_pSensorModeData->resolution.outputHeight, bandwidthMultiplierAB,
            m_FPS, previewFPS, m_usecaseNumBatchedFrames);

        m_BWResourceConfigV2.numPaths = numIFEs * (DataPathMax);

        for (UINT IFEIndex = 0; IFEIndex < numIFEs; IFEIndex++)
        {
            UINT64     camnocBWbytes          = 0;
            UINT64     externalBWbytes        = 0;
            DOUBLE     PDAFLineBW             = 0;
            DOUBLE     camnocLineBW           = 0;
            UINT32     maxPixelPathInputWidth = 0;
            UINT64     rawPortsBWbytesAB;
            BOOL       statsUpdate;
            ISPInternalData*   pISPData       = NULL;

            CAMX_ASSERT(((IFEModuleMode::DualIFENormal == m_mode) && (0 == ISPDataIndexBase)) ||
                        (2 == ISPDataIndexBase));

            if (CamxInvalidRequestId == requestId)
            {
                pISPData       = &m_ISPData[ISPDataIndexBase + IFEIndex];
            }
            else
            {   // Per frame ISPData
                pISPData      = &m_IFEPerFrameData[perFrameDataIndex].ISPData[ISPDataIndexBase + IFEIndex];
            }


            IFEResourceBWVote*  pBWVote;
            statsUpdate = FALSE;

            if (static_cast<UINT>(IFESplitID::Left) == IFEIndex)
            {
                pBWVote = &m_BWResourceConfig.leftPixelVote;
            }
            else
            {
                pBWVote = &m_BWResourceConfig.rightPixelVote;
            }

            CAMX_LOG_VERBOSE(CamxLogGroupPower, "ReqId=%llu IFEIndex=%d", requestId, IFEIndex);

            for (UINT index = 0; index < numOutputPorts; index++)
            {
                // Calculating for all output ports or for output ports that are enabled for this request
                UINT32 portId = (NULL == pExecuteProcessRequestData) ?
                                    outputPortId[index] : pPerRequestPorts->pOutputPorts[index].portId;

                UINT                 outputPortIndex = OutputPortIndex(portId);
                UINT32               BWGroupPath     = GetBWPathGrouping(portId);
                CSLAXIperPathBWVote* pBWPathPerVote  =
                    &m_BWResourceConfigV2.outputPathBWInfo[(IFEIndex * DataPathMax) + (BWGroupPath)];

                pBWPathPerVote->transactionType = (IFEOutputPortRDIRD == portId) ?
                                                  CSLAXITransactionRead : CSLAXITransactionWrite;
                pBWPathPerVote->pathDataType    = BWGroupPath;
                pBWPathPerVote->usageData       = (static_cast<UINT>(IFESplitID::Left) == IFEIndex) ?
                                                  IFEUsageLeftPixelPath : IFEUsageRighftPixelPath;

                if (TRUE == IsRDIBWPath(static_cast<IFEBWPathGrouping>(BWGroupPath)))
                {
                    if (static_cast<UINT>(IFESplitID::Left) == IFEIndex)
                    {
                        pBWPathPerVote->usageData = IFEUsageRDIPath;
                    }
                    else
                    {
                        pBWPathPerVote->usageData = IFEUsageInvalid;
                    }
                }

                if (TRUE == m_isDisabledOutputPort[outputPortIndex])
                {
                    continue;
                }

                const  ImageFormat* pImageFormat = GetOutputPortImageFormat(outputPortIndex);
                if (NULL != pImageFormat)
                {
                    UINT32              outputLineWidthInPixels;
                    UINT32              outputLineHeightInPixels;
                    UINT32              portIdx;
                    UINT32              statsBW = 0;
                    FLOAT               bytesPerPix = 0.0f;
                    UINT32              croppedSensorInputWidth;
                    UINT32              croppedSensorInputHeight;
                    UINT32              outputWidthInBytes;
                    UINT32              outputWidthInBytesAB = 0;

                    PDAFLineBW = m_stripeConfigs[IFEIndex].stream[PDAFRawOutput].width * 2.0; // Plain16

                    switch (portId)
                    {
                        case IFEOutputPortFull:
                        case IFEOutputPortDisplayFull:
                        {
                            if (IFEOutputPortDisplayFull == portId)
                            {
                                portIdx = DisplayFullOutput;
                                croppedSensorInputWidth = pISPData->metadata.appliedCropInfo.displayFullPath.width;
                                croppedSensorInputHeight = pISPData->metadata.appliedCropInfo.displayFullPath.height;
                            }
                            else
                            {
                                portIdx = FullOutput;
                                croppedSensorInputWidth = pISPData->metadata.appliedCropInfo.fullPath.width;
                                croppedSensorInputHeight = pISPData->metadata.appliedCropInfo.fullPath.height;
                            }

                            outputLineWidthInPixels = m_stripeConfigs[IFEIndex].stream[portIdx].width;
                            outputLineHeightInPixels = m_stripeConfigs[IFEIndex].stream[portIdx].height;


                            UINT32 inputWidthInPixels = Utils::MaxUINT32(outputLineWidthInPixels, croppedSensorInputWidth);
                            maxPixelPathInputWidth = Utils::MaxUINT32(maxPixelPathInputWidth, inputWidthInPixels);

                            UINT32 inputHeightInPixels = Utils::MaxUINT32(outputLineHeightInPixels, croppedSensorInputHeight);

                            bytesPerPix = ImageFormatUtils::GetBytesPerPixel(pImageFormat);

                            UINT32 uncompressedOutputWidthInBytes;
                            FLOAT compressionRatio = 1.0f;

                            if (TRUE == ImageFormatUtils::IsUBWC(pImageFormat->format))
                            {
                                compressionRatio = GetUBWCCompressionRatio(inputWidthInPixels,
                                    inputHeightInPixels,
                                    pImageFormat);

                            }

                            outputWidthInBytes = static_cast<UINT32>(outputLineWidthInPixels * bytesPerPix);

                            externalBWbytes = CalculateExternalBWBytes(outputWidthInBytes / compressionRatio,
                                IFEIndex);

                            pBWPathPerVote->mnocABBW += externalBWbytes;
                            pBWVote->externalBWbytes += externalBWbytes;

                            uncompressedOutputWidthInBytes = static_cast<UINT32>(outputLineWidthInPixels * bytesPerPix);

                            outputWidthInBytesAB = m_stripeConfigs[IFEIndex].stream[portIdx].width *
                                m_stripeConfigs[IFEIndex].stream[portIdx].height *
                                bytesPerPix / compressionRatio;


                            camnocBWbytes = CalculatePixelPortLineBandwidth(uncompressedOutputWidthInBytes, IFEIndex);

                            pBWPathPerVote->camnocBW += camnocBWbytes;
                            pBWVote->camnocBWbytes += camnocBWbytes;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortFull Idx=%u inputWidthInPixels=%u "
                                "outputLineWidthInPixels=%u outputWidthInBytes=%u croppedSensorInputWidth=%u "
                                "uncompressedOutputWidthInBytes=%u format=%u "
                                "camnocBWbytes=%llu externalBWbytes %llu"
                                "Total for this path camnocBW=%llu mnocABBW=%llu",
                                requestId, portIdx, inputWidthInPixels, outputLineWidthInPixels,
                                outputWidthInBytes, croppedSensorInputWidth, uncompressedOutputWidthInBytes,
                                pImageFormat->format, camnocBWbytes, externalBWbytes,
                                pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);

                            if (IFEOutputPortFull == portId)
                            {
                                outputWidthInBytesAB *= m_FPS;
                            }
                            else
                            {
                                outputWidthInBytesAB *= previewFPS;
                            }

                            // Using mnocIBW variable temporarily to calculate
                            // AB bandwidth values that to be computed for SM7150

                            pBWPathPerVote->mnocIBBW += outputWidthInBytesAB;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortFull Idx=%u bytsPerPix:%f Dims:[%ux%u] "
                                "outputWidthInBytesAB=%llu externalBWbytesAB=%llu",
                                requestId, portIdx, bytesPerPix, m_stripeConfigs[IFEIndex].stream[portIdx].width,
                                m_stripeConfigs[IFEIndex].stream[portIdx].height, outputWidthInBytesAB,
                                pBWPathPerVote->mnocIBBW);

                            break;
                        }

                        case IFEOutputPortDS4:
                        case IFEOutputPortDisplayDS4:
                        {
                            if (IFEOutputPortDisplayDS4 == portId)
                            {
                                portIdx = DisplayDS4Output;
                                croppedSensorInputWidth = pISPData->metadata.appliedCropInfo.displayDS4Path.width;
                            }
                            else
                            {
                                portIdx = DS4Output;
                                croppedSensorInputWidth = pISPData->metadata.appliedCropInfo.DS4Path.width;
                            }
                            outputLineWidthInPixels   = m_stripeConfigs[IFEIndex].stream[portIdx].width;

                            UINT32 inputWidthInPixels = Utils::MaxUINT32(outputLineWidthInPixels, croppedSensorInputWidth);
                            maxPixelPathInputWidth    = Utils::MaxUINT32(maxPixelPathInputWidth, inputWidthInPixels);

                            bytesPerPix = ImageFormatUtils::GetBytesPerPixel(pImageFormat);

                            // 2: 2x2 tile in 8 bytes per 8 lines
                            outputWidthInBytes = static_cast<UINT32>(outputLineWidthInPixels * bytesPerPix); //  / 8.0f;
                            externalBWbytes    = CalculateExternalBWBytes(outputWidthInBytes, IFEIndex);

                            pBWPathPerVote->mnocABBW += externalBWbytes;
                            pBWVote->externalBWbytes += externalBWbytes;

                            camnocBWbytes = CalculatePixelPortLineBandwidth(outputWidthInBytes, IFEIndex);

                            pBWPathPerVote->camnocBW += camnocBWbytes;
                            pBWVote->camnocBWbytes   += camnocBWbytes;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortDS4  Idx=%u inputWidthInPixels=%u "
                                "outputLineWidthInPixels=%u outputWidthInBytes=%u croppedSensorInputWidth=%u "
                                "format=%u camnocBWbytes=%llu externalBWbytes=%llu"
                                "Total for this path camnocBW=%llu mnocABBW=%llu",
                                requestId, portIdx, inputWidthInPixels, outputLineWidthInPixels,
                                outputWidthInBytes, croppedSensorInputWidth,
                                pImageFormat->format, camnocBWbytes, externalBWbytes,
                                pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);

                            outputWidthInBytesAB = m_stripeConfigs[IFEIndex].stream[portIdx].width *
                                m_stripeConfigs[IFEIndex].stream[portIdx].height *
                                bytesPerPix;

                            if (IFEOutputPortDS4 == portId)
                            {
                                outputWidthInBytesAB *= m_FPS;
                            }
                            else
                            {
                                outputWidthInBytesAB *= previewFPS;
                            }

                            // Using mnocIBW variable temporarily to calculate
                            // AB bandwidth values that to be computed for SM7150
                            pBWPathPerVote->mnocIBBW += outputWidthInBytesAB;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortDS4  Idx=%u bytsPerPix:%f Dims:[%ux%u] "
                                "outputWidthInBytesAB=%llu externalBWbytesAB=%llu",
                                requestId, portIdx, bytesPerPix, m_stripeConfigs[IFEIndex].stream[portIdx].width,
                                m_stripeConfigs[IFEIndex].stream[portIdx].height, outputWidthInBytesAB,
                                pBWPathPerVote->mnocIBBW);


                            break;
                        }

                        case IFEOutputPortDS16:
                        case IFEOutputPortDisplayDS16:
                        {
                            if (IFEOutputPortDisplayDS16 == portId)
                            {
                                portIdx = DisplayDS16Output;
                                croppedSensorInputWidth = pISPData->metadata.appliedCropInfo.displayDS16Path.width;
                            }
                            else
                            {
                                portIdx = DS16Output;
                                croppedSensorInputWidth = pISPData->metadata.appliedCropInfo.DS16Path.width;
                            }
                            outputLineWidthInPixels   = m_stripeConfigs[IFEIndex].stream[portIdx].width;
                            UINT32 inputWidthInPixels = Utils::MaxUINT32(outputLineWidthInPixels, croppedSensorInputWidth);
                            maxPixelPathInputWidth    = Utils::MaxUINT32(maxPixelPathInputWidth, inputWidthInPixels);

                            bytesPerPix = ImageFormatUtils::GetBytesPerPixel(pImageFormat);

                            // 2: 2x2 tile in 8 bytes per 32 lines
                            outputWidthInBytes = static_cast<UINT32>(outputLineWidthInPixels * bytesPerPix); // / 32.0f;
                            externalBWbytes    = CalculateExternalBWBytes(outputWidthInBytes, IFEIndex);

                            pBWPathPerVote->mnocABBW += externalBWbytes;
                            pBWVote->externalBWbytes += externalBWbytes;

                            camnocBWbytes = CalculatePixelPortLineBandwidth(outputWidthInBytes, IFEIndex);

                            pBWPathPerVote->camnocBW += camnocBWbytes;
                            pBWVote->camnocBWbytes += camnocBWbytes;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortDS16 Idx=%u inputWidthInPixels=%u "
                                "outputLineWidthInPixels=%u outputWidthInBytes=%u croppedSensorInputWidth=%u "
                                "format=%u camnocBWbytes=%llu externalBWbytes=%llu"
                                "Total for this path camnocBW=%llu mnocABBW=%llu",
                                requestId, portIdx, inputWidthInPixels, outputLineWidthInPixels,
                                outputWidthInBytes, croppedSensorInputWidth,
                                pImageFormat->format, camnocBWbytes, externalBWbytes,
                                pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);

                            outputWidthInBytesAB = m_stripeConfigs[IFEIndex].stream[portIdx].width *
                                m_stripeConfigs[IFEIndex].stream[portIdx].height *
                                bytesPerPix;

                            if (IFEOutputPortDS16 == portId)
                            {
                                outputWidthInBytesAB *= m_FPS;
                            }
                            else
                            {
                                outputWidthInBytesAB *= previewFPS;
                            }

                            // Using mnocIBW variable temporarily to calculate
                            // AB bandwidth values that to be computed for SM7150
                            pBWPathPerVote->mnocIBBW += outputWidthInBytesAB;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortDS16 Idx=%u bytsPerPix:%f Dims:[%ux%u] "
                                "outputWidthInBytesAB=%llu externalBWbytesAB=%llu",
                                requestId, portIdx, bytesPerPix, m_stripeConfigs[IFEIndex].stream[portIdx].width,
                                m_stripeConfigs[IFEIndex].stream[portIdx].height, outputWidthInBytesAB,
                                pBWPathPerVote->mnocIBBW);


                            break;
                        }

                        case IFEOutputPortFD:
                        {
                            croppedSensorInputWidth   = pISPData->metadata.appliedCropInfo.FDPath.width;
                            outputLineWidthInPixels   = m_stripeConfigs[IFEIndex].stream[FDOutput].width;
                            UINT32 inputWidthInPixels = Utils::MaxUINT32(outputLineWidthInPixels, croppedSensorInputWidth);
                            maxPixelPathInputWidth    = Utils::MaxUINT32(maxPixelPathInputWidth, inputWidthInPixels);
                            bytesPerPix               = static_cast<FLOAT>(ImageFormatUtils::GetBytesPerPixel(pImageFormat));
                            outputWidthInBytes        = static_cast<UINT32>(outputLineWidthInPixels * bytesPerPix);
                            externalBWbytes           = CalculateExternalBWBytes(outputWidthInBytes, IFEIndex);;
                            pBWPathPerVote->mnocABBW += externalBWbytes;
                            pBWVote->externalBWbytes += externalBWbytes;

                            camnocBWbytes = CalculatePixelPortLineBandwidth(outputWidthInBytes, IFEIndex);

                            pBWPathPerVote->camnocBW += camnocBWbytes;
                            pBWVote->camnocBWbytes   += camnocBWbytes;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortFD   inputWidthInPixels=%u "
                                "outputLineWidthInPixels=%u outputWidthInBytes=%u croppedSensorInputWidth=%u "
                                "format=%u camnocBWbytes=%llu externalBWbytes=%llu"
                                "Total for this path camnocBW=%llu mnocABBW=%llu",
                                requestId, inputWidthInPixels, outputLineWidthInPixels,
                                outputWidthInBytes, croppedSensorInputWidth,
                                pImageFormat->format, camnocBWbytes, externalBWbytes,
                                pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);

                            outputWidthInBytesAB = m_stripeConfigs[IFEIndex].stream[FDOutput].width *
                                m_stripeConfigs[IFEIndex].stream[FDOutput].height *
                                bytesPerPix * previewFPS;

                            // Using mnocIBW variable temporarily to calculate
                            // AB bandwidth values that to be computed for SM7150
                            pBWPathPerVote->mnocIBBW += outputWidthInBytesAB;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortFD   bytsPerPix:%f Dims:[%ux%u] "
                                "outputWidthInBytesAB=%llu externalBWbytesAB=%llu",
                                requestId, bytesPerPix, m_stripeConfigs[IFEIndex].stream[FDOutput].width,
                                m_stripeConfigs[IFEIndex].stream[FDOutput].height, outputWidthInBytesAB,
                                pBWPathPerVote->mnocIBBW);

                            break;
                        }

                        case IFEOutputPortPDAF:
                            // hard-coded system modelling recommendation - may need to refine with actual PD ROI
                            // outputLineWidthInPixels = m_pSensorModeData->resolution.outputWidth * 0.6;

                            outputLineWidthInPixels   = m_stripeConfigs[IFEIndex].stream[PDAFRawOutput].width;
                            externalBWbytes           = CalculateExternalBWBytes(outputLineWidthInPixels * 2.0, IFEIndex);
                            pBWPathPerVote->mnocABBW += externalBWbytes;
                            pBWVote->externalBWbytes += externalBWbytes;

                            // Plain16
                            camnocBWbytes = CalculateCamnocBWBytes(outputLineWidthInPixels * 2.0, IFEIndex);

                            pBWPathPerVote->camnocBW += camnocBWbytes;
                            pBWVote->camnocBWbytes += camnocBWbytes;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortPDAF outputLineWidthInPixels=%u"
                                "camnocBWbytes=%llu externalBWbytes=%llu"
                                "Total for this path camnocBW=%llu mnocABBW=%llu",
                                requestId, outputLineWidthInPixels, camnocBWbytes, externalBWbytes,
                                pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);

                            bytesPerPix = static_cast<FLOAT>(ImageFormatUtils::GetBytesPerPixel(pImageFormat));
                            outputWidthInBytesAB = m_stripeConfigs[IFEIndex].stream[PDAFRawOutput].width  *
                                m_stripeConfigs[IFEIndex].stream[PDAFRawOutput].height *
                                bytesPerPix * m_FPS;

                            // Using mnocIBW variable temporarily to calculate
                            // AB bandwidth values that to be computed for SM7150
                            pBWPathPerVote->mnocIBBW += outputWidthInBytesAB;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortPDAF bytsPerPix:%f Dims:[%ux%u] "
                                "outputWidthInBytesAB=%llu externalBWbytesAB=%llu",
                                requestId, bytesPerPix, m_stripeConfigs[IFEIndex].stream[PDAFRawOutput].width,
                                m_stripeConfigs[IFEIndex].stream[PDAFRawOutput].height, outputWidthInBytesAB,
                                pBWPathPerVote->mnocIBBW);


                            break;

                        case IFEOutputPortCAMIFRaw:
                        case IFEOutputPortLSCRaw:
                            // Need to account for Crop block at PIXEL_RAW_DUMP_OUT
                            outputLineWidthInPixels = m_pSensorModeData->resolution.outputWidth;

                            // Bayer14 in 16 bit words
                            externalBWbytes = CalculateExternalBWBytes(outputLineWidthInPixels * 2, IFEIndex);

                            pBWPathPerVote->mnocABBW += externalBWbytes;
                            pBWVote->externalBWbytes += externalBWbytes;

                            camnocLineBW  = outputLineWidthInPixels * 2;
                            camnocBWbytes = CalculateCamnocBWBytes(camnocLineBW, IFEIndex);

                            pBWPathPerVote->camnocBW += camnocBWbytes;
                            pBWVote->camnocBWbytes   += camnocBWbytes;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu CAMIF/LSC Raw Port=%u outputLineWidthInPixels=%d"
                                "camnocBWbytes=%llu externalBWbytes=%llu"
                                "Total for this path camnocBW=%llu mnocABBW=%llu",
                                requestId, portId, outputLineWidthInPixels, camnocBWbytes, externalBWbytes,
                                pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);

                            rawPortsBWbytesAB = static_cast<UINT64>((camnocLineBW - PDAFLineBW)*
                                m_pSensorModeData->resolution.outputHeight * m_FPS);

                            // Using mnocIBW variable temporarily to calculate
                            // AB bandwidth values that to be computed for SM7150
                            pBWPathPerVote->mnocIBBW += rawPortsBWbytesAB;
                            CAMX_LOG_VERBOSE(CamxLogGroupPower, "ReqId=%llu IFEOutputPortsRAW rawPortsBWbytesAB=%llu",
                                requestId, rawPortsBWbytesAB);

                            break;

                        case IFEOutputPortGTMRaw:
                            // Need to account for Crop block at PIXEL_RAW_DUMP_OUT
                            outputLineWidthInPixels = m_pSensorModeData->resolution.outputWidth;
                            // ARGB Plain16 packed, 16bits for each of the 4 channels

                            externalBWbytes = CalculateExternalBWBytes((outputLineWidthInPixels * 4 * 2), IFEIndex);

                            pBWPathPerVote->mnocABBW += externalBWbytes;
                            pBWVote->externalBWbytes += externalBWbytes;

                            camnocLineBW  = (outputLineWidthInPixels * 4 * 2);
                            camnocBWbytes = CalculateCamnocBWBytes(camnocLineBW, IFEIndex);

                            pBWPathPerVote->camnocBW += camnocBWbytes;
                            pBWVote->camnocBWbytes   += camnocBWbytes;


                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortGTMRaw outputLineWidthInPixels=%d"
                                "camnocBWbytes=%llu externalBWbytes=%llu"
                                "Total for this path camnocBW=%llu mnocABBW=%llu",
                                requestId, outputLineWidthInPixels, camnocBWbytes, externalBWbytes,
                                pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);

                            rawPortsBWbytesAB = static_cast<UINT64>((camnocLineBW - PDAFLineBW)*
                                m_pSensorModeData->resolution.outputHeight * m_FPS);

                            // Using mnocIBW variable temporarily to calculate
                            // AB bandwidth values that to be computed for SM7150
                            pBWPathPerVote->mnocIBBW += rawPortsBWbytesAB;

                            CAMX_LOG_VERBOSE(CamxLogGroupPower, "ReqId=%llu IFEOutputPortsRAW rawPortsBWbytesAB=%llu",
                                requestId, rawPortsBWbytesAB);

                            break;

                        case IFEOutputPortLCR:
                            outputLineWidthInPixels = m_pSensorModeData->resolution.outputWidth;
                            // Bayer14 in 16 bit words
                            externalBWbytes = CalculateExternalBWBytes((outputLineWidthInPixels * 2), IFEIndex);

                            pBWPathPerVote->mnocABBW += externalBWbytes;
                            pBWVote->externalBWbytes += externalBWbytes;

                            camnocLineBW  = (outputLineWidthInPixels * 2);
                            camnocBWbytes = CalculateCamnocBWBytes(camnocLineBW, IFEIndex);

                            pBWPathPerVote->camnocBW += camnocBWbytes;
                            pBWVote->camnocBWbytes   += camnocBWbytes;


                            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                "ReqId=%llu IFEOutputPortLCR outputLineWidthInPixels=%d"
                                "camnocBWbytes=%llu externalBWbytes=%llu"
                                "Total for this path camnocBW=%llu mnocABBW=%llu",
                                requestId, outputLineWidthInPixels, camnocBWbytes, externalBWbytes,
                                pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);

                            rawPortsBWbytesAB = static_cast<UINT64>((camnocLineBW - PDAFLineBW)*
                                m_pSensorModeData->resolution.outputHeight * m_FPS);

                            // Using mnocIBW variable temporarily to calculate
                            // AB bandwidth values that to be computed for SM7150
                            pBWPathPerVote->mnocIBBW += rawPortsBWbytesAB;
                            CAMX_LOG_VERBOSE(CamxLogGroupPower, "ReqId=%llu IFEOutputPortsRAW rawPortsBWbytesAB=%llu",
                                requestId, rawPortsBWbytesAB);

                            break;

                        case IFEOutputPortStatsDualPD:
                            // Vote For Single IFE as the DUALPD output is from RDI currently
                            if (0 == IFEIndex)
                            {
                                UINT PDAFstreamIndex;

                                if (TRUE == FindSensorStreamConfigIndex(StreamType::PDAF, &PDAFstreamIndex))
                                {
                                    UINT32 PDAFWidth = m_pSensorModeData->streamConfig[PDAFstreamIndex].frameDimension.width;
                                    CAMX_LOG_VERBOSE(CamxLogGroupISP, "PDAF RDI_%d w=%d", portId, PDAFWidth);
                                    // PLAIN16 Format
                                    externalBWbytes = CalculateExternalBWBytes((PDAFWidth * 2), IFEIndex);

                                    pBWPathPerVote->mnocABBW += externalBWbytes;
                                    pBWVote->externalBWbytes += externalBWbytes;

                                    camnocLineBW  = (PDAFWidth * 2);
                                    camnocBWbytes = CalculateCamnocBWBytes(camnocLineBW, IFEIndex);

                                    pBWPathPerVote->camnocBW += camnocBWbytes;
                                    pBWVote->camnocBWbytes   += camnocBWbytes;

                                    CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                        "ReqId=%llu IFEOutputPortStatsDualPD camnocBWbytes=%llu externalBWbytes=%llu"
                                        "Total for this path camnocBW=%llu mnocABBW=%llu",
                                        requestId, camnocBWbytes, externalBWbytes,
                                        pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);

                                    rawPortsBWbytesAB = static_cast<UINT64>((camnocLineBW - PDAFLineBW)*
                                        m_pSensorModeData->resolution.outputHeight * m_FPS);

                                    // Using mnocIBW variable temporarily to calculate
                                    // AB bandwidth values that to be computed for SM7150
                                    pBWPathPerVote->mnocIBBW += rawPortsBWbytesAB;
                                    CAMX_LOG_VERBOSE(CamxLogGroupPower, "ReqId=%llu IFEOutputPortsRAW rawPortsBWbytesAB=%llu",
                                        requestId, rawPortsBWbytesAB);

                                }
                            }
                            break;

                        case IFEOutputPortRDI0:
                        case IFEOutputPortRDI1:
                        case IFEOutputPortRDI2:
                        case IFEOutputPortRDI3:
                            if (static_cast<UINT>(IFESplitID::Left) == IFEIndex) // KMD only uses IFE0 blob for RDI votes
                            {
                                UINT32 idx = portId - IFEOutputPortRDI0;
                                FLOAT  pdafRatio = 1.0f;

                                if (RDIMaxNum > idx)
                                {
                                    const RawFormat*   pRawFormat = &(pImageFormat->formatParams.rawFormat);

                                    if (CamX::Format::Blob == pImageFormat->format)
                                    {
                                        UINT32 streamType =
                                            TranslatePortSourceTypeToSensorStreamConfigType(
                                                GetOutputPortSourceType(outputPortIndex));

                                        UINT streamIndex;
                                        bytesPerPix = 0.0f;
                                        if (TRUE == FindSensorStreamConfigIndex(
                                            static_cast<StreamType>(streamType), &streamIndex))
                                        {
                                            bytesPerPix = m_pSensorModeData->streamConfig[streamIndex].bitWidth / 8.0f;
                                        }
                                        else
                                        {
                                            CAMX_LOG_ERROR(CamxLogGroupPower, "Invalid streamType=%u", streamType);
                                        }
                                    }
                                    else
                                    {
                                        bytesPerPix = ImageFormatUtils::GetBytesPerPixel(pImageFormat);
                                    }

                                    if (0.0f < bytesPerPix)
                                    {
                                        // UINT32 stride = pRawFormat->stride;
                                        UINT32 stride = m_pSensorModeData->resolution.outputWidth;

                                        DOUBLE bandwidth =
                                            (stride * 1000000.0f * bytesPerPix) / m_sensorLineTimeMSecs[IFEIndex];

                                        // Double bandwidth request in FS mode since there will be an additional read
                                        /// @todo (CAMX-4450) Update Bandwidth and Clock voting for FastShutter Usecase
                                        if ((TRUE == m_enableBusRead) &&
                                            (IFEOutputPortRDI0 == portId))
                                        {
                                            DOUBLE bw = bandwidth;
                                            bandwidth *= 2;
                                            CAMX_LOG_INFO(CamxLogGroupISP, "FS: Increase bandwidth %lf to %lf",
                                                bw, bandwidth);
                                        }

                                        if (TRUE == m_RDIOnlyUseCase)
                                        {
                                            // Compensate for overly aggressive DCD in RDI-only case - needs to be tuned
                                            bandwidth *= 5.0f;
                                        }
                                        else
                                        {
                                            // Overwrite if the RDI port is associated with PDAF port source type.
                                            if (PortSrcTypePDAF == GetOutputPortSourceType(outputPortIndex))
                                            {
                                                UINT PDAFstreamIndex;

                                                if (TRUE == FindSensorStreamConfigIndex(StreamType::PDAF, &PDAFstreamIndex))
                                                {
                                                    UINT32 PDAFHeight =
                                                        m_pSensorModeData->streamConfig[PDAFstreamIndex].frameDimension.height;
                                                    UINT32 PDAFWidth =
                                                        m_pSensorModeData->streamConfig[PDAFstreamIndex].frameDimension.width;
                                                    UINT32 sensorHeight = m_pSensorModeData->resolution.outputHeight;
                                                    UINT32 sensorWidth  = m_pSensorModeData->resolution.outputWidth;

                                                    CAMX_LOG_VERBOSE(CamxLogGroupPower, "PDAF RDI_%d h=%d w=%d full h=%d, w=%d",
                                                        portId, PDAFHeight, PDAFWidth, sensorHeight, sensorWidth);

                                                    if ( (sensorHeight > 0) && (sensorWidth > 0))
                                                    {
                                                        pdafRatio = static_cast<FLOAT>(PDAFHeight * PDAFWidth) /
                                                            static_cast<FLOAT>(sensorHeight * sensorWidth);
                                                    }
                                                    bandwidth *= static_cast<DOUBLE>(pdafRatio);
                                                }
                                            }
                                        }

                                        // We are multiplying each path with 5.0 for first few frames below at the end of loop,
                                        // so do not multiply here
                                        pBWPathPerVote->mnocABBW += static_cast<UINT64>(bandwidth);
                                        pBWPathPerVote->camnocBW += static_cast<UINT64>(bandwidth);

                                        if (m_highInitialBWCnt < GetStaticSettings()->IFENumFramesHighBW)
                                        {
                                            bandwidth *= 5.0f;
                                        }

                                        m_BWResourceConfig.rdiVote[idx].camnocBWbytes   = static_cast<UINT64>(bandwidth);
                                        m_BWResourceConfig.rdiVote[idx].externalBWbytes = static_cast<UINT64>(bandwidth);

                                        CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                            "ReqId=%llu IFEOutputPortRDIx[%d] format=%u stride=%u bytesPerPix=%f "
                                            "camnocBWbytes==externalBWbytes=%llu"
                                            "Total for this path camnocBW=%llu mnocABBW=%llu",
                                            requestId, idx, pImageFormat->format, stride, bytesPerPix,
                                            bandwidth, pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);

                                        bandwidth = m_pSensorModeData->resolution.outputWidth  *
                                            m_pSensorModeData->resolution.outputHeight *
                                            bytesPerPix * m_FPS * bandwidthMultiplierAB * pdafRatio;

                                        // Using mnocIBW variable temporarily to calculate
                                        // AB bandwidth values that to be computed for SM7150
                                        pBWPathPerVote->mnocIBBW += static_cast<UINT64>(bandwidth);
                                        CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                            "ReqId=%llu IFEOutputPortRDIx[%d] bytesPerPix=%f Dims:[%ux%u] "
                                            "externalBWbytesAB=%lf",
                                            requestId, idx, bytesPerPix,
                                            m_pSensorModeData->resolution.outputWidth,
                                            m_pSensorModeData->resolution.outputHeight, bandwidth);

                                    }
                                    else
                                    {
                                        CAMX_LOG_ERROR(CamxLogGroupPower, "Unsupported format: %u", pImageFormat->format);
                                    }
                                }
                            }
                            break;

                        case IFEOutputPortStatsHDRBE:
                        case IFEOutputPortStatsHDRBHIST:
                        case IFEOutputPortStatsTLBG:
                        case IFEOutputPortStatsBF:
                        case IFEOutputPortStatsAWBBG:
                        case IFEOutputPortStatsBHIST:
                        case IFEOutputPortStatsRS:
                        case IFEOutputPortStatsCS:
                        case IFEOutputPortStatsIHIST:
                            // Add fixed 90MB BW for stats outputs based on modelling recommendation
                            statsBW = 90000000 / numIFEs;
                            if (FALSE == statsUpdate)
                            {
                                pBWPathPerVote->mnocABBW = statsBW;
                                pBWPathPerVote->camnocBW = statsBW;
                                pBWPathPerVote->mnocIBBW = statsBW;
                                pBWVote->camnocBWbytes   += pBWPathPerVote->camnocBW;
                                pBWVote->externalBWbytes += pBWPathPerVote->mnocABBW;

                                statsUpdate = TRUE;
                                CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                    "ReqId=%llu IFEOutputPortStats camnocBWbytes=externalBWbytes=%llu"
                                    "Total for this path camnocBW=%llu mnocABBW=%llu",
                                    requestId, statsBW, pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);
                            }
                            break;

                        default:
                            CAMX_LOG_WARN(CamxLogGroupPower, "Not a valid Port ID  %d ", portId);
                            break;
                    }
                }
            }

            // Loop through all paths again to add additional BW needed
            // for zoom calculations and High BW for first few frames.
            for (UINT index = 0; index < DataPathMax; index++)
            {
                CSLAXIperPathBWVote* pBWPathPerVote =
                    &m_BWResourceConfigV2.outputPathBWInfo[(IFEIndex * DataPathMax) + index];

                // Account for zoom
                if ((0                      < maxPixelPathInputWidth)                              &&
                    (FALSE                  == IsRDIBWPath(static_cast<IFEBWPathGrouping>(index))) &&
                    (CSLAXIPathDataIFEStats != static_cast<IFEBWPathGrouping>(index)))
                {
                    UINT sensorWidth = m_pSensorModeData->resolution.outputWidth;
                    if (m_mode == IFEModuleMode::DualIFENormal)
                    {
                        sensorWidth /= 2;
                    }
                    pBWPathPerVote->mnocABBW =
                        pBWPathPerVote->mnocABBW * sensorWidth / maxPixelPathInputWidth;
                }

                if (FALSE == IsRDIBWPath(static_cast<IFEBWPathGrouping>(index)))
                {
                    pBWPathPerVote->mnocIBBW *= bandwidthMultiplierAB;
                }


                // Workaround for overflow on use case transitions
                if (m_highInitialBWCnt < GetStaticSettings()->IFENumFramesHighBW)
                {
                    pBWPathPerVote->camnocBW *= 5;
                    pBWPathPerVote->mnocABBW *= 5;
                }

                // The AB bandwidth values only need to be computed for SM7150
                // due to increase in size of the latency buffers in CAMNOC h/w
                if ((TRUE == GetStaticSettings()->enableActiveIFEABVote) &&
                    (TRUE == NeedsActiveIFEABVote()))
                {
                    // Swap values from IBBW and ABBW
                    UINT64 tempExternalBWValue  = pBWPathPerVote->mnocABBW;
                    pBWPathPerVote->mnocABBW    = pBWPathPerVote->mnocIBBW;
                    pBWPathPerVote->mnocIBBW    = tempExternalBWValue;
                }
                else
                {
                    pBWPathPerVote->mnocIBBW = pBWPathPerVote->camnocBW;
                }

                CAMX_LOG_VERBOSE(CamxLogGroupPower,
                                 "ReqId=%llu ------- PixelPathTotal IFE[%d]: InstanceID=%u "
                                 "Total for this path %d camnocBW = %llu mnocABBW = %llu",
                                 requestId, IFEIndex, InstanceID(), index , pBWPathPerVote->camnocBW, pBWPathPerVote->mnocABBW);
            }


            // Account for zoom
            if (0 < maxPixelPathInputWidth)
            {
                UINT sensorWidth = m_pSensorModeData->resolution.outputWidth;
                if (m_mode == IFEModuleMode::DualIFENormal)
                {
                    sensorWidth /= 2;
                }
                pBWVote->externalBWbytes = pBWVote->externalBWbytes * sensorWidth / maxPixelPathInputWidth;
            }

            // Workaround for overflow on use case transitions
            if (m_highInitialBWCnt < GetStaticSettings()->IFENumFramesHighBW)
            {
                pBWVote->externalBWbytes *= 5;
                pBWVote->camnocBWbytes   *= 5;
            }

            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                             "ReqId=%llu ------- PixelPathTotal IFE[%d]: InstanceID=%u "
                             "HalCrop.width=%u maxPixelPathInputWidth=%u camnocBWbytes=%llu externalBWbytes=%llu",
                             requestId, IFEIndex, InstanceID(), m_HALTagsData.HALCrop.width, maxPixelPathInputWidth,
                             pBWVote->camnocBWbytes, pBWVote->externalBWbytes);

        }
        m_highInitialBWCnt++;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculateExternalBWBytes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 IFENode::CalculateExternalBWBytes(
    DOUBLE externalLineBW,
    UINT   IFEIndex)
{
    FLOAT overheadCamnoc   = 1.0f; // This needs to be tuned
    FLOAT overheadExt      = 1.0f; // This needs to be tuned
    UINT64 externalBWbytes = 0L;

    if (0 != m_pSensorModeData->minHorizontalBlanking)
    {
        // We need to write out camnocLineBW bytes within one line time or IFE will overflow. Since
        // the IFE has a one-line buffer and we can use the entire line width including Horizontal blanking
        // duration to write out the data, we amortize (spread) the BW over the blanking period as well.
        // For Dual-IFE, the blanking will include the time remaining after CAMIF crop - so we have more
        // time to write out the same data. However, the net BW will be similar to single IFE since the
        externalBWbytes +=
            static_cast<UINT64>((overheadExt * 1000000.0f * externalLineBW) / m_sensorLineTimeMSecs[IFEIndex]);
    }
    else
    {
        // Be more conservative when we don't have accurate blanking data
        // DOUBLE sensorLineTime = 1 / m_pSensorModeData->maxFPS / m_pSensorModeData->numLinesPerFrame;
        // totalBW = (lineBW / sensorLineTime);
        // Reordered to avoid loss of precision and avoid divisions:
        externalBWbytes += static_cast<UINT64>(overheadExt * externalLineBW *
            m_pSensorModeData->resolution.outputHeight * m_pSensorModeData->maxFPS);
    }

    return externalBWbytes;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculateCamnocBWBytes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 IFENode::CalculateCamnocBWBytes(
    DOUBLE camnocLineBW,
    UINT   IFEIndex)
{
    FLOAT  overheadCamnoc        = 1.0f; // This needs to be tuned
    FLOAT  overheadExt           = 1.0f; // This needs to be tuned
    UINT64 camnocBWbytes         = 0L;
    UINT64 rawPortsCamnocBWbytes;

    if (0 != m_pSensorModeData->minHorizontalBlanking)
    {
        // We need to write out camnocLineBW bytes within one line time or IFE will overflow. Since
        // the IFE has a one-line buffer and we can use the entire line width including Horizontal blanking
        // duration to write out the data, we amortize (spread) the BW over the blanking period as well.
        // For Dual-IFE, the blanking will include the time remaining after CAMIF crop - so we have more
        // time to write out the same data. However, the net BW will be similar to single IFE since the
        // line is split over 2 IFEs.
        rawPortsCamnocBWbytes =
            static_cast<UINT64>((overheadCamnoc * 1000000.0f * camnocLineBW) / m_sensorLineTimeMSecs[IFEIndex]);
    }
    else
    {
        // Be more conservative when we don't have accurate blanking data
        // DOUBLE sensorLineTime = 1 / m_pSensorModeData->maxFPS / m_pSensorModeData->numLinesPerFrame;
        // totalBW = (lineBW / sensorLineTime);
        // Reordered to avoid loss of precision and avoid divisions:

        rawPortsCamnocBWbytes = static_cast<UINT64>(overheadCamnoc * camnocLineBW *
            m_pSensorModeData->resolution.outputHeight * m_pSensorModeData->maxFPS);
    }
    camnocBWbytes += rawPortsCamnocBWbytes;

    return camnocBWbytes;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupResourceClockConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupResourceClockConfig()
{
    CamxResult              result                  = CamxResultSuccess;
    SIZE_T                  resourceClockConfigSize = 0;
    UINT64                  maxCalculatedClock      = 0;
    IFEResourceClockConfig* pClockResourceConfig;

    resourceClockConfigSize      = sizeof(IFEResourceClockConfig) + (sizeof(UINT64) * (RDIMaxNum - 1));
    pClockResourceConfig         = static_cast<IFEResourceClockConfig*>(CAMX_CALLOC(resourceClockConfigSize));

    if (NULL != pClockResourceConfig)
    {
        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            pClockResourceConfig->usageType = ISPResourceUsageDual;
        }
        else
        {
            pClockResourceConfig->usageType         = ISPResourceUsageSingle;
            pClockResourceConfig->rightPixelClockHz = 0;
        }
    }
    else
    {
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        m_sensorLineTimeMSecs[0] = 0;
        m_sensorLineTimeMSecs[1] = 0;

        result = CamxResultEDisabled;
        pClockResourceConfig->leftPixelClockHz = GetPixelClockRate(IFESplitID::Left);
        maxCalculatedClock                     = pClockResourceConfig->leftPixelClockHz;
        if (0 < pClockResourceConfig->leftPixelClockHz)
        {
            result = CamxResultSuccess;

            m_sensorLineTimeMSecs[0] = CalculateSensorLineDuration(pClockResourceConfig->leftPixelClockHz);
            CAMX_ASSERT(0 < m_sensorLineTimeMSecs[0]);

            if (IFEModuleMode::DualIFENormal == m_mode)
            {
                pClockResourceConfig->rightPixelClockHz = GetPixelClockRate(IFESplitID::Right);
                if (0 < pClockResourceConfig->rightPixelClockHz)
                {
                    m_sensorLineTimeMSecs[1] = CalculateSensorLineDuration(pClockResourceConfig->rightPixelClockHz);
                    CAMX_ASSERT(0 < m_sensorLineTimeMSecs[1]);
                }
                if (maxCalculatedClock < pClockResourceConfig->rightPixelClockHz)
                {
                    maxCalculatedClock = pClockResourceConfig->rightPixelClockHz;
                }
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        pClockResourceConfig->rdiClockHz[0] = CalculateRDIClockRate();
        pClockResourceConfig->numRdi = RDIMaxNum;

        // In DUAL IFE DULA PD Case, both IFEs are consuming the PDDATA but voting for only IFE. This is a temp
        // workaround to solve the IFE violations due to less clock.
        if (IFEModuleMode::DualIFENormal == m_mode)
        {
            if (maxCalculatedClock < pClockResourceConfig->rdiClockHz[0])
            {
                maxCalculatedClock = pClockResourceConfig->rdiClockHz[0];
            }
            pClockResourceConfig->rightPixelClockHz = maxCalculatedClock;
            pClockResourceConfig->leftPixelClockHz  = maxCalculatedClock;
        }
        CAMX_LOG_INFO(CamxLogGroupPower, "IFE:%d leftIfePixClk=%llu Hz, rightIfePixClk=%llu Hz",
                      InstanceID(),
                      pClockResourceConfig->leftPixelClockHz, pClockResourceConfig->rightPixelClockHz);

        result = PacketBuilder::WriteGenericBlobData(m_pGenericBlobCmdBuffer,
                                                     IFEGenericBlobTypeResourceClockConfig,
                                                     static_cast<UINT32>(resourceClockConfigSize),
                                                     reinterpret_cast<BYTE*>(pClockResourceConfig));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPower, "Error: Writing Clock Blob data result=%d", result);
        }
    }
    else if (CamxResultEDisabled == result)
    {
        // Reset state since there is no real error
        result = CamxResultSuccess;
    }


    if (NULL != pClockResourceConfig)
    {
        CAMX_FREE(pClockResourceConfig);
        pClockResourceConfig = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupCSIDClockConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupCSIDClockConfig(
    UINT32 sensorBitWidth)
{
    CamxResult          result       = CamxResultSuccess;
    IFECSIDClockConfig* pClockConfig = { 0 };

    pClockConfig = static_cast<IFECSIDClockConfig*>(CAMX_CALLOC(sizeof(IFECSIDClockConfig)));

    if (NULL != pClockConfig)
    {
        result = CalculateCSIDClockRate(&pClockConfig->CSIDClockHz, sensorBitWidth);
    }

    if (CamxResultSuccess == result)
    {
        result = PacketBuilder::WriteGenericBlobData(m_pGenericBlobCmdBuffer,
            IFEGenericBlobTypeCSIDClockConfig,
            static_cast<UINT32>(sizeof(IFECSIDClockConfig)),
            reinterpret_cast<BYTE*>(pClockConfig));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPower, "Error: Writing Clock Blob data result=%d", result);
        }
    }
    else
    {
        result = CamxResultSuccess;
        CAMX_LOG_VERBOSE(CamxLogGroupPower, "Error: Fail to update clock config =%d", result);
    }

    if (NULL != pClockConfig)
    {
        CAMX_FREE(pClockConfig);
        pClockConfig = NULL;
    }

    return result;
 }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupResourceBWConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupResourceBWConfig(
    ExecuteProcessRequestData* pExecuteProcessRequestData,
    UINT64                     requestId)
{
    CamxResult           result = CamxResultEFailed;

    Utils::Memset(&m_BWResourceConfig, 0, sizeof(IFEResourceBWConfig));
    Utils::Memset(&m_BWResourceConfigV2, 0, sizeof(IFEResourceBWConfigVer2));

    m_BWResourceConfig.usageType = (IFEModuleMode::DualIFENormal == m_mode)? ISPResourceUsageDual: ISPResourceUsageSingle;
    m_BWResourceConfig.numRdi    = RDIMaxNum;

    InitializeAllBWPaths((IFEModuleMode::DualIFENormal == m_mode) ? 2 : 1, 0, 0);
    result = CalculateBandwidth(pExecuteProcessRequestData, requestId);

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPower,
                         "IFE:%d "
                         "Left  camnocBWbytes=%llu externalBWbytes=%llu "
                         "Right camnocBWbytes=%llu externalBWbytes=%llu "
                         "RDI_0 camnocBWbytes=%llu externalBWbytes=%llu "
                         "RDI_1 camnocBWbytes=%llu externalBWbytes=%llu "
                         "RDI_2 camnocBWbytes=%llu externalBWbytes=%llu "
                         "RDI_3 camnocBWbytes=%llu externalBWbytes=%llu ",
                         InstanceID(),
                         m_BWResourceConfig.leftPixelVote.camnocBWbytes,
                         m_BWResourceConfig.leftPixelVote.externalBWbytes,
                         m_BWResourceConfig.rightPixelVote.camnocBWbytes,
                         m_BWResourceConfig.rightPixelVote.externalBWbytes,
                         m_BWResourceConfig.rdiVote[0].camnocBWbytes,
                         m_BWResourceConfig.rdiVote[0].externalBWbytes,
                         m_BWResourceConfig.rdiVote[1].camnocBWbytes,
                         m_BWResourceConfig.rdiVote[1].externalBWbytes,
                         m_BWResourceConfig.rdiVote[2].camnocBWbytes,
                         m_BWResourceConfig.rdiVote[2].externalBWbytes,
                         m_BWResourceConfig.rdiVote[3].camnocBWbytes,
                         m_BWResourceConfig.rdiVote[3].externalBWbytes);

        result = PacketBuilder::WriteGenericBlobData(m_pGenericBlobCmdBuffer,
                                                     IFEGenericBlobTypeResourceBWConfig,
                                                     static_cast<UINT32>(sizeof(IFEResourceBWConfig)),
                                                     reinterpret_cast<BYTE*>(&m_BWResourceConfig));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPower, "Error: Writing BW Blob data result=%d", result);
        }
    }
    else if (CamxResultEDisabled == result)
    {
        // Reset state since there is no real error
        result = CamxResultSuccess;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupResourceBWConfigV2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupResourceBWConfigV2(
    ExecuteProcessRequestData* pExecuteProcessRequestData,
    UINT64                     requestId)
{
    CamxResult     result = CamxResultEFailed;

    Utils::Memset(&m_BWResourceConfigV2, 0, sizeof(IFEResourceBWConfigVer2));
    Utils::Memset(&m_BWResourceConfig, 0, sizeof(IFEResourceBWConfig));

    m_BWResourceConfigV2.usageType =
        (IFEModuleMode::DualIFENormal == m_mode) ? ISPResourceUsageDual : ISPResourceUsageSingle;

    InitializeAllBWPaths((IFEModuleMode::DualIFENormal == m_mode) ? 2 : 1, 0, 0);

    result = CalculateBandwidth(pExecuteProcessRequestData, requestId);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "BWConfig:: result %d ", result);

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPower, "Node[%s] Req[%lld] : m_mode=%d, numPaths=%d",
                       NodeIdentifierString(), requestId, m_mode, m_BWResourceConfigV2.numPaths);

        for (UINT i = 0; i < m_BWResourceConfigV2.numPaths; i++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPower, "Path[%d] : usageData=%d, Transac=%d, Path=%d, camnoc=%lld, mnoc=[%lld %lld]",
                           i,
                           m_BWResourceConfigV2.outputPathBWInfo[i].usageData,
                           m_BWResourceConfigV2.outputPathBWInfo[i].transactionType,
                           m_BWResourceConfigV2.outputPathBWInfo[i].pathDataType,
                           m_BWResourceConfigV2.outputPathBWInfo[i].camnocBW,
                           m_BWResourceConfigV2.outputPathBWInfo[i].mnocABBW,
                           m_BWResourceConfigV2.outputPathBWInfo[i].mnocIBBW);
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPower,
                         "IFE:%d "
                         "Left  camnocBWbytes=%llu externalBWbytes=%llu "
                         "Right camnocBWbytes=%llu externalBWbytes=%llu "
                         "RDI_0 camnocBWbytes=%llu externalBWbytes=%llu "
                         "RDI_1 camnocBWbytes=%llu externalBWbytes=%llu "
                         "RDI_2 camnocBWbytes=%llu externalBWbytes=%llu "
                         "RDI_3 camnocBWbytes=%llu externalBWbytes=%llu ",
                         InstanceID(),
                         m_BWResourceConfig.leftPixelVote.camnocBWbytes,
                         m_BWResourceConfig.leftPixelVote.externalBWbytes,
                         m_BWResourceConfig.rightPixelVote.camnocBWbytes,
                         m_BWResourceConfig.rightPixelVote.externalBWbytes,
                         m_BWResourceConfig.rdiVote[0].camnocBWbytes,
                         m_BWResourceConfig.rdiVote[0].externalBWbytes,
                         m_BWResourceConfig.rdiVote[1].camnocBWbytes,
                         m_BWResourceConfig.rdiVote[1].externalBWbytes,
                         m_BWResourceConfig.rdiVote[2].camnocBWbytes,
                         m_BWResourceConfig.rdiVote[2].externalBWbytes,
                         m_BWResourceConfig.rdiVote[3].camnocBWbytes,
                         m_BWResourceConfig.rdiVote[3].externalBWbytes);

        result = PacketBuilder::WriteGenericBlobData(m_pGenericBlobCmdBuffer,
                                                     IFEGenericBlobTypeResourceBWConfigV2,
                                                     static_cast<UINT32>(sizeof(IFEResourceBWConfigVer2)),
                                                     reinterpret_cast<BYTE*>(&m_BWResourceConfigV2));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPower, "Error: Writing BW Blob data result=%d", result);
        }
    }
    else if (CamxResultEDisabled == result)
    {
        // Reset state since there is no real error
        CAMX_LOG_VERBOSE(CamxLogGroupPower, "BWConfig:: Reset state since there is no real error result %d ", result);
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CanSkipAlgoProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::CanSkipAlgoProcessing(
    ISPInputData* pInputData
    ) const
{
    UINT skipFactor   = m_instanceProperty.IFEStatsSkipPattern;
    BOOL skipRequired = FALSE;

    if (0 != skipFactor)
    {
        UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(pInputData->frameNum);

        skipFactor = (requestIdOffsetFromLastFlush <= FirstValidRequestId + GetMaximumPipelineDelay()) ? 1
                                                                                                                   : skipFactor;
        skipRequired = ((requestIdOffsetFromLastFlush % skipFactor) == 0) ? FALSE : TRUE;
    }

    // This check is required to identify if this is flash snapshot.
    // For flash snapshot we should not skip tintless/LSC programming
    if ((0 < pInputData->numberOfLED) && (1.0f < pInputData->pAECUpdateData->LEDInfluenceRatio))
    {
        skipRequired = FALSE;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "skipRequired %u requestId %llu skipFactor %u",
                     skipRequired, pInputData->frameNum, skipFactor);

    return skipRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::IsADRCEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::IsADRCEnabled(
    ISPInputData* pInputData,
    FLOAT*        pGtmPercentage)
{
    BOOL        adrcEnabled = FALSE;

    // Get the Default ADRC status.
    if (FALSE == m_RDIOnlyUseCase)
    {
        IQInterface::GetADRCParams(pInputData, &adrcEnabled, pGtmPercentage, GetIFESWTMCModuleInstanceVersion());
    }
    else
    {
        adrcEnabled = FALSE;
    }
    return adrcEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFENode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result             = CamxResultSuccess;
    UINT32     tagCount           = 0;
    UINT32     numMetadataTags    = CAMX_ARRAY_SIZE(IFEMetadataOutputTags);
    UINT32     numRawMetadataTags = CAMX_ARRAY_SIZE(IFEMetadataRawOutputTags);
    UINT32     numVendorTags      = CAMX_ARRAY_SIZE(IFEOutputVendorTags);
    UINT32     tagID;

    if (TRUE == m_RDIOnlyUseCase)
    {
        if (numRawMetadataTags < MaxTagsPublished)
        {
            for (UINT32 tagIndex = 0; tagIndex < numRawMetadataTags; ++tagIndex)
            {
                pPublistTagList->tagArray[tagCount++] = IFEMetadataRawOutputTags[tagIndex];
            }
        }
        else
        {
            result = CamxResultEOutOfBounds;
            CAMX_LOG_ERROR(CamxLogGroupMeta, "ERROR More space needed to add publish tags (%d)",
                numMetadataTags);
        }
    }
    else
    {
        if (numMetadataTags + numRawMetadataTags + numVendorTags < MaxTagsPublished)
        {
            for (UINT32 tagIndex = 0; tagIndex < numMetadataTags; ++tagIndex)
            {
                pPublistTagList->tagArray[tagCount++] = IFEMetadataOutputTags[tagIndex];
            }

            for (UINT32 tagIndex = 0; tagIndex < numRawMetadataTags; ++tagIndex)
            {
                pPublistTagList->tagArray[tagCount++] = IFEMetadataRawOutputTags[tagIndex];
            }

            for (UINT32 tagIndex = 0; tagIndex < numVendorTags; ++tagIndex)
            {
                if (TRUE == SkipTagForPublishList(tagIndex))
                {
                    continue;
                }

                result = VendorTagManager::QueryVendorTagLocation(
                    IFEOutputVendorTags[tagIndex].pSectionName,
                    IFEOutputVendorTags[tagIndex].pTagName,
                    &tagID);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                        IFEOutputVendorTags[tagIndex].pSectionName,
                        IFEOutputVendorTags[tagIndex].pTagName);
                    break;
                }
                pPublistTagList->tagArray[tagCount++] = tagID;

                m_vendorTagArray[tagIndex] = tagID;
            }
        }
        else
        {
            result = CamxResultEOutOfBounds;
            CAMX_LOG_ERROR(CamxLogGroupMeta, "ERROR More space needed to add publish tags (%d %d %d)",
                numMetadataTags, numRawMetadataTags, numVendorTags);
        }
    }

    if (CamxResultSuccess == result)
    {
        pPublistTagList->tagCount = tagCount;
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tags will be published", tagCount);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::IsFSModeEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::IsFSModeEnabled(
    BOOL* pIsFSModeEnabled)
{
    CamxResult  result            = CamxResultSuccess;
    UINT32      metaTag           = 0;
    UINT8       isFSModeEnabled   = FALSE;

    result = VendorTagManager::QueryVendorTagLocation(
        "org.quic.camera.SensorModeFS", "SensorModeFS", &metaTag);

    if (CamxResultSuccess == result)
    {
        UINT   props[]                         = { metaTag | UsecaseMetadataSectionMask };
        VOID*  pData[CAMX_ARRAY_SIZE(props)]   = { 0 };
        UINT64 offsets[CAMX_ARRAY_SIZE(props)] = { 0 };
        GetDataList(props, pData, offsets, CAMX_ARRAY_SIZE(props));
        if (NULL != pData[0])
        {
            isFSModeEnabled = *reinterpret_cast<UINT8*>(pData[0]);
        }
        else
        {
            isFSModeEnabled = FALSE;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to read SensorModeFS tag %d", result);
        isFSModeEnabled = FALSE;
    }

    *pIsFSModeEnabled = isFSModeEnabled;
    CAMX_LOG_VERBOSE(CamxLogGroupCore, "Fast shutter mode enabled - %d",
        *pIsFSModeEnabled);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::IsFSSnapshot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFENode::IsFSSnapshot(
    ExecuteProcessRequestData* pExecuteProcessRequestData,
    UINT64                     requestID)
{
    // FS snapshot is similar to an RDI only usecase with RDI1 being used for snapshot
    BOOL isFSSnapshot                       = FALSE;
    PerRequestActivePorts* pPerRequestPorts = pExecuteProcessRequestData->pEnabledPortsInfo;

    if ((TRUE == m_enableBusRead) && (NULL != pPerRequestPorts))
    {
        PerRequestOutputPortInfo* pOutputPort = NULL;
        // Loop through all the ports to check if RDI1 i.e. snapshot request is made
        for (UINT i = 0; i < pPerRequestPorts->numOutputPorts; i++)
        {
            pOutputPort = &pPerRequestPorts->pOutputPorts[i];
            if ((NULL != pOutputPort) && (IFEOutputPortRDI1 == pOutputPort->portId))
            {
                isFSSnapshot = TRUE;
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "FS: IFE:%d Snapshot Requested for ReqID:%llu",
                                 InstanceID(), requestID);
                break;
            }
        }
    }

    return isFSSnapshot;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFENode::QueryCDMDMIType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdType IFENode::QueryCDMDMIType(
    UINT32 titanVersion)
{
    CmdType commandType = CmdType::CDMDMI;

    switch(titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan175:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan150:
            commandType = CmdType::CDMDMI32;
            break;
        case CSLCameraTitanVersion::CSLTitan480:
        default:
            commandType = CmdType::CDMDMI;
            break;
    }

    return commandType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetMetadataContrastLevel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::GetMetadataContrastLevel(
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
        UINT64 vendorTagsContrastIFEOffset[lengthContrast]  = { 0 };

        GetDataList(VendorTagContrast, pDataContrast, vendorTagsContrastIFEOffset, lengthContrast);
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
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Manual Contrast Level = %d", pHALTagsData->contrastLevel);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Cannot obtain Contrast Level. Set default to 5");
            pHALTagsData->contrastLevel = 5;
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupISP, "No Contrast Level available. Set default to 5");
        pHALTagsData->contrastLevel = 5; // normal without contrast change
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetMetadataTonemapCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::GetMetadataTonemapCurve(
    ISPHALTagsData* pHALTagsData)
{
    // Deep copy tone map curves, only when the tone map is contrast curve
    if (TonemapModeContrastCurve == pHALTagsData->tonemapCurves.tonemapMode)
    {
        ISPTonemapPoint* pBlueTonemapCurve  = NULL;
        ISPTonemapPoint* pGreenTonemapCurve = NULL;
        ISPTonemapPoint* pRedTonemapCurve   = NULL;

        static const UINT VendorTagsIFE[] =
        {
            InputTonemapCurveBlue,
            InputTonemapCurveGreen,
            InputTonemapCurveRed,
        };

        const static UINT length                    = CAMX_ARRAY_SIZE(VendorTagsIFE);
        VOID* pData[length]                         = { 0 };
        UINT64 vendorTagsTonemapIFEOffset[length]   = { 0 };

        GetDataList(VendorTagsIFE, pData, vendorTagsTonemapIFEOffset, length);

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
// IFENode::GetMetadataTMC12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::GetMetadataTMC12(
    ISPInputData*    pModuleInput)
{
    CamxResult  result = CamxResultSuccess;

    static const UINT PropertiesTMC12[] =
    {
        PropertyIDIFEADRCInfoOutput,
    };

    const SIZE_T numTags                          = CAMX_ARRAY_SIZE(PropertiesTMC12);
    VOID*        pPropertyDataTMC12[numTags]      = { 0 };
    UINT64       propertyDataTMC12Offset[numTags] = { 1 };

    result = GetDataList(PropertiesTMC12, pPropertyDataTMC12, propertyDataTMC12Offset, numTags);

    if (CamxResultSuccess == result)
    {
        //  Tag: PropertyIDIFEADRCInfoOutput
        if (NULL != pPropertyDataTMC12[0])
        {
            //  Get the the currentCalculatedHistogram/currentCalculatedCDF of previous frames
            pModuleInput->triggerData.pPreviousCalculatedHistogram =
                                                (static_cast<ADRCData*>(pPropertyDataTMC12[0]))->currentCalculatedHistogram;
            pModuleInput->triggerData.pPreviousCalculatedCDF       =
                                                (static_cast<ADRCData*>(pPropertyDataTMC12[0]))->currentCalculatedCDF;
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%u Failed to get PropertyIDIFEADRCInfoOutput for TMC12", InstanceID());
    }

    GetMetadataDarkBoostOffset(pModuleInput);
    GetMetadataFourthToneAnchor(pModuleInput);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%u TMC12 override data: dark boost offset = %f, fourth tone anchor = %f",
                     InstanceID(),
                     pModuleInput->triggerData.overrideDarkBoostOffset,
                     pModuleInput->triggerData.overrideFourthToneAnchor);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::SetupICAGrid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::SetupICAGrid()
{
    CamxResult result  = CamxResultSuccess;

    m_publishLDCGridData = IsLDCEnabledByFlow();
    if (TRUE == m_publishLDCGridData)
    {
        result = AllocateICAGridData();
        if (CamxResultSuccess == result)
        {
            FillICAChromatixGridData(NULL);
        }
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "%s, LDC Grid setup failed", NodeIdentifierString());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::AllocateICAGridData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::AllocateICAGridData()
{
    CamxResult result = CamxResultSuccess;

    for (UINT i = 0; i < GridMaxType; i++)
    {
        for (UINT j = 0; j < LDCMaxPath; j++)
        {
            if (NULL == m_pWarpGridDataOut[i][j])
            {
                m_pWarpGridDataOut[i][j] = static_cast<NcLibIcaGrid*>(CAMX_CALLOC(sizeof(NcLibIcaGrid)));
            }
            if (NULL == m_pWarpGridDataOut[i][j])
            {
                result = CamxResultENoMemory;
                CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to allocate memory for Warp Grid Data out type %d, path %d, Out %p",
                    i, j, m_pWarpGridDataOut[i][j]);
                break;
            }
        }
        if (CamxResultSuccess != result)
        {
            break;
        }

        if (NULL == m_pWarpGridDataIn[i])
        {
            m_pWarpGridDataIn[i] = static_cast<NcLibIcaGrid*>(CAMX_CALLOC(sizeof(NcLibIcaGrid)));
        }

        if (NULL == m_pWarpGridDataIn[i])
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to allocate memory for Warp Grid type %d, In %p",
                           i, m_pWarpGridDataIn[i]);
            break;
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::DeallocateICAGridData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::DeallocateICAGridData()
{
    for (UINT i = 0; i < GridMaxType; i++)
    {
        if (NULL != m_pWarpGridDataIn[i])
        {
            CAMX_FREE(m_pWarpGridDataIn[i]);
            m_pWarpGridDataIn[i] = NULL;
        }

        for (UINT j = 0; j < LDCMaxPath; j++)
        {
            if (NULL != m_pWarpGridDataOut[i][j])
            {
                CAMX_FREE(m_pWarpGridDataOut[i][j]);
                m_pWarpGridDataOut[i][j] = NULL;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFENode::FillICAChromatixGridData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::FillICAChromatixGridData(
    ChiTuningModeParameter* pTuningModeData)
{
    BOOL                            result              = FALSE;
    UINT32                          numRows             = 0;
    UINT32                          numColumns          = 0;
    UINT32                          QFactor             = Q4;
    UINT                            cameraId            = GetPipeline()->GetCameraId();
    TuningDataManager*              pTuningManager      = HwEnvironment::GetInstance()->GetTuningDataManager(cameraId);
    VOID*                           pICAChromatix       = NULL;
    VOID*                           pICARgnData         = NULL;
    NcLibIcaGrid*                   pWarpInGridOut2In   = m_pWarpGridDataIn[Out2InGrid];
    NcLibIcaGrid*                   pWarpInGridIn2Out   = m_pWarpGridDataIn[In2OutGrid];
    IFECapabilityInfo*              pCapability         = NULL;
    UINT                            numSelectors        = 1;
    TuningMode                      defaultSelectors[1] = { { ModeType::Default, { 0 } } };
    TuningMode*                     pSelectors          = &defaultSelectors[0];

    if (NULL != pTuningModeData)
    {
        numSelectors       = pTuningModeData->noOfSelectionParameter;
        pSelectors         = reinterpret_cast<TuningMode*>(&pTuningModeData->TuningMode[0]);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "#Of selector %d", numSelectors);

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
                CAMX_LOG_ERROR(CamxLogGroupISP, "ICA version is not defined");
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
                            chromatix_ica30_reserve.ld_u2i_grid_valid))) ?
                    TRUE : FALSE;

                m_ICAGridIn2OutEnabled =
                    ((TRUE == !!((static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                        enable_section.ctc_transform_grid_enable)) &&
                        (TRUE == !!((static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                            chromatix_ica30_reserve.ld_i2u_grid_valid))) ?
                    TRUE : FALSE;
                m_ICAGridGeometry = (1 == (static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                    chromatix_ica30_reserve.ld_i2u_grid_geometry) ?ICAGeometryCol67Row51 : ICAGeometryCol35Row27;

                pICARgnData = &(static_cast<ica_3_0_0::chromatix_ica30Type *>(pICAChromatix))->
                    chromatix_ica30_core.mod_ica30_lens_posn_data[0].
                    lens_posn_data.mod_ica30_lens_zoom_data[0].
                    lens_zoom_data.mod_ica30_aec_data[0].
                    ica30_rgn_data;
                break;
            case ICAVersion20:
                m_ICAGridOut2InEnabled = ((TRUE == !!((static_cast<ica_2_0_0::chromatix_ica20Type *>(pICAChromatix))->
                    enable_section.ctc_transform_grid_enable)) &&
                    (TRUE == !!((static_cast<ica_2_0_0::chromatix_ica20Type *>(pICAChromatix))->
                        chromatix_ica20_reserve.undistorted_to_lens_distorted_output_ld_grid_valid))) ?
                    TRUE : FALSE;

                m_ICAGridIn2OutEnabled = ((TRUE == !!((static_cast<ica_2_0_0::chromatix_ica20Type *>(pICAChromatix))->
                    enable_section.ctc_transform_grid_enable)) &&
                    (TRUE == !!((static_cast<ica_2_0_0::chromatix_ica20Type *>(pICAChromatix))->
                        chromatix_ica20_reserve.distorted_input_to_undistorted_ldc_grid_valid))) ?
                    TRUE : FALSE;

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
                        chromatix_ica10_reserve.undistorted_to_lens_distorted_output_ld_grid_valid))) ?
                    TRUE : FALSE;

                m_ICAGridIn2OutEnabled = ((TRUE == !!((static_cast<ica_1_0_0::chromatix_ica10Type *>(pICAChromatix))->
                    enable_section.ctc_transform_grid_enable)) &&
                    (TRUE == !!((static_cast<ica_1_0_0::chromatix_ica10Type *>(pICAChromatix))->
                        chromatix_ica10_reserve.distorted_input_to_undistorted_ldc_grid_valid))) ?
                    TRUE : FALSE;

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
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid warp grid chromatix");
    }
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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid ica grid geometry");
            break;
    }

    if (NULL != pICARgnData)
    {
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
            pWarpInGridIn2Out->enable                   = 1;
            pWarpInGridIn2Out->reuseGridTransform       = 0;
            pWarpInGridIn2Out->transformDefinedOnWidth  = IcaVirtualDomainWidth;
            pWarpInGridIn2Out->transformDefinedOnHeight = IcaVirtualDomainHeight;
            pWarpInGridIn2Out->geometry                 = static_cast<NcLibIcaGridGeometry>(m_ICAGridGeometry);
            pWarpInGridIn2Out->extrapolatedCorners      = 0;
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
                    m_ICAGridDomain = (static_cast<ica_3_0_0::ica30_rgn_dataType *>(pICARgnData))->ldc_calib_domain;
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
                    m_ICAGridDomain = 0;
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
                    m_ICAGridDomain = 0;
                    pWarpInGridIn2Out->extrapolatedCorners = 1;
                    break;
                default:
                    break;
            }
        }

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "LDC: ICA grid geometry %d, domain %d, InGrid Out2In enabled %d, In2Out enabled %d",
                         m_ICAGridGeometry, m_ICAGridDomain, m_ICAGridOut2InEnabled, m_ICAGridIn2OutEnabled);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid chromatix ICA data ptr");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ConvertIntoDistortedZoomWindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::ConvertIntoDistortedZoomWindow(
    CropWindow* pCrop,
    INT32 fullWidth,
    INT32 fullHeight)
{
    INT           ret         = 0;
    CamxResult    result      = CamxResultSuccess;
    // ldc out2in grid
    NcLibIcaGrid* pWarpGridIn = m_pWarpGridDataIn[Out2InGrid];

    NcLibWindowRegion unDistortedZoomWindow;
    Utils::Memset(&unDistortedZoomWindow, 0, sizeof(NcLibWindowRegion));

    unDistortedZoomWindow.windowLeft   = pCrop->left;
    unDistortedZoomWindow.windowTop    = pCrop->top;
    unDistortedZoomWindow.windowWidth  = pCrop->width;
    unDistortedZoomWindow.windowHeight = pCrop->height;
    unDistortedZoomWindow.fullWidth    = fullWidth;
    unDistortedZoomWindow.fullHeight   = fullHeight;

    NcLibWindowRegion distortedZoomWindow;
    Utils::Memset(&distortedZoomWindow, 0, sizeof(NcLibWindowRegion));

    if (NULL != pWarpGridIn)
    {
        // get the minimal distorted window for undistorted input crop using out-to-in LDC grid
        ret = NcLibCalcMinInputWindow(&unDistortedZoomWindow, pWarpGridIn, &distortedZoomWindow);
        result = (0 == ret) ? CamxResultSuccess : CamxResultEFailed;
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid grid pointer");
    }

    if (CamxResultSuccess == result)
    {
        pCrop->left   = static_cast<INT32>(distortedZoomWindow.windowLeft);
        pCrop->top    = static_cast<INT32>(distortedZoomWindow.windowTop);
        pCrop->width  = static_cast<INT32>(distortedZoomWindow.windowWidth);
        pCrop->height = static_cast<INT32>(distortedZoomWindow.windowHeight);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "IFE zoom window domain conversion to distorted failed");
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%d Input zoom window in Output domain: [%d, %d, %d, %d] %d, %d "
                     "and in Input domain: [%d,%d,%d,%d] %d, %d", InstanceID(),
                     unDistortedZoomWindow.windowLeft, unDistortedZoomWindow.windowTop,
                     unDistortedZoomWindow.windowWidth, unDistortedZoomWindow.windowHeight,
                     unDistortedZoomWindow.fullWidth, unDistortedZoomWindow.fullHeight,
                     distortedZoomWindow.windowLeft, distortedZoomWindow.windowTop,
                     distortedZoomWindow.windowWidth, distortedZoomWindow.windowHeight,
                     distortedZoomWindow.fullWidth, distortedZoomWindow.fullHeight);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ResampleAndPublishICAGridPerCropWindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ResampleAndPublishICAGridPerCropWindow(
    const ISPInputData* pInputData)
{
    CamxResult       result                  = CamxResultSuccess;
    INT32            ret                     = 0;
    BOOL             isDisplayOutPortEnabled = FALSE;
    BOOL             isFullOutPortEnabled    = FALSE;

    Titan17xContext* pContext = static_cast<Titan17xContext *>(GetHwContext());

    NcLibWindowRegion ifeZoomWindowOutDomain;
    Utils::Memset(&ifeZoomWindowOutDomain, 0x0, sizeof(NcLibWindowRegion));

    for (UINT portIndex = 0; portIndex < GetNumOutputPorts(); portIndex++)
    {
        OutputPort* pOutputPort = GetOutputPort(portIndex);

        if (IFEOutputPortDisplayFull == pOutputPort->portId)
        {
            isDisplayOutPortEnabled = TRUE;
        }
        else if (IFEOutputPortFull == pOutputPort->portId)
        {
            isFullOutPortEnabled = TRUE;
        }
    }

    for (UINT32 i = 0; i < GridMaxType; i++)
    {

        for (UINT32 j = 0; j < LDCMaxPath; j++)
        {
            if (((static_cast<UINT32>(FullPath) == j) && (TRUE == isFullOutPortEnabled)) ||
                ((static_cast<UINT32>(DisplayFullPath) == j) && (TRUE == isDisplayOutPortEnabled)))
            {
                if (((static_cast<UINT32>(Out2InGrid) == i) && (TRUE == m_ICAGridOut2InEnabled)) ||
                    ((static_cast<UINT32>(In2OutGrid) == i) && (TRUE == m_ICAGridIn2OutEnabled)))
                {
                    if (static_cast<UINT32>(In2OutGrid) == i)
                    {
                        // Calculate total applied crop zoom window in undistorted domain using in2out input ldc grid
                        // In : distorted, Out : Un-Distorted
                        ret = NcLibCalcMaxOutputWindow(&m_ifeZoomWindowInDomain[j],
                                                       &m_ifeZoomWindowInDomain[j],
                                                       m_pWarpGridDataIn[i],
                                                       &ifeZoomWindowOutDomain);
                        result = (0 == ret) ? CamxResultSuccess : CamxResultEFailed;
                    }

                    if ((static_cast<UINT32>(In2OutGrid) == i) && (CamxResultSuccess == result))
                    {
                        // get resampled output in2out grid as per current zoom window
                        ret = NcLibResampleGrid(m_pWarpGridDataIn[i],
                                                &m_ifeZoomWindowInDomain[j],
                                                (TRUE == m_ICAGridIn2OutEnabled) ?
                                                    &ifeZoomWindowOutDomain : &m_ifeZoomWindowInDomain[j],
                                                m_pWarpGridDataOut[i][j]);
                        result = (0 == ret) ? CamxResultSuccess : CamxResultEFailed;
                    }
                    else if ((static_cast<UINT32>(Out2InGrid) == i) && (CamxResultSuccess == result))
                    {
                        // get resampled output out2in grid as per current zoom window
                        ret = NcLibResampleGrid(m_pWarpGridDataIn[i],
                                                (TRUE == m_ICAGridIn2OutEnabled) ?
                                                    &ifeZoomWindowOutDomain : &m_ifeZoomWindowInDomain[j],
                                                &m_ifeZoomWindowInDomain[j],
                                                m_pWarpGridDataOut[i][j]);
                        result = (0 == ret) ? CamxResultSuccess : CamxResultEFailed;
                    }

                    if (CamxResultSuccess == result)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%d distorted domain zoom window: [%d,%d,%d,%d] %d, %d",
                                         InstanceID(),
                                         m_ifeZoomWindowInDomain[j].windowLeft, m_ifeZoomWindowInDomain[j].windowTop,
                                         m_ifeZoomWindowInDomain[j].windowWidth, m_ifeZoomWindowInDomain[j].windowHeight,
                                         m_ifeZoomWindowInDomain[j].fullWidth, m_ifeZoomWindowInDomain[j].fullHeight);

                        CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%d un-distorted zoom window: [%d,%d,%d,%d] %d, %d",
                                         InstanceID(),
                                         ifeZoomWindowOutDomain.windowLeft, ifeZoomWindowOutDomain.windowTop,
                                         ifeZoomWindowOutDomain.windowWidth, ifeZoomWindowOutDomain.windowHeight,
                                         ifeZoomWindowOutDomain.fullWidth, ifeZoomWindowOutDomain.fullHeight);

                        if (TRUE == pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->dumpICAOut)
                        {
                            DumpICAGridTransform(pInputData, m_pWarpGridDataIn[i], j, i, "In");
                            DumpICAGridTransform(pInputData, m_pWarpGridDataOut[i][j], j, i, "Out");
                        }
                        PublishICAGridTransform(m_pWarpGridDataOut[i][j], j, i);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP, "Grid in %p, out %p resampling failed, type %d, IFE LDC path %d",
                                       m_pWarpGridDataIn[i], m_pWarpGridDataOut[i][j], i, j);
                        CAMX_LOG_ERROR(CamxLogGroupISP, "IFE:%d distorted zoom window: [%d,%d,%d,%d] %d, %d", InstanceID(),
                                       m_ifeZoomWindowInDomain[j].windowLeft, m_ifeZoomWindowInDomain[j].windowTop,
                                       m_ifeZoomWindowInDomain[j].windowWidth, m_ifeZoomWindowInDomain[j].windowHeight,
                                       m_ifeZoomWindowInDomain[j].fullWidth, m_ifeZoomWindowInDomain[j].fullHeight);

                        CAMX_LOG_ERROR(CamxLogGroupISP, "IFE:%d un-distorted zoom window: [%d,%d,%d,%d] %d, %d", InstanceID(),
                            ifeZoomWindowOutDomain.windowLeft, ifeZoomWindowOutDomain.windowTop,
                            ifeZoomWindowOutDomain.windowWidth, ifeZoomWindowOutDomain.windowHeight,
                            ifeZoomWindowOutDomain.fullWidth, ifeZoomWindowOutDomain.fullHeight);

                        break;
                    }
                }
                else
                {
                    PublishICAGridTransform(NULL, j, i);
                }
            }
            if (CamxResultSuccess != result)
            {
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::PublishICAGridTransform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::PublishICAGridTransform(
    NcLibIcaGrid* pWarpGridDataOut,
    UINT32 ifeLDCPath,
    UINT32 gridType)
{
    CamxResult    result            = CamxResultSuccess;
    UINT32        publishTagId      = 0;
    UINT32        numRows           = 0;
    UINT32        numColumns        = 0;

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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid ica grid geometry");
            break;
    }

    UINT              propertiesICAGrid[1] = { 0 };
    const VOID*       pICAGridData[1]      = { 0 };
    UINT              pICAGridDataCount[1] = { 0 };

    if (NULL != pWarpGridDataOut)
    {
        if (Out2InGrid == gridType)
        {
            publishTagId = m_vendorTagArray[ICAInGridOut2InTransformIndex];
        }
        else if (In2OutGrid == gridType)
        {
            publishTagId = m_vendorTagArray[ICAInGridIn2OutTransformIndex];
        }

        IPEICAGridTransform gridTransform;
        Utils::Memset(&gridTransform, 0x0, sizeof(IPEICAGridTransform));

        gridTransform.gridTransformEnable      = 1;
        gridTransform.reuseGridTransform       = 0;
        gridTransform.transformDefinedOnWidth  = IcaVirtualDomainWidth;
        gridTransform.transformDefinedOnHeight = IcaVirtualDomainHeight;
        gridTransform.geometry                 = static_cast<ICAGridGeometry>(m_ICAGridGeometry);

        for (UINT idx = 0; idx < (numColumns * numRows); idx++)
        {
            gridTransform.gridTransformArray[idx].x = pWarpGridDataOut->grid[idx].x;
            gridTransform.gridTransformArray[idx].y = pWarpGridDataOut->grid[idx].y;
        }

        // Grid Corners are filled for ICA 1.0 only. For others they are set to zero.
        for (UINT idx = 0; idx < 4; idx++)
        {
            gridTransform.gridTransformArrayCorners[idx].x = pWarpGridDataOut->gridCorners[idx].x;
            gridTransform.gridTransformArrayCorners[idx].y = pWarpGridDataOut->gridCorners[idx].y;
        }
        gridTransform.gridTransformArrayExtrapolatedCorners =
            pWarpGridDataOut->extrapolatedCorners;

        propertiesICAGrid[0] = { publishTagId };
        pICAGridData[0]      = &gridTransform;
        pICAGridDataCount[0] = sizeof(IPEICAGridTransform);

        if (FullPath == ifeLDCPath)
        {
            WritePSDataList(IFEOutputPortFull, propertiesICAGrid, pICAGridData, pICAGridDataCount, 1);
        }
        else if (DisplayFullPath == ifeLDCPath)
        {
            WritePSDataList(IFEOutputPortDisplayFull, propertiesICAGrid, pICAGridData, pICAGridDataCount, 1);
        }

        PublishPSData(publishTagId, NULL);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "%s: published ica grid data, tagId %d, path %d",
                         NodeIdentifierString(), publishTagId, ifeLDCPath);
    }
    else
    {
        if (Out2InGrid == gridType)
        {
            publishTagId = m_vendorTagArray[ICAInGridOut2InTransformIndex];
        }
        else if (In2OutGrid == gridType)
        {
            publishTagId = m_vendorTagArray[ICAInGridIn2OutTransformIndex];
        }

        IPEICAGridTransform gridTransform;
        Utils::Memset(&gridTransform, 0x0, sizeof(IPEICAGridTransform));
        propertiesICAGrid[0] = { publishTagId };
        pICAGridData[0] = &gridTransform;
        pICAGridDataCount[0] = sizeof(IPEICAGridTransform);

        if (FullPath == ifeLDCPath)
        {
            WritePSDataList(IFEOutputPortFull, propertiesICAGrid, pICAGridData, pICAGridDataCount, 1);
        }
        else if (DisplayFullPath == ifeLDCPath)
        {
            WritePSDataList(IFEOutputPortDisplayFull, propertiesICAGrid, pICAGridData, pICAGridDataCount, 1);
        }
        PublishPSData(publishTagId, NULL);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "%s: published ica grid disabled, tagId %d, path %d",
                         NodeIdentifierString(), publishTagId, ifeLDCPath);

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::FillLDCIFEOutZoomwindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::FillLDCIFEOutZoomwindow(
    const IFECropInfo* pAppliedCropInfo,
    const IFECropInfo* pResidualCropInfo,
    UINT32 fullWidth,
    UINT32 fullHeight)
{
    BOOL isDisplayOutPortEnabled = FALSE;
    BOOL isFullOutPortEnabled    = FALSE;

    for (UINT portIndex = 0; portIndex < GetNumOutputPorts(); portIndex++)
    {
        OutputPort* pOutputPort = GetOutputPort(portIndex);

        if (IFEOutputPortDisplayFull == pOutputPort->portId)
        {
            isDisplayOutPortEnabled = TRUE;
        }
        else if (IFEOutputPortFull == pOutputPort->portId)
        {
            isFullOutPortEnabled = TRUE;
        }
    }

    if (0 == m_ICAGridDomain)
    {
        // get total crop (CSID + sensor analog + sensor digital) done till IFE input wrt sensor
        WindowRegion sensorAppliedCropZW;

        sensorAppliedCropZW.windowLeft   = m_pSensorModeData->activeArrayCropWindow.left;
        sensorAppliedCropZW.windowTop    = m_pSensorModeData->activeArrayCropWindow.top;
        sensorAppliedCropZW.windowWidth  = m_pSensorModeData->activeArrayCropWindow.width;
        sensorAppliedCropZW.windowHeight = m_pSensorModeData->activeArrayCropWindow.height;
        sensorAppliedCropZW.fullWidth    = m_sensorActiveArrayWidth;
        sensorAppliedCropZW.fullHeight   = m_sensorActiveArrayHeight;

        if (TRUE == isFullOutPortEnabled)
        {
            WindowRegion ifeAppliedFullPathCropZW;

            ifeAppliedFullPathCropZW.windowLeft     = pAppliedCropInfo->fullPath.left;
            ifeAppliedFullPathCropZW.windowTop      = pAppliedCropInfo->fullPath.top;
            ifeAppliedFullPathCropZW.windowWidth    = pAppliedCropInfo->fullPath.width;
            ifeAppliedFullPathCropZW.windowHeight   = pAppliedCropInfo->fullPath.height;
            ifeAppliedFullPathCropZW.fullWidth      = fullWidth;
            ifeAppliedFullPathCropZW.fullHeight     = fullHeight;

            CalculateLDCTotalDistortedDomainCropZW(
                &sensorAppliedCropZW, &ifeAppliedFullPathCropZW, &m_ifeZoomWindowInDomain[FullPath]);
        }

        if (TRUE == isDisplayOutPortEnabled)
        {
            WindowRegion ifeAppliedDisplayPathCropZW;

            ifeAppliedDisplayPathCropZW.windowLeft     = pAppliedCropInfo->displayFullPath.left;
            ifeAppliedDisplayPathCropZW.windowTop      = pAppliedCropInfo->displayFullPath.top;
            ifeAppliedDisplayPathCropZW.windowWidth    = pAppliedCropInfo->displayFullPath.width;
            ifeAppliedDisplayPathCropZW.windowHeight   = pAppliedCropInfo->displayFullPath.height;
            ifeAppliedDisplayPathCropZW.fullWidth      = fullWidth;
            ifeAppliedDisplayPathCropZW.fullHeight     = fullHeight;

            CalculateLDCTotalDistortedDomainCropZW(
                &sensorAppliedCropZW, &ifeAppliedDisplayPathCropZW, &m_ifeZoomWindowInDomain[DisplayFullPath]);
        }
    }
    else if (1 == m_ICAGridDomain)
    {
        if (NULL != pAppliedCropInfo)
        {
            if (TRUE == isFullOutPortEnabled)
            {
                // Update the IFE crop for full path for LDC grid resampling (domain 1) with respect to IFE input
                m_ifeZoomWindowInDomain[FullPath].windowLeft    = pAppliedCropInfo->fullPath.left;
                m_ifeZoomWindowInDomain[FullPath].windowTop     = pAppliedCropInfo->fullPath.top;
                m_ifeZoomWindowInDomain[FullPath].windowWidth   = pAppliedCropInfo->fullPath.width;
                m_ifeZoomWindowInDomain[FullPath].windowHeight  = pAppliedCropInfo->fullPath.height;
                m_ifeZoomWindowInDomain[FullPath].fullWidth     = fullWidth;
                m_ifeZoomWindowInDomain[FullPath].fullHeight    = fullHeight;
            }

            if (TRUE == isDisplayOutPortEnabled)
            {
                // Update the IFE crop for display full path for LDC grid resampling (domain 1) with respect to IFE input
                m_ifeZoomWindowInDomain[DisplayFullPath].windowLeft     = pAppliedCropInfo->displayFullPath.left;
                m_ifeZoomWindowInDomain[DisplayFullPath].windowTop      = pAppliedCropInfo->displayFullPath.top;
                m_ifeZoomWindowInDomain[DisplayFullPath].windowWidth    = pAppliedCropInfo->displayFullPath.width;
                m_ifeZoomWindowInDomain[DisplayFullPath].windowHeight   = pAppliedCropInfo->displayFullPath.height;
                m_ifeZoomWindowInDomain[DisplayFullPath].fullWidth      = fullWidth;
                m_ifeZoomWindowInDomain[DisplayFullPath].fullHeight     = fullHeight;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid IFE applied crop");
        }
    }
    else if (2 == m_ICAGridDomain)
    {
        if (NULL != pResidualCropInfo)
        {
            if (TRUE == isFullOutPortEnabled)
            {
                // Update the crop (Residual )for display full path for LDC (grid domain 2) with respect to IFE output
                m_ifeZoomWindowInDomain[FullPath].windowLeft    = pResidualCropInfo->fullPath.left;
                m_ifeZoomWindowInDomain[FullPath].windowTop     = pResidualCropInfo->fullPath.top;
                m_ifeZoomWindowInDomain[FullPath].windowWidth   = pResidualCropInfo->fullPath.width;
                m_ifeZoomWindowInDomain[FullPath].windowHeight  = pResidualCropInfo->fullPath.height;
                m_ifeZoomWindowInDomain[FullPath].fullWidth     = m_ifeOutputImageSize.widthPixels;
                m_ifeZoomWindowInDomain[FullPath].fullHeight    = m_ifeOutputImageSize.heightLines;
            }

            if (TRUE == isDisplayOutPortEnabled)
            {
                // Update the crop (Residual )for display full path for LDC (grid domain 2) with respect to IFE output
                m_ifeZoomWindowInDomain[DisplayFullPath].windowLeft     = pResidualCropInfo->displayFullPath.left;
                m_ifeZoomWindowInDomain[DisplayFullPath].windowTop      = pResidualCropInfo->displayFullPath.top;
                m_ifeZoomWindowInDomain[DisplayFullPath].windowWidth    = pResidualCropInfo->displayFullPath.width;
                m_ifeZoomWindowInDomain[DisplayFullPath].windowHeight   = pResidualCropInfo->displayFullPath.height;
                m_ifeZoomWindowInDomain[DisplayFullPath].fullWidth      = m_ifeOutputImageSize.widthPixels;
                m_ifeZoomWindowInDomain[DisplayFullPath].fullHeight     = m_ifeOutputImageSize.heightLines;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid IFE residual crop");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::CalculateLDCTotalDistortedDomainCropZW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::CalculateLDCTotalDistortedDomainCropZW(
    const WindowRegion* pSensorAppliedCropInfoZW,
    const WindowRegion* pIFEAppliedCropInfoZW,
    NcLibWindowRegion* pDistortedDomainZoomWindow)
{
    if ((0 != pSensorAppliedCropInfoZW->windowWidth)  &&
        (0 != pSensorAppliedCropInfoZW->windowHeight) &&
        (0 != pSensorAppliedCropInfoZW->fullWidth)    &&
        (0 != pSensorAppliedCropInfoZW->fullHeight)   &&
        (0 != pIFEAppliedCropInfoZW->windowWidth)     &&
        (0 != pIFEAppliedCropInfoZW->windowHeight)    &&
        (0 != pIFEAppliedCropInfoZW->fullWidth)       &&
        (0 != pIFEAppliedCropInfoZW->fullHeight)      &&
        (pSensorAppliedCropInfoZW->fullWidth >= pIFEAppliedCropInfoZW->fullWidth) &&
        (pSensorAppliedCropInfoZW->fullHeight >= pIFEAppliedCropInfoZW->fullHeight))
    {
        // sensor applied crop is defined wrt sensor size, need to find total crop
        // happened wrt sensor size as grid is also defined wrt sensor size
        pDistortedDomainZoomWindow->fullWidth  = pSensorAppliedCropInfoZW->fullWidth;
        pDistortedDomainZoomWindow->fullHeight = pSensorAppliedCropInfoZW->fullHeight;

        // mapping sensor total crop to adjust IFE applied crop window width
        pDistortedDomainZoomWindow->windowWidth =
            (static_cast<FLOAT>(pIFEAppliedCropInfoZW->windowWidth) / pIFEAppliedCropInfoZW->fullWidth) *
            pSensorAppliedCropInfoZW->windowWidth;

        // mapping sensor total crop to adjust IFE applied crop window height
        pDistortedDomainZoomWindow->windowHeight =
            (static_cast<FLOAT>(pIFEAppliedCropInfoZW->windowHeight) / pIFEAppliedCropInfoZW->fullHeight) *
            pSensorAppliedCropInfoZW->windowHeight;

        // finding distorted domain window left and top which wrt to sensor size by adding sensor crop left and top
        // to the amount of IFE applied crop window left and top wrt sensor crop window
        pDistortedDomainZoomWindow->windowLeft =
            pSensorAppliedCropInfoZW->windowLeft +
            ((static_cast<FLOAT>(pIFEAppliedCropInfoZW->windowLeft) / pIFEAppliedCropInfoZW->fullWidth) *
            pSensorAppliedCropInfoZW->windowWidth);

        pDistortedDomainZoomWindow->windowTop =
            pSensorAppliedCropInfoZW->windowTop +
            ((static_cast<FLOAT>(pIFEAppliedCropInfoZW->windowTop) / pIFEAppliedCropInfoZW->fullHeight) *
            pSensorAppliedCropInfoZW->windowHeight);

        if (pDistortedDomainZoomWindow->windowWidth !=
            (pDistortedDomainZoomWindow->fullWidth - (pDistortedDomainZoomWindow->windowLeft * 2)))
        {
            pDistortedDomainZoomWindow->windowWidth =
                (pDistortedDomainZoomWindow->fullWidth - (pDistortedDomainZoomWindow->windowLeft * 2));
        }

        if (pDistortedDomainZoomWindow->windowHeight !=
            (pDistortedDomainZoomWindow->fullHeight - (pDistortedDomainZoomWindow->windowTop * 2)))
        {
            pDistortedDomainZoomWindow->windowHeight =
                (pDistortedDomainZoomWindow->fullHeight - (pDistortedDomainZoomWindow->windowTop * 2));
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid zoom window, SensorAppliedCropZW [%d, %d, %d, %d], %d, %d and "
                       "IFEAppliedCropZW [%d, %d, %d, %d], %d, %d",
                       pSensorAppliedCropInfoZW->windowLeft,
                       pSensorAppliedCropInfoZW->windowTop,
                       pSensorAppliedCropInfoZW->windowWidth,
                       pSensorAppliedCropInfoZW->windowHeight,
                       pSensorAppliedCropInfoZW->fullWidth,
                       pSensorAppliedCropInfoZW->fullHeight,
                       pIFEAppliedCropInfoZW->windowLeft,
                       pIFEAppliedCropInfoZW->windowTop,
                       pIFEAppliedCropInfoZW->windowWidth,
                       pIFEAppliedCropInfoZW->windowHeight,
                       pIFEAppliedCropInfoZW->fullWidth,
                       pIFEAppliedCropInfoZW->fullHeight);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::DumpICAGridTransform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::DumpICAGridTransform(
    const ISPInputData* pInput,
    NcLibIcaGrid* pWarpGridData,
    UINT32 ifeLDCPath,
    UINT32 gridType,
    const CHAR* pGridDomain)
{
    CHAR          dumpFilename[256];
    FILE*         pFile           = NULL;

    OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
        "%s/ife_ldc_path_%u_grid%s_type_%u_reqId_%llu_instance_%d.txt",
        ConfigFileDirectory, ifeLDCPath, pGridDomain, gridType, pInput->frameNum, InstanceID());
    pFile = OsUtils::FOpen(dumpFilename, "w");

    if (NULL != pFile)
    {
        IQInterface::DumpICAGrid(pFile, pWarpGridData);
        OsUtils::FClose(pFile);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::ResubmitInitPacket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFENode::ResubmitInitPacket(
    const UINT64 requestId)
{
    static const UINT64 RequestIdForInitPacket = 0;
    PacketResource*     pPacketResource        = NULL;

    CamxResult result = m_pIQPacketManager->CheckBufferWithRequest(RequestIdForInitPacket, &pPacketResource);

    if (CamxResultSuccess == result)
    {
        result = GetHwContext()->Submit(GetCSLSession(), m_hDevice, static_cast<Packet*>(pPacketResource));
        CAMX_LOG_CONFIG(CamxLogGroupISP, "Node::%s Resend the init packet, requestId = %llu",
                        NodeIdentifierString(),
                        requestId);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Node::%s CSL Submit Failed: %s, requestId = %llu",
                           NodeIdentifierString(),
                           Utils::CamxResultToString(result),
                           requestId);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Node::%s Failed to get the init packet, requestId = %llu",
                       NodeIdentifierString(),
                       requestId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetMetadataDarkBoostOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::GetMetadataDarkBoostOffset(
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
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%u Can't query vendor tag: isValidDarkBoostOffset", InstanceID());
    }

    //  Tag: darkBoostOffset
    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.tmcusercontrol",
                                                          "darkBoostOffset",
                                                          &tagDarkBoostOffset);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%u Can't query vendor tag: darkBoostOffset", InstanceID());
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
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%u Failed to get TMC user control DarkBoostOffset", InstanceID());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFENode::GetMetadataFourthToneAnchor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFENode::GetMetadataFourthToneAnchor(
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
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%u Can't query vendor tag: isValidFourthToneAnchor", InstanceID());
    }

    if (CamxResultSuccess == result)
    {
        //  Tag: fourthToneAnchor
        result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.tmcusercontrol",
                                                          "fourthToneAnchor",
                                                          &tagFourthToneAnchor);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%u Can't query vendor tag: fourthToneAnchor", InstanceID());
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
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "IFE:%u Failed to get TMC user control FourthToneAnchor", InstanceID());
    }
}

CAMX_NAMESPACE_END
