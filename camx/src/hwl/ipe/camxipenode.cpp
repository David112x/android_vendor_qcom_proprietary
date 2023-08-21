////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipenode.cpp
/// @brief IPE Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if (!defined(LE_CAMERA)) // ANDROID
#include "qdMetaData.h"
#endif // ANDROID
#include "camxtrace.h"
#include "camxpipeline.h"
#include "camxcdmdefs.h"
#include "camxcslicpdefs.h"
#include "camxcslispdefs.h"
#include "camxcslresourcedefs.h"
#include "camxhal3metadatautil.h"
#include "camxhal3module.h"
#include "camxhwcontext.h"
#include "camximagebuffer.h"
#include "camximageformatutils.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxtitan17xcontext.h"
#include "camxtitan17xdefs.h"
#include "camxipenode.h"
#include "camxipepipelinetitan150.h"
#include "camxipepipelinetitan160.h"
#include "camxipepipelinetitan170.h"
#include "camxipepipelinetitan175.h"
#include "camxipepipelinetitan480.h"
#include "camxiqinterface.h"
#include "ipdefs.h"
#include "titan170_base.h"
#include "parametertuningtypes.h"
#include "camxtranslator.h"
#include "camxipeicatestdata.h"
#include "ipeStripingLib.h"
#include "camxpacketdefs.h"
#include "NcLibWarp.h"
#include "camxnodeutils.h"
#include "GeoLibUtils.h"

// This function needs to be outside the CAMX_NAMESPACE because firmware uses "ImageFormat" that is used in UMD as well
ImageFormat TranslateFormatToFirmwareImageFormat(
    CamX::Format format)
{
    ImageFormat firmwareFormat = IMAGE_FORMAT_INVALID;

    switch (format)
    {
        case CamX::Format::YUV420NV12:
        case CamX::Format::YUV420NV21:
            firmwareFormat = IMAGE_FORMAT_LINEAR_NV12;
            break;
        case CamX::Format::UBWCTP10:
            firmwareFormat = IMAGE_FORMAT_UBWC_TP_10;
            break;
        case CamX::Format::UBWCNV12:
        case CamX::Format::UBWCNV12FLEX:
            firmwareFormat = IMAGE_FORMAT_UBWC_NV_12;
            break;
        case CamX::Format::UBWCNV124R:
            firmwareFormat = IMAGE_FORMAT_UBWC_NV12_4R;
            break;
        case CamX::Format::UBWCP010:
            firmwareFormat = IMAGE_FORMAT_UBWC_P010;
            break;
        case CamX::Format::YUV420NV12TP10:
        case CamX::Format::YUV420NV21TP10:
            firmwareFormat = IMAGE_FORMAT_LINEAR_TP_10;
            break;
        case CamX::Format::PD10:
            firmwareFormat = IMAGE_FORMAT_PD_10;
            break;
        case CamX::Format::P010:
            firmwareFormat = IMAGE_FORMAT_LINEAR_P010;
            break;
        case CamX::Format::Blob:
            firmwareFormat = IMAGE_FORMAT_INDICATIONS;
            break;
        default:
            firmwareFormat = IMAGE_FORMAT_INVALID;
            break;
    }

    return firmwareFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IsValidConfigIOData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IsValidConfigIOData(
    ImageFormat      firmwareFormat,
    IPE_IO_IMAGES    firmwarePortId,
    UINT32           titanVersion)
{
    BOOL bValidConfig = TRUE;

    switch (firmwarePortId)
    {
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_FULL:
        case IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_DISPLAY:
        case IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_VIDEO:
            if ((IMAGE_FORMAT_LINEAR_NV12  == firmwareFormat) ||
                (IMAGE_FORMAT_UBWC_NV_12   == firmwareFormat) ||
                (IMAGE_FORMAT_UBWC_TP_10   == firmwareFormat) ||
                (IMAGE_FORMAT_LINEAR_TP_10 == firmwareFormat) ||
                (IMAGE_FORMAT_LINEAR_P010  == firmwareFormat) ||
                (IMAGE_FORMAT_UBWC_NV12_4R == firmwareFormat) ||
                (IMAGE_FORMAT_PD_10        == firmwareFormat))
            {
                bValidConfig = TRUE;
            }
            else if (IMAGE_FORMAT_UBWC_P010 == firmwareFormat)
            {
                bValidConfig = (IPE_IO_IMAGES::IPE_INPUT_IMAGE_FULL == firmwarePortId) ? TRUE : FALSE;
            }
            else
            {
                bValidConfig = FALSE;
            }

            break;
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS4:
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS16:
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS64:
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS4_REF:
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS16_REF:
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS64_REF:
        case IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_DS4_REF:
        case IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_DS16_REF:
        case IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_DS64_REF:
            bValidConfig = ((IMAGE_FORMAT_PD_10 == firmwareFormat) ||
                            (IMAGE_FORMAT_LINEAR_P010 == firmwareFormat)) ? TRUE : FALSE;

            // Bitra DSx Port support Linear NV12 format as well.
            if ( (CSLCameraTitanVersion::CSLTitan170 == titanVersion) && (IMAGE_FORMAT_LINEAR_NV12 == firmwareFormat))
            {
                bValidConfig = TRUE;
            }

            break;
        case IPE_IO_IMAGES::IPE_INPUT_OUTPUT_SCRATCHBUFFER:
            bValidConfig = (IMAGE_FORMAT_INDICATIONS == firmwareFormat) ? TRUE : FALSE;
            break;
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_FULL_REF:
        case IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_FULL_REF:
            if ((IMAGE_FORMAT_LINEAR_NV12  == firmwareFormat) ||
                (IMAGE_FORMAT_UBWC_TP_10   == firmwareFormat) ||
                (IMAGE_FORMAT_LINEAR_TP_10 == firmwareFormat) ||
                (IMAGE_FORMAT_LINEAR_P010  == firmwareFormat) ||
                (IMAGE_FORMAT_UBWC_NV12_4R == firmwareFormat) ||
                (IMAGE_FORMAT_PD_10        == firmwareFormat))
            {
                bValidConfig = TRUE;
            }
            else
            {
                bValidConfig = FALSE;
            }

            break;

        default:
            bValidConfig = FALSE;
            break;
    }

    return bValidConfig;
}

CAMX_NAMESPACE_BEGIN

static const UINT IPEMaxInput                    = 8;    ///< Number of Input Ports : 4 Input for image buffers and 4 for ref
static const UINT IPEMaxOutput                   = 7;    ///< Number of Output Ports: Display,  Video plus 4 ref output ports
static const UINT IPEMaxTopCmdBufferPatchAddress = 546;  ///< Number of Max address patching for top level payload
static const UINT IPEMaxPreLTMPatchAddress       = 17;   ///< Number of Max address patching for preLTM IQ modules
static const UINT IPEMaxPostLTMPatchAddress      = 16;   ///< Number of Max address patching for postLTM IQ modules
static const UINT IPEMaxDMIPatchAddress          = 53;   ///< Number of Max address patching for DMI headers
static const UINT IPEMaxNPSPatchAddress          = 8;    ///< Number of Max address patching for NPS (ANR/TF)
static const UINT IPEMaxPatchAddress             = IPEMaxTopCmdBufferPatchAddress +
                                                   IPEMaxPreLTMPatchAddress       +
                                                   IPEMaxPostLTMPatchAddress      +
                                                   IPEMaxNPSPatchAddress;         ///< Max address patches for packet;
static const UINT IPEDefaultDownScalarMode       = 1;
static const UINT IPEMidDownScalarMode           = 2;
static const UINT IPECustomDownScalarMode        = 3;
static const UINT IPEMidDownScalarWidth          = 4928;
static const UINT IPEMidDownScalarHeight         = 3808;

static const FLOAT IPEDownscaleThresholdMin      = 1.0f;  ///< Min scale ratio above which downscaling causes IQ issues
static const FLOAT IPEDownscaleThresholdMax      = 1.05f; ///< Max scale ratio below which downscaling causes IQ issues

static const FLOAT IPEUpscaleThresholdMin        = 0.95f; ///< Min scale ratio above which upscaling causes IQ issues
static const FLOAT IPEUpscaleThresholdMax        = 1.0f;  ///< Max scale ratio below which upscaling causes IQ issues

static const UINT IPEMaxFWCmdBufferManagers      = 10;  ///< Max FW Command Buffer Managers

// Alignment fusion defaults
static const UINT IPEAlignmentFusionImgConfHighThreshold    = 150;
static const UINT IPEAlignmentFusionImgConfLowThreshold     = 100;
static const UINT IPEAlignmentFusionMinAge                  = 10;
static const UINT IPEAlignmentFusionAbsMotionThreshold      = 1;
static const UINT IPEAlignmentFusionAbsMotionConfThreshold  = 100;



/// @brief Information about android to native effect.
struct IPEAndroidToCamxEffect
{
    ModeEffectSubModeType   from;           ///< Effect mode value
    ControlEffectModeValues to;             ///< Control effect Mode Value
};

/// @brief Information about android to native effect.
struct IPEAndroidToCamxScene
{
    ModeSceneSubModeType   from;            ///< Scene mode value
    ControlSceneModeValues to;              ///< Control scene mode value
};

// Map effects from CamX to Android
static IPEAndroidToCamxEffect IPEEffectMap[] =
{
    { ModeEffectSubModeType::None,       ControlEffectModeOff },
    { ModeEffectSubModeType::Mono,       ControlEffectModeMono },
    { ModeEffectSubModeType::Sepia,      ControlEffectModeSepia },
    { ModeEffectSubModeType::Negative,   ControlEffectModeNegative },
    { ModeEffectSubModeType::Solarize,   ControlEffectModeSolarize },
    { ModeEffectSubModeType::Posterize,  ControlEffectModePosterize },
    { ModeEffectSubModeType::Aqua,       ControlEffectModeAqua },
    { ModeEffectSubModeType::Emboss,     ControlEffectModeOff },
    { ModeEffectSubModeType::Sketch,     ControlEffectModeOff },
    { ModeEffectSubModeType::Neon,       ControlEffectModeOff },
    { ModeEffectSubModeType::Blackboard, ControlEffectModeBlackboard },
    { ModeEffectSubModeType::Whiteboard, ControlEffectModeWhiteboard },
};

// Map scene modes from CamX to Android
static IPEAndroidToCamxScene IPESceneMap[] =
{
    { ModeSceneSubModeType::None,          ControlSceneModeDisabled },
    { ModeSceneSubModeType::Landscape,     ControlSceneModeLandscape },
    { ModeSceneSubModeType::Snow,          ControlSceneModeSnow },
    { ModeSceneSubModeType::Beach,         ControlSceneModeBeach },
    { ModeSceneSubModeType::Sunset,        ControlSceneModeSunset },
    { ModeSceneSubModeType::Night,         ControlSceneModeNight },
    { ModeSceneSubModeType::Portrait,      ControlSceneModePortrait },
    { ModeSceneSubModeType::BackLight,     ControlSceneModeDisabled },
    { ModeSceneSubModeType::Sports,        ControlSceneModeSports },
    { ModeSceneSubModeType::AntiShake,     ControlSceneModeDisabled },
    { ModeSceneSubModeType::Flowers,       ControlSceneModeDisabled },
    { ModeSceneSubModeType::CandleLight,   ControlSceneModeCandlelight },
    { ModeSceneSubModeType::Fireworks,     ControlSceneModeFireworks },
    { ModeSceneSubModeType::Party,         ControlSceneModeParty },
    { ModeSceneSubModeType::NightPortrait, ControlSceneModeNightPortrait },
    { ModeSceneSubModeType::Theater,       ControlSceneModeTheatre },
    { ModeSceneSubModeType::Action,        ControlSceneModeAction },
    { ModeSceneSubModeType::AR,            ControlSceneModeDisabled },
    { ModeSceneSubModeType::FacePriority,  ControlSceneModeFacePriority },
    { ModeSceneSubModeType::Barcode,       ControlSceneModeBarcode },
    { ModeSceneSubModeType::BestShot,      ControlSceneModeDisabled },
};

static const UINT CmdBufferFrameProcessSizeBytes = sizeof(IpeFrameProcess) +                    ///< firmware requires different
    (static_cast<UINT>(CDMProgramArrayOrder::ProgramArrayMax) * sizeof(CDMProgramArray)) +      ///< CDM programs in payload
    (static_cast<UINT>(PreLTMCDMProgramOrder::ProgramIndexMaxPreLTM) +                          ///< are appended to Frame
     static_cast<UINT>(PostLTMCDMProgramOrder::ProgramIndexMaxPostLTM) * sizeof(CdmProgram));   ///< process data

// Private Static member holding fixed values of Frame buffer offsets within IpeFrameProcess struct, for ease of patching
FrameBuffers IPENode::s_frameBufferOffset[IPEMaxSupportedBatchSize][IPE_IO_IMAGES_MAX];

static const UINT CmdBufferGenericBlobSizeInBytes = (
    CSLGenericBlobHeaderSizeInDwords * sizeof(UINT32)             +
    sizeof(CSLICPClockBandwidthRequest)                           +
    (sizeof(CSLICPClockBandwidthRequestV2)                        +
    (sizeof(CSLAXIperPathBWVote) * (CSLAXIPathDataIPEMaxNum -1))) +
    CSLGenericBlobHeaderSizeInDwords * sizeof(UINT32)             +
    sizeof(ConfigIORequest)                                       +
    CSLGenericBlobHeaderSizeInDwords * sizeof(UINT32)             +
    sizeof(CSLICPMemoryMapUpdate));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::IPENode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPENode::IPENode()
{
    m_pNodeName                 = "IPE";
    m_OEMStatsSettingEnable     = GetStaticSettings()->IsOEMStatSettingEnable;
    m_enableIPEHangDump         = GetStaticSettings()->enableIPEHangDump;
    m_enableIPEStripeDump       = GetStaticSettings()->enableIPEStripeDump;
    m_enableIPEIQModulesMask    = ((GetStaticSettings()->enableIPEIQModulesMask) | IPEMustEnabledIQModulesMask);
    m_pPreTuningDataManager     = NULL;
    m_hStripingLib              = NULL;
    m_compressiononOutput       = FALSE;
    m_inputPortIPEPassesMask    = 0;
    m_numPasses                 = 0;
    m_scratchBufferPortEnabled  = FALSE;
    m_configIOMem               = { 0 };
    m_deviceResourceRequest     = { 0 };
    m_curIntermediateDimension  = { 0, 0, 1.0f };
    m_prevIntermediateDimension = { 0, 0, 1.0f };
    m_maxICAUpscaleLimit        = 2.0f; // by default is 2.0f
    m_stabilizationMargin       = { 0 };
    m_EISMarginRequest          = { 0 };
    m_additionalCropOffset      = { 0 };
    m_referenceBufferCount      = ReferenceBufferCount;
    m_camIdChanged              = TRUE;
    m_currentAlignmentType      = NCLIB_NO_ALIGNMENT;
    m_currentAlignmentAge       = 0;
    m_createdDownscalebuffers   = FALSE;
    m_fdDataOffset              = 0;
    BOOL      enableRefDump     = FALSE;
    UINT32    enabledNodeMask   = GetStaticSettings()->autoImageDumpMask;
    UINT32    refOutputPortMask = 0x0;

    for (UINT pass = PASS_NAME_FULL; pass < PASS_NAME_MAX; pass++)
    {
        //  Intialize loop back portIDs
        refOutputPortMask |= (1 << (pass + IPE_OUTPUT_IMAGE_FULL_REF));
    }

    if (TRUE == IsIPEReferenceDumpEnabled(enabledNodeMask, refOutputPortMask))
    {
        // When Reference output dump buffer count is 2; and UMD is trying to
        // dump an IPE Reference output buffer for given request x then there is
        // a chance that request x+2 might be updating the same bufer concurrently
        // and thus creating a race condition. So, to avoid this scenario increase
        // Reference output buffer count to more than 2 buffers, say 4.
        m_referenceBufferCount = MaxReferenceBufferCount;
    }

    for (UINT batchIndex = 0; batchIndex < IPEMaxSupportedBatchSize; batchIndex++)
    {
        for (UINT port = 0; port < IPE_IO_IMAGES_MAX; port++)
        {
            for (UINT plane = 0; plane < MAX_NUM_OF_IMAGE_PLANES; plane++)
            {
                // Precompute the frame buffer offset for all ports
                s_frameBufferOffset[batchIndex][port].bufferPtr[plane] =
                    static_cast<UINT32>(offsetof(IpeFrameProcessData, frameSets[0]) + sizeof(FrameSet) * batchIndex) +
                    static_cast<UINT32>(offsetof(FrameSet, buffers[0]) + sizeof(FrameBuffers) * port) +
                    static_cast<UINT32>(offsetof(FrameBuffers, bufferPtr[0]) + (sizeof(FrameBufferPtr) * plane));

                s_frameBufferOffset[batchIndex][port].metadataBufferPtr[plane] =
                    static_cast<UINT32>(offsetof(IpeFrameProcessData, frameSets[0]) + sizeof(FrameSet) * batchIndex) +
                    static_cast<UINT32>(offsetof(FrameSet, buffers[0]) + sizeof(FrameBuffers) * port) +
                    static_cast<UINT32>(offsetof(FrameBuffers, metadataBufferPtr[0]) + (sizeof(FrameBufferPtr) * plane));
            }
        }
    }

    Utils::Memset(&m_loopBackBufferParams[0], 0x0, sizeof(m_loopBackBufferParams));

    InitializeGeoLibData();

    m_dumpGeolibResult = FALSE;

    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICAInPerspectiveTransform",
                                                                              &m_IPEICATAGLocation[0]));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICAInGridOut2InTransform",
                                                                              &m_IPEICATAGLocation[1]));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICAInGridIn2OutTransform",
                                                                              &m_IPEICATAGLocation[2]));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICAInInterpolationParams",
                                                                              &m_IPEICATAGLocation[3]));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICARefPerspectiveTransform",
                                                                              &m_IPEICATAGLocation[4]));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICARefGridTransform",
                                                                              &m_IPEICATAGLocation[5]));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICARefInterpolationParams",
                                                                              &m_IPEICATAGLocation[6]));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICAReferenceParams",
                                                                              &m_IPEICATAGLocation[7]));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICAReferenceParamsLookAhead",
                                                                              &m_IPEICATAGLocation[8]));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.mfnrconfigs",
                                                                              "MFNRTotalNumFrames",
                                                                              &m_MFNRTotalNumFramesTAGLocation));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.mfnrconfigs",
                                                                              "MFNRBlendFrameNum",
                                                                              &m_MFNRBlendFrameNumTAGLocation));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.mfsrconfigs",
                                                                              "MFSRBlendFrameNum",
                                                                              &m_MFSRBlendFrameNumTAGLocation));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.mfsrconfigs",
                                                                              "MFSRTotalNumFrames",
                                                                              &m_MFSRTotalNumFramesTAGLocation));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera.geoLibStillFrameConfig",
                                                                              "GeoLibStillFrameConfigPrefilter",
                                                                              &m_stillFrameConfigurationTagIdPrefilter));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera.geoLibStillFrameConfig",
                                                                              "GeoLibStillFrameConfigBlending",
                                                                              &m_stillFrameConfigurationTagIdBlending));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera.geoLibStillFrameConfig",
                                                                              "GeoLibStillFrameConfigPostFiler",
                                                                              &m_stillFrameConfigurationTagIdPostfilter));
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera.scratchbufferdata",
                                                                              "ScratchBufferData",
                                                                              &m_scratchBufferDataLocation));

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::~IPENode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPENode::~IPENode()
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

        ReleaseDevice();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DumpPayload
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult DumpPayload(
    IPECmdBufferId  index,
    CmdBuffer*      pIPECmdBuffer,
    UINT64          requestId)
{
    CamxResult result = CamxResultSuccess;
    CHAR       filename[100];

    /// @todo (CAMX-2491) Remove this way of getting settings
    if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->dumpIPEFirmwarePayload)
    {
        switch (index)
        {
            case CmdBufferFrameProcess:
                CamX::OsUtils::SNPrintF(filename, sizeof(filename), "%s/IPEDumpFrameProcessData_%lld.txt",
                                        ConfigFileDirectory, requestId);
                break;
            case CmdBufferStriping:
                CamX::OsUtils::SNPrintF(filename, sizeof(filename), "%s/IPEDumpStripingOutput_%lld.txt",
                                        ConfigFileDirectory, requestId);
                break;
            case CmdBufferIQSettings:
                CamX::OsUtils::SNPrintF(filename, sizeof(filename), "%s/IPEDumpIQSettings_%lld.txt",
                                        ConfigFileDirectory, requestId);
                break;
            case CmdBufferPreLTM:
                CamX::OsUtils::SNPrintF(filename, sizeof(filename), "%s/IPEDumpPreLTM_%lld.txt",
                                        ConfigFileDirectory, requestId);
                break;
            case CmdBufferPostLTM:
                CamX::OsUtils::SNPrintF(filename, sizeof(filename), "%s/IPEDumpPostLTM_%lld.txt",
                                        ConfigFileDirectory, requestId);
                break;
            case CmdBufferDMIHeader:
                CamX::OsUtils::SNPrintF(filename, sizeof(filename), "%s/IPEDumpDMIHeader_%lld.txt",
                                        ConfigFileDirectory, requestId);
                break;
            case CmdBufferNPS:
                CamX::OsUtils::SNPrintF(filename, sizeof(filename), "%s/IPEDumpNPS_%lld.txt",
                                        ConfigFileDirectory, requestId);
                break;
            default:
                result = CamxResultEInvalidArg;
                break;
        }

        if (CamxResultSuccess == result)
        {
            FILE* pFile = CamX::OsUtils::FOpen(filename, "wb");
            if (NULL != pFile)
            {
                CamX::OsUtils::FWrite(pIPECmdBuffer->GetHostAddr(), pIPECmdBuffer->GetMaxLength(), 1, pFile);
                CamX::OsUtils::FClose(pFile);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DumpDebug
///
/// @brief: This is called when firmware signal an error and UMD needs firmware dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult DumpDebug(
    IPECmdBufferId          index,
    CmdBuffer*              pBuffer,
    UINT64                  requestId,
    UINT32                  realtime,
    IPEInstanceProperty     instanceProperty,
    const CHAR*             pPipelineName,
    UINT                    error)
{
    CamxResult result = CamxResultSuccess;
    CHAR       filename[512];

    if (index == CmdBufferBLMemory)
    {
        switch (index)
        {
            case CmdBufferBLMemory:
                CamX::OsUtils::SNPrintF(filename, sizeof(filename),
                    "%s/IPEBLMemoryDump_%s_%llu_rt_%d_processType_%d_profId_%d_stabT_%d_err_%d.txt",
                    ConfigFileDirectory, pPipelineName,
                    requestId, realtime,
                    instanceProperty.processingType, instanceProperty.profileId,
                    instanceProperty.stabilizationType, error);
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "dump bl buffer %s", filename);
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
                CAMX_LOG_ERROR(CamxLogGroupPProc, "### Can't open file");
                return CamxResultEFailed;
            }
            CamX::OsUtils::FWrite(pBuffer->GetHostAddr(), pBuffer->GetMaxLength(), 1, pFile);
            CamX::OsUtils::FClose(pFile);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DumpScratchBuffer
///
/// @brief: This is called when scratchBuffer dump is enabled in static settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void DumpScratchBuffer(
            CSLBufferInfo**       ppScratchBuffer,
            UINT                  numScratchBuffer,
            UINT64                requestId,
            UINT32                instanceID,
            UINT32                realtime,
            IPEInstanceProperty   instanceProperty,
            Pipeline*             pPipelineName)
{
    CHAR filename[512] = { 0 };

    for (UINT count = 0; count < numScratchBuffer; count++)
    {
        if ((NULL != ppScratchBuffer[count]) && (NULL != ppScratchBuffer[count]->pVirtualAddr))
        {
            CamX::OsUtils::SNPrintF(filename, sizeof(filename),
                "%s/IPEScratchBufferDump_Pipeline_%p_req_%llu_inst_%d_rt_%d_profId_%d.txt",
                ConfigFileDirectory,
                pPipelineName,
                requestId, instanceID, realtime, instanceProperty.profileId);
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Dumping Scratch buffer: %s", filename);

            FILE* pFile = CamX::OsUtils::FOpen(filename, "wb");
            if (!pFile)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Can't open Scratch Buffer file");
            }
            else
            {
                CamX::OsUtils::FWrite(ppScratchBuffer[count]->pVirtualAddr, ppScratchBuffer[count]->size,
                                        1, pFile);
                CamX::OsUtils::FClose(pFile);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::TranslateToFirmwarePortId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE VOID IPENode::TranslateToFirmwarePortId(
    UINT32          portId,
    IPE_IO_IMAGES*  pFirmwarePortId)
{
    CAMX_ASSERT(portId < static_cast<UINT32>(IPE_IO_IMAGES::IPE_IO_IMAGES_MAX));

    *pFirmwarePortId = static_cast<IPE_IO_IMAGES>(portId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPENode* IPENode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    if ((NULL != pCreateInputData) && (NULL != pCreateInputData->pNodeInfo))
    {
        INT32            stabType        = 0;
        UINT32           propertyCount   = pCreateInputData->pNodeInfo->nodePropertyCount;
        PerNodeProperty* pNodeProperties = pCreateInputData->pNodeInfo->pNodeProperties;

        IPENode* pNodeObj = CAMX_NEW IPENode;

        if (NULL != pNodeObj)
        {
            pNodeObj->m_nodePropDisableZoomCrop = FALSE;

            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "nodePropertyCount %d", propertyCount);

            // There can be multiple IPE instances in a pipeline, each instance can have differnt IQ modules enabled
            for (UINT32 count = 0; count < propertyCount; count++)
            {
                UINT32 nodePropertyId    = pNodeProperties[count].id;
                VOID* pNodePropertyValue = pNodeProperties[count].pValue;

                switch (nodePropertyId)
                {
                    case NodePropertyProfileId:
                        pNodeObj->m_instanceProperty.profileId = static_cast<IPEProfileId>(
                            atoi(static_cast<const CHAR*>(pNodePropertyValue)));
                        break;
                    case NodePropertyStabilizationType:
                        // If EIS is enabled, IPE instance needs to know if its in EIS2.0 path 3.0.
                        // Node property value is shifted to use multiple stabilization type together.
                        stabType |= (1 << (atoi(static_cast<const CHAR*>(pNodePropertyValue))));

                        // Check if EIS ICA dependency need to be bypassed
                        if ((TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->bypassIPEICADependency) &&
                            ((IPEStabilizationTypeEIS2 & stabType) ||
                            (IPEStabilizationTypeEIS3 & stabType)))
                        {
                            stabType &= ~(IPEStabilizationTypeEIS2 | IPEStabilizationTypeEIS3);
                            CAMX_LOG_INFO(CamxLogGroupPProc, "EIS stabalization disabled stabType %d", stabType);
                        }

                        if ((IPEStabilizationTypeEIS3   == (IPEStabilizationTypeEIS3 & stabType)) ||
                            (IPEStabilizationTypeSWEIS3 == (IPEStabilizationTypeSWEIS3 & stabType)))
                        {
                            pCreateOutputData->createFlags.hasDelayedNotification = TRUE;
                        }

                        pNodeObj->m_instanceProperty.stabilizationType = static_cast<IPEStabilizationType>(stabType);
                        break;
                    case NodePropertyProcessingType:
                        // If MFNR is enabled, IPE instance needs to know if its in prefilter/blend/scale or postfilter.
                        pNodeObj->m_instanceProperty.processingType = static_cast<IPEProcessingType>(
                            atoi(static_cast<const CHAR*>(pNodePropertyValue)));
                        break;
                    case NodePropertyIPEDownscale:
                        pNodeObj->m_instanceProperty.ipeOnlyDownscalerMode =
                            atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyIPEDownscaleWidth:
                        pNodeObj->m_instanceProperty.ipeDownscalerWidth =
                            atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyIPEDownscaleHeight:
                        pNodeObj->m_instanceProperty.ipeDownscalerHeight =
                            atoi(static_cast<const CHAR*>(pNodePropertyValue));
                        break;
                    case NodePropertyEnbaleIPECHICropDependency:
                        pNodeObj->m_instanceProperty.enableCHICropInfoPropertyDependency = static_cast<BOOL>(
                            atoi(static_cast<const CHAR*>(pNodePropertyValue)));
                        break;
                    case NodePropertyEnableFOVC:
                        pNodeObj->m_instanceProperty.enableFOVC = *static_cast<UINT*>(pNodePropertyValue);
                        break;
                    default:
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Unhandled node property Id %d", nodePropertyId);
                        break;
                }
            }

            CAMX_LOG_INFO(CamxLogGroupPProc, "IPE Instance profileId: %d stabilization: %d processing: %d, "
                          "ipeOnlyDownscalerMode: %d, width: %d, height: %d, enbaleIPECHICropDependency: %d, "
                          "enableFOVC: %d",
                          pNodeObj->m_instanceProperty.profileId,
                          pNodeObj->m_instanceProperty.stabilizationType,
                          pNodeObj->m_instanceProperty.processingType,
                          pNodeObj->m_instanceProperty.ipeOnlyDownscalerMode,
                          pNodeObj->m_instanceProperty.ipeDownscalerWidth,
                          pNodeObj->m_instanceProperty.ipeDownscalerHeight,
                          pNodeObj->m_instanceProperty.enableCHICropInfoPropertyDependency,
                          pNodeObj->m_instanceProperty.enableFOVC);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to create IPENode, no memory");
        }

        return pNodeObj;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Null input pointer");
        return NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);

    CamxResult      result                      = CamxResultSuccess;

    INT32           deviceIndex                 = -1;
    UINT            indicesLengthRequired       = 0;

    CAMX_ASSERT(IPE == Type());
    CAMX_ASSERT(NULL != pCreateOutputData);

    Titan17xContext* pContext = static_cast<Titan17xContext*>(GetHwContext());
    m_OEMICASettingEnable     = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->IsICAIQSettingEnable;
    m_OEMIQSettingEnable      = pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->IsOEMIQSettingEnable;

    pCreateOutputData->maxOutputPorts                          = IPEMaxOutput;
    pCreateOutputData->maxInputPorts                           = IPEMaxInput;
    pCreateOutputData->createFlags.isDeferNotifyPipelineCreate = TRUE;

    UINT numOutputPorts = 0;
    UINT outputPortId[MaxBufferComposite];

    GetAllOutputPortIds(&numOutputPorts, &outputPortId[0]);

    CAMX_ASSERT(MaxBufferComposite >= numOutputPorts);

    UINT32 groupID = ISPOutputGroupIdMAX;
    for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
    {
        switch (outputPortId[outputPortIndex])
        {
            case IPEOutputPortDisplay:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                                                                               ISPOutputGroupId0;
                break;

            case IPEOutputPortVideo:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                    ISPOutputGroupId1;
                break;

            case IPEOutputPortFullRef:
            case IPEOutputPortDS4Ref:
            case IPEOutputPortDS16Ref:
            case IPEOutputPortDS64Ref:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] = ISPOutputGroupId2;
                break;

            default:
                pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] =
                    groupID++;
                break;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Index %d, Port ID %d is maped group %d",
                       outputPortIndex,
                       outputPortId[outputPortIndex],
                       pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]]);
    }

    pCreateOutputData->bufferComposite.hasCompositeMask = TRUE;

    // Add device indices
    result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeICP, &deviceIndex, 1, &indicesLengthRequired);

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(indicesLengthRequired == 1);
        result = AddDeviceIndex(deviceIndex);
        m_deviceIndex = deviceIndex;
    }

    SetLoopBackPortEnable();

    // If IPE tuning-data enable, initialize debug-data writer
    if ((CamxResultSuccess  == result)                                      &&
        (TRUE               == GetStaticSettings()->enableTuningMetadata)   &&
        (0                  != GetStaticSettings()->tuningDumpDataSizeIPE))
    {
        m_pTuningMetadata = static_cast<IPETuningMetadata*>(CAMX_CALLOC(sizeof(IPETuningMetadata)));
        if (NULL == m_pTuningMetadata)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate Tuning metadata.");
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            m_pDebugDataWriter = CAMX_NEW TDDebugDataWriter();
            if (NULL == m_pDebugDataWriter)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate Tuning metadata.");
                result = CamxResultENoMemory;
            }
        }
    }

    m_OEMStatsConfig = GetStaticSettings()->IsOEMStatSettingEnable;
    Utils::Memset(&m_adrcInfo, 0, sizeof(PropertyISPADRCInfo));
    m_adrcInfo.enable = FALSE;

    // Configure IPE Capability
    result = ConfigureIPECapability();

    if (result == CamxResultSuccess)
    {
        result = OpenStripingLibrary();
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetGammaOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetGammaOutput(
    ISPInternalData* pISPData,
    UINT32           parentID)
{
    CamxResult result       = CamxResultSuccess;
    UINT32*    pGammaOutput = NULL;
    UINT32     gammaLength  = 0;
    UINT32     metaTag      = 0;
    BOOL       isGammaValid = FALSE;

    //  Get the Gamma output from IFE / BPS
    if (parentID == IFE)
    {
        metaTag = PropertyIDIFEGammaOutput;
    }
    else if (parentID == BPS)
    {
        metaTag = PropertyIDBPSGammaOutput;
    }
    else if (IPEProcessingType::IPEMFSRPostfilter == m_instanceProperty.processingType)
    {
        metaTag = PropertyIDBPSGammaOutput | InputMetadataSectionMask;
    }
    else
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.gammainfo",
                                                          "GammaInfo",
                                                           &metaTag);
        if (CamxResultSuccess == result)
        {
            metaTag |= InputMetadataSectionMask;
        }
    }

    const UINT PropertiesIPE[]               = { metaTag };
    const UINT length                        = CAMX_ARRAY_SIZE(PropertiesIPE);
    VOID*      pData[length]                 = { 0 };
    UINT64     propertyDataIPEOffset[length] = { 0 };

    GetDataList(PropertiesIPE, pData, propertyDataIPEOffset, length);
    if (NULL != pData[0])
    {
        pGammaOutput = reinterpret_cast<GammaInfo*>(pData[0])->gammaG;
        gammaLength  = sizeof(reinterpret_cast<GammaInfo*>(pData[0])->gammaG);
        isGammaValid = reinterpret_cast<GammaInfo*>(pData[0])->isGammaValid;
    }

    pISPData->gammaOutput.isGammaValid = isGammaValid;
    if ((NULL != pGammaOutput) && (gammaLength == sizeof(pISPData->gammaOutput.gammaG)))
    {
        Utils::Memcpy(pISPData->gammaOutput.gammaG, pGammaOutput, gammaLength);
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupPProc, "%s: Cannot get output gamma %p, length %d, isGammaValid %d, publisher node %u",
            NodeIdentifierString(), pGammaOutput, gammaLength, isGammaValid, parentID);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetGammaPreCalculationOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetGammaPreCalculationOutput(
    ISPInternalData* pISPData,
    UINT32           parentID)
{
    //  The Gamma pre-calculated output comes from gamma15 module(in IPE) that executed in TMC1.1.
    //  It is different from the IPENode::GetGammaOutput function, which its output comes from Gamma16 module in IFE/BPS.
    //  Due to TMC 1.1 needs Gamma15 LUT output as its input, but Gamma15 is actually located in IPE.
    //  So we do a pre-calculation at TMC 1.1 in IFE/BPS. Then we can get the pre-calculated output from metadata for
    //  Gamma15 in IPE instead of calculating it again.

    CamxResult result = CamxResultSuccess;

    UINT PropertiesIPE[] =
    {
        PropertyIDIPEGamma15PreCalculation,
    };

    if (IPEProcessingType::IPEMFSRPostfilter == m_instanceProperty.processingType)
    {
        PropertiesIPE[0] =
        {
            PropertyIDIPEGamma15PreCalculation | InputMetadataSectionMask,
        };
    }

    const UINT length                        = CAMX_ARRAY_SIZE(PropertiesIPE);
    VOID*      pData[length]                 = { 0 };
    UINT64     propertyDataIPEOffset[length] = { 0 };
    GetDataList(PropertiesIPE, pData, propertyDataIPEOffset, length);

    if (NULL != pData[0])
    {
        pISPData->IPEGamma15PreCalculationOutput = *reinterpret_cast<IPEGammaPreOutput*>(pData[0]);
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupPProc, "%s: Cannot get data of gamma15 pre-calculation from meta, publisher node: %u",
            NodeIdentifierString(), parentID);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetIntermediateDimension
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetIntermediateDimension()
{
    CamxResult result  = CamxResultSuccess;
    UINT32     metaTag = 0;

    IntermediateDimensions*  pIntermediateDimension = NULL;

    if ((IPEProcessingType::IPEMFSRPrefilter < m_instanceProperty.processingType) &&
        (IPEProcessingType::IPEMFSRPostfilter >= m_instanceProperty.processingType))
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.intermediateDimension",
                                                          "IntermediateDimension",
                                                          &metaTag);
        if (CamxResultSuccess == result)
        {
            static const UINT PropertiesIPE[]        = { metaTag | InputMetadataSectionMask };
            const UINT        length                 = CAMX_ARRAY_SIZE(PropertiesIPE);
            VOID*             pData[length]          = { 0 };
            UINT64            pDataIPEOffset[length] = { 0 };
            GetDataList(PropertiesIPE, pData, pDataIPEOffset, length);
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

            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d intermediate width=%d height=%d ratio=%f",
                             InstanceID(),
                             m_curIntermediateDimension.width,
                             m_curIntermediateDimension.height,
                             m_curIntermediateDimension.ratio);
        }
        else
        {
            result = CamxResultEInvalidPointer;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%d Error in getting intermediate dimension from input pool",
                InstanceID());
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetADRCInfoOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetADRCInfoOutput()
{
    CamxResult           result           = CamxResultSuccess;
    PropertyISPADRCInfo* pADRCProperty    = NULL;
    UINT                 propertiesIPE[1] = {0};

    if ((TRUE == IsNodeInPipeline(IFE)) && (TRUE == m_realTimeIPE) && (FALSE == IsCameraRunningOnBPS()))
    {
        //  Check m_realTimeIPE for the case which IFE and BPS are both in the pipeline.
        propertiesIPE[0] = PropertyIDIFEADRCInfoOutput;
    }
    else if (TRUE == IsNodeInPipeline(BPS))
    {
        propertiesIPE[0] = PropertyIDBPSADRCInfoOutput;
    }
    else
    {
        if (IPEProcessingType::IPEMFSRPostfilter == m_instanceProperty.processingType)
        {
            propertiesIPE[0] = PropertyIDBPSADRCInfoOutput | InputMetadataSectionMask;
        }
        else
        {
            //  For the standalone IPE pipeline which is not scale type, we'll go to input pool for the ADRC info.
            propertiesIPE[0] = PropertyIDIFEADRCInfoOutput | InputMetadataSectionMask;
        }
    }

    const UINT length                        = CAMX_ARRAY_SIZE(propertiesIPE);
    VOID*      pData[length]                 = { 0 };
    UINT64     propertyDataIPEOffset[length] = { 0 };
    GetDataList(propertiesIPE, pData, propertyDataIPEOffset, length);
    if (NULL != pData[0])
    {
        pADRCProperty = reinterpret_cast<PropertyISPADRCInfo*>(pData[0]);

        m_adrcInfo.enable    = pADRCProperty->enable;
        m_adrcInfo.version   = pADRCProperty->version;
        m_adrcInfo.gtmEnable = pADRCProperty->gtmEnable;
        m_adrcInfo.ltmEnable = pADRCProperty->ltmEnable;

        if (SWTMCVersion::TMC10 == m_adrcInfo.version)
        {
            Utils::Memcpy(&m_adrcInfo.kneePoints.KneePointsTMC10.kneeX,
                            pADRCProperty->kneePoints.KneePointsTMC10.kneeX,
                            sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC10);
            Utils::Memcpy(&m_adrcInfo.kneePoints.KneePointsTMC10.kneeY,
                            pADRCProperty->kneePoints.KneePointsTMC10.kneeY,
                            sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC10);
        }
        else if (SWTMCVersion::TMC11 == m_adrcInfo.version)
        {
            Utils::Memcpy(&m_adrcInfo.kneePoints.KneePointsTMC11.kneeX,
                            pADRCProperty->kneePoints.KneePointsTMC11.kneeX,
                            sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC11);
            Utils::Memcpy(&m_adrcInfo.kneePoints.KneePointsTMC11.kneeY,
                            pADRCProperty->kneePoints.KneePointsTMC11.kneeY,
                            sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC11);
        }
        else if (SWTMCVersion::TMC12 == m_adrcInfo.version)
        {
            Utils::Memcpy(&m_adrcInfo.kneePoints.KneePointsTMC12.kneeX,
                            pADRCProperty->kneePoints.KneePointsTMC12.kneeX,
                            sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC11);
            Utils::Memcpy(&m_adrcInfo.kneePoints.KneePointsTMC12.kneeY,
                            pADRCProperty->kneePoints.KneePointsTMC12.kneeY,
                            sizeof(FLOAT) * MAX_ADRC_LUT_KNEE_LENGTH_TMC11);
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Unknown SWTMCVersion %u, from property 0x%x",
                                m_adrcInfo.version,
                                propertiesIPE[0]);
        }

        m_adrcInfo.drcGainDark   = pADRCProperty->drcGainDark;
        m_adrcInfo.ltmPercentage = pADRCProperty->ltmPercentage;
        m_adrcInfo.gtmPercentage = pADRCProperty->gtmPercentage;

        Utils::Memcpy(&m_adrcInfo.coefficient,
                        pADRCProperty->coefficient,
                        sizeof(FLOAT) * MAX_ADRC_LUT_COEF_SIZE);

        Utils::Memcpy(&m_adrcInfo.pchipCoeffficient,
                        pADRCProperty->pchipCoeffficient,
                        sizeof(FLOAT) * MAX_ADRC_LUT_PCHIP_COEF_SIZE);
        Utils::Memcpy(&m_adrcInfo.contrastEnhanceCurve,
                        pADRCProperty->contrastEnhanceCurve,
                        sizeof(FLOAT) * MAX_ADRC_CONTRAST_CURVE);

        m_adrcInfo.curveModel       = pADRCProperty->curveModel;
        m_adrcInfo.contrastHEBright = pADRCProperty->contrastHEBright;
        m_adrcInfo.contrastHEDark   = pADRCProperty->contrastHEDark;
    }
    else
    {
        m_adrcInfo.enable = FALSE;
        CAMX_LOG_WARN(CamxLogGroupPProc, "Can't get ADRC Info!!!, pData is NULL for property 0x%x", propertiesIPE[0]);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "adrcEnabled = %d, percentageOfGTM = %f, TMC version = %d, GTM:%d, LTM:%d",
                     m_adrcInfo.enable,
                     m_adrcInfo.gtmPercentage,
                     m_adrcInfo.version,
                     m_adrcInfo.gtmEnable,
                     m_adrcInfo.ltmEnable);
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillIQSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillIQSetting(
    ISPInputData*            pInputData,
    IpeIQSettings*           pIPEIQsettings,
    PerRequestActivePorts*   pPerRequestPorts)
{
    CamxResult             result               = CamxResultSuccess;
    UINT32                 clampValue           = 0;
    const ImageFormat*     pInputImageFormat    = NULL;
    const ImageFormat*     pOutputImageFormat   = NULL;
    BOOL                   isInput10bit         = FALSE;

    if (NULL != pPerRequestPorts)
    {
        // Loop to find if the port is IPE full port
        for (UINT portIndex = 0; portIndex < pPerRequestPorts->numInputPorts; portIndex++)
        {
            PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[portIndex];

            if (IPEInputPortFull == pInputPort->portId)
            {
                pInputImageFormat = GetInputPortImageFormat(InputPortIndex(pInputPort->portId));
                if ((NULL != pInputImageFormat) &&
                    (TRUE == ImageFormatUtils::Is10BitFormat(pInputImageFormat->format)))
                {
                    isInput10bit = TRUE;
                    break;
                }
            }
        }

        for (UINT portIndex = 0; portIndex < pPerRequestPorts->numOutputPorts; portIndex++)
        {
            PerRequestOutputPortInfo* pOutputPort = &pPerRequestPorts->pOutputPorts[portIndex];

            if (NULL != pOutputPort)
            {
                for (UINT bufferIndex = 0; bufferIndex < pOutputPort->numOutputBuffers; bufferIndex++)
                {
                    if (NULL != pOutputPort->ppImageBuffer[bufferIndex])
                    {
                        pOutputImageFormat = pOutputPort->ppImageBuffer[bufferIndex]->GetFormat();

                        if (NULL != pOutputImageFormat)
                        {
                            // Clamp value depends on output format. For 10-bit Max clamp value
                            // is 0x3FF and for 8-bit it is 0xFF. OEM can choose lower value at the
                            // cost of image quality. Clamp value should be read from chromatix
                            /// @todo (CAMX-2276) Temporary workaround until pImageBuffer for HAL buffers fix is in place
                            if (TRUE == ImageFormatUtils::Is10BitFormat(pOutputImageFormat->format))
                            {
                                clampValue = Max10BitValue;
                            }
                            else
                            {
                                clampValue = Max8BitValue;
                            }
                            if (NULL != pIPEIQsettings)
                            {
                                if (IPEOutputPortDisplay == pOutputPort->portId)
                                {
                                    pIPEIQsettings->displayClampParameters.lumaClamp.clampMin   = 0;
                                    pIPEIQsettings->displayClampParameters.lumaClamp.clampMax   = clampValue;
                                    pIPEIQsettings->displayClampParameters.chromaClamp.clampMin = 0;
                                    pIPEIQsettings->displayClampParameters.chromaClamp.clampMax = clampValue;
                                    if ((TRUE == isInput10bit) &&
                                        (TRUE == ImageFormatUtils::Is8BitFormat(pOutputImageFormat->format)))
                                    {
                                        pIPEIQsettings->displayRoundingParameters.lumaRoundingPattern =
                                            ROUNDING_PATTERN_REGULAR;
                                        pIPEIQsettings->displayRoundingParameters.chromaRoundingPattern =
                                            ROUNDING_PATTERN_CHECKERBOARD_ODD;
                                    }
                                    else
                                    {
                                        pIPEIQsettings->displayRoundingParameters.lumaRoundingPattern =
                                            ROUNDING_PATTERN_REGULAR;
                                        pIPEIQsettings->displayRoundingParameters.chromaRoundingPattern =
                                            ROUNDING_PATTERN_REGULAR;
                                    }
                                }

                                if (IPEOutputPortVideo == pOutputPort->portId)
                                {
                                    pIPEIQsettings->videoClampParameters.lumaClamp.clampMin   = 0;
                                    pIPEIQsettings->videoClampParameters.lumaClamp.clampMax   = clampValue;
                                    pIPEIQsettings->videoClampParameters.chromaClamp.clampMin = 0;
                                    pIPEIQsettings->videoClampParameters.chromaClamp.clampMax = clampValue;
                                    if ((TRUE == isInput10bit) &&
                                        (TRUE == ImageFormatUtils::Is8BitFormat(pOutputImageFormat->format)))
                                    {
                                        pIPEIQsettings->videoRoundingParameters.lumaRoundingPattern   =
                                            ROUNDING_PATTERN_REGULAR;
                                        pIPEIQsettings->videoRoundingParameters.chromaRoundingPattern =
                                            ROUNDING_PATTERN_CHECKERBOARD_ODD;
                                    }
                                    else
                                    {
                                        pIPEIQsettings->videoRoundingParameters.lumaRoundingPattern   =
                                            ROUNDING_PATTERN_REGULAR;
                                        pIPEIQsettings->videoRoundingParameters.chromaRoundingPattern =
                                            ROUNDING_PATTERN_REGULAR;
                                    }
                                }
                            }
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupPProc, "pOutputImageFormat is NULL");
                            result = CamxResultEInvalidPointer;
                        }
                    }
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "pOutputPort is NULL");
                result = CamxResultEInvalidPointer;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "pPerRequestPorts is NULL");
        result = CamxResultEInvalidPointer;
    }

    // DS parameters
    TuningDataManager* pTuningManager = pInputData->pTuningDataManager;
    if (NULL != pIPEIQsettings)
    {
        ds4to1_1_0_0::chromatix_ds4to1v10Type* pChromatix = NULL;
        if ((NULL != pTuningManager) && (TRUE == pTuningManager->IsValidChromatix()))
        {
            pChromatix = pTuningManager->GetChromatix()->GetModule_ds4to1v10_ipe(
                            reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                            pInputData->pTuningData->noOfSelectionParameter);

            if (NULL != pChromatix)
            {
                INT pass_dc64 = static_cast<INT>(ispglobalelements::trigger_pass::PASS_DC64);
                INT pass_dc16 = static_cast<INT>(ispglobalelements::trigger_pass::PASS_DC16);
                INT pass_dc4  = static_cast<INT>(ispglobalelements::trigger_pass::PASS_DC4);
                INT pass_full = static_cast<INT>(ispglobalelements::trigger_pass::PASS_FULL);
                ds4to1_1_0_0::chromatix_ds4to1v10_reserveType*  pReserveData = &pChromatix->chromatix_ds4to1v10_reserve;

                pIPEIQsettings->ds4Parameters.dc64Parameters.coefficient7  =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_dc64].pass_data.coeff_07;
                pIPEIQsettings->ds4Parameters.dc64Parameters.coefficient16 =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_dc64].pass_data.coeff_16;
                pIPEIQsettings->ds4Parameters.dc64Parameters.coefficient25 =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_dc64].pass_data.coeff_25;

                pIPEIQsettings->ds4Parameters.dc16Parameters.coefficient7  =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_dc16].pass_data.coeff_07;
                pIPEIQsettings->ds4Parameters.dc16Parameters.coefficient16 =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_dc16].pass_data.coeff_16;
                pIPEIQsettings->ds4Parameters.dc16Parameters.coefficient25 =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_dc16].pass_data.coeff_25;

                pIPEIQsettings->ds4Parameters.dc4Parameters.coefficient7  =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_dc4].pass_data.coeff_07;
                pIPEIQsettings->ds4Parameters.dc4Parameters.coefficient16 =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_dc4].pass_data.coeff_16;
                pIPEIQsettings->ds4Parameters.dc4Parameters.coefficient25 =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_dc4].pass_data.coeff_25;

                pIPEIQsettings->ds4Parameters.fullpassParameters.coefficient7  =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_full].pass_data.coeff_07;
                pIPEIQsettings->ds4Parameters.fullpassParameters.coefficient16 =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_full].pass_data.coeff_16;
                pIPEIQsettings->ds4Parameters.fullpassParameters.coefficient25 =
                    pReserveData->mod_ds4to1v10_pass_reserve_data[pass_full].pass_data.coeff_25;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid chromatix pointer");
                result = CamxResultEInvalidPointer;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid %s",
                (NULL != pTuningManager) ? "Chromatix" : "TuningManager");
            result = CamxResultEInvalidPointer;
        }

        if (NULL == pChromatix)
        {
            pIPEIQsettings->ds4Parameters.dc64Parameters.coefficient7  = 125;
            pIPEIQsettings->ds4Parameters.dc64Parameters.coefficient16 = 91;
            pIPEIQsettings->ds4Parameters.dc64Parameters.coefficient25 = 144;

            pIPEIQsettings->ds4Parameters.dc16Parameters.coefficient7  = 125;
            pIPEIQsettings->ds4Parameters.dc16Parameters.coefficient16 = 91;
            pIPEIQsettings->ds4Parameters.dc16Parameters.coefficient25 = 144;

            pIPEIQsettings->ds4Parameters.dc4Parameters.coefficient7  = 125;
            pIPEIQsettings->ds4Parameters.dc4Parameters.coefficient16 = 91;
            pIPEIQsettings->ds4Parameters.dc4Parameters.coefficient25 = 144;

            pIPEIQsettings->ds4Parameters.fullpassParameters.coefficient7  = 125;
            pIPEIQsettings->ds4Parameters.fullpassParameters.coefficient16 = 91;
            pIPEIQsettings->ds4Parameters.fullpassParameters.coefficient25 = 144;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                        "DC64: Coeff07 = %d, Coeff16 = %d, Coeff25 = %d",
                        pIPEIQsettings->ds4Parameters.dc64Parameters.coefficient7,
                        pIPEIQsettings->ds4Parameters.dc64Parameters.coefficient16,
                        pIPEIQsettings->ds4Parameters.dc64Parameters.coefficient25);

        CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                        "DC16: Coeff07 = %d, Coeff16 = %d, Coeff25 = %d",
                        pIPEIQsettings->ds4Parameters.dc16Parameters.coefficient7,
                        pIPEIQsettings->ds4Parameters.dc16Parameters.coefficient16,
                        pIPEIQsettings->ds4Parameters.dc16Parameters.coefficient25);

        CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                        "DC4: Coeff07 = %d, Coeff16 = %d, Coeff25 = %d",
                        pIPEIQsettings->ds4Parameters.dc4Parameters.coefficient7,
                        pIPEIQsettings->ds4Parameters.dc4Parameters.coefficient16,
                        pIPEIQsettings->ds4Parameters.dc4Parameters.coefficient25);

        CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                        "FULL: Coeff07 = %d, Coeff16 = %d, Coeff25 = %d",
                        pIPEIQsettings->ds4Parameters.fullpassParameters.coefficient7,
                        pIPEIQsettings->ds4Parameters.fullpassParameters.coefficient16,
                        pIPEIQsettings->ds4Parameters.fullpassParameters.coefficient25);


        //  set ICA's invalidPixelModeInterpolationEnabled to be always 1
        //  especially for the DS64 image from IPE (avoid green lines in its bottom)
        pIPEIQsettings->ica1Parameters.invalidPixelModeInterpolationEnabled = 1;
        pIPEIQsettings->ica1Parameters.eightBitOutputAlignment              = 1;
        pIPEIQsettings->ica2Parameters.invalidPixelModeInterpolationEnabled = 1;
        pIPEIQsettings->ica2Parameters.eightBitOutputAlignment              = 1;

        if (result == CamxResultSuccess)
        {
            if (TRUE == IsSrEnabled())
            {
                pIPEIQsettings->useGeoLibOutput = TRUE;
                // Non realtime
                GeoLibIpeStripingConfig*  pIPEStripingInputs = (TRUE == m_geolibData.IPEModeRealtime) ?
                     &m_geolibData.geoLibStreamData.rtGeolibdata.videoframeConfig.ipeStripingInputs :
                     &m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig.ipeStripingInputs;
                pIPEIQsettings->mainInputSize.widthPixels = pIPEStripingInputs->mainInputSize.widthPixels;
                pIPEIQsettings->mainInputSize.heightLines = pIPEStripingInputs->mainInputSize.heightLines;
                CAMX_LOG_INFO(CamxLogGroupPProc, "%s: mainInputSize %d, %d", NodeIdentifierString(),
                              pIPEIQsettings->mainInputSize.widthPixels,
                              pIPEIQsettings->mainInputSize.heightLines);
                pIPEIQsettings->tfCropWindow.offset.x = pIPEStripingInputs->tfCropWindow.offset.x;
                pIPEIQsettings->tfCropWindow.offset.y = pIPEStripingInputs->tfCropWindow.offset.y;
                pIPEIQsettings->tfCropWindow.size.x   = pIPEStripingInputs->tfCropWindow.size.x;
                pIPEIQsettings->tfCropWindow.size.y   = pIPEStripingInputs->tfCropWindow.size.y;

                CAMX_LOG_INFO(CamxLogGroupPProc, "%s: tfCropWindow offset-x,y %f, %f, size-x,y %f, %f",
                              NodeIdentifierString(),
                              pIPEIQsettings->tfCropWindow.offset.x,
                              pIPEIQsettings->tfCropWindow.offset.y,
                              pIPEIQsettings->tfCropWindow.size.x,
                              pIPEIQsettings->tfCropWindow.size.y);

                pIPEIQsettings->videoOutFOV.offset.x = pIPEStripingInputs->videoOutFOV.offset.x;
                pIPEIQsettings->videoOutFOV.offset.y = pIPEStripingInputs->videoOutFOV.offset.y;
                pIPEIQsettings->videoOutFOV.size.x   = pIPEStripingInputs->videoOutFOV.size.x;
                pIPEIQsettings->videoOutFOV.size.y   = pIPEStripingInputs->videoOutFOV.size.y;

                CAMX_LOG_INFO(CamxLogGroupPProc, "%s: videoOutFOV offset-x,y %f, %f, size-x,y %f, %f",
                              NodeIdentifierString(),
                              pIPEIQsettings->videoOutFOV.offset.x,
                              pIPEIQsettings->videoOutFOV.offset.y,
                              pIPEIQsettings->videoOutFOV.size.x,
                              pIPEIQsettings->videoOutFOV.size.y);

                pIPEIQsettings->displayOutFOV.offset.x = (FALSE == m_IPESIMOMode) ?
                    pIPEStripingInputs->videoOutFOV.offset.x : pIPEStripingInputs->displayOutFOV.offset.x;
                pIPEIQsettings->displayOutFOV.offset.y = (FALSE == m_IPESIMOMode) ?
                    pIPEStripingInputs->videoOutFOV.offset.y : pIPEStripingInputs->displayOutFOV.offset.y;
                pIPEIQsettings->displayOutFOV.size.x   = (FALSE == m_IPESIMOMode) ?
                    pIPEStripingInputs->videoOutFOV.size.x : pIPEStripingInputs->displayOutFOV.size.x;
                pIPEIQsettings->displayOutFOV.size.y   = (FALSE == m_IPESIMOMode) ?
                    pIPEStripingInputs->videoOutFOV.size.y : pIPEStripingInputs->displayOutFOV.size.y;

                CAMX_LOG_INFO(CamxLogGroupPProc, "%s: m_IPESIMOMode %d displayOutFOV offset-x,y %f, %f, size-x,y %f, %f",
                              NodeIdentifierString(),
                              m_IPESIMOMode,
                              pIPEIQsettings->displayOutFOV.offset.x,
                              pIPEIQsettings->displayOutFOV.offset.y,
                              pIPEIQsettings->displayOutFOV.size.x,
                              pIPEIQsettings->displayOutFOV.size.y);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE IQ settings is NULL");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillFramePerfParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillFramePerfParams(
    IpeFrameProcessData* pFrameProcessData)
{
    CamxResult    result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pFrameProcessData);

    pFrameProcessData->maxNumOfCoresToUse       = (GetStaticSettings()->numIPECoresToUse <= m_capability.numIPE) ?
                     GetStaticSettings()->numIPECoresToUse : m_capability.numIPE;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillFrameUBWCParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillFrameUBWCParams(
    CmdBuffer*                pMainCmdBuffer,
    IpeFrameProcessData*      pFrameProcessData,
    CSLBufferInfo*            pUBWCStatsBuf,
    PerRequestOutputPortInfo* pOutputPort,
    UINT                      outputPortIndex)
{
    CamxResult             result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pMainCmdBuffer);
    CAMX_ASSERT(NULL != pFrameProcessData);
    CAMX_ASSERT(NULL != pOutputPort);

    if (IsRealTime() && IsSinkPortWithBuffer(outputPortIndex) && (NULL != pUBWCStatsBuf) &&
        ((IPEOutputPortDisplay == pOutputPort->portId) || (IPEOutputPortVideo == pOutputPort->portId)))
    {
        UINT32 ubwcStatsBufferOffset =
            static_cast <UINT32>(offsetof(IpeFrameProcessData, ubwcStatsBufferAddress));

        result = pMainCmdBuffer->AddNestedBufferInfo(ubwcStatsBufferOffset,
                                                    pUBWCStatsBuf->hHandle,
                                                    pUBWCStatsBuf->offset);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Error in patching ubwc stats address: %s", Utils::CamxResultToString(result));
            pFrameProcessData->ubwcStatsBufferSize = 0;
        }
        else
        {
            pFrameProcessData->ubwcStatsBufferSize = sizeof(UbwcStatsBuffer);
        }
    }
    else
    {
        pFrameProcessData->ubwcStatsBufferAddress = 0;
        pFrameProcessData->ubwcStatsBufferSize    = 0;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::IsValidDimension
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPENode::IsValidDimension(
    IpeZoomWindow* pZoomWindow)
{
    UINT32 IPEMaxInputWidth       = m_capability.maxInputWidth[ICA_MODE_DISABLED];
    UINT32 IPEMaxInputHeight      = m_capability.maxInputHeight[ICA_MODE_DISABLED];
    UINT32 IPEMinInputWidthLimit  = m_capability.minInputWidth;
    UINT32 IPEMinInputHeightLimit = m_capability.minInputHeight;


    if (pZoomWindow->windowLeft < 0                           ||
        pZoomWindow->windowTop < 0                            ||
        (pZoomWindow->windowHeight <= IPEMinInputHeightLimit  ||
         pZoomWindow->windowWidth <= IPEMinInputWidthLimit)   ||
        (pZoomWindow->windowHeight > IPEMaxInputHeight)       ||
        (pZoomWindow->windowWidth > IPEMaxInputWidth))
    {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::ValidateCorrectZoom
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::ValidateCorrectZoom(
    UINT          parentNodeId,
    CHIRectangle* pCropInfo,
    INT32         inputWidth,
    INT32         inputHeight)
{

    INT32 width  = (inputWidth  > m_fullOutputWidth)  ? inputWidth : m_fullOutputWidth;
    INT32 height = (inputHeight > m_fullOutputHeight) ? inputHeight: m_fullOutputHeight;
    // Check croped height or width  is not less then max zoom factor
    if ( ((pCropInfo->height < height * MaxZoomFactor) ||
         (pCropInfo->width  < width  * MaxZoomFactor) ) && (pCropInfo->height != 0) && (pCropInfo->width != 0) )
    {
        const CHAR* parentName = "Unknown";
        if (parentNodeId == IFE)
        {
            parentName = "IFE";
        }
        else if (parentNodeId == BPS)
        {
            parentName = "BPS";
        }
        CAMX_LOG_WARN(CamxLogGroupPProc, "Node:%s Adjusting crop height:%d to %f "
            "parentNode ID:%d Name:%s "
            "IsPipelineHasSnapshotJPEGStream:%d IsRealTime:%d "
            "enableCHICropInfoPropertyDependency:%d "
            "m_instanceProperty.enableFOVC:%d m_prevFOVC:%f",
            NodeIdentifierString(),
            pCropInfo->height, height * MaxZoomFactor,
            parentNodeId, parentName,
            IsPipelineHasSnapshotJPEGStream(),
            IsRealTime(),
            m_instanceProperty.enableCHICropInfoPropertyDependency,
            m_instanceProperty.enableFOVC, m_prevFOVC);

        // Keep current center coordinate, because some algorithm like SAT may use a non-centralized crop region.
        CHIRectangle origCropInfo = *pCropInfo;

        pCropInfo->height = height * MaxZoomFactor;
        pCropInfo->width  = width  * MaxZoomFactor;

        pCropInfo->top = pCropInfo->top - (pCropInfo->height - origCropInfo.height) / 2;
        pCropInfo->top = Utils::MaxINT32(pCropInfo->top, 0);
        pCropInfo->top = Utils::MinINT32(pCropInfo->top, height - pCropInfo->height - 1);

        pCropInfo->left = pCropInfo->left - (pCropInfo->width - origCropInfo.width) / 2;
        pCropInfo->left = Utils::MaxINT32(pCropInfo->left, 0);
        pCropInfo->left = Utils::MinINT32(pCropInfo->left, width - pCropInfo->width - 1);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillFrameZoomWindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillFrameZoomWindow(
    ISPInputData*     pInputData,
    UINT              parentNodeId,
    CmdBuffer*        pCmdBuffer,
    UINT64            requestId,
    BOOL              disableZoom)
{
    CamxResult        result             = CamxResultSuccess;
    IpeZoomWindow*    pZoomWindowICA1    = NULL;
    IpeZoomWindow*    pZoomWindowICA2    = NULL;
    CHIRectangle*     pCropInfo          = NULL;
    IFEScalerOutput*  pIFEScalerOutput   = NULL;
    CHIRectangle      cropInfo           = { 0 };
    int32_t           adjustedWidth      = 0;
    int32_t           adjustedHeight     = 0;
    UINT32            adjustedFullWidth  = 0;
    UINT32            adjustedFullHeight = 0;
    FLOAT             cropFactor         = 1.0f;
    UINT32            fullInputWidth     = m_fullInputWidth;
    UINT32            fullInputHeight    = m_fullInputHeight;
    UINT32            fullOutputWidth    = m_fullOutputWidth;
    UINT32            fullOutputHeight   = m_fullOutputHeight;
    IpeIQSettings*    pIPEIQsettings     = reinterpret_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    if (NULL != pIPEIQsettings)
    {
        cropInfo.left   = 0;
        cropInfo.top    = 0;
        cropInfo.width  = fullInputWidth;
        cropInfo.height = fullInputHeight;

        if (parentNodeId == IFE)
        {
            const UINT props[] =
            {
                PropertyIDIFEScaleOutput,
            };
            const SIZE_T length         = CAMX_ARRAY_SIZE(props);
            VOID*        pData[length]  = { 0 };
            UINT64       offset[length] = { 0 };

            GetDataList(props, pData, offset, length);
            pIFEScalerOutput = reinterpret_cast<IFEScalerOutput*>(pData[0]);
            if (NULL != pIFEScalerOutput)
            {
                PortCropInfo portCropInfo = { { 0 } };
                result = Node::GetPortCrop(this, IPEInputPortFull, &portCropInfo, NULL);
                if (CamxResultSuccess == result)
                {
                    cropInfo = portCropInfo.residualCrop;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc,
                        "Error while getting full input port crop from parent node IFE, result=%d", result);
                }
                pCropInfo = &cropInfo;

                CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                                 "IFE: crop left:%d top:%d width:%d, height:%d, Scaler ratio = %f, requestId %llu",
                                 pCropInfo->left,
                                 pCropInfo->top,
                                 pCropInfo->width,
                                 pCropInfo->height,
                                 pIFEScalerOutput->scalingFactor,
                                 requestId);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid IFE scaler output");
                result = CamxResultEInvalidArg;
            }
        }
        // Don't need to add parent check, because ipe just use this property when parent node is chinode.
        else if (TRUE == m_instanceProperty.enableCHICropInfoPropertyDependency)
        {
            UINT32 metaTag = 0;

            result = VendorTagManager::QueryVendorTagLocation("com.qti.cropregions",
                                                              "ChiNodeResidualCrop",
                                                              &metaTag);
            if (CamxResultSuccess == result)
            {
                static const UINT tagInfo[] =
                {
                    metaTag,
                    PropertyIDIFEScaleOutput // For RT pipeline
                };

                const UINT  length             = CAMX_ARRAY_SIZE(tagInfo);
                VOID*       pData[length]      = { 0 };
                UINT64      dataOffset[length] = { 0 };
                CropWindow* pCropWindow        = NULL;

                GetDataList(tagInfo, pData, dataOffset, length);

                pCropWindow      = reinterpret_cast<CropWindow*>(pData[0]);
                pIFEScalerOutput = reinterpret_cast<IFEScalerOutput*>(pData[1]);

                if (NULL != pCropWindow)
                {
                    cropInfo.left   = pCropWindow->left;
                    cropInfo.top    = pCropWindow->top;
                    cropInfo.width  = pCropWindow->width;
                    cropInfo.height = pCropWindow->height;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get ChiNodeResidualCrop from parent chinode, requestId %llu",
                        requestId);
                }

                if (NULL == pIFEScalerOutput)
                {
                    // Query from Input data(Offline Pipeline where ChiNode is parent node)
                    static const UINT PropertiesOfflinePreScale[] =
                    {
                        PropertyIDIFEScaleOutput | InputMetadataSectionMask
                    };

                    const  SIZE_T length             = CAMX_ARRAY_SIZE(PropertiesOfflinePreScale);
                    VOID*         pData[length]      = { 0 };
                    UINT64        dataOffset[length] = { 0 };

                    GetDataList(PropertiesOfflinePreScale, pData, dataOffset, length);
                    if (NULL != pData[0])
                    {
                        pIFEScalerOutput = reinterpret_cast<IFEScalerOutput*>(pData[0]);
                        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IFE Scaler output for Offline pipeline %p mask 0x%x",
                                         pIFEScalerOutput,
                                         PropertyIDIFEScaleOutput | InputMetadataSectionMask);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid IFE scaler Output");
                        result = CamxResultEInvalidArg;
                    }
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to query ChiNode Crop vendor tags, requestId %llu", requestId);
            }

            pCropInfo = &cropInfo;
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ChiNode pCropInfo %p, IFE Scaler Output %p", pCropInfo, pIFEScalerOutput);
        }
        else if ((parentNodeId == BPS) ||
                 ((TRUE == IsPipelineHasSnapshotJPEGStream() && (FALSE == IsRealTime()))) ||
                 (FALSE == IsRealTime()))
        {
            UINT32 metaTag = 0;

            result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ref.cropsize",
                                                              "RefCropSize",
                                                              &metaTag);

            if (CamxResultSuccess == result)
            {
                metaTag |= InputMetadataSectionMask;
                CropWindow*        pCropWindow     = NULL;
                RefCropWindowSize* pRefCropWindow  = NULL;
                const UINT  PropertiesIPE[] =
                {
                    InputScalerCropRegion,
                    metaTag
                };
                const SIZE_T length                        = CAMX_ARRAY_SIZE(PropertiesIPE);
                VOID*        pData[length]                 = { 0 };
                UINT64       propertyDataIPEOffset[length] = { 0 , 0};

                GetDataList(PropertiesIPE, pData, propertyDataIPEOffset, length);
                pCropWindow    = (static_cast<CropWindow*>(pData[0]));
                pRefCropWindow = (static_cast<RefCropWindowSize*>(pData[1]));

                if ((NULL != pCropWindow) && (NULL != pRefCropWindow))
                {
                    if ((0 == pRefCropWindow->width) || (0 == pRefCropWindow->height))
                    {
                        pRefCropWindow->width  = fullInputWidth;
                        pRefCropWindow->height = fullInputHeight;
                    }

                    CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                                     "ZDBG IPE crop Window [%d, %d, %d, %d] full size %dX%d active %dX%d, requestId %llu",
                                     pCropWindow->left,
                                     pCropWindow->top,
                                     pCropWindow->width,
                                     pCropWindow->height,
                                     fullInputWidth,
                                     fullInputHeight,
                                     pRefCropWindow->width,
                                     pRefCropWindow->height,
                                     requestId);

                    cropInfo.left   = (pCropWindow->left   * fullInputWidth) / pRefCropWindow->width;
                    cropInfo.top    = (pCropWindow->top    * fullInputHeight) / pRefCropWindow->height;
                    cropInfo.width  = (pCropWindow->width  * fullInputWidth) / pRefCropWindow->width;
                    cropInfo.height = (pCropWindow->height * fullInputHeight) / pRefCropWindow->height;

                    FLOAT widthRatio  = static_cast<FLOAT>(cropInfo.width) / static_cast<FLOAT>(fullOutputWidth);
                    FLOAT heightRatio = static_cast<FLOAT>(cropInfo.height) / static_cast<FLOAT>(fullOutputHeight);

                    if ((widthRatio > IPEDownscaleThresholdMin && widthRatio < IPEDownscaleThresholdMax) &&
                        (heightRatio > IPEDownscaleThresholdMin && heightRatio < IPEDownscaleThresholdMax))
                    {
                        cropInfo.left   = (cropInfo.width  - fullOutputWidth)  / 2 + cropInfo.left;
                        cropInfo.top    = (cropInfo.height - fullOutputHeight) / 2 + cropInfo.top;
                        cropInfo.width  = fullOutputWidth;
                        cropInfo.height = fullOutputHeight;
                    }
                    else if ((widthRatio  > IPEUpscaleThresholdMin && widthRatio  < IPEUpscaleThresholdMax) &&
                             (heightRatio > IPEUpscaleThresholdMin && heightRatio < IPEUpscaleThresholdMax))
                    {
                        INT32 tempVal   = (static_cast<INT32>(fullOutputWidth) - static_cast<INT32>(cropInfo.width) )  / 2;

                        cropInfo.left   = IQSettingUtils::ClampINT32(static_cast<INT32>(cropInfo.left) - tempVal,
                                                                     0, fullOutputWidth);
                        tempVal         = (static_cast<INT32>(fullOutputHeight) - static_cast<INT32>(cropInfo.height) ) / 2;
                        cropInfo.top    = IQSettingUtils::ClampINT32((static_cast<INT32>(cropInfo.top) - tempVal),
                                                                     0, fullOutputHeight);
                        cropInfo.width  = fullOutputWidth;
                        cropInfo.height = fullOutputHeight;
                    }

                    if (((cropInfo.left + cropInfo.width) > (static_cast<INT32>(fullInputWidth))) ||
                        ((cropInfo.top + cropInfo.height) > (static_cast<INT32>(fullInputHeight))))
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc,
                                       "ZDBG wrong IPE crop Window [%d, %d, %d, %d] full %dX%d, requestId %llu",
                                       cropInfo.left,
                                       cropInfo.top,
                                       cropInfo.width,
                                       cropInfo.height,
                                       fullInputWidth,
                                       fullInputHeight,
                                       requestId);

                        cropInfo.left   = 0;
                        cropInfo.top    = 0;
                        cropInfo.width  = fullInputWidth;
                        cropInfo.height = fullInputHeight;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to get pCropWindow %p, pRefCropWindow %p, from BPS/Snapshot",
                        pCropWindow, pRefCropWindow);
                }

                pCropInfo = &cropInfo;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "cannot find vendor tag ref.cropsize, requestId %llu", requestId);
            }
        }
        else
        {
            // Check IFE scale output from the result metadata (RT pipeline)
            const UINT PropertiesIPE[] =
            {
                PropertyIDIFEScaleOutput,
            };
            const SIZE_T length                        = CAMX_ARRAY_SIZE(PropertiesIPE);
            VOID*        pData[length]                 = { 0 };
            UINT64       propertyDataIPEOffset[length] = { 0 };

            GetDataList(PropertiesIPE, pData, propertyDataIPEOffset, length);
            pIFEScalerOutput = reinterpret_cast<IFEScalerOutput*>(pData[0]);

            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "CropInfo from RT pipeline %p mask 0x%x 0x%x",
                             pIFEScalerOutput, PropertyIDIFEScaleOutput);

            if (pIFEScalerOutput == NULL)
            {
                // Query from Input data(Offline Pipeline)
                const UINT PropertiesOfflineIPE[] =
                {
                    PropertyIDIFEScaleOutput | InputMetadataSectionMask
                };
                const SIZE_T length                        = CAMX_ARRAY_SIZE(PropertiesOfflineIPE);
                VOID*        pData[length]                 = { 0 };
                UINT64       propertyDataIPEOffset[length] = { 0 };
                GetDataList(PropertiesOfflineIPE, pData, propertyDataIPEOffset, length);
                pIFEScalerOutput = reinterpret_cast<IFEScalerOutput*>(pData[0]);

                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "CropInfo from Offline pipeline %p mask 0x%x",
                                 pIFEScalerOutput,
                                 PropertyIDIFEScaleOutput | InputMetadataSectionMask);
            }

            if (NULL == pIFEScalerOutput)
            {
                result = CamxResultEInvalidArg;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Can not get IFE scaler output");
            }

            PortCropInfo portCropInfo = { { 0 } };

            // Check residual crop
            // To find port crop of IFE for SWEIS case, recursion is required.
            if ((0 != (IPEStabilizationType::IPEStabilizationTypeSWEIS2 & m_instanceProperty.stabilizationType)) ||
                (0 != (IPEStabilizationType::IPEStabilizationTypeSWEIS3 & m_instanceProperty.stabilizationType)) ||
                (0 != (IPEStabilizationType::IPEStabilizationTypeEIS2 & m_instanceProperty.stabilizationType)) ||
                (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType)))
            {
                UINT32 findbyRecurssion = 1;
                if (CamxResultSuccess == Node::GetPortCrop(this, IPEInputPortFull, &portCropInfo, &findbyRecurssion))
                {
                    cropInfo = portCropInfo.residualCrop;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to get residual crop for SW EIS from IFE, requestId %llu",
                        requestId);
                }
            }
            else if (CamxResultSuccess == Node::GetPortCrop(this, IPEInputPortFull, &portCropInfo, NULL))
            {
                cropInfo = portCropInfo.residualCrop;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to get residual crop, requestId %llu", requestId);
            }

            pCropInfo = &cropInfo;
        }


        // Fill in ICA1 Zoom Params
        pZoomWindowICA1    = &pIPEIQsettings->ica1Parameters.zoomWindow;
        adjustedFullWidth  = fullInputWidth;
        adjustedFullHeight = fullInputHeight;

        //< Update Crop info if EIS is enabled. firmware expects crop window wrt window after margin crop (ICA1 output)
        UpdateZoomWindowForStabilization(pCropInfo,
                                         &adjustedFullWidth,
                                         &adjustedFullHeight,
                                         requestId);

        // Apply fixed FOV correction requested by stats
        ApplyStatsFOVCCrop(pCropInfo, parentNodeId, adjustedFullWidth, adjustedFullHeight, requestId);

        BOOL  upscaleonStream = ((FALSE == disableZoom) &&
                                 (FALSE == IsSrEnabled()) &&
                                 (TRUE == IsValidProfileWithUpscaler())) ? TRUE : FALSE;
        if (TRUE == upscaleonStream)
        {
            if (NULL != pCropInfo)
            {
                ValidateCorrectZoom(parentNodeId, pCropInfo, adjustedFullWidth, adjustedFullHeight);
                pZoomWindowICA1->windowTop    = pCropInfo->top;
                pZoomWindowICA1->windowLeft   = pCropInfo->left;
                pZoomWindowICA1->windowWidth  = pCropInfo->width;
                pZoomWindowICA1->windowHeight = pCropInfo->height;
                pZoomWindowICA1->fullWidth    = adjustedFullWidth;
                pZoomWindowICA1->fullHeight   = adjustedFullHeight;
            }
            else if ((0 != m_previousCropInfo.width) && (0 != m_previousCropInfo.height))
            {
                pZoomWindowICA1->windowTop    = m_previousCropInfo.top;
                pZoomWindowICA1->windowLeft   = m_previousCropInfo.left;
                pZoomWindowICA1->windowWidth  = m_previousCropInfo.width;
                pZoomWindowICA1->windowHeight = m_previousCropInfo.height;
                pZoomWindowICA1->fullWidth    = adjustedFullWidth;
                pZoomWindowICA1->fullHeight   = adjustedFullHeight;
            }

            // override ICA1 zoom window in MFSR postfilter stage
            if ((IPEProcessingType::IPEMFSRPostfilter == m_instanceProperty.processingType) &&
                (IPEProfileId::IPEProfileIdDefault    == m_instanceProperty.profileId))
            {
                pZoomWindowICA1->windowTop    = 0;
                pZoomWindowICA1->windowLeft   = 0;
                pZoomWindowICA1->windowWidth  = 0;
                pZoomWindowICA1->windowHeight = 0;
                pZoomWindowICA1->fullWidth    = 0;
                pZoomWindowICA1->fullHeight   = 0;
            }

            /// @todo (CAMX-2313) Enable ifeZoomWIndow param in IPE IQSettings
            // Fill in ICA2 Zoom Params
            pZoomWindowICA2 = &pIPEIQsettings->ica2Parameters.zoomWindow;

            // ICA2 Zoom Window is needed for reference frames. Hence this is populated from previous crop value which is stored
            if ((TRUE == IsLoopBackPortEnabled()) &&
                (0 != m_previousCropInfo.width) && (0 != m_previousCropInfo.height))
            {
                pZoomWindowICA2->windowTop    = m_previousCropInfo.top;
                pZoomWindowICA2->windowLeft   = m_previousCropInfo.left;
                pZoomWindowICA2->windowWidth  = m_previousCropInfo.width;
                pZoomWindowICA2->windowHeight = m_previousCropInfo.height;
                pZoomWindowICA2->fullWidth    = adjustedFullWidth;
                pZoomWindowICA2->fullHeight   = adjustedFullHeight;
                /// @todo (CAMX-2313) Enable ifeZoomWIndow param in IPE IQSettings
            }
            else
            {
                pZoomWindowICA2->windowTop    = 0;
                pZoomWindowICA2->windowLeft   = 0;
                pZoomWindowICA2->windowWidth  = 0;
                pZoomWindowICA2->windowHeight = 0;
                pZoomWindowICA2->fullWidth    = 0;
                pZoomWindowICA2->fullHeight   = 0;
            }

            // Save crop info for next frame reference crop information for ICA2
            if (NULL != pCropInfo)
            {
                Utils::Memcpy(&m_previousCropInfo, pCropInfo, sizeof(CHIRectangle));
            }
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ZDBG: %s ICA1 Zoom Window [%d, %d, %d, %d] full %d x %d, requestId %llu",
                             NodeIdentifierString(),
                             pIPEIQsettings->ica1Parameters.zoomWindow.windowLeft,
                             pIPEIQsettings->ica1Parameters.zoomWindow.windowTop,
                             pIPEIQsettings->ica1Parameters.zoomWindow.windowWidth,
                             pIPEIQsettings->ica1Parameters.zoomWindow.windowHeight,
                             pIPEIQsettings->ica1Parameters.zoomWindow.fullWidth,
                             pIPEIQsettings->ica1Parameters.zoomWindow.fullHeight,
                             requestId);
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ZDBG: %s ICA1 IFE Zoom Window [%d, %d, %d, %d] full %d x %d, requestId %llu",
                             NodeIdentifierString(),
                             pIPEIQsettings->ica1Parameters.ifeZoomWindow.windowLeft,
                             pIPEIQsettings->ica1Parameters.ifeZoomWindow.windowTop,
                             pIPEIQsettings->ica1Parameters.ifeZoomWindow.windowWidth,
                             pIPEIQsettings->ica1Parameters.ifeZoomWindow.windowHeight,
                             pIPEIQsettings->ica1Parameters.ifeZoomWindow.fullWidth,
                             pIPEIQsettings->ica1Parameters.ifeZoomWindow.fullHeight,
                             requestId);
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ZDBG: %s ICA2 Zoom Window [%d, %d, %d, %d] full %d x %d, requestId %llu",
                             NodeIdentifierString(),
                             pIPEIQsettings->ica2Parameters.zoomWindow.windowLeft,
                             pIPEIQsettings->ica2Parameters.zoomWindow.windowTop,
                             pIPEIQsettings->ica2Parameters.zoomWindow.windowWidth,
                             pIPEIQsettings->ica2Parameters.zoomWindow.windowHeight,
                             pIPEIQsettings->ica2Parameters.zoomWindow.fullWidth,
                             pIPEIQsettings->ica2Parameters.zoomWindow.fullHeight,
                             requestId);
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ZDBG: %s ICA2 IFE Zoom Window [%d, %d, %d, %d] full %d x %d, requestId %llu",
                             NodeIdentifierString(),
                             pIPEIQsettings->ica2Parameters.ifeZoomWindow.windowLeft,
                             pIPEIQsettings->ica2Parameters.ifeZoomWindow.windowTop,
                             pIPEIQsettings->ica2Parameters.ifeZoomWindow.windowWidth,
                             pIPEIQsettings->ica2Parameters.ifeZoomWindow.windowHeight,
                             pIPEIQsettings->ica2Parameters.ifeZoomWindow.fullWidth,
                             pIPEIQsettings->ica2Parameters.ifeZoomWindow.fullHeight,
                             requestId);
        }
        if (FALSE == SetScaleRatios(pInputData, parentNodeId, pCropInfo, pIFEScalerOutput, pCmdBuffer, upscaleonStream))
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot Set Scale Ratios! Use default 1.0f");
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateZoomWindowForStabilization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::UpdateZoomWindowForStabilization(
    CHIRectangle*   pCropInfo,
    UINT32*         pAdjustedFullWidth,
    UINT32*         pAdjustedFullHeight,
    UINT64          requestId)
{
    //< Update Crop info if EIS is enabled. firmware expects crop window wrt window after margin crop (ICA1 output)
    if ((NULL != pCropInfo) &&
        (NULL != pAdjustedFullWidth) &&
        (NULL != pAdjustedFullHeight) &&
        ((0   != (IPEStabilizationType::IPEStabilizationTypeEIS2 & m_instanceProperty.stabilizationType)) ||
        (0    != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType))))
    {
        UINT32 fullInputWidth  = *pAdjustedFullWidth;
        UINT32 fullInputHeight = *pAdjustedFullHeight;

        GetSizeWithoutStablizationMargin(fullInputWidth, fullInputHeight, pAdjustedFullWidth, pAdjustedFullHeight);

        FLOAT cropFactor            = static_cast<FLOAT>(pCropInfo->height) / fullInputHeight;
        FLOAT cropFactorOffsetLeft  = static_cast<FLOAT>(pCropInfo->left) / fullInputWidth;
        FLOAT cropFactorOffsetTop   = static_cast<FLOAT>(pCropInfo->top) / fullInputHeight;

        UINT32 adjustedAdditionalCropWidth  = *pAdjustedFullWidth  - m_additionalCropOffset.widthPixels;
        UINT32 adjustedAdditionalCropHeight = *pAdjustedFullHeight - m_additionalCropOffset.heightLines;

        pCropInfo->width  = static_cast<INT32>((adjustedAdditionalCropWidth  * cropFactor));
        pCropInfo->height = static_cast<INT32>((adjustedAdditionalCropHeight * cropFactor));
        pCropInfo->left   = static_cast<INT32>((adjustedAdditionalCropWidth  * cropFactorOffsetLeft) +
                                               (m_additionalCropOffset.widthPixels / 2));
        pCropInfo->top    = static_cast<INT32>((adjustedAdditionalCropHeight * cropFactorOffsetTop) +
                                               (m_additionalCropOffset.heightLines / 2));

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ZDBG: After IPE ICA1 Zoom Window [%d, %d, %d, %d] full %d x %d "
                         "crop_factor %f, leftOffsetCropFactor %f, topOffsetCropFactor %f, Stabilization type %d,"
                         "additional crop w %d h %d request ID %llu",
                         pCropInfo->left,
                         pCropInfo->top,
                         pCropInfo->width,
                         pCropInfo->height,
                         *pAdjustedFullWidth,
                         *pAdjustedFullHeight,
                         cropFactor,
                         cropFactorOffsetLeft,
                         cropFactorOffsetTop,
                         m_instanceProperty.stabilizationType,
                         m_additionalCropOffset.widthPixels,
                         m_additionalCropOffset.heightLines,
                         requestId);
    }
    else if ((NULL != pCropInfo) &&
             (NULL != pAdjustedFullWidth) &&
             (NULL != pAdjustedFullHeight) &&
             ((0   != (IPEStabilizationType::IPEStabilizationTypeSWEIS2 & m_instanceProperty.stabilizationType)) ||
             (0    != (IPEStabilizationType::IPEStabilizationTypeSWEIS3 & m_instanceProperty.stabilizationType))))
    {
        FLOAT cropFactor            = static_cast<FLOAT>(pCropInfo->height) / *pAdjustedFullHeight;
        FLOAT cropFactorOffsetLeft  = static_cast<FLOAT>(pCropInfo->left) / *pAdjustedFullWidth;
        FLOAT cropFactorOffsetTop   = static_cast<FLOAT>(pCropInfo->top) / *pAdjustedFullHeight;

        UINT32 adjustedAdditionalCropWidth  = *pAdjustedFullWidth  - m_additionalCropOffset.widthPixels;
        UINT32 adjustedAdditionalCropHeight = *pAdjustedFullHeight - m_additionalCropOffset.heightLines;

        pCropInfo->width  = static_cast<INT32>((adjustedAdditionalCropWidth  * cropFactor));
        pCropInfo->height = static_cast<INT32>((adjustedAdditionalCropHeight * cropFactor));

        pCropInfo->width  = Utils::FloorUINT32(16, pCropInfo->width);
        pCropInfo->height = Utils::FloorUINT32(16, pCropInfo->height);

        pCropInfo->left   = static_cast<INT32>((adjustedAdditionalCropWidth  * cropFactorOffsetLeft));
        pCropInfo->top    = static_cast<INT32>((adjustedAdditionalCropHeight * cropFactorOffsetTop));

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ZDBG: Zoom Window [%d, %d, %d, %d] full %d x %d "
                         "crop_factor %f, leftOffsetCropFactor %f, topOffsetCropFactor %f, Stabilization type %d,"
                         " request ID %llu",
                         pCropInfo->left,
                         pCropInfo->top,
                         pCropInfo->width,
                         pCropInfo->height,
                         *pAdjustedFullWidth,
                         *pAdjustedFullHeight,
                         cropFactor,
                         cropFactorOffsetLeft,
                         cropFactorOffsetTop,
                         m_instanceProperty.stabilizationType,
                         requestId);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillFrameBufferData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillFrameBufferData(
    CmdBuffer*      pMainCmdBuffer,
    ImageBuffer*    pImageBuffer,
    UINT32          payloadBatchFrameIdx,
    UINT32          bufferBatchFrameIdx,
    UINT32          portId)
{
    CamxResult      result       = CamxResultSuccess;
    SIZE_T          planeOffset  = 0;
    SIZE_T          metadataSize = 0;
    CSLMemHandle    hMem;
    UINT32          numPlanes;

    numPlanes = pImageBuffer->GetNumberOfPlanes();

    if (IPEMaxSupportedBatchSize > payloadBatchFrameIdx)
    {
        // Prepare Patching struct for smmu addresses
        for (UINT32 i = 0; i < numPlanes; i++)
        {
            if (1 < pImageBuffer->GetNumFramesInBatch())
            {
                // For super buffer output from IFE, IPE node shall get new frame offset within same buffer.
                pImageBuffer->GetPlaneCSLMemHandle(bufferBatchFrameIdx, i, &hMem, &planeOffset, &metadataSize);
            }
            else
            {
                // For IPE video port,
                //     batch mode is enabled on link but instead of super buffer, these are individual HAL buffers.
                pImageBuffer->GetPlaneCSLMemHandle(0, i, &hMem, &planeOffset, &metadataSize);
            }

            const ImageFormat* pFormat = pImageBuffer->GetFormat();
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //  Following is the memory layout of UBWC formats. Pixel data is located after Meta data.                        //
            //                                                                                                                //
            //   4K aligned  Address -->  ----------------------------                                                        //
            //                            |  Y  - Meta Data Plane    |                                                        //
            //                            ----------------------------                                                        //
            //                            |  Y  - Pixel Data Plane   |                                                        //
            //   4K aligned  Address -->  ----------------------------                                                        //
            //                            |  UV - Meta Data Plane    |                                                        //
            //                            ----------------------------                                                        //
            //                            |  UV - Pixel Data Plane   |                                                        //
            //                            ----------------------------                                                        //
            //                                                                                                                //
            //  So, metadata Size for the plane needs to be added to get the Pixel data address.                              //
            //  Note for Linear formats metadataSize should be 0.                                                             //
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            if (TRUE == ImageFormatUtils::IsUBWC(pFormat->format))
            {
                result = pMainCmdBuffer->AddNestedBufferInfo(
                    s_frameBufferOffset[payloadBatchFrameIdx][portId].metadataBufferPtr[i],
                    hMem,
                    static_cast <UINT32>(planeOffset));

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Error in patching address portId %d plane %d", portId, i);
                    break;
                }
            }

            result = pMainCmdBuffer->AddNestedBufferInfo(
                s_frameBufferOffset[payloadBatchFrameIdx][portId].bufferPtr[i],
                hMem,
                (static_cast <UINT32>(planeOffset) + static_cast <UINT32>(metadataSize)));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Error in patching address portId %d plane %d", portId, i);
                break;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Batch frame index %d is greater than IPEMaxSupportedBatchSize %d",
                       payloadBatchFrameIdx,
                       IPEMaxSupportedBatchSize);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillInputFrameSetData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillInputFrameSetData(
    CmdBuffer*      pFrameProcessCmdBuffer,
    UINT            portId,
    ImageBuffer*    pImageBuffer,
    UINT32          numFramesInBuffer)
{
    CamxResult result = CamxResultSuccess;

    for (UINT32 batchedFrameIndex = 0; batchedFrameIndex < numFramesInBuffer; batchedFrameIndex++)
    {
        result = FillFrameBufferData(pFrameProcessCmdBuffer,
                                     pImageBuffer,
                                     batchedFrameIndex,
                                     batchedFrameIndex,
                                     portId);
        if (CamxResultSuccess != result)
        {
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateOutputBufferConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::UpdateOutputBufferConfig(
    IpeFrameProcessData*            pFrameProcessData,
    CmdBuffer*                      pFrameProcessCmdBuffer,
    PerRequestOutputPortInfo*       pOutputPort,
    BOOL                            HALOutputBufferCombined,
    UINT64                          requestId,
    UINT32                          portIndex)
{
    CamxResult    result          = CamxResultSuccess;
    UINT32        numBuffers      = pOutputPort->numOutputBuffers;
    BOOL          requestUBWCMeta = FALSE;
    ImageBuffer*  pImageBuffer    = NULL;

    if (CamxResultSuccess == result)
    {
        if (FALSE == HALOutputBufferCombined)
        {
            for (UINT bufferIndex = 0; bufferIndex < numBuffers; bufferIndex++)
            {
                pImageBuffer                = pOutputPort->ppImageBuffer[bufferIndex];
                const ImageFormat* pFormat  = NULL;
                if (NULL != pImageBuffer)
                {
                    pFormat = pImageBuffer->GetFormat();
                }

                if (NULL != pFormat)
                {
                    if (CamxResultSuccess == result)
                    {
                        result = FillOutputFrameSetData(pFrameProcessCmdBuffer,
                            pOutputPort->portId,
                            pImageBuffer,
                            bufferIndex);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fill output buffer frame data result=%d", result);
                            break;
                        }

                        CAMX_LOG_INFO(CamxLogGroupPProc,
                            "node %s reporting I/O config,portId=%d,imgBuf=0x%x,wxh=%dx%d,hFence=%d,request=%llu",
                            NodeIdentifierString(),
                            pOutputPort->portId,
                            pImageBuffer,
                            pFormat->width,
                            pFormat->height,
                            *(pOutputPort->phFence),
                            requestId);
                    }

                    if ((CamxResultSuccess == result) &&
                        (TRUE == m_realTimeIPE) &&
                        (TRUE != requestUBWCMeta) &&
                        (NULL != pFormat && TRUE == ImageFormatUtils::IsUBWC(pFormat->format)) &&
                        (FALSE == IsSecureMode()))
                    {
                        CSLBufferInfo  UBWCStatsBuf = { 0 };
                        if (CamxResultSuccess == GetUBWCStatBuffer(&UBWCStatsBuf, requestId))
                        {
                            // UBWC meta is obtained for all the outport with a single command buffer request
                            requestUBWCMeta = TRUE;
                            result = FillFrameUBWCParams(pFrameProcessCmdBuffer,
                                pFrameProcessData,
                                &UBWCStatsBuf,
                                pOutputPort,
                                portIndex);
                        }
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Output Port/Image is Null ", __FUNCTION__);
                    result = CamxResultEInvalidArg;
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Output Port: UpdateOutputBufferConfig failed for port %d",
                        NodeIdentifierString(), pOutputPort->portId);
                    break;
                }
            }
        }
        else
        {
            UINT32 numFramesInBuffer    = 1;
            pImageBuffer                = pOutputPort->ppImageBuffer[0];
            const ImageFormat* pFormat  = NULL;
            if (NULL != pImageBuffer)
            {
                pFormat = pImageBuffer->GetFormat();
            }

            if (NULL != pFormat)
            {
                if (CamxResultSuccess == result)
                {
                    if (TRUE == pImageBuffer->IsBatchImageFormat())
                    {
                        numFramesInBuffer = pFrameProcessData->numFrameSetsInBatch;
                    }
                    for (UINT32 batchedFrameIndex = 0; batchedFrameIndex < numFramesInBuffer; batchedFrameIndex++)
                    {
                        result = FillOutputFrameSetData(pFrameProcessCmdBuffer,
                            pOutputPort->portId,
                            pImageBuffer,
                            batchedFrameIndex);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fill output buffer frame data result=%d", result);
                            break;
                        }
                    }

                    CAMX_LOG_INFO(CamxLogGroupPProc,
                        "node %s reporting I/O config,portId=%d,imgBuf=0x%x,wxh=%dx%d,hFence=%d,request=%llu",
                        NodeIdentifierString(),
                        pOutputPort->portId,
                        pImageBuffer,
                        pFormat->width,
                        pFormat->height,
                        *(pOutputPort->phFence),
                        requestId);
                }

                if ((CamxResultSuccess == result) &&
                    (TRUE == m_realTimeIPE) &&
                    (TRUE != requestUBWCMeta) &&
                    (NULL != pFormat && TRUE == ImageFormatUtils::IsUBWC(pFormat->format)) &&
                    (FALSE == IsSecureMode()))
                {
                    CSLBufferInfo  UBWCStatsBuf = { 0 };
                    if (CamxResultSuccess == GetUBWCStatBuffer(&UBWCStatsBuf, requestId))
                    {
                        // UBWC meta is obtained for all the outport with a single command buffer request
                        requestUBWCMeta = TRUE;
                        result = FillFrameUBWCParams(pFrameProcessCmdBuffer,
                            pFrameProcessData,
                            &UBWCStatsBuf,
                            pOutputPort,
                            portIndex);
                    }
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Output Port/Image is Null ", __FUNCTION__);
                result = CamxResultEInvalidArg;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillOutputFrameSetData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillOutputFrameSetData(
    CmdBuffer*              pFrameProcessCmdBuffer,
    UINT                    portId,
    ImageBuffer*            pImageBuffer,
    UINT32                  numFramesInBuffer)
{
    CamxResult              result = CamxResultSuccess;

    result = FillFrameBufferData(pFrameProcessCmdBuffer,
                                 pImageBuffer,
                                 numFramesInBuffer,
                                 numFramesInBuffer,
                                 portId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::InitializeUBWCStatsBuf
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::InitializeUBWCStatsBuf()
{
    CamxResult result        = CamxResultSuccess;
    BOOL       UBWCStatsFlag = FALSE;
    UINT       numOutputPort = 0;
    UINT32     bufferOffset  = 0;
    UINT       outputPortId[IPEMaxOutput];

    // Get Output Port List
    GetAllOutputPortIds(&numOutputPort, &outputPortId[0]);

    for (UINT index = 0; index < numOutputPort; index++)
    {
        if ((IPEOutputPortVideo == outputPortId[index]) ||
            (IPEOutputPortDisplay == outputPortId[index]))
        {
            const ImageFormat* pImageFormat = GetOutputPortImageFormat(OutputPortIndex(outputPortId[index]));

            if ((NULL != pImageFormat) && (TRUE == IsSinkPortWithBuffer(OutputPortIndex(outputPortId[index]))) &&
                (TRUE == ImageFormatUtils::IsUBWC(pImageFormat->format)) &&
                (FALSE == IsSecureMode()))
            {
                UBWCStatsFlag = TRUE;
                break;
            }
        }
    }

    if ((TRUE == m_realTimeIPE) && (TRUE == UBWCStatsFlag))
    {
        CSLBufferInfo* pBufferInfo = NULL;

        pBufferInfo = static_cast<CSLBufferInfo*>(CAMX_CALLOC(sizeof(CSLBufferInfo)));
        if (NULL == pBufferInfo)
        {
            result = CamxResultENoMemory;
        }
        else
        {

            result = CSLAlloc(NodeIdentifierString(),
                              pBufferInfo,
                              sizeof(UbwcStatsBuffer) * MaxUBWCStatsBufferSize,
                              1,
                              (CSLMemFlagUMDAccess | CSLMemFlagSharedAccess | CSLMemFlagHw),
                              &DeviceIndices()[0],
                              1);
        }

        if (CamxResultSuccess == result)
        {
            for (UINT count = 0; count < MaxUBWCStatsBufferSize; count++)
            {
                m_UBWCStatBufInfo.pUBWCStatsBuffer = pBufferInfo;
                m_UBWCStatBufInfo.offset[count]    = bufferOffset;
                bufferOffset                      += sizeof(UbwcStatsBuffer);

            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::PostPipelineCreate()
{
    CamxResult      result                  = CamxResultSuccess;
    UINT32          memFlags                = (CSLMemFlagUMDAccess | CSLMemFlagHw);
    ResourceParams  params                  = { 0 };
    BOOL            bOverrideScaleProfile   = FALSE;

    IQInterface::IQSetHardwareVersion(
        static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion(),
        static_cast<Titan17xContext *>(GetHwContext())->GetHwVersion());

    UINT32 durationINFrames =
        static_cast<Titan17xContext*>(GetHwContext())->GetTitan17xSettingsManager()->
        GetTitan17xStaticSettings()->IPEDurationInNumFrames;

    // Needs to be called early, since CheckIsIPERealtime() checks IsScalerOnlyIPE()
    bOverrideScaleProfile = GetOverrideScaleProfile();

    if (m_instanceProperty.profileId == IPEProfileId::IPEProfileIdScale)
    {
        if (TRUE == bOverrideScaleProfile)
        {
            m_instanceProperty.profileId = IPEProfileId::IPEProfileIdDefault;
            CAMX_LOG_INFO(CamxLogGroupPProc, "IPE Node %s IPEProfileIdScale is overriden by the vendor tag",
                NodeIdentifierString(), m_instanceProperty.profileId);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE Node %s IPEProfileIdScale is NOT overriden by the vendor tag",
                NodeIdentifierString(), m_instanceProperty.profileId);
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupPProc, "IPE Node %s IPEProfileId is not in Scale Profile %d",
            NodeIdentifierString(), m_instanceProperty.profileId);
    }

    m_IPETimeoutInNumFrames = (TRUE == m_realTimeIPE) ?
        // first 16 bits correspond to delay for realtime streams
        (durationINFrames & 0xFFFF) :
        // next 16 bits correspond to delay for non realtime streams
        ((durationINFrames & 0xFFFF0000) >> 16);

    // Assemble IPE IQ Modules
    result = CreateIPEIQModules();

    m_tuningData.noOfSelectionParameter = 1;
    m_tuningData.TuningMode[0].mode     = ChiModeType::Default;

    m_IPECmdBlobCount = GetPipeline()->GetRequestQueueDepth();

    CAMX_LOG_INFO(CamxLogGroupPProc, "IPE Instance ID %d numbufs %d", InstanceID(), m_IPECmdBlobCount);

    SetSrParams();

    if (CamxResultSuccess == result)
    {
        UpdateIQCmdSize();
        result = InitializeCmdBufferManagerList(IPECmdBufferMaxIds);
    }

    if (CamxResultSuccess == result)
    {
        params.usageFlags.packet               = 1;
        // 1 Command Buffer for all the IQ Modules
        // 2 KMD command buffer
        params.packetParams.maxNumCmdBuffers   = CSLIPECmdBufferMaxIds;

        // 8 Input and 6 Outputs
        params.packetParams.maxNumIOConfigs    = IPEMaxInput + IPEMaxOutput;
        params.packetParams.enableAddrPatching = 1;
        params.packetParams.maxNumPatches      = IPEMaxPatchAddress;
        params.resourceSize                    = Packet::CalculatePacketSize(&params.packetParams);
        params.memFlags                        = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
        params.pDeviceIndices                  = &m_deviceIndex;
        params.numDevices                      = 1;

        // Same number as cmd buffers
        params.poolSize                        = m_IPECmdBlobCount * params.resourceSize;
        params.alignment                       = CamxPacketAlignmentInBytes;

        result = CreateCmdBufferManager("IQPacketManager", &params, &m_pIQPacketManager);


        if (CamxResultSuccess == result)
        {
            ResourceParams               resourceParams[IPEMaxFWCmdBufferManagers];
            CHAR                         bufferManagerName[IPEMaxFWCmdBufferManagers][MaxStringLength256];
            struct CmdBufferManagerParam createParam[IPEMaxFWCmdBufferManagers];
            UINT32                       numberOfBufferManagers = 0;

            FillCmdBufferParams(&params,
                                CmdBufferGenericBlobSizeInBytes,
                                CmdType::Generic,
                                CSLMemFlagUMDAccess | CSLMemFlagKMDAccess,
                                0,
                                NULL,
                                m_IPECmdBlobCount);
            result = CreateCmdBufferManager("CmdBufferGenericBlob", &params, &m_pIPECmdBufferManager[CmdBufferGenericBlob]);
            if (CamxResultSuccess == result)
            {
                if (m_maxCmdBufferSizeBytes[CmdBufferPreLTM] > 0)
                {
                    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                                        m_maxCmdBufferSizeBytes[CmdBufferPreLTM],
                                        CmdType::CDMDirect,
                                        CSLMemFlagUMDAccess,
                                        IPEMaxPreLTMPatchAddress,
                                        &m_deviceIndex,
                                        m_IPECmdBlobCount);
                    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                                      sizeof(CHAR) * MaxStringLength256,
                                      "CBM_%s_%s",
                                      NodeIdentifierString(),
                                      "CmdBufferPreLTM");
                    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
                    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
                    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pIPECmdBufferManager[CmdBufferPreLTM];
                    numberOfBufferManagers++;
                }

                if (m_maxCmdBufferSizeBytes[CmdBufferPostLTM] > 0)
                {
                    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                                        m_maxCmdBufferSizeBytes[CmdBufferPostLTM],
                                        CmdType::CDMDirect,
                                        CSLMemFlagUMDAccess,
                                        IPEMaxPostLTMPatchAddress,
                                        &m_deviceIndex,
                                        m_IPECmdBlobCount);

                    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                                      sizeof(CHAR) * MaxStringLength256,
                                      "CBM_%s_%s",
                                      NodeIdentifierString(),
                                      "CmdBufferPostLTM");
                    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
                    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
                    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pIPECmdBufferManager[CmdBufferPostLTM];
                    numberOfBufferManagers++;
                }

                if (m_maxCmdBufferSizeBytes[CmdBufferDMIHeader] > 0)
                {
                    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                                        m_maxCmdBufferSizeBytes[CmdBufferDMIHeader],
                                        CmdType::CDMDMI,
                                        CSLMemFlagUMDAccess,
                                        IPEMaxDMIPatchAddress,
                                        &m_deviceIndex,
                                        m_IPECmdBlobCount);

                    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                                      sizeof(CHAR) * MaxStringLength256,
                                      "CBM_%s_%s",
                                      NodeIdentifierString(),
                                      "CmdBufferDMIHeader");
                    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
                    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
                    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pIPECmdBufferManager[CmdBufferDMIHeader];
                    numberOfBufferManagers++;
                }

                if (0 != numberOfBufferManagers)
                {
                    result = CreateMultiCmdBufferManager(createParam, numberOfBufferManagers);
                }
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Cmd Buffer Creation failed result %d", result);
            }
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("%s: Failed to Creat Cmd Buffer Manager", __FUNCTION__);
        }
    }

    if (CamxResultSuccess != result)
    {
        Cleanup();
    }

    if (CamxResultSuccess == result)
    {
        // Save required static metadata
        GetStaticMetadata();
    }

    if (CamxResultSuccess == result)
    {
        /// @todo (CAMX-738) Find input port dimensions/format from metadata / use case pool and do acquire.
        result = AcquireDevice();
    }

    if (CamxResultSuccess == result)
    {
        if (FALSE == m_scratchBufferPortEnabled)
        {
            result = AllocateScratchBuffers();
        }
        // Allocate buffer for UBWC stats
        if (CamxResultSuccess == result)
        {
            result = InitializeUBWCStatsBuf();
        }

        if (CamxResultSuccess == result)
        {
            UINT32         numberOfMappings = 0;
            CSLBufferInfo  bufferInfo = { 0 };
            CSLBufferInfo* pBufferInfo[CSLICPMaxMemoryMapRegions];

            if (NULL != m_pIPECmdBufferManager[CmdBufferFrameProcess])
            {
                if (NULL != m_pIPECmdBufferManager[CmdBufferFrameProcess]->GetMergedCSLBufferInfo())
                {
                    Utils::Memcpy(&bufferInfo,
                                  m_pIPECmdBufferManager[CmdBufferFrameProcess]->GetMergedCSLBufferInfo(),
                                  sizeof(CSLBufferInfo));
                    pBufferInfo[numberOfMappings] = &bufferInfo;
                    numberOfMappings++;

                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to get Merged CSL Buffer Info");
                    result = CamxResultEFailed;
                }
            }

            if (NULL != m_UBWCStatBufInfo.pUBWCStatsBuffer)
            {
                pBufferInfo[numberOfMappings] = m_UBWCStatBufInfo.pUBWCStatsBuffer;
                numberOfMappings++;
            }

            if (0 != numberOfMappings)
            {
                result = SendFWCmdRegionInfo(CSLICPGenericBlobCmdBufferMapFWMemRegion,
                                             pBufferInfo,
                                             numberOfMappings);
            }
        }
    }

    m_dumpGeolibResult = GetStaticSettings()->dumpGeolibResult;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::CheckDimensionRequirementsForIPEDownscaler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPENode::CheckDimensionRequirementsForIPEDownscaler(
    UINT32 width,
    UINT32 height,
    UINT   downScalarMode)
{
    BOOL    result          = TRUE;
    UINT32  referenceWidth  = 0;
    UINT32  referenceHeight = 0;
    FLOAT   aspectRatio     = static_cast<FLOAT>(width) / height;

    if (aspectRatio <= 1.0f)
    {
        referenceWidth  = 1440;
        referenceHeight = 1440;
    }
    else if ((aspectRatio > 1.0f) && (aspectRatio <= 4.0f/3.0f))
    {
        referenceWidth  = 1920;
        referenceHeight = 1440;
    }
    else if ((aspectRatio > 4.0f/3.0f) && (aspectRatio <= 16.0f/9.0f))
    {
        referenceWidth  = 1920;
        referenceHeight = 1080;
    }
    else if ((aspectRatio > (18.5 / 9 + 0.1)) && (aspectRatio <= (19.9 / 9 + 0.1)))
    {
        referenceWidth  = 2288;
        referenceHeight = 1080;
    }
    else
    {   // (aspectRatio > 16.0f/9.0f)
        referenceWidth  = 1920;
        referenceHeight = 1440;
    }

    if (IPEMidDownScalarMode == downScalarMode)
    {
        referenceWidth  = IPEMidDownScalarWidth;
        referenceHeight = IPEMidDownScalarHeight;
    }
    else if (IPECustomDownScalarMode == downScalarMode)
    {
        referenceWidth  = m_instanceProperty.ipeDownscalerWidth;
        referenceHeight = m_instanceProperty.ipeDownscalerHeight;
    }

    if ((width >= referenceWidth) && (height >= referenceHeight))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Streams dim %dx%d bigger than ref dims %dx%d",
            width, height, referenceWidth, referenceHeight);
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::IsStandardAspectRatio
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPENode::IsStandardAspectRatio(
    FLOAT aspectRatio)
{
    BOOL    result          = TRUE;

    // Use a reduced precision for comparing aspect ratios as updating dimensions should not be very sensitive to small
    // differences in aspect ratios
    if (Utils::FEqualCoarse(aspectRatio, 1.0f)     ||
        Utils::FEqualCoarse(aspectRatio, 4.0f/3.0f)   ||
        Utils::FEqualCoarse(aspectRatio, 16.0f/9.0f)  ||
        Utils::FEqualCoarse(aspectRatio, 18.5f/9.0f) ||
        Utils::FEqualCoarse(aspectRatio, 19.9f / 9.0f) ||
        Utils::FEqualCoarse(aspectRatio, 18.0f/9.0f))
    {
        result = TRUE;
    }
    else
    {
        result = FALSE;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetIPEDownscalerOnlyDimensions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::GetIPEDownscalerOnlyDimensions(
    UINT32  width,
    UINT32  height,
    UINT32* pMaxWidth,
    UINT32* pMaxHeight,
    FLOAT   downscaleLimit,
    UINT    downScalarMode)
{
    FLOAT heightRatio   = 0.0f;
    FLOAT widthRatio    = 0.0f;
    FLOAT aspectRatio   = static_cast<FLOAT>(width) / height;

    if (aspectRatio <= 1.0f)
    {
        *pMaxWidth  = 1440;
        *pMaxHeight = 1440;
    }
    else if ((aspectRatio > 1.0f) && (aspectRatio <= 4.0f/3.0f))
    {
        *pMaxWidth  = 1920;
        *pMaxHeight = 1440;
    }
    else if ((aspectRatio > 4.0f/3.0f) && (aspectRatio <= 16.0f/9.0f))
    {
        *pMaxWidth  = 1920;
        *pMaxHeight = 1080;
    }
    else if ((aspectRatio > (18.5 / 9 + 0.1)) && (aspectRatio <= (19.9 / 9 + 0.1)))
    {
        *pMaxWidth  = 2288;
        *pMaxHeight = 1080;
    }
    else
    {   // (aspectRatio > 16.0f/9.0f)
        *pMaxWidth  = 1920;
        *pMaxHeight = 1440;
    }

    if (IPEMidDownScalarMode == downScalarMode)
    {
        *pMaxWidth  = IPEMidDownScalarWidth;
        *pMaxHeight = IPEMidDownScalarHeight;
    }
    else if (IPECustomDownScalarMode == downScalarMode)
    {
        *pMaxWidth  = m_instanceProperty.ipeDownscalerWidth;
        *pMaxHeight = m_instanceProperty.ipeDownscalerHeight;
    }

    widthRatio  = *reinterpret_cast<FLOAT*>(pMaxWidth) / width;
    heightRatio = *reinterpret_cast<FLOAT*>(pMaxHeight) / height;

    if ((widthRatio > downscaleLimit) ||
        (heightRatio > downscaleLimit))
    {
        *pMaxWidth  = width * static_cast<UINT32>(downscaleLimit);
        *pMaxHeight = height * static_cast<UINT32>(downscaleLimit);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE Downscaler resolution selected: %d X %d", *pMaxWidth, *pMaxHeight);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::IsIPEOnlyDownscalerEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPENode::IsIPEOnlyDownscalerEnabled(
    BufferNegotiationData* pBufferNegotiationData)
{
    BOOL    isDimensionRequirementValid = FALSE;

    if ((0 != m_instanceProperty.ipeOnlyDownscalerMode) ||
        (TRUE == GetStaticSettings()->enableIPEOnlyDownscale))
    {
        for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
        {
            OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
            for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
            {
                BufferRequirement* pInputPortRequirement = &pOutputPortNegotiationData->inputPortRequirement[inputIndex];
                isDimensionRequirementValid =
                    CheckDimensionRequirementsForIPEDownscaler(pInputPortRequirement->optimalWidth,
                                                               pInputPortRequirement->optimalHeight,
                                                               m_instanceProperty.ipeOnlyDownscalerMode);
            }
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE Downscaler only not enabled");
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE Downscaler enabled: %d", isDimensionRequirementValid);

    return isDimensionRequirementValid;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CAMX_ASSERT(NULL != pBufferNegotiationData);

    CamxResult          result                         = CamxResultSuccess;
    UINT32              optimalInputWidth              = 0;
    UINT32              optimalInputHeight             = 0;
    UINT32              maxInputWidth                  = 0xffff;
    UINT32              maxInputHeight                 = 0xffff;
    UINT32              minInputWidth                  = 0;
    UINT32              minInputHeight                 = 0;
    UINT32              perOutputPortOptimalWidth      = 0;
    UINT32              perOutputPortOptimalHeight     = 0;
    UINT32              perOutputPortMinWidth          = 0;
    UINT32              perOutputPortMinHeight         = 0;
    UINT32              perOutputPortMaxWidth          = 0xffff;
    UINT32              perOutputPortMaxHeight         = 0xffff;
    FLOAT               perOutputPortAspectRatio       = 0.0f;
    FLOAT               inputAspectRatio               = 0.0f;
    FLOAT               optimalAspectRatio             = 0.0f;
    FLOAT               selectedAspectRatio            = 0.0f;
    FLOAT               prevOutputPortAspectRatio      = 0.0f;
    const ImageFormat*  pFormat                        = NULL;
    FLOAT               upscaleLimit                   = 1.0;
    FLOAT               downscaleLimit                 = 1.0;
    UINT32              IPEMaxInputWidth               = 0;
    UINT32              IPEMaxInputHeight              = 0;
    UINT32              IPEMinInputWidthLimit          = 0;
    UINT32              IPEMinInputHeightLimit         = 0;
    BOOL                isIPEDownscalerEnabled         = FALSE;
    const FLOAT         FFOV_PER                       = 0.06f;
    AlignmentInfo       alignmentLCM[FormatsMaxPlanes] = { {0} };
    CHIDimension        stabilizedOutputDimensions     = { 0 };
    UINT32              sensorOutputWidth              = 0;
    UINT32              sensorOutputHeight             = 0;
    FLOAT               sensorAspectRatio              = 0.0f;

    CAMX_ASSERT(NULL != pBufferNegotiationData);

    // Update IPE IO capability info based on ports enabled
    result = UpdateIPEIOLimits(pBufferNegotiationData);
    if (result != CamxResultSuccess)
    {
        CAMX_LOG_INFO(CamxLogGroupPProc, "IPE:%d Unable to update the capability", InstanceID());
    }

    /// @todo (CAMX-2013) Read ICA if enabled and take respective IO limits
    IPEMaxInputWidth       = m_capability.maxInputWidth[ICA_MODE_DISABLED];
    IPEMaxInputHeight      = m_capability.maxInputHeight[ICA_MODE_DISABLED];
    IPEMinInputWidthLimit  = m_capability.minInputWidth;
    IPEMinInputHeightLimit = m_capability.minInputHeight;

    // Get Sensor Aspect Ratio
    const UINT sensorInfoTag[] =
    {
        StaticSensorInfoActiveArraySize,
    };

    const UINT length         = CAMX_ARRAY_SIZE(sensorInfoTag);
    VOID*      pData[length]  = { 0 };
    UINT64     offset[length] = { 0 };

    result = GetDataList(sensorInfoTag, pData, offset, length);

    if (CamxResultSuccess == result)
    {
        if (NULL != pData[0])
        {
            Region region      = *static_cast<Region*>(pData[0]);
            sensorOutputWidth  = region.width;
            sensorOutputHeight = region.height;
            if (sensorOutputHeight != 0)
            {
                sensorAspectRatio =
                    static_cast<FLOAT>(sensorOutputWidth) / static_cast<FLOAT>(sensorOutputHeight);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Sensor Dimensions");
            }
        }
    }

    // The IPE node will have to loop through all the output ports which are connected to a child node or a HAL target.
    // The input buffer requirement will be the super resolution after looping through all the output ports.
    // The super resolution may have different aspect ratio compared to the original output port aspect ratio, but
    // this will be taken care of by the crop hardware associated with the output port.
    UINT isUBWCFormat = 0;
    UINT IPEmainout   = 0;

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        UINT                       outputPortId               = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);

        if (TRUE == IPEInstanceNeedsBufferNegotiation(outputPortId))
        {
            perOutputPortOptimalWidth  = 0;
            perOutputPortOptimalHeight = 0;
            perOutputPortMinWidth      = 0;
            perOutputPortMinHeight     = 0;
            perOutputPortMaxWidth      = 0xffff;
            perOutputPortMaxHeight     = 0xffff;
            perOutputPortAspectRatio   = 0.0f;

            if ((outputPortId == IPEOutputPortDisplay) || (outputPortId == IPEOutputPortVideo))
            {
                IPEmainout++;
            }
            pFormat        = static_cast<const ImageFormat *>
                (&pBufferNegotiationData->pOutputPortNegotiationData[index].pFinalOutputBufferProperties->imageFormat);
            isUBWCFormat   = ImageFormatUtils::IsUBWC(pFormat->format);

            upscaleLimit   = m_capability.maxUpscale[isUBWCFormat];
            downscaleLimit = m_capability.maxDownscale[isUBWCFormat];

            Utils::Memset(&pOutputPortNegotiationData->outputBufferRequirementOptions, 0, sizeof(BufferRequirement));

            // Go through the requirements of the input ports connected to the output port
            for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
            {
                BufferRequirement* pInputPortRequirement = &pOutputPortNegotiationData->inputPortRequirement[inputIndex];
                /// @todo (CAMX-2013) take into account aspect ratio and format as well during negotiation.
                // Take the max of the min dimensions, min of the max dimensions and the
                // max of the optimal dimensions
                perOutputPortOptimalWidth  = Utils::MaxUINT32(perOutputPortOptimalWidth, pInputPortRequirement->optimalWidth);
                perOutputPortOptimalHeight = Utils::MaxUINT32(perOutputPortOptimalHeight, pInputPortRequirement->optimalHeight);

                perOutputPortMinWidth  = Utils::MaxUINT32(perOutputPortMinWidth, pInputPortRequirement->minWidth);
                perOutputPortMinHeight = Utils::MaxUINT32(perOutputPortMinHeight, pInputPortRequirement->minHeight);

                perOutputPortMaxWidth  = Utils::MinUINT32(perOutputPortMaxWidth, pInputPortRequirement->maxWidth);
                perOutputPortMaxHeight = Utils::MinUINT32(perOutputPortMaxHeight, pInputPortRequirement->maxHeight);

                inputAspectRatio = static_cast<FLOAT>(pInputPortRequirement->optimalWidth) /
                                       pInputPortRequirement->optimalHeight;
                perOutputPortAspectRatio = Utils::MaxFLOAT(perOutputPortAspectRatio, inputAspectRatio);

                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d, Output port %d, Idx:%d In=%dx%d Opt:%dx%d inAR:%f peroutAR:%f",
                    InstanceID(), outputPortId, inputIndex, pInputPortRequirement->optimalWidth,
                    pInputPortRequirement->optimalHeight, perOutputPortOptimalWidth, perOutputPortOptimalHeight,
                    inputAspectRatio, perOutputPortAspectRatio);

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

            // Store the buffer requirements for this output port which will be reused to set, during forward walk.
            // The values stored here could be final output dimensions unless it is overridden by forward walk.

            // Optimal dimension should lie between the min and max, ensure the same.
            // There is a chance of the Optimal dimension going over the max dimension.
            // Correct the same.
            perOutputPortOptimalWidth =
                Utils::ClampUINT32(perOutputPortOptimalWidth, perOutputPortMinWidth, perOutputPortMaxWidth);
            perOutputPortOptimalHeight =
                Utils::ClampUINT32(perOutputPortOptimalHeight, perOutputPortMinHeight, perOutputPortMaxHeight);

            // This current output port requires resolution that IPE cannot handle for UBWC format
            if ((0 != isUBWCFormat) &&
                ((perOutputPortOptimalWidth < m_capability.minOutputWidthUBWC) ||
                (perOutputPortOptimalHeight < m_capability.minOutputHeightUBWC)))
            {
                CAMX_LOG_WARN(CamxLogGroupPProc,
                              "IPE:%d unabled to handle resolution %dx%d with current format %d for output port %d ",
                              InstanceID(),
                              perOutputPortOptimalWidth,
                              perOutputPortOptimalHeight,
                              pFormat->format,
                              outputPortId);

                result = CamxResultEFailed;
                break; // break out of loop as IPE fails to work with current resolution and format
            }

            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth  = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight = perOutputPortOptimalHeight;
            pOutputPortNegotiationData->outputBufferRequirementOptions.minWidth      = perOutputPortMinWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.minHeight     = perOutputPortMinHeight;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxWidth      = perOutputPortMaxWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxHeight     = perOutputPortMaxHeight;

            Utils::Memcpy(&pOutputPortNegotiationData->outputBufferRequirementOptions.planeAlignment[0],
                          &alignmentLCM[0],
                          sizeof(AlignmentInfo) * FormatsMaxPlanes);

            Utils::Memset(&alignmentLCM[0], 0, sizeof(AlignmentInfo) * FormatsMaxPlanes);

            if (IPEStabilizationTypeEIS2  == (m_instanceProperty.stabilizationType & IPEStabilizationTypeEIS2)    ||
                (IPEStabilizationTypeEIS3 == (m_instanceProperty.stabilizationType & IPEStabilizationTypeEIS3)))
            {
                stabilizedOutputDimensions.width  = Utils::MaxUINT32(stabilizedOutputDimensions.width,
                                                                     perOutputPortOptimalWidth);
                stabilizedOutputDimensions.height = Utils::MaxUINT32(stabilizedOutputDimensions.height,
                                                                     perOutputPortOptimalHeight);
            }

            optimalInputWidth  = Utils::MaxUINT32(optimalInputWidth, perOutputPortOptimalWidth);
            optimalInputHeight = Utils::MaxUINT32(optimalInputHeight, perOutputPortOptimalHeight);

            FLOAT minInputLimitAspectRatio = static_cast<FLOAT>(IPEMinInputWidthLimit) /
                                                static_cast<FLOAT>(IPEMinInputHeightLimit);

            if (minInputLimitAspectRatio > perOutputPortAspectRatio)
            {
                IPEMinInputHeightLimit =
                    Utils::EvenFloorUINT32(static_cast<UINT32>(IPEMinInputWidthLimit / perOutputPortAspectRatio));
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Updated  IPEMinInputHeightLimit %d ",
                                   IPEMinInputHeightLimit);
            }
            else
            {
                IPEMinInputWidthLimit =
                    Utils::EvenFloorUINT32(static_cast<UINT32>(IPEMinInputHeightLimit * perOutputPortAspectRatio));
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Updated  IPEMinInputWidthLimit %d ",
                                   IPEMinInputWidthLimit);
            }

            optimalInputWidth  = Utils::MaxUINT32(IPEMinInputWidthLimit, optimalInputWidth);
            optimalInputHeight = Utils::MaxUINT32(IPEMinInputHeightLimit, optimalInputHeight);

            optimalAspectRatio = static_cast<FLOAT>(optimalInputWidth) / optimalInputHeight;

            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d Port %d, OptimalIn:%dx%d OptAR:%f perOutAR:%f",
                InstanceID(), outputPortId, optimalInputWidth, optimalInputHeight, optimalAspectRatio,
                perOutputPortAspectRatio);

            if ((0 != prevOutputPortAspectRatio) &&
                (FALSE == Utils::FEqualCoarse(perOutputPortAspectRatio, prevOutputPortAspectRatio)) &&
                (sensorAspectRatio != 0))
            {
                if (optimalAspectRatio > sensorAspectRatio)
                {
                    optimalInputHeight =
                        Utils::EvenFloorUINT32(static_cast<UINT32>(optimalInputWidth / sensorAspectRatio));
                    CAMX_LOG_INFO(CamxLogGroupPProc, "Change Opt AspectRatio %f to Sensor AR: %f, Updated Height %d",
                                                       optimalAspectRatio, sensorAspectRatio, optimalInputHeight);
                }
                else
                {
                    optimalInputWidth =
                        Utils::EvenFloorUINT32(static_cast<UINT32>(optimalInputHeight * sensorAspectRatio));
                    CAMX_LOG_INFO(CamxLogGroupPProc, "Change Opt AspectRatio %f to Sensor AR: %f, Updated Width %d",
                                                       optimalAspectRatio, sensorAspectRatio, optimalInputWidth);
                }
                selectedAspectRatio = sensorAspectRatio;
            }
            else
            {

                // Based on the various negotiations above it is possible that the optimal dimensions as input
                // to IPE could end up with an arbitrary aspect ratio. Hence make sure that the dimensions conform
                // to the maximum of the aspect ratio from the output dimensions. Assumption here is that the
                // output dimensions requested from IPE are proper. The dimensions are only adapted for the IPE input.
                if ((TRUE == IsStandardAspectRatio(optimalAspectRatio))                         ||
                    (TRUE == Utils::FEqualCoarse(optimalAspectRatio, perOutputPortAspectRatio)) ||
                    ((TRUE == IsMFProcessingType()) && (FALSE == IsPostfilterWithDefault())))
                {
                    // The dimensions are fine. Do nothing
                }
                else if (optimalAspectRatio > perOutputPortAspectRatio)
                {
                    if (perOutputPortAspectRatio != 0)
                    {
                        optimalInputHeight =
                            Utils::EvenFloorUINT32(static_cast<UINT32>(optimalInputWidth / perOutputPortAspectRatio));
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "NonConformant AspectRatio:%f Change Height %d using AR:%f",
                            optimalAspectRatio, optimalInputHeight, perOutputPortAspectRatio);
                    }
                }
                else
                {
                    optimalInputWidth =
                        Utils::EvenFloorUINT32(static_cast<UINT32>(optimalInputHeight * perOutputPortAspectRatio));
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "NonConformant AspectRatio:%f Change Width %d using AR:%f",
                        optimalAspectRatio, optimalInputWidth, perOutputPortAspectRatio);
                }
                selectedAspectRatio = perOutputPortAspectRatio;
            }
            prevOutputPortAspectRatio = perOutputPortAspectRatio;

            // Minimum IPE input dimension should be big enough to give the
            // max output required for a connected to one of IPE destination ports,
            // considering the upscale limitations.
            if ((FALSE == GetStaticSettings()->enableIPEUpscale) &&
                ((GetOutputPortId(pOutputPortNegotiationData->outputPortIndex) == IPEOutputPortVideo) ||
                (GetOutputPortId(pOutputPortNegotiationData->outputPortIndex) == IPEOutputPortDisplay)))
            {
                minInputHeight = Utils::MaxUINT32(minInputHeight,
                                                  static_cast<UINT32>(optimalInputHeight / 1.0f));
                minInputWidth  = Utils::MaxUINT32(minInputWidth,
                                                 static_cast<UINT32>(optimalInputWidth / 1.0f));
            }
            else
            {
                minInputHeight = Utils::MaxUINT32(minInputHeight,
                                                  static_cast<UINT32>(optimalInputHeight / upscaleLimit));
                minInputWidth  = Utils::MaxUINT32(minInputWidth,
                                                 static_cast<UINT32>(optimalInputWidth / upscaleLimit));
            }
            // Set the value at the minInputLimit of IPE if the current value is smaller than required.
            minInputWidth  = Utils::MaxUINT32(IPEMinInputWidthLimit, minInputWidth);
            minInputHeight = Utils::MaxUINT32(IPEMinInputHeightLimit, minInputHeight);

            // Maximum input dimension should be small enough to give the
            // min output required for a connected IPE destination port,
            // considering the downscale limitations.
            if (IsMFProcessingType())
            {
                // do no use the downscaleLimit
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Do not use the downscaleLimit");
            }
            else
            {
                maxInputHeight = Utils::MinUINT32(maxInputHeight,
                                                  static_cast<UINT32>(perOutputPortMinHeight * downscaleLimit));
                maxInputWidth  = Utils::MinUINT32(maxInputWidth,
                                                 static_cast<UINT32>(perOutputPortMinWidth * downscaleLimit));
            }

            // Cap the value at the IPE limitations if the current value is bigger than required.
            maxInputWidth  = Utils::MinUINT32(IPEMaxInputWidth, maxInputWidth);
            maxInputHeight = Utils::MinUINT32(IPEMaxInputHeight, maxInputHeight);
        }
        else if (TRUE == IsReferenceOutputPort(outputPortId))
        {
            // For certain pipelines IPE is operated with only Reference ouput ports and there won't
            // be any video or display ports configured then optimal res will be zero.
            // So, cap them to supported resolutions and avoid buffer negotiation failure.
            optimalInputWidth  = Utils::MaxUINT32(IPEMinInputWidthLimit, optimalInputWidth);
            optimalInputHeight = Utils::MaxUINT32(IPEMinInputHeightLimit, optimalInputHeight);

            minInputWidth      = Utils::MaxUINT32(IPEMinInputWidthLimit, minInputWidth);
            minInputHeight     = Utils::MaxUINT32(IPEMinInputHeightLimit, minInputHeight);

            maxInputWidth      = Utils::MinUINT32(IPEMaxInputWidth, maxInputWidth);
            maxInputHeight     = Utils::MinUINT32(IPEMaxInputHeight, maxInputHeight);
        }
    }

    if (CamxResultSuccess == result)
    {
        UINT metaTag = 0;
        if (0 != (m_instanceProperty.stabilizationType & IPEStabilizationTypeEIS2))
        {
            result = VendorTagManager::QueryVendorTagLocation(VendorTagSectionEISRealTimeConfig,
                                                              "StabilizedOutputDims",
                                                              &metaTag);
        }
        else if (0 != (m_instanceProperty.stabilizationType & IPEStabilizationTypeEIS3))
        {
            result = VendorTagManager::QueryVendorTagLocation(VendorTagSectionEISLookAheadConfig,
                                                              "StabilizedOutputDims",
                                                              &metaTag);
        }

        if ((CamxResultSuccess == result) && (0 != metaTag))
        {
            const UINT  stabilizationOutDimsTag[]   = { metaTag | UsecaseMetadataSectionMask };
            const UINT  length                      = CAMX_ARRAY_SIZE(stabilizationOutDimsTag);
            const VOID* pDimensionData[length]      = { &stabilizedOutputDimensions };
            UINT        pDimensionDataCount[length] = { sizeof(CHIDimension) };

            result = WriteDataList(stabilizationOutDimsTag, pDimensionData, pDimensionDataCount, length);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc,
                               "IPE:%d failed to write stabilization output dimensions type %d to vendor data list error = %d",
                               m_instanceProperty.stabilizationType, InstanceID(), result);
            }
        }

        result = GetEISMargin();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%d Unable to determine EIS margins", InstanceID());
        }

        // Account for additional margin need in EIS usecases
        if (0 != (m_instanceProperty.stabilizationType & IPEStabilizationTypeEIS2))
        {
            optimalInputWidth  +=
                Utils::EvenCeilingUINT32(static_cast<UINT32>(optimalInputWidth * m_EISMarginRequest.widthMargin));
            optimalInputHeight +=
                Utils::EvenCeilingUINT32(static_cast<UINT32>(optimalInputHeight * m_EISMarginRequest.heightMargin));
        }
        else if (0 != (m_instanceProperty.stabilizationType & IPEStabilizationTypeEIS3))
        {
            optimalInputWidth  +=
                Utils::EvenCeilingUINT32(static_cast<UINT32>(optimalInputWidth * m_EISMarginRequest.widthMargin));
            optimalInputHeight +=
                Utils::EvenCeilingUINT32(static_cast<UINT32>(optimalInputHeight * m_EISMarginRequest.heightMargin));
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Not EIS 2 or 3 for node %d instance %d, stabType %d",
                             Type(), InstanceID(), m_instanceProperty.stabilizationType);
        }

        // Add extra 3% for left and 3% for right(total 6%) on optimal input dim
        if ((TRUE == m_instanceProperty.enableFOVC) &&
            (optimalInputWidth < maxInputWidth)     &&
            (optimalInputHeight < maxInputHeight)   &&
            IsRealTime())
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d bfr Add extra margin for fixed FOV width %d height %d",
                InstanceID(), optimalInputWidth, optimalInputHeight);

            optimalInputWidth  += static_cast<UINT32>(optimalInputWidth * (FFOV_PER));
            optimalInputHeight += static_cast<UINT32>(optimalInputHeight * (FFOV_PER));

            minInputWidth  += static_cast<UINT32>(minInputWidth * (FFOV_PER));
            minInputHeight += static_cast<UINT32>(minInputHeight * (FFOV_PER));

            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d Afr Add extra margin for fixed FOV width %d height %d",
                InstanceID(), optimalInputWidth, optimalInputHeight);
            // if AF's fov factor is 0 then IPE should cropout 6%
            m_prevFOVC = FFOV_PER;
        }

        optimalInputWidth  = Utils::AlignGeneric32(optimalInputWidth, 4);
        optimalInputHeight = Utils::AlignGeneric32(optimalInputHeight, 4);

        minInputWidth  = Utils::AlignGeneric32(minInputWidth, 4);
        minInputHeight = Utils::AlignGeneric32(minInputHeight, 4);

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d optimal input dimension after alignment width %d height %d",
                         InstanceID(), optimalInputWidth, optimalInputHeight);
    }

    if (CamxResultSuccess == result)
    {
        if ((optimalInputWidth == 0) || (optimalInputHeight == 0))
        {
            result = CamxResultEFailed;

            CAMX_LOG_ERROR(CamxLogGroupPProc,
                           "IPE:%d Buffer Negotiation Failed, W:%d x H:%d!\n",
                           InstanceID(),
                           optimalInputWidth,
                           optimalInputHeight);
        }
        else
        {
            if ((minInputWidth > maxInputWidth) ||
                (minInputHeight > maxInputHeight))
            {
                CAMX_LOG_WARN(CamxLogGroupPProc,
                              "IPE:%d "
                              "minInputWidth=%d maxInputWidth=%d minInputHeight=%d maxInputHeight=%d "
                              "Min > Max, unable to use current format",
                              InstanceID(),
                              minInputWidth, maxInputWidth, minInputHeight, maxInputHeight);
                result = CamxResultEFailed;
            }
            // Ensure optimal dimension is within min and max dimension,
            // There are chances that the optmial dimension is more than max dimension.
            // Correct for the same.
            UINT32              tempOptimalInputWidth              = 0;
            UINT32              tempOptimalInputHeight             = 0;

            tempOptimalInputWidth  =
                Utils::ClampUINT32(optimalInputWidth, minInputWidth, maxInputWidth);
            tempOptimalInputHeight =
                Utils::ClampUINT32(optimalInputHeight, minInputHeight, maxInputHeight);

            if ((tempOptimalInputWidth != optimalInputWidth) ||
                (tempOptimalInputHeight != optimalInputHeight))
            {
                optimalAspectRatio = static_cast<FLOAT>(tempOptimalInputWidth) / tempOptimalInputHeight;
                if ((TRUE == Utils::FEqualCoarse(optimalAspectRatio, selectedAspectRatio)) ||
                    ((TRUE == IsMFProcessingType()) && (FALSE == IsPostfilterWithDefault())))
                {
                    // The dimensions are fine. Do nothing
                }
                else if (optimalAspectRatio > selectedAspectRatio)
                {
                    tempOptimalInputHeight =
                        Utils::EvenFloorUINT32(static_cast<UINT32>(tempOptimalInputWidth / selectedAspectRatio));
                    // ensure that we dont exceed max
                    optimalInputHeight =
                        Utils::ClampUINT32(tempOptimalInputHeight, minInputHeight, maxInputHeight);
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%d NonConformant AspectRatio:%f Change Height %d using AR:%f",
                        InstanceID(), optimalAspectRatio, optimalInputHeight, selectedAspectRatio);
                }
                else
                {
                    tempOptimalInputWidth =
                        Utils::EvenFloorUINT32(static_cast<UINT32>(tempOptimalInputHeight * selectedAspectRatio));
                    // ensure that we dont exceed max
                    optimalInputWidth =
                        Utils::ClampUINT32(tempOptimalInputWidth, minInputWidth, maxInputWidth);
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%d NonConformant AspectRatio:%f Change Width %d using AR:%f",
                        InstanceID(), optimalAspectRatio, optimalInputWidth, selectedAspectRatio);
                }
            }
            UINT32 numInputPorts = 0;
            UINT32 inputPortId[IPEMaxInput];

            // Get Input Port List
            GetAllInputPortIds(&numInputPorts, &inputPortId[0]);

            pBufferNegotiationData->numInputPorts = numInputPorts;

            isIPEDownscalerEnabled = IsIPEOnlyDownscalerEnabled(pBufferNegotiationData);
            if (TRUE == isIPEDownscalerEnabled)
            {
                UINT32  IPEDownscalerInputWidth = 0;
                UINT32  IPEDownscalerInputHeight = 0;

                GetIPEDownscalerOnlyDimensions(optimalInputWidth,
                    optimalInputHeight,
                    &IPEDownscalerInputWidth,
                    &IPEDownscalerInputHeight,
                    downscaleLimit,
                    m_instanceProperty.ipeOnlyDownscalerMode);

                optimalInputWidth  = IPEDownscalerInputWidth;
                optimalInputHeight = IPEDownscalerInputHeight;

                minInputWidth = IPEDownscalerInputWidth;
                minInputHeight = IPEDownscalerInputHeight;
            }

            for (UINT input = 0; input < numInputPorts; input++)
            {
                pBufferNegotiationData->inputBufferOptions[input].nodeId     = Type();
                pBufferNegotiationData->inputBufferOptions[input].instanceId = InstanceID();
                pBufferNegotiationData->inputBufferOptions[input].portId     = inputPortId[input];

                BufferRequirement* pInputBufferRequirement =
                    &pBufferNegotiationData->inputBufferOptions[input].bufferRequirement;

                pInputBufferRequirement->optimalWidth  = optimalInputWidth;
                pInputBufferRequirement->optimalHeight = optimalInputHeight;
                // If IPE is enabling SIMO and if one of the output is smaller than the other,
                // then the scale capabilities (min,max) needs to be adjusted after accounting for
                // the scaling needed on the smaller output port.
                pInputBufferRequirement->minWidth      = minInputWidth;
                pInputBufferRequirement->minHeight     = minInputHeight;

                pInputBufferRequirement->maxWidth      = maxInputWidth;
                pInputBufferRequirement->maxHeight     = maxInputHeight;

                CAMX_LOG_INFO(CamxLogGroupPProc,
                              "%s: Buffer Negotiation Result: %d dims IPE: %d, Port %d Optimal %d x %d, Min %d x %d,"
                              "Max %d x %d\n",
                              NodeIdentifierString(),
                              result,
                              InstanceID(),
                              inputPortId[input],
                              optimalInputWidth,
                              optimalInputHeight,
                              minInputWidth,
                              minInputHeight,
                              maxInputWidth,
                              maxInputHeight);
            }
        }
    }

    // if both the main outputs are enabled
    if (IPEmainout == 2)
    {
        m_IPESIMOMode = TRUE;
    }

    if (FALSE == GetPipeline()->HasStatsNode())
    {
        m_isStatsNodeAvailable = FALSE;
    }
    else
    {
        m_isStatsNodeAvailable = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    CamxResult result  = CamxResultSuccess;
    UINT               numInputPort;
    UINT               inputPortId[IPEMaxInput];
    const ImageFormat* pImageFormat = NULL;

    CAMX_ASSERT(NULL != pBufferNegotiationData);

    // Get Input Port List
    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    // Loop through input ports to get IPEInputPortFull
    for (UINT index = 0; index < numInputPort; index++)
    {
        if (pBufferNegotiationData->pInputPortNegotiationData[index].inputPortId == IPEInputPortFull)
        {
            pImageFormat = pBufferNegotiationData->pInputPortNegotiationData[index].pImageFormat;
            break;
        }
    }

    CAMX_ASSERT(NULL != pImageFormat);

    if (NULL != pImageFormat)
    {
        m_fullInputWidth  = pImageFormat->width;
        m_fullInputHeight = pImageFormat->height;
    }

    // Check for MF processing capabilities
    if (FALSE == IsSupportedResolutionforMFProcessing())
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "MFNR/MFSR snapshot is not supported supported!!"
            "Width = %d, Height = %d for node %s",
            m_fullInputWidth, m_fullInputHeight, NodeIdentifierString());
        OsUtils::RaiseSignalAbort();
    }

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData   = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        InputPortNegotiationData*  pInputPortNegotiationData    = &pBufferNegotiationData->pInputPortNegotiationData[0];
        BufferProperties*          pFinalOutputBufferProperties = pOutputPortNegotiationData->pFinalOutputBufferProperties;
        UINT outputPortId = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);

        if ((FALSE == IsSinkPortWithBuffer(pOutputPortNegotiationData->outputPortIndex)) &&
            (FALSE == IsNonSinkHALBufferOutput(pOutputPortNegotiationData->outputPortIndex)))
        {
            switch (outputPortId)
            {
                case IPEOutputPortDisplay:
                    if (FALSE == m_nodePropDisableZoomCrop)
                    {
                        pFinalOutputBufferProperties->imageFormat.width  =
                        pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth;
                        pFinalOutputBufferProperties->imageFormat.height =
                        pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight;
                    }
                    else
                    {
                        CAMX_LOG_INFO(CamxLogGroupPProc, "IPE Profile ID is IPEProfileWithoutScale so no zoom");
                        pFinalOutputBufferProperties->imageFormat.width  =
                                pInputPortNegotiationData->pImageFormat->width;
                        pFinalOutputBufferProperties->imageFormat.height =
                                pInputPortNegotiationData->pImageFormat->height;
                    }
                    break;
                case IPEOutputPortVideo:
                    pFinalOutputBufferProperties->imageFormat.width  =
                        pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth;
                    pFinalOutputBufferProperties->imageFormat.height =
                        pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight;
                    break;
                case IPEOutputPortFullRef:
                    CAMX_ASSERT(0 < m_fullInputWidth);
                    CAMX_ASSERT(0 < m_fullInputHeight);
                    pFinalOutputBufferProperties->imageFormat.width  = m_fullInputWidth;
                    pFinalOutputBufferProperties->imageFormat.height = m_fullInputHeight;
                    break;
                case IPEOutputPortDS4Ref:
                    if ((m_instanceProperty.processingType == IPEProcessingType::IPEMFNRPostfilter) &&
                        (m_instanceProperty.profileId != IPEProfileId::IPEProfileIdScale))
                    {
                        pFinalOutputBufferProperties->imageFormat.width  = m_fullInputWidth;
                        pFinalOutputBufferProperties->imageFormat.height = m_fullInputHeight;
                        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "DS4 port dim %d x %d",
                            pFinalOutputBufferProperties->imageFormat.width,
                            pFinalOutputBufferProperties->imageFormat.height);
                    }
                    else
                    {
                        CAMX_ASSERT(0 < m_fullInputWidth);
                        CAMX_ASSERT(0 < m_fullInputHeight);
                        pFinalOutputBufferProperties->imageFormat.width  =
                            Utils::EvenCeilingUINT32(Utils::AlignGeneric32(m_fullInputWidth, DS4Factor) / DS4Factor);
                        pFinalOutputBufferProperties->imageFormat.height =
                            Utils::EvenCeilingUINT32(Utils::AlignGeneric32(m_fullInputHeight, DS4Factor) / DS4Factor);
                    }
                    break;
                case IPEOutputPortDS16Ref:
                    CAMX_ASSERT(0 < m_fullInputWidth);
                    CAMX_ASSERT(0 < m_fullInputHeight);
                    pFinalOutputBufferProperties->imageFormat.width  =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(m_fullInputWidth, DS16Factor) / DS16Factor);
                    pFinalOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(m_fullInputHeight, DS16Factor) / DS16Factor);
                    break;
                case IPEOutputPortDS64Ref:
                    CAMX_ASSERT(0 < m_fullInputWidth);
                    CAMX_ASSERT(0 < m_fullInputHeight);
                    pFinalOutputBufferProperties->imageFormat.width  =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(m_fullInputWidth, DS64Factor) / DS64Factor);
                    pFinalOutputBufferProperties->imageFormat.height =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(m_fullInputHeight, DS64Factor) / DS64Factor);
                    break;
                case IPE_INPUT_OUTPUT_SCRATCHBUFFER:
                    // Update this with actual size of buffer based on new FW API.
                    m_scratchBufferPortEnabled = TRUE;
                    result  = GetScratchBufferSize(&m_scratchBufferData);
                    if ((CamxResultSuccess == result) && (0 != m_scratchBufferData.scratchBufferSize))
                    {
                        pFinalOutputBufferProperties->imageFormat.width  = m_scratchBufferData.scratchBufferSize;
                        pFinalOutputBufferProperties->imageFormat.height = 1;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: , result %d, scratchBufferSize %d",
                            NodeIdentifierString(), result, m_scratchBufferData.scratchBufferSize);
                    }
                    break;
                default:
                    break;
            }
            Utils::Memcpy(&pFinalOutputBufferProperties->imageFormat.planeAlignment[0],
                          &pOutputPortNegotiationData->outputBufferRequirementOptions.planeAlignment[0],
                          sizeof(AlignmentInfo) * FormatsMaxPlanes);
        }

        CAMX_LOG_INFO(CamxLogGroupPProc, "%s: output port %d, Final dim %d x %d",
                         NodeIdentifierString(),
                         outputPortId,
                         pFinalOutputBufferProperties->imageFormat.width,
                         pFinalOutputBufferProperties->imageFormat.height);
    }

    // Check ports buffer properties and disable ports as per firmware limitation
    if (TRUE == IsPortStatusUpdatedByOverride())
    {
        // Update number of passes
        UpdateNodeParamsOnPortStatus();

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, " %s: ports with unsupported buffer properties got disabled, m_numPasses %d",
            NodeIdentifierString(), m_numPasses);
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::CommitAllCommandBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::CommitAllCommandBuffers(
    CmdBuffer**  ppIPECmdBuffer)
{
    CamxResult  result = CamxResultSuccess;

    CAMX_ASSERT(NULL != ppIPECmdBuffer[CmdBufferFrameProcess]);
    CAMX_ASSERT(NULL != ppIPECmdBuffer[CmdBufferIQSettings]);

    result = ppIPECmdBuffer[CmdBufferFrameProcess]->CommitCommands();
    if (CamxResultSuccess == result)
    {
        result = ppIPECmdBuffer[CmdBufferIQSettings]->CommitCommands();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "failed to commit CmdBufferIQSettings");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "failed to commit CmdBufferFrameProcess");
    }

    if ((NULL != ppIPECmdBuffer[CmdBufferStriping]) && (CamxResultSuccess == result))
    {
        result = ppIPECmdBuffer[CmdBufferStriping]->CommitCommands();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "failed to commit CmdBufferStriping");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetCDMProgramArrayOffsetFromBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT IPENode::GetCDMProgramArrayOffsetFromBase(
    CDMProgramArrayOrder    arrayIndex)
{
    INT offset = -1;

    CAMX_ASSERT(arrayIndex <= ProgramArrayICA2);
    /// @todo (CAMX-1033) Remove this function and make static variable holding offsets.
    if (arrayIndex <= ProgramArrayPreLTM)
    {
        offset = sizeof(CDMProgramArray) * arrayIndex;
    }
    else if (arrayIndex == ProgramArrayPostLTM)
    {
        offset = (sizeof(CDMProgramArray) * ProgramArrayPreLTM) + (sizeof(CdmProgram) * ProgramIndexMaxPreLTM);
    }
    else if (arrayIndex == ProgramArrayICA1)
    {
        offset =
            // size of postLTM CDM programs
            ((sizeof(CDMProgramArray) * ProgramArrayPostLTM) +
             (sizeof(CdmProgram) * ProgramIndexMaxPostLTM)   +
             (sizeof(CdmProgram) * ProgramIndexMaxPreLTM));
    }
    else if (arrayIndex == ProgramArrayICA2)
    {
        offset =
            // size of postLTM CDM programs
            ((sizeof(CDMProgramArray) * ProgramArrayICA1)  +
             (sizeof(CdmProgram) * ProgramIndexMaxPostLTM) +
             (sizeof(CdmProgram) * ProgramIndexMaxPreLTM));
    }

    return offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetCDMProgramArrayOffsetFromTop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT IPENode::GetCDMProgramArrayOffsetFromTop(
    CDMProgramArrayOrder    arrayIndex)
{
    INT offset = 0;

    offset = GetCDMProgramArrayOffsetFromBase(arrayIndex);
    if (offset >= 0)
    {
        offset += sizeof(IpeFrameProcess);
    }

    return offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetCDMProgramOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT IPENode::GetCDMProgramOffset(
    CDMProgramArrayOrder    arrayIndex,
    UINT                    CDMProgramIndex)
{
    INT     offset              = 0;
    UINT    CDMProgramOffset    = offsetof(CDMProgramArray, programs) + offsetof(CdmProgram, cdmBaseAndLength) +
        offsetof(CDM_BASE_LENGHT, bitfields);

    offset = GetCDMProgramArrayOffsetFromTop(arrayIndex);
    if (offset >= 0)
    {
        offset += sizeof(CdmProgram) * CDMProgramIndex + CDMProgramOffset;
    }

    return offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillPreLTMCDMProgram
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillPreLTMCDMProgram(
    CmdBuffer**             ppIPECmdBuffer,
    CDMProgramArray*        pCDMProgramArray,
    CdmProgram*             pCDMProgram,
    ProgramType             programType,
    PreLTMCDMProgramOrder   programIndex,
    UINT                    identifier)
{
    CamxResult result = CamxResultSuccess;

    if (m_preLTMLUTCount[programIndex] > 0)
    {
        UINT numPrograms                                    = pCDMProgramArray->numPrograms;
        pCDMProgram                                         = &pCDMProgramArray->programs[numPrograms];
        pCDMProgram->hasSingleReg                           = 0;
        pCDMProgram->programType                            = programType;
        pCDMProgram->uID                                    = identifier;
        pCDMProgram->cdmBaseAndLength.bitfields.LEN         = ((cdm_get_cmd_header_size(CDMCmdDMI) * RegisterWidthInBytes)
                                                               * m_preLTMLUTCount[programIndex]) - 1;
        pCDMProgram->cdmBaseAndLength.bitfields.RESERVED    = 0;
        pCDMProgram->cdmBaseAndLength.bitfields.BASE        = 0;
        pCDMProgram->bufferAllocatedInternally              = 0;

        /// @todo (CAMX-1033) Change below numPrograms to ProgramIndex once firmware support of program skip is available.
        INT offset  = GetCDMProgramOffset(ProgramArrayPreLTM, pCDMProgramArray->numPrograms);
        CAMX_ASSERT(offset >= 0);

        result      = ppIPECmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset,
                                                                                    ppIPECmdBuffer[CmdBufferDMIHeader],
                                                                                    m_preLTMLUTOffset[programIndex]);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Patching failed for PreLTM DMI LUT:%d, result %d",
                NodeIdentifierString(), programIndex, result);
        }

        (pCDMProgramArray->numPrograms)++;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillPostLTMCDMProgram
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillPostLTMCDMProgram(
    CmdBuffer**             ppIPECmdBuffer,
    CDMProgramArray*        pCDMProgramArray,
    CdmProgram*             pCDMProgram,
    ProgramType             programType,
    PostLTMCDMProgramOrder  programIndex,
    UINT                    identifier)
{
    CamxResult result = CamxResultSuccess;
    if (m_postLTMLUTCount[programIndex] > 0)
    {
        UINT numPrograms                                    = pCDMProgramArray->numPrograms;
        pCDMProgram                                         = &pCDMProgramArray->programs[numPrograms];
        pCDMProgram->hasSingleReg                           = 0;
        pCDMProgram->programType                            = programType;
        pCDMProgram->uID                                    = identifier;
        pCDMProgram->cdmBaseAndLength.bitfields.LEN         = ((cdm_get_cmd_header_size(CDMCmdDMI) * RegisterWidthInBytes)
                                                               * m_postLTMLUTCount[programIndex]) - 1;
        pCDMProgram->cdmBaseAndLength.bitfields.RESERVED    = 0;
        pCDMProgram->cdmBaseAndLength.bitfields.BASE        = 0;

        /// @todo (CAMX-1033) Change below numPrograms to ProgramIndex once firmware support of program skip is available.
        INT offset  = GetCDMProgramOffset(ProgramArrayPostLTM, pCDMProgramArray->numPrograms);
        CAMX_ASSERT(offset >= 0);
        result      = ppIPECmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset,
                                                                                    ppIPECmdBuffer[CmdBufferDMIHeader],
                                                                                    m_postLTMLUTOffset[programIndex]);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Patching failed for PostLTM LUTs:%d, result %d",
                NodeIdentifierString(), programIndex, result);
        }

        (pCDMProgramArray->numPrograms)++;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillNPSCDMProgram
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillNPSCDMProgram(
    CmdBuffer**             ppIPECmdBuffer,
    CDMProgramArray*        pCDMProgramArray,
    CdmProgram*             pCDMProgram,
    ProgramType             programType,
    CDMProgramArrayOrder    arrayIndex,
    UINT32                  passCmdBufferSize,
    UINT32                  passOffset)
{
    CamxResult result     = CamxResultSuccess;

    UINT numPrograms                                 = pCDMProgramArray->numPrograms;
    pCDMProgram                                      = &pCDMProgramArray->programs[numPrograms];
    pCDMProgram->hasSingleReg                        = 0;
    pCDMProgram->programType                         = programType;
    pCDMProgram->uID                                 = 0;
    pCDMProgram->cdmBaseAndLength.bitfields.LEN      = passCmdBufferSize - 1;
    pCDMProgram->cdmBaseAndLength.bitfields.RESERVED = 0;
    pCDMProgram->bufferAllocatedInternally           = 0;

    /// @todo (CAMX-1033) Change below numPrograms to ProgramIndex once firmware support of program skip is available.
    INT offset = GetCDMProgramOffset(arrayIndex, pCDMProgramArray->numPrograms);
    CAMX_ASSERT(offset >= 0);
    result = ppIPECmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset,
                                                                           ppIPECmdBuffer[CmdBufferNPS],
                                                                           passOffset);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Patching failed for NPS payload:%d, result %d",
            NodeIdentifierString(), arrayIndex, result);
    }

    (pCDMProgramArray->numPrograms)++;


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillCDMProgramArrays
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillCDMProgramArrays(
    IpeFrameProcessData*    pFrameProcessData,
    IpeIQSettings*          pIpeIQSettings,
    CmdBuffer**             ppIPECmdBuffer,
    UINT                    batchFrames)
{
    INT                 offset;
    CdmProgram*         pCDMProgram;
    CDMProgramArray*    pCDMProgramArray;
    UINT8*              pCDMPayload;
    UINT                numPrograms     = 0;
    CamxResult          result          = CamxResultSuccess;
    ProgramType         type;
    UINT32              identifier      = 0;

    // Patch IQSettings buffer in IpeFrameProcessData
    offset = static_cast <UINT32>(offsetof(IpeFrameProcessData, iqSettingsAddr));
    result = ppIPECmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset, ppIPECmdBuffer[CmdBufferIQSettings], 0);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Patching failed for IQSettings result=%d", NodeIdentifierString(), result);
    }
    else
    {
        // Patch cdmProgramArrayBase, which is allocated contiguously below IpeFrameProcessData
        offset = static_cast <UINT32>(offsetof(IpeFrameProcessData, cdmProgramArrayBase));
        result = ppIPECmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset,
                                                                               ppIPECmdBuffer[CmdBufferFrameProcess],
                                                                               sizeof(IpeFrameProcess));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Patching failed for FrameProcess result=%d", NodeIdentifierString(), result);
        }
    }

    // Populate offsets of all cdmPrograArrays in IpeFrameProcessData with respect to Base
    pFrameProcessData->cdmProgramArrayAnrFullPassAddr   =
        GetCDMProgramArrayOffsetFromBase(ProgramArrayANRFullPass);
    pFrameProcessData->cdmProgramArrayAnrDc4Addr        =
        GetCDMProgramArrayOffsetFromBase(ProgramArrayANRDS4);
    pFrameProcessData->cdmProgramArrayAnrDc16Addr       =
        GetCDMProgramArrayOffsetFromBase(ProgramArrayANRDS16);
    pFrameProcessData->cdmProgramArrayAnrDc64Addr       =
        GetCDMProgramArrayOffsetFromBase(ProgramArrayANRDS64);
    pFrameProcessData->cdmProgramArrayTfFullPassAddr    =
        GetCDMProgramArrayOffsetFromBase(ProgramArrayTFFullPass);
    pFrameProcessData->cdmProgramArrayTfDc4Addr         =
        GetCDMProgramArrayOffsetFromBase(ProgramArrayTFDS4);
    pFrameProcessData->cdmProgramArrayTfDc16Addr        =
        GetCDMProgramArrayOffsetFromBase(ProgramArrayTFDS16);
    pFrameProcessData->cdmProgramArrayTfDc64Addr        =
        GetCDMProgramArrayOffsetFromBase(ProgramArrayTFDS64);
    pFrameProcessData->cdmProgramArrayPreLtmAddr        =
        GetCDMProgramArrayOffsetFromBase(ProgramArrayPreLTM);
    pFrameProcessData->cdmProgramArrayPostLtmAddr       =
        GetCDMProgramArrayOffsetFromBase(ProgramArrayPostLTM);
    pFrameProcessData->cdmProgramArrayDsxDc4Addr       = 0;
    pFrameProcessData->cdmProgramArrayDsxDc16Addr      = 0;
    pFrameProcessData->cdmProgramArrayDsxDc64Addr      = 0;
    pFrameProcessData->cdmProgramArrayMfhdrPreLtmAddr  = 0;
    pFrameProcessData->cdmProgramArrayMfhdrPostLtmAddr = 0;

    for (UINT i = 0; i < batchFrames; i++)
    {
        pFrameProcessData->frameSets[i].cdmProgramArrayIca1Addr =
            GetCDMProgramArrayOffsetFromBase(ProgramArrayICA1);
        pFrameProcessData->frameSets[i].cdmProgramArrayIca2Addr =
            GetCDMProgramArrayOffsetFromBase(ProgramArrayICA2);

    }

    identifier = pFrameProcessData->requestId % GetPipeline()->GetRequestQueueDepth();

    pCDMPayload         = reinterpret_cast<UINT8*>(pFrameProcessData);
    pCDMProgramArray    =
        reinterpret_cast<CDMProgramArray*>(pCDMPayload + GetCDMProgramArrayOffsetFromTop(ProgramArrayPreLTM));
    pCDMProgramArray->allocator     = 0;
    pCDMProgramArray->numPrograms   = 0;

    if (CamxResultSuccess == result)
    {
        type = ((TRUE == pIpeIQSettings->cacParameters.moduleCfg.EN) ||
                (TRUE == pIpeIQSettings->colorTransformParameters.moduleCfg.EN)) ?
                PROGRAM_TYPE_GENERIC : PROGRAM_TYPE_SKIP;
        // CDMProgramArray :: Pre LTM Section
        numPrograms                            = pCDMProgramArray->numPrograms;
        pCDMProgram                            = &pCDMProgramArray->programs[numPrograms];
        pCDMProgram->hasSingleReg              = 0;
        pCDMProgram->programType               = type;
        pCDMProgram->uID                       = 0;
        pCDMProgram->bufferAllocatedInternally = 0;

        if ((NULL != ppIPECmdBuffer[CmdBufferPreLTM]))
        {
            UINT length = (ppIPECmdBuffer[CmdBufferPreLTM]->GetResourceUsedDwords() * RegisterWidthInBytes);
            if (length > 0)
            {
                pCDMProgram->cdmBaseAndLength.bitfields.LEN         = length - 1;
                pCDMProgram->cdmBaseAndLength.bitfields.RESERVED    = 0;

                // CDMProgram :: Pre LTM :: GENERIC Cmd buffer
                offset = GetCDMProgramOffset(ProgramArrayPreLTM, ProgramIndexPreLTMGeneric);
                CAMX_ASSERT(offset >= 0);

                result =
                    ppIPECmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset, ppIPECmdBuffer[CmdBufferPreLTM], 0);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Patching failed for PreLTM result=%d", NodeIdentifierString(),
                        result);
                }

                (pCDMProgramArray->numPrograms)++;
            }
        }
        else if (0 < m_maxCmdBufferSizeBytes[CmdBufferPreLTM])
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid cmd buffer, PreLTM payload: %p, result: %d",
                ppIPECmdBuffer[CmdBufferPreLTM], result);
        }
    }

    if (NULL != ppIPECmdBuffer[CmdBufferDMIHeader])
    {
        // CDMProgram :: HNR LUT
        if (CamxResultSuccess == result)
        {
            type   = ((TRUE == pIpeIQSettings->hnrParameters.moduleCfg.EN) ?
                      IPE_HNR_LUT_PROGRAM : PROGRAM_TYPE_SKIP);
            result = FillPreLTMCDMProgram(ppIPECmdBuffer,
                                          pCDMProgramArray,
                                          pCDMProgram,
                                          type,
                                          ProgramIndexHNR,
                                          identifier);
        }

        // CDMProgram::LTM LUT
        if (CamxResultSuccess == result)
        {
            type   = ((TRUE == pIpeIQSettings->ltmParameters.moduleCfg.EN) ?
                      IPE_LTM_LUT_PROGRAM : PROGRAM_TYPE_SKIP);
            result = FillPreLTMCDMProgram(ppIPECmdBuffer,
                                          pCDMProgramArray,
                                          pCDMProgram,
                                          type,
                                          ProgramIndexLTM,
                                          identifier);
        }
    }
    else if (0 < m_maxCmdBufferSizeBytes[CmdBufferDMIHeader])
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid cmd buffer, PreLTM LUTs: %p, result=%d",
            ppIPECmdBuffer[CmdBufferDMIHeader], result);
    }

    // CDMProgramArray :: Post LTM Section
    pCDMProgramArray =
        reinterpret_cast<CDMProgramArray*>(pCDMPayload + GetCDMProgramArrayOffsetFromTop(ProgramArrayPostLTM));
    pCDMProgramArray->allocator     = 0;
    pCDMProgramArray->numPrograms   = 0;

    if (CamxResultSuccess == result)
    {
        type = (
            (TRUE == pIpeIQSettings->glutParameters.moduleCfg.EN)              ||
            (TRUE == pIpeIQSettings->chromaEnhancementParameters.moduleCfg.EN) ||
            (TRUE == pIpeIQSettings->lut2dParameters.moduleCfg.EN)             ||
            (TRUE == pIpeIQSettings->asfParameters.moduleCfg.EN)               ||
            (TRUE == pIpeIQSettings->chromaSupressionParameters.moduleCfg.EN)  ||
            (TRUE == pIpeIQSettings->skinEnhancementParameters.moduleCfg.EN)   ||
            (TRUE == pIpeIQSettings->colorCorrectParameters.moduleCfg.EN))     ?
            PROGRAM_TYPE_GENERIC : PROGRAM_TYPE_SKIP;

        // CDMProgram :: Generic
        numPrograms                            = pCDMProgramArray->numPrograms;
        pCDMProgram                            = &pCDMProgramArray->programs[numPrograms];
        pCDMProgram->hasSingleReg              = 0;
        pCDMProgram->programType               = type;
        pCDMProgram->uID                       = 0;
        pCDMProgram->bufferAllocatedInternally = 0;

        if ((NULL != ppIPECmdBuffer[CmdBufferPostLTM]))
        {
            UINT length = (ppIPECmdBuffer[CmdBufferPostLTM]->GetResourceUsedDwords() * RegisterWidthInBytes);
            if (length > 0)
            {
                pCDMProgram->cdmBaseAndLength.bitfields.LEN      = length - 1;
                pCDMProgram->cdmBaseAndLength.bitfields.RESERVED = 0;

                // Generic Reg Random CDM from pre LTM Modules
                offset = GetCDMProgramOffset(ProgramArrayPostLTM, ProgramIndexPostLTMGeneric);
                CAMX_ASSERT(offset >= 0);

                result =
                    ppIPECmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset, ppIPECmdBuffer[CmdBufferPostLTM], 0);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Patching failed for PostLTM result=%d", NodeIdentifierString(),
                        result);
                }

                (pCDMProgramArray->numPrograms)++;
            }
        }
        else if (0 < m_maxCmdBufferSizeBytes[CmdBufferPostLTM])
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid cmd buffer, PostLTM payload: %p, result: %d",
                ppIPECmdBuffer[CmdBufferPostLTM], result);
        }
    }

    if (NULL != ppIPECmdBuffer[CmdBufferDMIHeader])
    {
        // CDMProgram :: Gamma LUT
        if (CamxResultSuccess == result)
        {
            type   = ((TRUE == pIpeIQSettings->glutParameters.moduleCfg.EN) ?
                      IPE_GAMMA_GLUT_LUT_PROGRAM : PROGRAM_TYPE_SKIP);
            result = FillPostLTMCDMProgram(ppIPECmdBuffer,
                                           pCDMProgramArray,
                                           pCDMProgram,
                                           type,
                                           ProgramIndexGLUT,
                                           identifier);
        }
        // CDMProgram :: 2D LUT
        if (CamxResultSuccess == result)
        {
            type   = ((TRUE == pIpeIQSettings->lut2dParameters.moduleCfg.EN) ?
                      IPE_2D_LUT_LUT_PROGRAM : PROGRAM_TYPE_SKIP);
            result = FillPostLTMCDMProgram(ppIPECmdBuffer,
                                           pCDMProgramArray,
                                           pCDMProgram,
                                           type,
                                           ProgramIndex2DLUT,
                                           identifier);
        }
        // CDMProgram :: ASF LUT
        if (CamxResultSuccess == result)
        {
            type   = ((TRUE == pIpeIQSettings->asfParameters.moduleCfg.EN) ?
                      IPE_ASF_LUT_PROGRAM : PROGRAM_TYPE_SKIP);
            result = FillPostLTMCDMProgram(ppIPECmdBuffer,
                                           pCDMProgramArray,
                                           pCDMProgram,
                                           type,
                                           ProgramIndexASF,
                                           identifier);
        }
        // CDMProgram :: GRA LUT
        if (CamxResultSuccess == result)
        {
            type   = ((TRUE == pIpeIQSettings->graParameters.moduleCfg.EN) ?
                      IPE_GRA_LUT_PROGRAM : PROGRAM_TYPE_SKIP);
            result = FillPostLTMCDMProgram(ppIPECmdBuffer,
                                           pCDMProgramArray,
                                           pCDMProgram,
                                           type,
                                           ProgramIndexGRA,
                                           identifier);
        }
    }
    else if (0 < m_maxCmdBufferSizeBytes[CmdBufferDMIHeader])
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid cmd buffer, PostLTM IQ modules LUTs: %p, result: %d",
            ppIPECmdBuffer[CmdBufferDMIHeader], result);
    }

    if (NULL != ppIPECmdBuffer[CmdBufferDMIHeader])
    {
        if (CamxResultSuccess == result)
        {
            // CDMProgram :: ICA1 LUT
            // if module is disabled dynamically skip the CDM program
            type   = ((TRUE == pIpeIQSettings->ica1Parameters.isGridEnable) ||
                      (TRUE == pIpeIQSettings->ica1Parameters.isPerspectiveEnable)) ?
                      IPE_ICA1_LUT_PROGRAM : PROGRAM_TYPE_SKIP;
            result = FillICACDMprograms(pFrameProcessData,
                                        ppIPECmdBuffer,
                                        type,
                                        ProgramArrayICA1,
                                        ProgramIndexICA1,
                                        identifier);
        }

        if (CamxResultSuccess == result)
        {
            // CDMProgram :: ICA2 LUT
            type   = ((TRUE == pIpeIQSettings->ica2Parameters.isGridEnable) ||
                      (TRUE == pIpeIQSettings->ica2Parameters.isPerspectiveEnable)) ?
                      IPE_ICA2_LUT_PROGRAM : PROGRAM_TYPE_SKIP;
            result = FillICACDMprograms(pFrameProcessData,
                                        ppIPECmdBuffer,
                                        type,
                                        ProgramArrayICA2,
                                        ProgramIndexICA2,
                                        identifier);
        }
    }
    else if (0 < m_maxCmdBufferSizeBytes[CmdBufferDMIHeader])
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid cmd buffer, ICA LUTs: %p, result: %d",
            ppIPECmdBuffer[CmdBufferDMIHeader], result);
    }

    if (NULL != ppIPECmdBuffer[CmdBufferNPS])
    {
        // CDMProgramArray :: NPS : ANR Full Pass
        pCDMProgramArray =
            reinterpret_cast<CDMProgramArray*>(pCDMPayload +
                GetCDMProgramArrayOffsetFromTop(ProgramArrayANRFullPass));

        pCDMProgramArray->allocator   = 0;
        pCDMProgramArray->numPrograms = 0;

        numPrograms = pCDMProgramArray->numPrograms;
        pCDMProgram = &pCDMProgramArray->programs[numPrograms];
        if (CamxResultSuccess == result)
        {
            type   = (TRUE == pIpeIQSettings->anrParameters.parameters[0].moduleCfg.EN) ?
                     IPE_ANR_CYLPF_PROGRAM : PROGRAM_TYPE_SKIP;
            result = FillNPSCDMProgram(ppIPECmdBuffer,
                                       pCDMProgramArray,
                                       pCDMProgram,
                                       type,
                                       ProgramArrayANRFullPass,
                                       m_ANRSinglePassCmdBufferSize,
                                       m_ANRPassOffset[PASS_NAME_FULL]);
        }

        // CDMProgramArray :: NPS : ANR DS4 Pass
        pCDMProgramArray =
            reinterpret_cast<CDMProgramArray*>(pCDMPayload + GetCDMProgramArrayOffsetFromTop(ProgramArrayANRDS4));

        pCDMProgramArray->allocator   = 0;
        pCDMProgramArray->numPrograms = 0;

        numPrograms = pCDMProgramArray->numPrograms;
        pCDMProgram = &pCDMProgramArray->programs[numPrograms];

        if (CamxResultSuccess == result)
        {
            type   = (TRUE == pIpeIQSettings->anrParameters.parameters[1].moduleCfg.EN) ?
                     IPE_ANR_CYLPF_PROGRAM : PROGRAM_TYPE_SKIP;
            result = FillNPSCDMProgram(ppIPECmdBuffer,
                                       pCDMProgramArray,
                                       pCDMProgram,
                                       type,
                                       ProgramArrayANRDS4,
                                       m_ANRSinglePassCmdBufferSize,
                                       m_ANRPassOffset[PASS_NAME_DC_4]);
        }

        // CDMProgramArray :: NPS : ANR DS16 Pass
        pCDMProgramArray =
            reinterpret_cast<CDMProgramArray*>(pCDMPayload + GetCDMProgramArrayOffsetFromTop(ProgramArrayANRDS16));

        pCDMProgramArray->allocator   = 0;
        pCDMProgramArray->numPrograms = 0;

        numPrograms = pCDMProgramArray->numPrograms;
        pCDMProgram = &pCDMProgramArray->programs[numPrograms];

        if (CamxResultSuccess == result)
        {
            type   = (TRUE == pIpeIQSettings->anrParameters.parameters[2].moduleCfg.EN) ?
                     IPE_ANR_CYLPF_PROGRAM : PROGRAM_TYPE_SKIP;
            result = FillNPSCDMProgram(ppIPECmdBuffer,
                                       pCDMProgramArray,
                                       pCDMProgram,
                                       type,
                                       ProgramArrayANRDS16,
                                       m_ANRSinglePassCmdBufferSize,
                                       m_ANRPassOffset[PASS_NAME_DC_16]);

        }

        // CDMProgramArray :: NPS : ANR DS64 Pass
        pCDMProgramArray =
            reinterpret_cast<CDMProgramArray*>(pCDMPayload + GetCDMProgramArrayOffsetFromTop(ProgramArrayANRDS64));

        pCDMProgramArray->allocator   = 0;
        pCDMProgramArray->numPrograms = 0;

        numPrograms = pCDMProgramArray->numPrograms;
        pCDMProgram = &pCDMProgramArray->programs[numPrograms];

        if (CamxResultSuccess == result)
        {
            type   = (TRUE == pIpeIQSettings->anrParameters.parameters[3].moduleCfg.EN) ?
                     IPE_ANR_CYLPF_PROGRAM : PROGRAM_TYPE_SKIP;
            result = FillNPSCDMProgram(ppIPECmdBuffer,
                                       pCDMProgramArray,
                                       pCDMProgram,
                                       type,
                                       ProgramArrayANRDS64,
                                       m_ANRSinglePassCmdBufferSize,
                                       m_ANRPassOffset[PASS_NAME_DC_64]);
        }

        // CDMProgramArray :: NPS : TF Full Pass
        pCDMProgramArray =
            reinterpret_cast<CDMProgramArray*>(pCDMPayload + GetCDMProgramArrayOffsetFromTop(ProgramArrayTFFullPass));

        pCDMProgramArray->allocator   = 0;
        pCDMProgramArray->numPrograms = 0;

        numPrograms = pCDMProgramArray->numPrograms;
        pCDMProgram = &pCDMProgramArray->programs[numPrograms];

        if (CamxResultSuccess == result)
        {
            type   = (TRUE == pIpeIQSettings->tfParameters.parameters[0].moduleCfg.EN) ?
                     IPE_TF_PROGRAM : PROGRAM_TYPE_SKIP;
            result = FillNPSCDMProgram(ppIPECmdBuffer,
                                       pCDMProgramArray,
                                       pCDMProgram,
                                       type,
                                       ProgramArrayTFFullPass,
                                       m_TFSinglePassCmdBufferSize,
                                       m_TFPassOffset[PASS_NAME_FULL]);
        }

        // CDMProgramArray :: NPS : TF DS4 Pass
        pCDMProgramArray =
            reinterpret_cast<CDMProgramArray*>(pCDMPayload + GetCDMProgramArrayOffsetFromTop(ProgramArrayTFDS4));

        pCDMProgramArray->allocator   = 0;
        pCDMProgramArray->numPrograms = 0;

        numPrograms = pCDMProgramArray->numPrograms;
        pCDMProgram = &pCDMProgramArray->programs[numPrograms];

        if (CamxResultSuccess == result)
        {
            type   = (TRUE == pIpeIQSettings->tfParameters.parameters[1].moduleCfg.EN) ?
                     IPE_TF_PROGRAM : PROGRAM_TYPE_SKIP;
            result = FillNPSCDMProgram(ppIPECmdBuffer,
                                       pCDMProgramArray,
                                       pCDMProgram,
                                       type,
                                       ProgramArrayTFDS4,
                                       m_TFSinglePassCmdBufferSize,
                                       m_TFPassOffset[PASS_NAME_DC_4]);
        }

        // CDMProgramArray :: NPS : TF DS16 Pass
        pCDMProgramArray =
            reinterpret_cast<CDMProgramArray*>(pCDMPayload + GetCDMProgramArrayOffsetFromTop(ProgramArrayTFDS16));

        pCDMProgramArray->allocator   = 0;
        pCDMProgramArray->numPrograms = 0;

        numPrograms = pCDMProgramArray->numPrograms;
        pCDMProgram = &pCDMProgramArray->programs[numPrograms];

        if (CamxResultSuccess == result)
        {
            type   = (TRUE == pIpeIQSettings->tfParameters.parameters[2].moduleCfg.EN) ?
                     IPE_TF_PROGRAM : PROGRAM_TYPE_SKIP;
            result = FillNPSCDMProgram(ppIPECmdBuffer,
                                       pCDMProgramArray,
                                       pCDMProgram,
                                       type,
                                       ProgramArrayTFDS16,
                                       m_TFSinglePassCmdBufferSize,
                                       m_TFPassOffset[PASS_NAME_DC_16]);
        }

        // CDMProgramArray :: NPS : TF DS64 Pass
        pCDMProgramArray =
            reinterpret_cast<CDMProgramArray*>(pCDMPayload + GetCDMProgramArrayOffsetFromTop(ProgramArrayTFDS64));

        pCDMProgramArray->allocator   = 0;
        pCDMProgramArray->numPrograms = 0;

        numPrograms = pCDMProgramArray->numPrograms;
        pCDMProgram = &pCDMProgramArray->programs[numPrograms];

        if (CamxResultSuccess == result)
        {
            type   = (TRUE == pIpeIQSettings->tfParameters.parameters[3].moduleCfg.EN) ?
                     IPE_TF_PROGRAM : PROGRAM_TYPE_SKIP;
            result = FillNPSCDMProgram(ppIPECmdBuffer,
                                       pCDMProgramArray,
                                       pCDMProgram,
                                       type,
                                       ProgramArrayTFDS64,
                                       m_TFSinglePassCmdBufferSize,
                                       m_TFPassOffset[PASS_NAME_DC_64]);
        }
    }
    else if (0 < m_maxCmdBufferSizeBytes[CmdBufferNPS])
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid cmd buffer, NPS payload: %p, result: %d",
            ppIPECmdBuffer[CmdBufferNPS], result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillICACDMprograms
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillICACDMprograms(
    IpeFrameProcessData*    pFrameProcessData,
    CmdBuffer**             ppIPECmdBuffer,
    ProgramType             programType,
    CDMProgramArrayOrder    programArrayOrder,
    ICAProgramOrder         programIndex,
    UINT                    identifier)
{
    CamxResult          result           = CamxResultSuccess;
    UINT8*              pCDMPayload      = NULL;
    CDMProgramArray*    pCDMProgramArray = NULL;
    CdmProgram*         pCDMProgram      = NULL;

    if (m_ICALUTCount[programIndex] > 0)
    {
        pCDMPayload                                      = reinterpret_cast<UINT8*>(pFrameProcessData);
        pCDMProgramArray                                 =
            reinterpret_cast<CDMProgramArray*>(pCDMPayload + GetCDMProgramArrayOffsetFromTop(programArrayOrder));

        pCDMProgramArray->allocator                      = 0;
        pCDMProgramArray->numPrograms                    = 0;
        pCDMProgram                                      = &pCDMProgramArray->programs[pCDMProgramArray->numPrograms];
        pCDMProgram->hasSingleReg                        = 0;
        pCDMProgram->programType                         = programType;
        pCDMProgram->uID                                 = identifier;
        pCDMProgram->cdmBaseAndLength.bitfields.LEN      = ((cdm_get_cmd_header_size(CDMCmdDMI) * RegisterWidthInBytes)
            * m_ICALUTCount[programIndex]) - 1;
        pCDMProgram->cdmBaseAndLength.bitfields.RESERVED = 0;
        pCDMProgram->cdmBaseAndLength.bitfields.BASE     = 0;

        /// @todo (CAMX-1033) Change below numPrograms to ProgramIndex once firmware support of program skip is available.
        INT offset = GetCDMProgramOffset(programArrayOrder, pCDMProgramArray->numPrograms);
        CAMX_ASSERT(offset >= 0);
        result = ppIPECmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset,
                                                                               ppIPECmdBuffer[CmdBufferDMIHeader],
                                                                               m_ICALUTOffset[programIndex]);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Patching failed for ICA LUT:%d, result %d",
                NodeIdentifierString(), programIndex, result);
        }

        (pCDMProgramArray->numPrograms)++;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetMetadataTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetMetadataTags(
    ISPInputData* pModuleInput)
{
    CamxResult                result                       = CamxResultSuccess;
    CamxResult                resultContrast               = CamxResultSuccess;
    UINT32                    metaTag                      = 0;
    UINT32                    metaTagSharpness             = 0;
    UINT32                    metaTagContrast              = 0;
    UINT32                    metaTagDynamicContrast       = 0;
    UINT32                    metaTagDarkBoostStrength     = 0;
    UINT32                    metaTagBrightSupressStrength = 0;
    ISPTonemapPoint*          pBlueTonemapCurve            = NULL;
    ISPTonemapPoint*          pGreenTonemapCurve           = NULL;
    ISPTonemapPoint*          pRedTonemapCurve             = NULL;
    const PlatformStaticCaps* pStaticCaps                  = HwEnvironment::GetInstance()->GetPlatformStaticCaps();

    // Populate default values
    pModuleInput->pHALTagsData->saturation                                     = 5;
    pModuleInput->pHALTagsData->sharpness                                      = 1.0f;
    pModuleInput->pHALTagsData->noiseReductionMode                             = NoiseReductionModeFast;
    pModuleInput->pHALTagsData->edgeMode                                       = EdgeModeOff;
    pModuleInput->pHALTagsData->controlVideoStabilizationMode                  = ControlVideoStabilizationModeOff;
    pModuleInput->pHALTagsData->ltmContrastStrength.ltmDynamicContrastStrength = 0.0f;
    pModuleInput->pHALTagsData->ltmContrastStrength.ltmDarkBoostStrength       = 0.0f;
    pModuleInput->pHALTagsData->ltmContrastStrength.ltmBrightSupressStrength   = 0.0f;

    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.saturation",
                                                      "use_saturation",
                                                      &metaTag);
    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sharpness",
                                                      "strength",
                                                      &metaTagSharpness);
    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.ltmDynamicContrast",
                                                       "ltmDynamicContrastStrength",
                                                       &metaTagDynamicContrast);
    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.ltmDynamicContrast",
                                                       "ltmDarkBoostStrength",
                                                       &metaTagDarkBoostStrength);
    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.ltmDynamicContrast",
                                                      "ltmBrightSupressStrength",
                                                       &metaTagBrightSupressStrength);

    static const UINT VendorTagsIPE[] =
    {
        metaTag | InputMetadataSectionMask,
        InputEdgeMode,
        InputControlVideoStabilizationMode,
        metaTagSharpness| InputMetadataSectionMask,
        m_MFNRTotalNumFramesTAGLocation | InputMetadataSectionMask,
        m_MFSRTotalNumFramesTAGLocation | InputMetadataSectionMask,
        InputColorCorrectionAberrationMode,
        InputNoiseReductionMode,
        InputTonemapMode,
        InputColorCorrectionMode,
        InputControlMode,
        InputTonemapCurveBlue,
        InputTonemapCurveGreen,
        InputTonemapCurveRed,
        InputColorCorrectionGains,
        InputColorCorrectionMode,
        InputColorCorrectionTransform,
        InputControlAEMode,
        InputControlAWBMode,
        InputControlAWBLock,
        metaTagDynamicContrast | InputMetadataSectionMask,
        metaTagDarkBoostStrength | InputMetadataSectionMask,
        metaTagBrightSupressStrength | InputMetadataSectionMask,
        m_MFNRBlendFrameNumTAGLocation | InputMetadataSectionMask,
        m_MFSRBlendFrameNumTAGLocation | InputMetadataSectionMask,
    };

    const static UINT length                             = CAMX_ARRAY_SIZE(VendorTagsIPE);
    VOID*             pData[length]                      = { 0 };
    UINT64            vendorTagsIPEDataIPEOffset[length] = { 0 };

    GetDataList(VendorTagsIPE, pData, vendorTagsIPEDataIPEOffset, length);

    if (NULL != pData[0])
    {
        Utils::Memcpy(&pModuleInput->pHALTagsData->saturation, pData[0], sizeof(&pModuleInput->pHALTagsData->saturation));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain saturation value");
    }

    if (NULL != pData[1])
    {
        Utils::Memcpy(&pModuleInput->pHALTagsData->edgeMode, pData[1], sizeof(&pModuleInput->pHALTagsData->edgeMode));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain edgeMode value");
    }

    if (NULL != pData[2])
    {
        Utils::Memcpy(&pModuleInput->pHALTagsData->controlVideoStabilizationMode,
            pData[2], sizeof(&pModuleInput->pHALTagsData->controlVideoStabilizationMode));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain video stabilization mode value");
    }

    // Query MF configuration
    switch(m_instanceProperty.processingType)
    {
        case IPEProcessingType::IPEMFNRPrefilter:
        case IPEProcessingType::IPEMFNRBlend:
        case IPEProcessingType::IPEMFNRScale:
        case IPEProcessingType::IPEMFNRPostfilter:
            if (NULL != pData[4])
            {
                pModuleInput->pipelineIPEData.numOfFrames = *(static_cast<UINT *>(pData[4]));
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Total number of MFNR Frames = %d",
                    pModuleInput->pipelineIPEData.numOfFrames);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Missing Total number of MFNR frames");
            }
            if (NULL != pData[23])
            {
                pModuleInput->mfFrameNum = *(static_cast<UINT *>(pData[23]));
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, " MFNR Blend Frame Number = %d",
                               pModuleInput->mfFrameNum);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Missing Total number of MFSR frames");
            }
            break;
        case IPEProcessingType::IPEMFSRPrefilter:
        case IPEProcessingType::IPEMFSRBlend:
        case IPEProcessingType::IPEMFSRPostfilter:
            if (NULL != pData[5])
            {
                pModuleInput->pipelineIPEData.numOfFrames = *(static_cast<UINT *>(pData[5]));
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Total number of MFSR Frames = %d",
                                 pModuleInput->pipelineIPEData.numOfFrames);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Missing Total number of MFSR frames");
            }
            if (NULL != pData[24])
            {
                pModuleInput->mfFrameNum = *(static_cast<UINT *>(pData[24]));
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, " MFSR Blend Frame Number = %d",
                                 pModuleInput->mfFrameNum);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Missing Total number of MFSR frames");
            }
            break;
        default:
            // Do nothing
            break;
    }

    if (pModuleInput->pipelineIPEData.numOfFrames < 3)
    {
        pModuleInput->pipelineIPEData.numOfFrames = 3;
        CAMX_LOG_WARN(CamxLogGroupPProc, "hardcoded Total number of MFNR/MFSR frames to 3");
    }
    else if (pModuleInput->pipelineIPEData.numOfFrames > 8)
    {
        pModuleInput->pipelineIPEData.numOfFrames = 8;
        CAMX_LOG_WARN(CamxLogGroupPProc, "hardcoded Total number of MFNR/MFSR frames to 8");
    }

    if (NULL != pData[3])
    {
        pModuleInput->pHALTagsData->sharpness =
            static_cast<FLOAT> (*(static_cast<UINT *>(pData[3]))) / pStaticCaps->sharpnessRange.defValue;
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain sharpness value");
    }

    if (NULL != pData[6])
    {
        pModuleInput->pHALTagsData->colorCorrectionAberrationMode = *(static_cast<UINT8*>(pData[6]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain colorCorrectionAberration Mode value");
    }

    if (NULL != pData[7])
    {
        pModuleInput->pHALTagsData->noiseReductionMode = *(static_cast<UINT8*>(pData[7]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain noise reduction mode value");
    }

    if (NULL != pData[8])
    {
        pModuleInput->pHALTagsData->tonemapCurves.tonemapMode = *(static_cast<UINT8*>(pData[8]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain tonemap mode value");
    }

    if (NULL != pData[9])
    {
        pModuleInput->pHALTagsData->colorCorrectionMode = *(static_cast<UINT8*>(pData[9]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain colorCorrection mode value");
    }

    if (NULL != pData[10])
    {
        pModuleInput->pHALTagsData->controlMode = *(static_cast<UINT8*>(pData[10]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain main control mode switch value");
    }

    if (NULL != pData[11])
    {
        pBlueTonemapCurve = static_cast<ISPTonemapPoint*>(pData[11]);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Cannot obtain blue tonemap curve");
    }

    if (NULL != pData[12])
    {
        pGreenTonemapCurve = static_cast<ISPTonemapPoint*>(pData[12]);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Cannot obtain green tonemap curve");
    }

    if (NULL != pData[13])
    {
        pRedTonemapCurve = static_cast<ISPTonemapPoint*>(pData[13]);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Cannot obtain red tonemap curve");
    }

    if (NULL != pData[14])
    {
        pModuleInput->pHALTagsData->colorCorrectionGains = *(static_cast<ColorCorrectionGain*>(pData[14]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain color correction gains");
    }

    if (NULL != pData[15])
    {
        pModuleInput->pHALTagsData->colorCorrectionMode = *(static_cast<UINT8*>(pData[15]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain color correction mode value");
    }

    if (NULL != pData[16])
    {
        pModuleInput->pHALTagsData->colorCorrectionTransform = *(static_cast<ISPColorCorrectionTransform*>(pData[16]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain color correction transform");
    }

    if (NULL != pData[17])
    {
        pModuleInput->pHALTagsData->controlAEMode = *(static_cast<UINT8*>(pData[17]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain AE mode");
    }

    if (NULL != pData[18])
    {
        pModuleInput->pHALTagsData->controlAWBMode = *(static_cast<UINT8*>(pData[18]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain AWB mode");
    }

    if (NULL != pData[19])
    {
        pModuleInput->pHALTagsData->controlAWBLock = *(static_cast<UINT8*>(pData[19]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain AWB lock");
    }

    if (NULL != pData[20] && NULL != pData[21] && NULL != pData[22])
    {
        pModuleInput->pHALTagsData->ltmContrastStrength.ltmDynamicContrastStrength  = *(static_cast<FLOAT*>(pData[20]));
        pModuleInput->pHALTagsData->ltmContrastStrength.ltmDarkBoostStrength        = *(static_cast<FLOAT*>(pData[21]));
        pModuleInput->pHALTagsData->ltmContrastStrength.ltmBrightSupressStrength    = *(static_cast<FLOAT*>(pData[22]));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain ltm dynamicContrast %p, darkBoost %p, brightSupress %p",
            pData[20], pData[21], pData[22]);
    }

    resultContrast = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.contrast",
                                                              "level",
                                                              &metaTagContrast);
    if (CamxResultSuccess == resultContrast)
    {
        static const UINT VendorTagContrast[] =
        {
            metaTagContrast | InputMetadataSectionMask,
        };

        const static UINT lengthContrast                              = CAMX_ARRAY_SIZE(VendorTagContrast);
        VOID*             pDataContrast[lengthContrast]               = { 0 };
        UINT64            vendorTagsContrastIPEOffset[lengthContrast] = { 0 };

        GetDataList(VendorTagContrast, pDataContrast, vendorTagsContrastIPEOffset, lengthContrast);
        if (NULL != pDataContrast[0])
        {
            UINT8 appLevel = *(static_cast<UINT8*>(pDataContrast[0]));
            if (appLevel > 0)
            {
                pModuleInput->pHALTagsData->contrastLevel = appLevel - 1;
            }
            else
            {
                pModuleInput->pHALTagsData->contrastLevel = 5;
            }
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Manual Contrast Level = %d", pModuleInput->pHALTagsData->contrastLevel);
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Cannot obtain Contrast Level. Set default to 5");
            pModuleInput->pHALTagsData->contrastLevel = 5;
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "No Contrast Level available. Set default to 5");
        pModuleInput->pHALTagsData->contrastLevel = 5; // normal without contrast change
    }

    // Deep copy tone map curves, only when the tone map is contrast curve
    if (TonemapModeContrastCurve == pModuleInput->pHALTagsData->tonemapCurves.tonemapMode)
    {
        CAMX_ASSERT(NULL != pBlueTonemapCurve);
        CAMX_ASSERT(NULL != pGreenTonemapCurve);
        CAMX_ASSERT(NULL != pRedTonemapCurve);

        pModuleInput->pHALTagsData->tonemapCurves.curvePoints = static_cast<INT32>(
            GetDataCountFromPipeline(InputTonemapCurveRed, 0, GetPipeline()->GetPipelineId(), TRUE));

        if ((pModuleInput->pHALTagsData->tonemapCurves.curvePoints > 0) && (NULL != pRedTonemapCurve))
        {
            // Red tone map curve
            Utils::Memcpy(pModuleInput->pHALTagsData->tonemapCurves.tonemapCurveRed,
                          pRedTonemapCurve,
                          (sizeof(ISPTonemapPoint) * pModuleInput->pHALTagsData->tonemapCurves.curvePoints));
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Error getting red tonemap contrast curve %p data, curvepoints %d,",
                pRedTonemapCurve, pModuleInput->pHALTagsData->tonemapCurves.curvePoints);
        }

        pModuleInput->pHALTagsData->tonemapCurves.curvePoints = static_cast<INT32>(
            GetDataCountFromPipeline(InputTonemapCurveBlue, 0, GetPipeline()->GetPipelineId(), TRUE));

        if ((pModuleInput->pHALTagsData->tonemapCurves.curvePoints > 0) && (NULL != pBlueTonemapCurve))
        {
            // Blue tone map curve
            Utils::Memcpy(pModuleInput->pHALTagsData->tonemapCurves.tonemapCurveBlue,
                          pBlueTonemapCurve,
                          (sizeof(ISPTonemapPoint) * pModuleInput->pHALTagsData->tonemapCurves.curvePoints));
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Error getting blue tonemap contrast curve %p data, curvepoints %d,",
                pBlueTonemapCurve, pModuleInput->pHALTagsData->tonemapCurves.curvePoints);
        }

        pModuleInput->pHALTagsData->tonemapCurves.curvePoints = static_cast<INT32>(
            GetDataCountFromPipeline(InputTonemapCurveGreen, 0, GetPipeline()->GetPipelineId(), TRUE));

        if ((pModuleInput->pHALTagsData->tonemapCurves.curvePoints > 0) && (NULL != pGreenTonemapCurve))
        {
            // Green tone map curve
            Utils::Memcpy(pModuleInput->pHALTagsData->tonemapCurves.tonemapCurveGreen,
                          pGreenTonemapCurve,
                          (sizeof(ISPTonemapPoint) * pModuleInput->pHALTagsData->tonemapCurves.curvePoints));
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Error getting green tonemap contrast curve %p data, curvepoints %d,",
                pGreenTonemapCurve, pModuleInput->pHALTagsData->tonemapCurves.curvePoints);
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetEISMargin
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetEISMargin()
{
    CamxResult    result          = CamxResultSuccess;
    UINT32        marginEISV2Tag  = 0;
    UINT32        marginEISV3Tag  = 0;

    if (0 != (IPEStabilizationType::IPEStabilizationTypeEIS2 & m_instanceProperty.stabilizationType))
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime", "RequestedMargin", &marginEISV2Tag);
        CAMX_ASSERT(CamxResultSuccess == result);

        UINT   marginTags[1] = { marginEISV2Tag | UsecaseMetadataSectionMask };
        VOID*  pData[1]      = { 0 };
        UINT64 offset[1]     = { 0 };

        result = GetDataList(marginTags, pData, offset, CAMX_ARRAY_SIZE(marginTags));
        if (CamxResultSuccess == result)
        {
            if (NULL != pData[0])
            {
                m_EISMarginRequest = *static_cast<MarginRequest*>(pData[0]);
            }
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "EISv2 margin requested: W %f, H %f", m_EISMarginRequest.widthMargin,
                         m_EISMarginRequest.heightMargin);
    }
    else if (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType))
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eislookahead", "RequestedMargin", &marginEISV3Tag);
        CAMX_ASSERT(CamxResultSuccess == result);

        UINT   marginTags[1] = { marginEISV3Tag | UsecaseMetadataSectionMask };
        VOID*  pData[1]      = { 0 };
        UINT64 offset[1]     = { 0 };

        result = GetDataList(marginTags, pData, offset, CAMX_ARRAY_SIZE(marginTags));
        if (CamxResultSuccess == result)
        {

            if (NULL != pData[0])
            {
                m_EISMarginRequest = *static_cast<MarginRequest*>(pData[0]);
            }
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "EISv3 margin requested:  W %f, H %f", m_EISMarginRequest.widthMargin,
                         m_EISMarginRequest.heightMargin);
    }
    else if (0 != m_instanceProperty.stabilizationType)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "No margin for stabilization type %d", m_instanceProperty.stabilizationType);
    }

    CAMX_ASSERT(m_EISMarginRequest.widthMargin == m_EISMarginRequest.heightMargin);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateClock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::UpdateClock(
    ExecuteProcessRequestData*   pExecuteProcessRequestData,
    IpeFrameProcessData*         pFrameProcessData,
    IPEClockAndBandwidth*        pIPEClockAndBandwidth)
{
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT64                  requestId        = pNodeRequestData->pCaptureRequest->requestId;
    UINT                    frameCycles;
    UINT                    FPS               = DefaultFPS;
    UINT64                  budgetNS;
    FLOAT                   budget;

    // Framecycles calculation considers Number of Pixels processed in the current frame, Overhead and Efficiency
    if (0 != m_FPS)
    {
        FPS = m_FPS;
    }

    frameCycles                             = pIPEClockAndBandwidth->frameCycles;
    frameCycles                             = static_cast<UINT>(frameCycles / m_IPEClockEfficiency);
    // Budget is the Max duration of current frame to process
    budget                                  = 1.0f / FPS;
    budgetNS                                = static_cast<UINT64>(budget * NanoSecondMult);

    pIPEClockAndBandwidth->budgetNS     = budgetNS;
    pIPEClockAndBandwidth->frameCycles  = frameCycles;
    pIPEClockAndBandwidth->realtimeFlag = m_realTimeIPE;

    // Add a three frame delay for  targetCompletiontime for FW to timeout
    pFrameProcessData->targetCompletionTimeInNs = budgetNS * m_IPETimeoutInNumFrames * m_maxBatchSize;

    CAMX_LOG_INFO(CamxLogGroupPower, "[%s][%llu] FPS=%d budget=%lf budgetNS=%lld fc=%d,targetCompletionTimeInNs=%u,"
                     "batchSize %d", NodeIdentifierString(), requestId, FPS, budget, budgetNS, frameCycles,
                     pFrameProcessData->targetCompletionTimeInNs, m_maxBatchSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateUBWCCompressionRatio
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::UpdateUBWCCompressionRatio(
    UINT   srcWidth,
    UINT   srcHeight,
    FLOAT  bpp,
    UINT32 ubwcVersion,
    BOOL   ubwcLossy,
    FLOAT* pIPEUbwcRdCr,
    FLOAT* pIPEUbwcMCTFr,
    FLOAT* pIPEUBWCWrCr)
{
    if (UBWCVersion::UBWCVersion2 == ubwcVersion)
    {
        // Setting UBWC 2.0 Compression ratio
        if (IPEBpp10Bit == bpp)
        {
            *pIPEUbwcRdCr  = IPEUBWCvr2RdCompressionRatio10Bit;
            *pIPEUbwcMCTFr = IPEUBWCvr2MctfReadCompressionRatio10Bit;
            *pIPEUBWCWrCr  = IPEUBWCvr2WrPreviewCompressionRatio10Bit;
        }
        else
        {
            *pIPEUbwcRdCr  = IPEUBWCvr2RdCompressionRatio8Bit;
            *pIPEUbwcMCTFr = IPEUBWCvr2MctfReadCompressionRatio8Bit;
            *pIPEUBWCWrCr  = IPEUBWCvr2WrPreviewCompressionRatio8Bit;
        }
    }
    else if (UBWCVersion::UBWCVersion3 == ubwcVersion)
    {
        // Setting UBWC 3.0 Compression ratio
        if (ImageFormatUtils::IsUHDResolution(srcWidth, srcHeight))
        {
            if (IPEBpp10Bit == bpp)
            {
                if (TRUE == ubwcLossy)
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd4k10bitCompressionRatioLossy;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd4k10bitCompressionRatioLossy;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr4k10bitCompressionRatioLossy;
                }
                else
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd4k10bitCompressionRatioLossless;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd4k10bitCompressionRatioLossless;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr4k10bitCompressionRatioLossless;
                }
            }
            else
            {
                if (TRUE == ubwcLossy)
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd4k8bitCompressionRatioLossy;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd4k8bitCompressionRatioLossy;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr4k8bitCompressionRatioLossy;
                }
                else
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd4k8bitCompressionRatioLossless;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd4k8bitCompressionRatioLossless;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr4k8bitCompressionRatioLossless;
                }
            }
        }
        else
        {
            // Setting UBWC 3.0 Compression ratio for NON- UHD resolution
            if (IPEBpp10Bit == bpp)
            {
                if (TRUE == ubwcLossy)
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd1080p10bitCompressionRatioLossy;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd1080p10bitCompressionRatioLossy;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr1080p10bitCompressionRatioLossy;
                }
                else
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd1080p10bitCompressionRatioLossless;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd1080p10bitCompressionRatioLossless;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr1080p10bitCompressionRatioLossless;
                }
            }
            else
            {
                if (TRUE == ubwcLossy)
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd1080p8bitCompressionRatioLossy;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd1080p8bitCompressionRatioLossy;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr1080p8bitCompressionRatioLossy;
                }
                else
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd1080p8bitCompressionRatioLossless;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd1080p8bitCompressionRatioLossless;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr1080p8bitCompressionRatioLossless;
                }
            }
        }
    }
    else
    {
        // Setting UBWC 4.0 Compression ratio
        if (ImageFormatUtils::IsUHDResolution(srcWidth, srcHeight))
        {
            if (IPEBpp10Bit == bpp)
            {
                if (TRUE == ubwcLossy)
                {
                    *pIPEUbwcRdCr  = IPEUBWCv4Rd4k10bitCompressionRatio;
                    *pIPEUbwcMCTFr = IPETFUBWCv4Rd4k10bitCompressionRatio;
                    *pIPEUBWCWrCr  = IPEUBWCv4Wr4k10bitCompressionRatio;
                }
                else
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd4k10bitCompressionRatioLossless;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd4k10bitCompressionRatioLossless;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr4k10bitCompressionRatioLossless;
                }
            }
            else
            {
                if (TRUE == ubwcLossy)
                {
                    *pIPEUbwcRdCr  = IPEUBWCv4Rd4k8bitCompressionRatio;
                    *pIPEUbwcMCTFr = IPETFUBWCv4Rd4k8bitCompressionRatio;
                    *pIPEUBWCWrCr  = IPEUBWCv4Wr4k8bitCompressionRatio;
                }
                else
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd4k8bitCompressionRatioLossless;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd4k8bitCompressionRatioLossless;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr4k8bitCompressionRatioLossless;
                }
            }
        }
        else
        {
            // Setting UBWC 4.0 Compression ratio for NON- UHD resolution
            if (IPEBpp10Bit == bpp)
            {
                if (TRUE == ubwcLossy)
                {
                    *pIPEUbwcRdCr  = IPEUBWCv4Rd1080p10bitCompressionRatio;
                    *pIPEUbwcMCTFr = IPETFUBWCv4Rd1080p10bitCompressionRatio;
                    *pIPEUBWCWrCr  = IPEUBWCv4Wr1080p10bitCompressionRatio;
                }
                else
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd1080p10bitCompressionRatioLossless;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd1080p10bitCompressionRatioLossless;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr1080p10bitCompressionRatioLossless;
                }
            }
            else
            {
                if (TRUE == ubwcLossy)
                {
                    *pIPEUbwcRdCr  = IPEUBWCv4Rd1080p8bitCompressionRatio;
                    *pIPEUbwcMCTFr = IPETFUBWCv4Rd1080p8bitCompressionRatio;
                    *pIPEUBWCWrCr  = IPEUBWCv4Wr1080p8bitCompressionRatio;
                }
                else
                {
                    *pIPEUbwcRdCr  = IPEUBWCv3Rd1080p8bitCompressionRatioLossless;
                    *pIPEUbwcMCTFr = IPETFUBWCv3Rd1080p8bitCompressionRatioLossless;
                    *pIPEUBWCWrCr  = IPEUBWCv3Wr1080p8bitCompressionRatioLossless;
                }
            }
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPower,
                     "IPE RD/WR srcWidth: %d srcHeight: %d bpp: %f ubwcVersion: %d ubwcLossy: %d IPEUbwcRdCr: %f"
                     "IPEUbwcMCTFr: %f IPEUBWCWrCr: %f",
                     srcWidth, srcHeight, bpp, ubwcVersion, ubwcLossy, *pIPEUbwcRdCr, *pIPEUbwcMCTFr, *pIPEUBWCWrCr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::CalculateIPERdBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::CalculateIPERdBandwidth(
    PerRequestActivePorts*  pPerRequestPorts,
    IPEClockAndBandwidth*   pIPEClockAndBandwidth)
{
    UINT   srcWidth               = 0;
    UINT   srcHeight              = 0;
    FLOAT  bppSrc                 = IPEBpp8Bit;
    FLOAT  overhead               = IPEBandwidthOverhead;
    FLOAT  EISOverhead            = IPEEISOverhead;
    FLOAT  IPEUbwcRdCr            = IPEUBWCRdCompressionRatio;
    FLOAT  IPEUbwcMCTFr           = IPEUBWCMctfReadCompressionRatio;
    FLOAT  IPEUBWCWrCr            = 0.0F;
    DOUBLE swMargin               = IPESwMargin;
    BOOL   UBWCEnable             = FALSE;
    UINT32 UBWCVersion            = 0;
    BOOL   UBWCLossyMode          = FALSE;
    UINT   FPS                    = pIPEClockAndBandwidth->FPS;
    UINT64 readBandwidthPartial   = 0;
    UINT64 readBandwidthPass0     = 0;
    UINT64 readBandwidthPass1     = 0;
    UINT64 readBandwidthPass2     = 0;
    UINT64 readBandwidthPass3     = 0;
    UINT64 readReferenceBandwidth = 0;

    pIPEClockAndBandwidth->inputReadBW.unCompressedBW    = 0;
    pIPEClockAndBandwidth->inputReadBW.compressedBW      = 0;

    pIPEClockAndBandwidth->refInputReadBW.unCompressedBW = 0;
    pIPEClockAndBandwidth->refInputReadBW.compressedBW   = 0;

    if (FALSE == IsEISEnabled())
    {
        // EIS is disabled. make the EIS overhead factor to 1.0.
        EISOverhead = 1.0F;
    }

    for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
    {
        PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];

        if (pInputPort->portId == CSLIPEInputPortIdFull)
        {
            const ImageFormat* pImageFormat = GetInputPortImageFormat(InputPortIndex(pInputPort->portId));

            GetSizeWithoutStablizationMargin(m_fullInputWidth, m_fullInputHeight, &srcWidth, &srcHeight);

            if (srcHeight < IPEPartialRdSourceHeight)
            {
                readBandwidthPartial = IPEPartialRdMultiplication * FPS;
                readBandwidthPartial = (readBandwidthPartial * srcHeight) / IPEPartialRdSourceHeight;
            }
            else
            {
                readBandwidthPartial = IPEPartialRdMultiplication * FPS;
            }

            pIPEClockAndBandwidth->partialBW = readBandwidthPartial;

            if (NULL != pImageFormat)
            {
                if ((TRUE == ImageFormatUtils::Is10BitFormat(pImageFormat->format)))
                {
                    bppSrc = IPEBpp10Bit;
                }

                UBWCEnable       = ImageFormatUtils::IsUBWC(pImageFormat->format);
                UBWCVersion      = pImageFormat->ubwcVerInfo.version;
                UBWCLossyMode    = pImageFormat->ubwcVerInfo.lossy;
            }

            break;
        }
    }

    if (FALSE == m_realTimeIPE)
    {
        // Pass0_RdAB = (((src_ImgW/64/2 * src_ImgH/64/2) * 8 * jpegOvhd ) * fps
        readBandwidthPass0 = static_cast<UINT64>(
                             (((srcWidth / 64.0 / 2) * (srcHeight / 64.0 / 2)) * 8 * IPESnapshotOverhead) * FPS);

        // Pass1_RdAB = ((src_ImgW/16/2 * src_ImgH/16/2 * 8 * jpegOvhd ) +
        // ((src_ImgW/64/2 * src_ImgH/64/2 * 102)/8 * jpegOvhd) ) * fps
        readBandwidthPass1 = static_cast<UINT64>(
                             (((srcWidth / 16.0 / 2) * (srcHeight / 16.0 / 2)) * 8 * IPESnapshotOverhead) +
                             ((((((srcWidth / 64.0 / 2) * (srcHeight / 64.0 / 2)) * 102) / 8.0) * IPESnapshotOverhead) * FPS));

        // Pass2_RdAB = ((src_ImgW/4/2 * src_ImgH/4/2 * 8 * jpegOvhd ) +
        // ((src_ImgW/16/2 * src_ImgH/16/2 * 102)/8 * jpegOvhd) ) * fps
        readBandwidthPass2 = static_cast<UINT64>(
                             (((srcWidth / 4.0 / 2) * (srcHeight / 4.0 / 2)) * 8 * IPESnapshotOverhead) +
                             ((((((srcWidth / 16.0 / 2) * (srcHeight / 16.0 / 2)) * 102) / 8.0) * IPESnapshotOverhead)* FPS));

        // Pass3_RdAB = ((src_ImgW  *  src_ImgH  *   jpegRdBPP * Ovhd/IPE_UBWC_RdCr )  +
        // ((src_ImgW/4/2 * src_imgH/4/2 * 102)/8) ) * fps
        readBandwidthPass3 = static_cast<UINT64>(
                             ((srcWidth * srcHeight) * IPESnapshotRdBPP10bit * IPESnapshotOverhead / IPEUbwcRdCr) +
                             ((((((srcWidth / 4.0 / 2) * (srcHeight / 4.0 / 2)) * 102) / 8.0)) * FPS));

        // IPE_RdAB_Frame   =  (Pass0_RdAB (DS64) + Pass1_RdAB (DS16) + Pass2_RdAB (DS4) + Pass3_RdAB (1:1)) * SW_Margin
        pIPEClockAndBandwidth->inputReadBW.unCompressedBW = static_cast<UINT64>((readBandwidthPass0 +
                                                                                 readBandwidthPass1 +
                                                                                 readBandwidthPass2 +
                                                                                 readBandwidthPass3) * swMargin);

        CAMX_LOG_VERBOSE(CamxLogGroupPower,
                         "Snapshot bw: sw = %d sh = %d FPS = %d Pass0: %d Pass1:%llu Pass2: %llu Pass3: %llu BW = %llu",
                         srcWidth, srcHeight, FPS,
                         readBandwidthPass0, readBandwidthPass1, readBandwidthPass2, readBandwidthPass3,
                         pIPEClockAndBandwidth->inputReadBW.unCompressedBW);

        if (TRUE == UBWCEnable)
        {
            IPEUbwcRdCr  = IPEUBWCvr2RdCompressionRatio10Bit; // As BPS output is always 10bit

            readBandwidthPass3 = static_cast<UINT64>(
                                 ((srcWidth * srcHeight) * IPESnapshotRdBPP10bit * IPESnapshotOverhead / IPEUbwcRdCr) +
                                 ((((((srcWidth / 4.0 / 2) * (srcHeight / 4.0 / 2)) * 102) / 8.0)) * FPS));

            pIPEClockAndBandwidth->inputReadBW.compressedBW = static_cast<UINT64>((readBandwidthPass0 +
                                                                   readBandwidthPass1 +
                                                                   readBandwidthPass2 +
                                                                   readBandwidthPass3) * swMargin);

            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                             "Snapshot cbw: sw = %d sh = %d IPEUbwcRdCr = %f FPS = %d "
                             "Pass0: %llu Pass1:%llu Pass2:%llu Pass3:%llu cbw = %llu",
                             srcWidth, srcHeight, IPEUbwcRdCr, FPS,
                             readBandwidthPass0, readBandwidthPass1, readBandwidthPass2, readBandwidthPass3,
                             pIPEClockAndBandwidth->inputReadBW.compressedBW);
        }
        else
        {
            pIPEClockAndBandwidth->inputReadBW.compressedBW = pIPEClockAndBandwidth->inputReadBW.unCompressedBW;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "Snapshot Rd: cbw = %llu bw = %llu",
            pIPEClockAndBandwidth->inputReadBW.compressedBW, pIPEClockAndBandwidth->inputReadBW.unCompressedBW);
    }
    else
    {
        // Calculate Uncompressed Bandwidth
        // Pass0_RdAB(DS16) = ( ((src_Img_w/DS16/N_PDT * src_Img_H/DS16/N_PDT) * PD_TS * Overhead * NS  ) * fps
        readBandwidthPass0 = static_cast<UINT64>((((srcWidth / 16.0 / 2) * (srcHeight / 16.0 / 2)) * 8 * overhead * 2) * FPS);

        // Pass1_RdAB(DS4) = ( ((src_Img_w/DS4/N_PDT * src_Img_H/DS4/N_PDT) * PD_TS * Overhead * NS  ) +
        //                  ((src_Img_W/DS16/N_PDT * src_Img_H/DS16/N_PDT * PDI_bits)/8 * Oveahead) +
        //                  ((src_Img_W/DS16 * src_Img_H/DS16 * TFI_bits)/8  * Overhead)   ) * fps
        readBandwidthPass1 = static_cast<UINT64>(
                             (((srcWidth / 4.0 / 2) * (srcHeight / 4.0 / 2)) * 8 * overhead * 2) +
                             (((((srcWidth / 16.0 / 2) * (srcHeight / 16.0 / 2)) * 102) / 8.0) * overhead) +
                             (((((srcWidth/16.0) * (srcHeight/16.0)) * 4) / 8.0) * overhead)
                             ) * FPS;

        // Pass2_RdAB(1:1) = ( (src_Img_W * src_Img_H * Bytes_per_pix * Overhead /  UBWC_Comp * fmt)  +
        //                  ((src_Img_W/DS4/N_PDT * src_img_H/DS4/N_PDT * PDI_bits)/8 * Overhead) +
        //                  ((src_ImgW/DS4 * src_imgH/DS4 * TFI_bits)/8) *Ovearhead ) * fps
        readBandwidthPass2 = static_cast<UINT64>(
            ((srcWidth * srcHeight * bppSrc * overhead * EISOverhead) / IPEUbwcRdCr) +
            ((((srcWidth/4.0/2) * (srcHeight/4.0/2)) * 102) / 8.0) +
            ((((srcWidth/4.0) * (srcHeight/4.0)) * 4) / 8.0)) * FPS;

        pIPEClockAndBandwidth->inputReadBW.unCompressedBW = static_cast<UINT64>(
            (readBandwidthPass0 + readBandwidthPass1 + readBandwidthPass2 + readBandwidthPartial)* swMargin);


        if (TRUE == IsLoopBackPortEnabled())
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPower, "Including MCTF BW");
            readReferenceBandwidth  = static_cast<UINT64>((srcWidth * srcHeight * bppSrc * overhead) / IPEUbwcMCTFr);
            readReferenceBandwidth *= swMargin * FPS;
            pIPEClockAndBandwidth->refInputReadBW.unCompressedBW = readReferenceBandwidth;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPower,
            "Preview/Video bw: sw = %d sh = %d Pass0:%llu Pass1:%llu Pass2:%llu  reference %llu, pr: %llu BW = %llu, ref BW",
            srcWidth, srcHeight, readBandwidthPass0, readBandwidthPass1, readBandwidthPass2,
            readReferenceBandwidth, readBandwidthPartial,
            pIPEClockAndBandwidth->inputReadBW.unCompressedBW,
            pIPEClockAndBandwidth->refInputReadBW.unCompressedBW);

        // Calculate Compressed Bandwidth
        if (TRUE == UBWCEnable)
        {
            UpdateUBWCCompressionRatio(srcWidth,
                                       srcHeight,
                                       bppSrc,
                                       UBWCVersion,
                                       UBWCLossyMode,
                                       &IPEUbwcRdCr,
                                       &IPEUbwcMCTFr,
                                       &IPEUBWCWrCr);

            readBandwidthPass2 = static_cast<UINT64>(
                ((srcWidth * srcHeight * bppSrc * overhead * EISOverhead) / IPEUbwcRdCr) +
                ((((srcWidth/4.0/2) * (srcHeight/4.0/2)) * 102) / 8.0) +
                ((((srcWidth/4.0) * (srcHeight/4.0)) * 4) / 8.0)) * FPS;

            pIPEClockAndBandwidth->inputReadBW.compressedBW = static_cast<UINT64>(
                (readBandwidthPass0 + readBandwidthPass1 + readBandwidthPass2 + readBandwidthPartial) * swMargin);

            if (TRUE == IsLoopBackPortEnabled())
            {
                readReferenceBandwidth = static_cast<UINT64>((srcWidth * srcHeight * bppSrc * overhead) / IPEUbwcMCTFr);
                readReferenceBandwidth *= FPS;
            }

            pIPEClockAndBandwidth->refInputReadBW.compressedBW = readReferenceBandwidth;

            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                "bppSrc %f overhead %f IPEUbwcRdCr %f IPEUbwcMCTFr %f swMargin %d UBWCEnable %d FPS %d",
                bppSrc, overhead, IPEUbwcRdCr, IPEUbwcMCTFr, swMargin, UBWCEnable, FPS);

            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                "Preview/Video cbw: sw = %d sh = %d Pass0:%llu Pass1:%llu Pass2:%llu refread %llu, pr: %llu cbw = %llu",
                srcWidth, srcHeight, readBandwidthPass0, readBandwidthPass1, readBandwidthPass2, readReferenceBandwidth,
                readBandwidthPartial, pIPEClockAndBandwidth->inputReadBW.compressedBW,
                pIPEClockAndBandwidth->refInputReadBW.compressedBW);
        }
        else
        {
            pIPEClockAndBandwidth->inputReadBW.compressedBW    = pIPEClockAndBandwidth->inputReadBW.unCompressedBW;
            pIPEClockAndBandwidth->refInputReadBW.compressedBW = pIPEClockAndBandwidth->refInputReadBW.unCompressedBW;

        }

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "Preview/Video Rd: cbw = %llu bw = %llu, reference Read cbw = %llu, bw = %llu",
            pIPEClockAndBandwidth->inputReadBW.compressedBW,
            pIPEClockAndBandwidth->inputReadBW.unCompressedBW,
            pIPEClockAndBandwidth->refInputReadBW.compressedBW,
            pIPEClockAndBandwidth->refInputReadBW.unCompressedBW);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::CalculateIPEWrBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::CalculateIPEWrBandwidth(
    PerRequestActivePorts*  pPerRequestPorts,
    IPEClockAndBandwidth*        pIPEClockAndBandwidth)
{
    IPEImageInfo        source;
    IPEImageInfo        video;
    IPEImageInfo        preview;
    BOOL                video_enable;
    BOOL                previewEnable;
    DOUBLE              swMargin              = IPESwMargin;
    FLOAT               IPEUbwcPreviewCr      = IPEUBWCWrPreviewCompressionRatio;
    FLOAT               IPEUbwcVideoCr        = IPEUBWCWrVideoCompressionRatio;
    DOUBLE              IPEUbwcMCTFCr         = IPEUBWCWrMctfCompressionRatio;
    FLOAT               IPEUbwcRdCr           = 0.0F;
    FLOAT               IPEUbwcMCTFr          = 0.0F;
    FLOAT               IPEUBWCWrCr           = 0.0F;
    UINT                FPS                   = pIPEClockAndBandwidth->FPS;
    UINT                previewFPS            = pIPEClockAndBandwidth->FPS;
    UINT64              writeBandwidthPass0;
    UINT64              writeBandwidthPass1;
    UINT64              writeBandwidthPass2;
    UINT64              writeBandwidthPass3;
    UINT64              writeBandwidthPartial;
    UINT64              writeReferenceBandwidth;

    UINT                numBuffers;

    pIPEClockAndBandwidth->displayWriteBW.compressedBW     = 0;
    pIPEClockAndBandwidth->displayWriteBW.unCompressedBW   = 0;
    pIPEClockAndBandwidth->videoWriteBW.compressedBW       = 0;
    pIPEClockAndBandwidth->videoWriteBW.unCompressedBW     = 0;
    pIPEClockAndBandwidth->refOutputWriteBW.compressedBW   = 0;
    pIPEClockAndBandwidth->refOutputWriteBW.unCompressedBW = 0;
    pIPEClockAndBandwidth->inputWriteBW.compressedBW       = 0;
    pIPEClockAndBandwidth->inputWriteBW.unCompressedBW     = 0;

    source.width                              = 0;
    source.height                             = 0;
    source.bpp                                = IPEBpp8Bit;
    source.UBWCEnable                         = FALSE;
    source.UBWCLossyMode                      = FALSE;

    if (m_maxBatchSize > 1)
    {
        previewFPS = FPS / m_maxBatchSize;
    }

    // Check UBWC and BPP Info for Input Ports and get Dimesions
    for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
    {
        PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];
        if (pInputPort->portId == CSLIPEInputPortIdFull)
        {
            const ImageFormat* pImageFormat = GetInputPortImageFormat(InputPortIndex(pInputPort->portId));
            GetSizeWithoutStablizationMargin(m_fullInputWidth, m_fullInputHeight, &source.width, &source.height);

            if (NULL != pImageFormat)
            {
                if ((TRUE == ImageFormatUtils::Is10BitFormat(pImageFormat->format)))
                {
                    source.bpp                  = IPEBpp10Bit;
                }

                source.UBWCEnable               = ImageFormatUtils::IsUBWC(pImageFormat->format);
                source.UBWCVersion              = pImageFormat->ubwcVerInfo.version;
                source.UBWCLossyMode            = pImageFormat->ubwcVerInfo.lossy;
            }
            break;
        }
    }
    if (FALSE == m_realTimeIPE)
    {
        // Pass0_WrAB = ( ((src_ImgW/64/2 * src_ImgH/64/2 * 102)/8) ) * fps

        writeBandwidthPass0 = static_cast<UINT64>(((((source.width/64.0/2) * (source.height/64.0/2)) * 102) / 8.0) * FPS);

        // Pass1_WrAB = (((src_ImgW/16/2 * src_ImgH/16/2 * 102)/8) )  * fps

        writeBandwidthPass1 = static_cast<UINT64>(((((source.width/16.0/2) * (source.height/16.0/2)) * 102) / 8.0) * FPS);

        // Pass2_WrAB = ( ((src_ImgW/4/2 * src_ImgH/4/2 * 102)/8) )  *  fps

        writeBandwidthPass2 = static_cast<UINT64>(((((source.width/4.0/2) * (source.height/4.0/2)) * 102) / 8.0) * FPS);

        // Pass3_WrAB = ((src_ImgW  *  src_ImgH     *  jpegWrBPP) )  * fps

        writeBandwidthPass3 = static_cast<UINT64>((((source.width) * (source.height)) * IPESnapshotWrBPP8bit) * FPS);

        // IPE_WrAB_Frame   =  (Pass0_WrAB (DS64) + Pass1_WrAB (DS16) + Pass2_WrAB (DS4) + Pass3_WrAB (1:1))*SW_Margin

        pIPEClockAndBandwidth->inputWriteBW.unCompressedBW = static_cast<UINT64>(
            (writeBandwidthPass0 + writeBandwidthPass1 + writeBandwidthPass2 ) * swMargin);

        pIPEClockAndBandwidth->inputWriteBW.compressedBW = pIPEClockAndBandwidth->inputWriteBW.unCompressedBW;

        pIPEClockAndBandwidth->displayWriteBW.unCompressedBW = static_cast<UINT64>(writeBandwidthPass3 * swMargin);
        pIPEClockAndBandwidth->displayWriteBW.compressedBW   = pIPEClockAndBandwidth->displayWriteBW.unCompressedBW;


        CAMX_LOG_VERBOSE(CamxLogGroupPower, "Snapshot bw: pass0:%llu pass1:%llu pass2:%llu pass3 = %llu BW = %llu,disp= %llu",
                         writeBandwidthPass0, writeBandwidthPass1, writeBandwidthPass2, writeBandwidthPass2,
                         pIPEClockAndBandwidth->inputWriteBW.compressedBW,
                         pIPEClockAndBandwidth->displayWriteBW.compressedBW);
    }
    else
    {
        video_enable                              = FALSE;
        video.width                               = 0;
        video.height                              = 0;
        video.bpp                                 = IPEBpp8Bit;
        video.UBWCEnable                          = FALSE;
        video.UBWCLossyMode                       = FALSE;

        previewEnable                             = FALSE;
        preview.width                             = 0;
        preview.height                            = 0;
        preview.bpp                               = IPEBpp8Bit;
        preview.UBWCEnable                        = FALSE;
        preview.UBWCLossyMode                     = FALSE;

        // Check UBWC and BPP Info for Output Ports and get Dimensions
        for (UINT i = 0; i < pPerRequestPorts->numOutputPorts; i++)
        {
            PerRequestOutputPortInfo* pOutputPort   = &pPerRequestPorts->pOutputPorts[i];
            numBuffers                              = pOutputPort->numOutputBuffers;
            if (IPEOutputPortVideo == pOutputPort->portId)
            {
                const ImageFormat* pImageFormatVideo = GetOutputPortImageFormat(OutputPortIndex(pOutputPort->portId));
                if (NULL != pImageFormatVideo)
                {
                    video.width  = pImageFormatVideo->width;
                    video.height = pImageFormatVideo->height;
                    video_enable = TRUE;
                    if (TRUE == ImageFormatUtils::Is10BitFormat(pImageFormatVideo->format))
                    {
                        video.bpp = IPEBpp10Bit;
                    }

                    video.UBWCEnable    = ImageFormatUtils::IsUBWC(pImageFormatVideo->format);
                    video.UBWCVersion   = pImageFormatVideo->ubwcVerInfo.version;
                    video.UBWCLossyMode = pImageFormatVideo->ubwcVerInfo.lossy;
                }
            }
            if (IPEOutputPortDisplay == pOutputPort->portId)
            {
                const ImageFormat* pImageFormatPreview = GetOutputPortImageFormat(OutputPortIndex(pOutputPort->portId));
                if (NULL != pImageFormatPreview)
                {
                    preview.width  = pImageFormatPreview->width;
                    preview.height = pImageFormatPreview->height;
                    previewEnable  = TRUE;
                    if (TRUE == ImageFormatUtils::Is10BitFormat(pImageFormatPreview->format))
                    {
                        preview.bpp = IPEBpp10Bit;
                    }

                    preview.UBWCEnable  = ImageFormatUtils::IsUBWC(pImageFormatPreview->format);
                    preview.UBWCVersion = pImageFormatPreview->ubwcVerInfo.version;
                    video.UBWCLossyMode = pImageFormatPreview->ubwcVerInfo.lossy;
                }
            }
        }

        // Calculate uncompressed bandwidth
        // Pass0_WrAB(DS16) = ( ((src_Img_W/DS16/N_PDT * src_Img_H/DS16/N_PDT * PDI_bits)/8) +
        //                       ((src_Img_W/DS16 * src_Img_H/DS16 * TFI_bits)/8 ) ) * FPS
        writeBandwidthPass0 = static_cast<UINT64>(
            ((((source.width/16.0/2) * (source.height/16.0/2)) * 102) / 8.0) +
            ((((source.width/16.0) * (source.height/16.0)) * 4) / 8.0)) * FPS;

        // Pass1_WrAB = ( (src_Img_W/DS16/N_PDT  *  src_Img_H/DS16/N_PDT* PD_TS)
        //                ((src_Img_W/DS4/N_PDT * src_Img_H/DS4/N_PDT * PDI_bits)/8)  +
        //                ((src_Img_W/DS4 * src_Img_H/DS4 * TFI_bits)/8)   )  *  FPS
        writeBandwidthPass1 = static_cast<UINT64>(
            (((source.width/16.0/2) * (source.height/16.0/2)) * 8) +
            ((((source.width/4.0/2) * (source.height/4.0/2)) * 102) / 8.0) +
            ((((source.width/4.0) * (source.height/4.0)) * 4) / 8.0)) * FPS;

        // Pass2_WrAB = ((src_Img_W  *  src_Img_H  *  Bytes_per_pix  *  UBWC_CompMCTF )   +
        //               (vid_Img_W  *  vid_img_H  *  Bytes_per_pix  /  UBWC_CompVideo )  *  vid_enable  +
        //               (prev_Img_W  *  pre_img_H  *  Bytes_per_pix  / UBWC_CompPrev )  * prev_enable  +
        //               (src_Img_W/DS4/N_PDT  *  src_Img_H/DS4/N_PDT * PD_TS)  )    *  FPS
        writeBandwidthPass2 = static_cast<UINT64>((((source.width/4.0/2)  * (source.height/4.0/2)) * 8));
        writeBandwidthPass2 *= FPS;

        writeBandwidthPartial = static_cast<UINT64>(pIPEClockAndBandwidth->partialBW);

        pIPEClockAndBandwidth->inputWriteBW.unCompressedBW = static_cast<UINT64>(
            (writeBandwidthPass0 + writeBandwidthPass1 + writeBandwidthPass2 + writeBandwidthPartial) * swMargin);

        pIPEClockAndBandwidth->displayWriteBW.unCompressedBW = static_cast<UINT64>(
            (((preview.width * preview.height * preview.bpp) / IPEUbwcPreviewCr) * previewEnable * previewFPS));
        pIPEClockAndBandwidth->videoWriteBW.unCompressedBW   =
            static_cast<UINT64>((((video.width * video.height * video.bpp) / IPEUbwcVideoCr) * video_enable * FPS));

        if (TRUE == IsLoopBackPortEnabled())
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPower, "Including MCTF BW");
            writeReferenceBandwidth = static_cast<UINT64>((source.width * source.height * source.bpp * FPS) / IPEUbwcMCTFCr);
            pIPEClockAndBandwidth->refOutputWriteBW.unCompressedBW = writeReferenceBandwidth;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "Preview/Video bw: srcw=%u srch=%u vidw=%u vidh=%u prevw=%u prevh=%u",
            source.width, source.height, video.width, video.height, preview.width, preview.height);

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "Preview/Video bw pass0:%llu pass1:%llu pass2:%llu pw=%llu uBW=%llu,ref BW=%llu"
            "displayWrite=%llu, videoWrite = %llu",
            writeBandwidthPass0, writeBandwidthPass1, writeBandwidthPass2, writeBandwidthPartial,
            pIPEClockAndBandwidth->inputWriteBW.unCompressedBW, pIPEClockAndBandwidth->refOutputWriteBW.unCompressedBW,
            pIPEClockAndBandwidth->displayWriteBW.unCompressedBW, pIPEClockAndBandwidth->videoWriteBW.unCompressedBW);

        // Calculate Compressed bandwidth
        if ((TRUE == preview.UBWCEnable) || (TRUE == video.UBWCEnable) || (TRUE == source.UBWCEnable))
        {
            if (TRUE == preview.UBWCEnable)
            {
                UpdateUBWCCompressionRatio(preview.width,
                                           preview.height,
                                           preview.bpp,
                                           preview.UBWCVersion,
                                           preview.UBWCLossyMode,
                                           &IPEUbwcRdCr,
                                           &IPEUbwcMCTFr,
                                           &IPEUBWCWrCr);

                IPEUbwcPreviewCr = IPEUbwcRdCr;
            }

            if (TRUE == video.UBWCEnable)
            {
                UpdateUBWCCompressionRatio(video.width,
                                           video.height,
                                           video.bpp,
                                           video.UBWCVersion,
                                           video.UBWCLossyMode,
                                           &IPEUbwcRdCr,
                                           &IPEUbwcMCTFr,
                                           &IPEUBWCWrCr);

                IPEUbwcVideoCr =IPEUBWCWrCr;
            }

            if (TRUE == source.UBWCEnable)
            {
                UpdateUBWCCompressionRatio(source.width,
                                           source.height,
                                           source.bpp,
                                           source.UBWCVersion,
                                           source.UBWCLossyMode,
                                           &IPEUbwcRdCr,
                                           &IPEUbwcMCTFr,
                                           &IPEUBWCWrCr);

                IPEUbwcMCTFCr = IPEUbwcMCTFr;
            }

            pIPEClockAndBandwidth->inputWriteBW.compressedBW   =
                static_cast<UINT64>((((source.width / 4.0 / 2)  * (source.height / 4.0 / 2)) * 8) * FPS * swMargin);
            pIPEClockAndBandwidth->displayWriteBW.compressedBW = static_cast<UINT64>(
                (((preview.width * preview.height * preview.bpp) / IPEUbwcPreviewCr) * previewEnable * previewFPS * swMargin));
            pIPEClockAndBandwidth->videoWriteBW.compressedBW   = static_cast<UINT64>(
                (((video.width * video.height * video.bpp) / IPEUbwcVideoCr) * video_enable * FPS * swMargin));

            if (TRUE == IsLoopBackPortEnabled())
            {
                pIPEClockAndBandwidth->refOutputWriteBW.compressedBW =
                    static_cast<UINT64>((source.width * source.height * source.bpp * FPS * swMargin) / IPEUbwcMCTFCr);
            }
            pIPEClockAndBandwidth->inputWriteBW.compressedBW  += static_cast<UINT64>(
                (writeBandwidthPass0 + writeBandwidthPass1 + writeBandwidthPartial) * swMargin);

            CAMX_LOG_VERBOSE(CamxLogGroupPower,
                "IPEUbwcPreviewCr %f IPEUbwcVideoCr %f IPEUbwcMCTFCr %f prev.bpp %f video.bpp %f source.bpp %f",
                IPEUbwcPreviewCr, IPEUbwcVideoCr, IPEUbwcMCTFCr, preview.bpp, video.bpp, source.bpp);

            CAMX_LOG_VERBOSE(CamxLogGroupPower, "Preview/Video cbw: srcw=%u srch=%u vidw = %u vidh = %u prevw = %u prevh = %u",
                source.width, source.height, video.width, video.height, preview.width, preview.height);

            CAMX_LOG_VERBOSE(CamxLogGroupPower, "Preview/Video cbw: pass0:%llu pass1:%llu pass2:%llu pw = %llu"
                "i_p BW = %llu, display BW = %llu, video BW = %llu, refOut BW = %llu",
                writeBandwidthPass0, writeBandwidthPass1, writeBandwidthPass2, writeBandwidthPartial,
                pIPEClockAndBandwidth->inputWriteBW.compressedBW,
                pIPEClockAndBandwidth->displayWriteBW.compressedBW,
                pIPEClockAndBandwidth->videoWriteBW.compressedBW,
                pIPEClockAndBandwidth->refOutputWriteBW.compressedBW);
        }
        else
        {
            pIPEClockAndBandwidth->inputWriteBW.compressedBW     = pIPEClockAndBandwidth->inputWriteBW.unCompressedBW;
            pIPEClockAndBandwidth->displayWriteBW.compressedBW   = pIPEClockAndBandwidth->displayWriteBW.unCompressedBW;;
            pIPEClockAndBandwidth->videoWriteBW.compressedBW     = pIPEClockAndBandwidth->videoWriteBW.unCompressedBW;;
            pIPEClockAndBandwidth->refOutputWriteBW.compressedBW = pIPEClockAndBandwidth->refOutputWriteBW.unCompressedBW;;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPower, "Uncompressed:i_p BW = %llu, display BW = %llu, video BW = %llu, refOut BW = %llu"
                     "Compressed: i_p BW = %llu, display BW = %llu, video BW = %llu, refOut BW = %llu",
                     pIPEClockAndBandwidth->inputWriteBW.unCompressedBW,
                     pIPEClockAndBandwidth->displayWriteBW.unCompressedBW,
                     pIPEClockAndBandwidth->videoWriteBW.unCompressedBW,
                     pIPEClockAndBandwidth->refOutputWriteBW.unCompressedBW,
                     pIPEClockAndBandwidth->inputWriteBW.compressedBW,
                     pIPEClockAndBandwidth->displayWriteBW.compressedBW,
                     pIPEClockAndBandwidth->videoWriteBW.compressedBW,
                     pIPEClockAndBandwidth->refOutputWriteBW.compressedBW);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateBandwidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::UpdateBandwidth(
    ExecuteProcessRequestData*   pExecuteProcessRequestData,
    IPEClockAndBandwidth*        pIPEClockAndBandwidth)
{
    PerRequestActivePorts*  pPerRequestPorts   = pExecuteProcessRequestData->pEnabledPortsInfo;
    NodeProcessRequestData* pNodeRequestData   = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT64                  requestId          = pNodeRequestData->pCaptureRequest->requestId;
    // struct IPEBandwidth     bandwidth;
    UINT                    FPS                = DefaultFPS;
    INT64                   requestDelta       = requestId - m_currentrequestID;

    if (0 != m_FPS)
    {
        FPS = m_FPS;
    }

    if (requestDelta > 1)
    {
        FPS = (FPS / requestDelta) ? (FPS / requestDelta) : m_FPS;
    }

    pIPEClockAndBandwidth->FPS = FPS;
    CalculateIPERdBandwidth(pPerRequestPorts, pIPEClockAndBandwidth);
    CalculateIPEWrBandwidth(pPerRequestPorts, pIPEClockAndBandwidth);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::CheckAndUpdateClockBW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::CheckAndUpdateClockBW(
    CmdBuffer*                   pCmdBuffer,
    ExecuteProcessRequestData*   pExecuteProcessRequestData,
    IpeFrameProcessData*         pFrameProcessData,
    IPEClockAndBandwidth*        pIPEClockAndBandwidth)
{
    PerRequestActivePorts*  pPerRequestPorts = pExecuteProcessRequestData->pEnabledPortsInfo;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT64                  requestId        = pNodeRequestData->pCaptureRequest->requestId;

    UpdateClock(pExecuteProcessRequestData, pFrameProcessData, pIPEClockAndBandwidth);
    UpdateBandwidth(pExecuteProcessRequestData, pIPEClockAndBandwidth);

    if (CSLICPGenericBlobCmdBufferClkV2 == m_pISPPipeline->GetICPClockAndBandWidthConfigurationVersion())
    {
        CSLICPClockBandwidthRequestV2* pICPClockBandwidthRequestV2 =
            reinterpret_cast<CSLICPClockBandwidthRequestV2*>(m_pIPEGenericClockAndBandwidthData);
        pICPClockBandwidthRequestV2->budgetNS     = pIPEClockAndBandwidth->budgetNS;
        pICPClockBandwidthRequestV2->frameCycles  = pIPEClockAndBandwidth->frameCycles;
        pICPClockBandwidthRequestV2->realtimeFlag = pIPEClockAndBandwidth->realtimeFlag;

        // Fill all the valid entries.
        pICPClockBandwidthRequestV2->numPaths = CSLAXIPathDataIPEWriteReference - CSLAXIPathDataIPEStartOffset + 1;

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "[%s][%llu] : fps=%d, budget= %lu, frameCycles=%u, realtimeFlag=%d, paths %d",
                         NodeIdentifierString(),
                         requestId,
                         pIPEClockAndBandwidth->FPS,
                         pICPClockBandwidthRequestV2->budgetNS,
                         pICPClockBandwidthRequestV2->frameCycles,
                         pICPClockBandwidthRequestV2->realtimeFlag,
                         pICPClockBandwidthRequestV2->numPaths);

        CSLAXIperPathBWVote* pBWPathPerVote = &pICPClockBandwidthRequestV2->outputPathBWInfo[0];
        pBWPathPerVote->transactionType     = CSLAXITransactionRead;
        pBWPathPerVote->pathDataType        = CSLAXIPathDataIPEReadInput;
        pBWPathPerVote->camnocBW            = pIPEClockAndBandwidth->inputReadBW.unCompressedBW;
        pBWPathPerVote->mnocABBW            = pIPEClockAndBandwidth->inputReadBW.compressedBW;
        pBWPathPerVote->mnocIBBW            = pIPEClockAndBandwidth->inputReadBW.compressedBW;

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "idx0: [%s][%llu] transactionType %d,path %d,camnocBW:[%lu], mnocAB-IB:[%lu, %lu]",
                         NodeIdentifierString(),
                         requestId,
                         pBWPathPerVote->transactionType,
                         pBWPathPerVote->pathDataType,
                         pBWPathPerVote->camnocBW,
                         pBWPathPerVote->mnocABBW,
                         pBWPathPerVote->mnocIBBW);

        pBWPathPerVote                      = &pICPClockBandwidthRequestV2->outputPathBWInfo[1];
        pBWPathPerVote->transactionType     = CSLAXITransactionRead;
        pBWPathPerVote->pathDataType        = CSLAXIPathDataIPEReadRef;
        pBWPathPerVote->camnocBW            = pIPEClockAndBandwidth->refInputReadBW.unCompressedBW;
        pBWPathPerVote->mnocABBW            = pIPEClockAndBandwidth->refInputReadBW.compressedBW;
        pBWPathPerVote->mnocIBBW            = pIPEClockAndBandwidth->refInputReadBW.compressedBW;

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "idx1: [%s][%llu] transactionType %d,path %d,camnocBW:[%lu], mnocAB-IB:[%lu, %lu]",
                         NodeIdentifierString(),
                         requestId,
                         pBWPathPerVote->transactionType,
                         pBWPathPerVote->pathDataType,
                         pBWPathPerVote->camnocBW,
                         pBWPathPerVote->mnocABBW,
                         pBWPathPerVote->mnocIBBW);

        pBWPathPerVote                      = &pICPClockBandwidthRequestV2->outputPathBWInfo[2];
        pBWPathPerVote->transactionType     = CSLAXITransactionWrite;
        pBWPathPerVote->pathDataType        = CSLAXIPathDataIPEWriteDisplay;
        pBWPathPerVote->camnocBW            = pIPEClockAndBandwidth->displayWriteBW.unCompressedBW;
        pBWPathPerVote->mnocABBW            = pIPEClockAndBandwidth->displayWriteBW.compressedBW;
        pBWPathPerVote->mnocIBBW            = pIPEClockAndBandwidth->displayWriteBW.compressedBW;
        CAMX_LOG_VERBOSE(CamxLogGroupPower, "idx2: [%s][%llu] transactionType %d,path %d,camnocBW:[%lu], mnocAB-IB:[%lu, %lu]",
                         NodeIdentifierString(),
                         requestId,
                         pBWPathPerVote->transactionType,
                         pBWPathPerVote->pathDataType,
                         pBWPathPerVote->camnocBW,
                         pBWPathPerVote->mnocABBW,
                         pBWPathPerVote->mnocIBBW);


        pBWPathPerVote                      = &pICPClockBandwidthRequestV2->outputPathBWInfo[3];
        pBWPathPerVote->transactionType     = CSLAXITransactionWrite;
        pBWPathPerVote->pathDataType        = CSLAXIPathDataIPEWriteVideo;
        pBWPathPerVote->camnocBW            = pIPEClockAndBandwidth->videoWriteBW.unCompressedBW;
        pBWPathPerVote->mnocABBW            = pIPEClockAndBandwidth->videoWriteBW.compressedBW;
        pBWPathPerVote->mnocIBBW            = pIPEClockAndBandwidth->videoWriteBW.compressedBW;
        CAMX_LOG_VERBOSE(CamxLogGroupPower, "idx3: [%s][%llu] transactionType %d,path %d,camnocBW:[%lu], mnocAB-IB:[%lu, %lu]",
                         NodeIdentifierString(),
                         requestId,
                         pBWPathPerVote->transactionType,
                         pBWPathPerVote->pathDataType,
                         pBWPathPerVote->camnocBW,
                         pBWPathPerVote->mnocABBW,
                         pBWPathPerVote->mnocIBBW);


        pBWPathPerVote                      = &pICPClockBandwidthRequestV2->outputPathBWInfo[4];
        pBWPathPerVote->transactionType     = CSLAXITransactionWrite;
        pBWPathPerVote->pathDataType        = CSLAXIPathDataIPEWriteReference;
        pBWPathPerVote->camnocBW            = pIPEClockAndBandwidth->inputWriteBW.unCompressedBW +
            pIPEClockAndBandwidth->refOutputWriteBW.unCompressedBW;
        pBWPathPerVote->mnocABBW            = pIPEClockAndBandwidth->inputWriteBW.compressedBW +
            pIPEClockAndBandwidth->refOutputWriteBW.compressedBW;
        pBWPathPerVote->mnocIBBW            = pIPEClockAndBandwidth->inputWriteBW.compressedBW +
            pIPEClockAndBandwidth->refOutputWriteBW.compressedBW;
        CAMX_LOG_VERBOSE(CamxLogGroupPower, "idx4: [%s][%llu] transactionType %d,path %d,camnocBW:[%lu], mnocAB-IB:[%lu, %lu]",
                         NodeIdentifierString(),
                         requestId,
                         pBWPathPerVote->transactionType,
                         pBWPathPerVote->pathDataType,
                         pBWPathPerVote->camnocBW,
                         pBWPathPerVote->mnocABBW,
                         pBWPathPerVote->mnocIBBW);

        // if total mnoc or camnoc BW is zero
        if (((pICPClockBandwidthRequestV2->outputPathBWInfo[0].camnocBW == 0)  &&
             (pICPClockBandwidthRequestV2->outputPathBWInfo[1].camnocBW == 0)) || // camnoc read zero
            ((pICPClockBandwidthRequestV2->outputPathBWInfo[2].camnocBW == 0)  &&
             (pICPClockBandwidthRequestV2->outputPathBWInfo[3].camnocBW == 0)  &&
             (pICPClockBandwidthRequestV2->outputPathBWInfo[4].camnocBW == 0)) || // camnoc write zero
            ((pICPClockBandwidthRequestV2->outputPathBWInfo[0].mnocABBW == 0)  &&
             (pICPClockBandwidthRequestV2->outputPathBWInfo[1].mnocABBW == 0)) || // mnoc read zero
            ((pICPClockBandwidthRequestV2->outputPathBWInfo[2].mnocABBW == 0)  &&
             (pICPClockBandwidthRequestV2->outputPathBWInfo[3].mnocABBW == 0)  &&
             (pICPClockBandwidthRequestV2->outputPathBWInfo[4].mnocABBW == 0)))   // mnoc write zero
        {
            CAMX_LOG_ERROR(CamxLogGroupPower, "%s: req[%llu]: CAMNOC /MNOC Write BW zero"
                           "camnoc index 0-4 : %lu %lu, %lu, %lu %lu"
                           "mnoc index 0-4 : %lu, %lu, %lu, %lu %lu",
                            NodeIdentifierString(), requestId,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[0].camnocBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[1].camnocBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[2].camnocBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[3].camnocBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[4].camnocBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[0].mnocABBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[1].mnocABBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[2].mnocABBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[3].mnocABBW,
                           pICPClockBandwidthRequestV2->outputPathBWInfo[4].mnocABBW);
            OsUtils::RaiseSignalAbort();
        }

        PacketBuilder::WriteGenericBlobData(pCmdBuffer,
                                            CSLICPGenericBlobCmdBufferClkV2,
                                            (sizeof(CSLICPClockBandwidthRequestV2) +
                                            (sizeof(CSLAXIperPathBWVote) * (CSLAXIPathDataIPEMaxNum - 1))),
                                            reinterpret_cast<BYTE*>(pICPClockBandwidthRequestV2));
    }
    else
    {
        CSLICPClockBandwidthRequest* pICPClockBandwidthRequest =
            reinterpret_cast<CSLICPClockBandwidthRequest*>(m_pIPEGenericClockAndBandwidthData);

        pICPClockBandwidthRequest->budgetNS     = pIPEClockAndBandwidth->budgetNS;
        pICPClockBandwidthRequest->frameCycles  = pIPEClockAndBandwidth->frameCycles;
        pICPClockBandwidthRequest->realtimeFlag = pIPEClockAndBandwidth->realtimeFlag;

        pICPClockBandwidthRequest->compressedBW =
            pIPEClockAndBandwidth->inputReadBW.compressedBW     +
            pIPEClockAndBandwidth->refInputReadBW.compressedBW  +
            pIPEClockAndBandwidth->displayWriteBW.compressedBW  +
            pIPEClockAndBandwidth->videoWriteBW.compressedBW    +
            pIPEClockAndBandwidth->inputWriteBW.compressedBW    +
            pIPEClockAndBandwidth->refOutputWriteBW.compressedBW;

        pICPClockBandwidthRequest->unCompressedBW =
            pIPEClockAndBandwidth->inputReadBW.unCompressedBW +
            pIPEClockAndBandwidth->refInputReadBW.unCompressedBW +
            pIPEClockAndBandwidth->displayWriteBW.unCompressedBW +
            pIPEClockAndBandwidth->videoWriteBW.unCompressedBW +
            pIPEClockAndBandwidth->inputWriteBW.unCompressedBW +
            pIPEClockAndBandwidth->refOutputWriteBW.unCompressedBW;

        CAMX_LOG_VERBOSE(CamxLogGroupPower, "[%s][%llu] : fps=%d, "
            "TotalBandwidth (uncompressed = %lld compressed = %lld), ",
            NodeIdentifierString(), requestId, pIPEClockAndBandwidth->FPS,
            pICPClockBandwidthRequest->unCompressedBW, pICPClockBandwidthRequest->compressedBW);

        PacketBuilder::WriteGenericBlobData(pCmdBuffer,
                                            CSLICPGenericBlobCmdBufferClk,
                                            sizeof(CSLICPClockBandwidthRequest),
                                            reinterpret_cast<BYTE*>(pICPClockBandwidthRequest));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::SendFWCmdRegionInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::SendFWCmdRegionInfo(
    UINT32                opcode,
    CSLBufferInfo** const ppBufferInfo,
    UINT32                numberOfMappings)
{
    CamxResult            result          = CamxResultSuccess;
    Packet*               pPacket         = NULL;
    CmdBuffer*            pCmdBuffer      = NULL;
    CSLICPMemoryMapUpdate memoryMapUpdate = { 0 };
    CSLBufferInfo*        pBufferInfo     = NULL;

    pPacket = GetPacket(m_pIQPacketManager);

    if ((NULL != pPacket) && (NULL != ppBufferInfo) && (numberOfMappings <= CSLICPMaxMemoryMapRegions))
    {
        pPacket->SetOpcode(CSLDeviceType::CSLDeviceTypeICP, CSLPacketOpcodesIPEMemoryMapUpdate);
        pCmdBuffer = GetCmdBuffer(m_pIPECmdBufferManager[CmdBufferGenericBlob]);
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
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Send Fw region info failed");
    }

    if (NULL != pCmdBuffer)
    {
        m_pIPECmdBufferManager[CmdBufferGenericBlob]->Recycle(reinterpret_cast<PacketResource*>(pCmdBuffer));
    }
    if (NULL != pPacket)
    {
        m_pIQPacketManager->Recycle(reinterpret_cast<PacketResource*>(pPacket));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::InitializeFrameProcessData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::InitializeFrameProcessData(
    IpeFrameProcessData*    pFrameProcessData,
    IpeIQSettings*          pIPEIQsettings)
{
    Utils::Memset(&pFrameProcessData->frameSets[0], 0x0, sizeof(pFrameProcessData->frameSets));
    pFrameProcessData->stripingLibOutAddr     = 0;
    pFrameProcessData->iqSettingsAddr         = 0;
    pFrameProcessData->ubwcStatsBufferAddress = 0;
    pFrameProcessData->cdmBufferAddress       = 0;
    pFrameProcessData->cdmProgramArrayBase    = 0;

    InitializeIPEIQSettings(pIPEIQsettings);
    InitializeCDMProgramArray(pFrameProcessData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CAMX_ASSERT(NULL != pExecuteProcessRequestData);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pNodeProcessRequestData);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pTuningModeData);

    CamxResult  result    = CamxResultSuccess;
    Packet*     pIQPacket = NULL;
    CmdBuffer*  pIPECmdBuffer[IPECmdBufferMaxIds] = { NULL };

    AECFrameControl         AECUpdateData    = {};
    AWBFrameControl         AWBUpdateData    = {};
    ISPInputData            moduleInput      = {};
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT64                  requestId        = pNodeRequestData->pCaptureRequest->requestId;
    PerRequestActivePorts*  pPerRequestPorts = pExecuteProcessRequestData->pEnabledPortsInfo;
    BOOL                    useDependencies  = GetHwContext()->GetStaticSettings()->enableIPEDependencies;
    UINT                    parentNodeID     = 0;
    INT                     sequenceNumber   = 0;
    UINT32                  cameraId         = 0;
    IPETuningMetadata*      pTuningMetadata  = NULL;
    BOOL                    isMasterCamera   = TRUE;
    BOOL                    isPendingBuffer  = FALSE;
    IpeFrameProcess*        pFrameProcess;
    IpeFrameProcessData*    pFrameProcessData;
    IpeIQSettings*          pIPEIQsettings;

    // Initialize ICA parameters
    moduleInput.ICAConfigData.ICAInGridParams.gridTransformEnable                  = 0;
    moduleInput.ICAConfigData.ICAInInterpolationParams.customInterpolationEnabled  = 0;
    moduleInput.ICAConfigData.ICAInPerspectiveParams.perspectiveTransformEnable    = 0;
    moduleInput.ICAConfigData.ICARefGridParams.gridTransformEnable                 = 0;
    moduleInput.ICAConfigData.ICARefPerspectiveParams.perspectiveTransformEnable   = 0;
    moduleInput.ICAConfigData.ICARefInterpolationParams.customInterpolationEnabled = 0;
    moduleInput.ICAConfigData.ICAReferenceParams.perspectiveTransformEnable        = 0;
    moduleInput.registerBETEn                                                      = FALSE;
    moduleInput.frameNum                                                           = requestId;
    moduleInput.icaVersion                                                         = m_capability.ICAVersion;
    // FD is connected in pipeline with sensor. if the node is part of realtime  then read from offset 1 (from request n-1)
    // otherwise from input data which is always n.
    m_fdDataOffset                                                                 = IsRealTime() ? 1 : 0;

    // PublishICADependencies(pNodeRequestData);

    for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
    {
        PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];

        if ((NULL != pInputPort) && (TRUE == pInputPort->flags.isPendingBuffer))
        {
            isPendingBuffer = TRUE;
        }
        if ((NULL != pInputPort) && (IPEInputPortFull == pInputPort->portId))
        {
            parentNodeID = GetParentNodeType(pInputPort->portId);
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Req ID %llu, sequenceId %d", requestId, pNodeRequestData->processSequenceId);

    if (TRUE == isPendingBuffer)
    {
        sequenceNumber = pNodeRequestData->processSequenceId;
    }
    else
    {
        sequenceNumber = 1;
    }

    if (TRUE == useDependencies)
    {
        sequenceNumber = pNodeRequestData->processSequenceId;
    }

    if (0 == sequenceNumber)
    {
        if (TRUE == useDependencies)
        {
            SetDependencies(pExecuteProcessRequestData, parentNodeID);
        }
    }

    if (1 == sequenceNumber)
    {
        UINT32 numberOfCamerasRunning;
        UINT32 currentCameraId;
        BOOL   isMultiCameraUsecase;

        UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);

        cameraId = GetPipeline()->GetCameraId();

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
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%u Can't get Current camera ID!, pData is NULL", InstanceID());
                    }
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%u Can't query vendor tag: MetadataOwner", InstanceID());
            }
            CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                "node: %s,req ID %llu numberOfCamerasRunning = %d, cameraId = %d isMasterCamera = %d",
                NodeIdentifierString(), requestId, numberOfCamerasRunning, cameraId, isMasterCamera);
        }

        // Assign debug-data
        pTuningMetadata = (TRUE == isMasterCamera) ? m_pTuningMetadata : NULL;

        moduleInput.pTuningDataManager         = GetTuningDataManagerWithCameraId(cameraId);
        moduleInput.pHwContext                 = GetHwContext();
        moduleInput.pipelineIPEData.instanceID = InstanceID();
        moduleInput.titanVersion               = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();

        UpdateICADependencies(&moduleInput);

        // Get CmdBuffer for request
        CAMX_ASSERT(NULL != m_pIQPacketManager);
        CAMX_ASSERT(NULL != m_pIPECmdBufferManager[CmdBufferFrameProcess]);
        CAMX_ASSERT(NULL != m_pIPECmdBufferManager[CmdBufferIQSettings]);

        pIQPacket                            = GetPacketForRequest(requestId, m_pIQPacketManager);
        pIPECmdBuffer[CmdBufferFrameProcess] =
            GetCmdBufferForRequest(requestId, m_pIPECmdBufferManager[CmdBufferFrameProcess]);
        pIPECmdBuffer[CmdBufferIQSettings]   =
            GetCmdBufferForRequest(requestId, m_pIPECmdBufferManager[CmdBufferIQSettings]);
        pIPECmdBuffer[CmdBufferGenericBlob]  =
            GetCmdBufferForRequest(requestId, m_pIPECmdBufferManager[CmdBufferGenericBlob]);

        IPEClockAndBandwidth    IPECbw;
        Utils::Memset(&IPECbw, 0x0, sizeof(IPEClockAndBandwidth));



        if (NULL != m_pIPECmdBufferManager[CmdBufferPreLTM])
        {
            pIPECmdBuffer[CmdBufferPreLTM] = GetCmdBufferForRequest(requestId, m_pIPECmdBufferManager[CmdBufferPreLTM]);
        }

        if (NULL != m_pIPECmdBufferManager[CmdBufferPostLTM])
        {
            pIPECmdBuffer[CmdBufferPostLTM] = GetCmdBufferForRequest(requestId, m_pIPECmdBufferManager[CmdBufferPostLTM]);
        }

        if (NULL != m_pIPECmdBufferManager[CmdBufferDMIHeader])
        {
            pIPECmdBuffer[CmdBufferDMIHeader] = GetCmdBufferForRequest(requestId, m_pIPECmdBufferManager[CmdBufferDMIHeader]);
        }

        if (m_pIPECmdBufferManager[CmdBufferNPS] != NULL)
        {
            pIPECmdBuffer[CmdBufferNPS] = GetCmdBufferForRequest(requestId, m_pIPECmdBufferManager[CmdBufferNPS]);
        }

        // Check for mandatory buffers (even for bypass test)
        if ((NULL == pIQPacket) ||
            (NULL == pIPECmdBuffer[CmdBufferFrameProcess]) ||
            (NULL == pIPECmdBuffer[CmdBufferGenericBlob]) ||
            (NULL == pIPECmdBuffer[CmdBufferIQSettings]))
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc,
                "%s: Null IQPacket or CmdBuffer %x, %x, %x",
                __FUNCTION__,
                pIQPacket,
                pIPECmdBuffer[CmdBufferFrameProcess],
                pIPECmdBuffer[CmdBufferGenericBlob]);

            result = CamxResultENoMemory;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s: Cmdbuffer vaddr request %llu, packet %p Frameprocess %p, Iqsetting %p, "
                    "preLTM %p, postLTM %p, DMI %p, NPS %p, Blob %p",
                    NodeIdentifierString(), requestId,
                    (NULL != pIQPacket) ? pIQPacket->GetHostAddr() : NULL,
                    (NULL != pIPECmdBuffer[CmdBufferFrameProcess]) ? pIPECmdBuffer[CmdBufferFrameProcess]->GetHostAddr() : NULL,
                    (NULL != pIPECmdBuffer[CmdBufferIQSettings]) ? pIPECmdBuffer[CmdBufferIQSettings]->GetHostAddr() : NULL,
                    (NULL != pIPECmdBuffer[CmdBufferPreLTM]) ? pIPECmdBuffer[CmdBufferPreLTM]->GetHostAddr() : NULL,
                    (NULL != pIPECmdBuffer[CmdBufferPostLTM]) ? pIPECmdBuffer[CmdBufferPostLTM]->GetHostAddr() : NULL,
                    (NULL != pIPECmdBuffer[CmdBufferDMIHeader]) ? pIPECmdBuffer[CmdBufferDMIHeader]->GetHostAddr() : NULL,
                    (NULL != pIPECmdBuffer[CmdBufferNPS]) ? pIPECmdBuffer[CmdBufferNPS]->GetHostAddr() : NULL,
                    (NULL != pIPECmdBuffer[CmdBufferGenericBlob]) ? pIPECmdBuffer[CmdBufferGenericBlob]->GetHostAddr() : NULL);
        }

        if (CamxResultSuccess == result)
        {
            pFrameProcess = reinterpret_cast<IpeFrameProcess*>(
                pIPECmdBuffer[CmdBufferFrameProcess]->BeginCommands(CmdBufferFrameProcessSizeBytes / 4));
            CAMX_ASSERT(NULL != pFrameProcess);

            pFrameProcess->userArg          = m_hDevice;
            pFrameProcessData               = &pFrameProcess->cmdData;
            pFrameProcessData->requestId    = static_cast<UINT32>(requestId);
            pFrameProcessData->frameOpMode  = m_frameOpMode;
            pIPEIQsettings                  =
                reinterpret_cast<IpeIQSettings*>(pIPECmdBuffer[CmdBufferIQSettings]->BeginCommands(sizeof(IpeIQSettings) / 4));
            InitializeFrameProcessData(pFrameProcessData, pIPEIQsettings);

            // Setup the Input data for IQ Parameter
            moduleInput.pHwContext                                 = GetHwContext();
            moduleInput.pAECUpdateData                             = &AECUpdateData;
            moduleInput.pAWBUpdateData                             = &AWBUpdateData;
            moduleInput.pIPETuningMetadata                         = pTuningMetadata;
            moduleInput.pipelineIPEData.pFrameProcessData          = pFrameProcessData;
            moduleInput.pipelineIPEData.pIPEIQSettings             = pIPEIQsettings;
            moduleInput.pipelineIPEData.ppIPECmdBuffer             = pIPECmdBuffer;
            moduleInput.pipelineIPEData.batchFrameNum              = pNodeRequestData->pCaptureRequest->numBatchedFrames;
            moduleInput.pipelineIPEData.numOutputRefPorts          = m_numOutputRefPorts;
            moduleInput.pipelineIPEData.realtimeFlag               = m_realTimeIPE;
            moduleInput.pHALTagsData                               = &m_HALTagsData;
            moduleInput.pipelineIPEData.instanceProperty           = m_instanceProperty;
            moduleInput.pipelineIPEData.inputDimension.widthPixels = m_fullInputWidth;
            moduleInput.pipelineIPEData.inputDimension.heightLines = m_fullInputHeight;
            moduleInput.pipelineIPEData.numPasses                  = m_numPasses;
            moduleInput.sensorID                                   = cameraId;
            moduleInput.pCalculatedData                            = &m_ISPData;
            moduleInput.opticalCenterX                             = m_fullInputWidth / 2;
            moduleInput.opticalCenterY                             = m_fullInputHeight / 2;
            moduleInput.fDData.numberOfFace                        = 0;
            moduleInput.pipelineIPEData.pWarpGeometryData          = NULL;
            moduleInput.pipelineIPEData.compressiononOutput        = m_compressiononOutput;
            moduleInput.pipelineIPEData.pGeoLibParameters          = &m_geolibData;

            Utils::Memset(moduleInput.fDData.faceCenterX, 0x0, sizeof(moduleInput.fDData.faceCenterX));
            Utils::Memset(moduleInput.fDData.faceCenterY, 0x0, sizeof(moduleInput.fDData.faceCenterY));
            Utils::Memset(moduleInput.fDData.faceRadius, 0x0, sizeof(moduleInput.fDData.faceRadius));

            moduleInput.pipelineIPEData.hasTFRefInput =
                IsIPEHasInputReferencePort((requestId - m_currentrequestID), requestIdOffsetFromLastFlush);

            if (cameraId == m_cameraId)
            {
                m_camIdChanged                            = FALSE;
            }
            else
            {
                moduleInput.pipelineIPEData.hasTFRefInput = 0; // Set the TF reference value to 0 when Camera ID changes
                m_cameraId                                = cameraId;
                m_camIdChanged                            = TRUE;
            }

            moduleInput.maximumPipelineDelay = GetMaximumPipelineDelay();

            moduleInput.pipelineIPEData.isLowResolution = IsZSLNRLowResolution();

            // @note: need to set it here now before getting the data
            moduleInput.pipelineIPEData.upscalingFactorMFSR        = 1.0f;
            moduleInput.mfFrameNum                                 = requestId;
            moduleInput.pipelineIPEData.numOfFrames                = 5;  // default number of frames in MFNR/MFSR case
            moduleInput.pipelineIPEData.isDigitalZoomEnabled       = 0;
            moduleInput.pipelineIPEData.digitalZoomStartX          = 0;

            SetICAConfigMode(moduleInput);

            moduleInput.pipelineIPEData.configIOTopology           = GetIPEConfigIOTopology();

            moduleInput.pipelineIPEData.fullInputDimension.widthPixels = m_fullInputWidth;
            moduleInput.pipelineIPEData.fullInputDimension.heightLines = m_fullInputHeight;

            if ((NULL != pTuningMetadata) && (TRUE == isMasterCamera) && (CamxResultSuccess == result))
            {
                // Only use debug data on the master camera
                PrepareTuningMetadataDump(&moduleInput, &pTuningMetadata);
            }
            else if (NULL != m_pTuningMetadata) // Print only if tuning-metadata is enable
            {
                CAMX_LOG_VERBOSE(CamxLogGroupDebugData,
                                 "Tuning-metadata:IPE: SKIP: reqID: %llu isMaster: %u RT: %u InstanceID: %u, "
                                 "profId: %u, procType: %u",
                                 moduleInput.frameNum,
                                 isMasterCamera,
                                 IsRealTime(),
                                 InstanceID(),
                                 m_instanceProperty.profileId,
                                 m_instanceProperty.processingType);
            }

            if (((TRUE == GetStaticSettings()->offlineImageDumpOnly) && (TRUE != m_realTimeIPE)) ||
                (TRUE  == static_cast<Titan17xContext*>(GetHwContext())->GetTitan17xSettingsManager()->
                     GetTitan17xStaticSettings()->dumpIPERegConfig))
            {
                CHAR  dumpFilename[256];
                FILE* pFile = NULL;
                CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/IPE_regdump.txt", ConfigFileDirectory);
                pFile = CamX::OsUtils::FOpen(dumpFilename, "w");
                CamX::OsUtils::FPrintF(pFile, "******** IPE REGISTER DUMP FOR BET ********************* \n");
                CamX::OsUtils::FClose(pFile);
                moduleInput.dumpRegConfig                          = static_cast<Titan17xContext*>(GetHwContext())->
                                                                     GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->
                                                                     dumpIPERegConfigMask;
                moduleInput.regOffsetIndex                         = OffsetOfIPEIQModuleIndex;
            }

            // Get HAL tags
            result = GetMetadataTags(&moduleInput);
            CAMX_ASSERT(CamxResultSuccess == result);

            CAMX_LOG_INFO(CamxLogGroupPProc, "[%s]: request=%d, total frame=%d, FrameNum = %d",
                          NodeIdentifierString(), requestId, moduleInput.pipelineIPEData.numOfFrames, moduleInput.mfFrameNum);

            result = UpdateTuningModeData(pExecuteProcessRequestData->pTuningModeData, &moduleInput);

            if ((TRUE == IsMFProcessingType()) &&
                ((NoiseReductionModeOff           == moduleInput.pHALTagsData->noiseReductionMode) ||
                 (NoiseReductionModeMinimal       == moduleInput.pHALTagsData->noiseReductionMode) ||
                 (NoiseReductionModeZeroShutterLag == moduleInput.pHALTagsData->noiseReductionMode)))
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "[%s]: request=%d, invalid noiseReductionMode %d for MFNR %d",
                              NodeIdentifierString(),
                              requestId,
                              moduleInput.pHALTagsData->noiseReductionMode,
                              IsMFProcessingType());
            }
        }


        if (CamxResultSuccess == result)
        {
            result = GetGeoLibStillFrameConfiguration(requestId);
        }

        if (CamxResultSuccess == result)
        {
            // remove after specific vendor tag is added
            if (m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2 ==
                ChiModeFeature2SubModeType::HDR10)
            {
                moduleInput.isHDR10On = TRUE;
            }
            else
            {
                moduleInput.isHDR10On = FALSE;
            }

            if (TRUE == useDependencies)
            {
                SetAAAInputData(&moduleInput, parentNodeID);
            }
            else
            {
                HardcodeAAAInputData(&moduleInput, parentNodeID);
            }

            if (TRUE == m_OEMStatsSettingEnable)
            {
                result = GetOEMStatsConfig(&moduleInput, parentNodeID);
            }

            if (TRUE == m_OEMIQSettingEnable)
            {
                result = GetOEMIQConfig(&moduleInput, parentNodeID);
            }
        }

        if ((CamxResultSuccess == result) &&
            (TRUE == IsFDEnabled())       &&
            (1 == pNodeRequestData->pCaptureRequest->numBatchedFrames))
        {
            result = GetFaceROI(&moduleInput, parentNodeID);
        }

        if (CamxResultSuccess == result)
        {
            result = GetIntermediateDimension();
        }

        if ((CamxResultSuccess == result) && (TRUE == m_scratchBufferPortEnabled))
        {
            PublishScratchBufferData();
        }

        if (FALSE == IsScalerOnlyIPE())
        {
            if (CamxResultSuccess == result)
            {
                result = GetGammaOutput(moduleInput.pCalculatedData, parentNodeID);
            }

            if (CamxResultSuccess == result)
            {
                result = GetGammaPreCalculationOutput(moduleInput.pCalculatedData, parentNodeID);
            }

            if (CamxResultSuccess == result)
            {
                result = GetADRCInfoOutput();
            }
        }

        if (CamxResultSuccess == result)
        {
            result = FillIQSetting(&moduleInput, pIPEIQsettings, pPerRequestPorts);
        }

        if (CamxResultSuccess == result)
        {
            GetInSensorSeamlessControltState(&moduleInput);

            if ((InSensorHDR3ExpStart   == moduleInput.seamlessInSensorState) ||
                (InSensorHDR3ExpEnabled == moduleInput.seamlessInSensorState))
            {
                moduleInput.sensorData.isIHDR = TRUE;
                CAMX_LOG_INFO(CamxLogGroupPProc, "IPE: %s Seamless In-sensor HDR 3 Exposure is set, control state = %u",
                              NodeIdentifierString(),
                              moduleInput.seamlessInSensorState);
            }
            else
            {
                // Sensor related data for the current mode
                const SensorMode* pSensorModeData = GetSensorModeData();
                if (NULL != pSensorModeData)
                {
                    for (UINT32 i = 0; i < pSensorModeData->capabilityCount; i++)
                    {
                        if (pSensorModeData->capability[i] == SensorCapability::IHDR)
                        {
                            moduleInput.sensorData.isIHDR = TRUE;
                            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "sensor I-HDR mode");
                            break;
                        }
                    }
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupPProc, "Sensor Mode Data is not available, so IHDR will be disabled "
                                  "Pipeline %s profileId %u processingType %u stabType %u",
                                  NodeIdentifierString(), m_instanceProperty.profileId,
                                  m_instanceProperty.processingType,
                                  m_instanceProperty.stabilizationType);
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
            {
                PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];
                ImageBuffer*              pImageBuffer = pInputPort->pImageBuffer;

                if ((NULL != pImageBuffer)             &&
                    (NULL != pInputPort)               &&
                    (NULL != pInputPort->pImageBuffer) &&
                    (NULL != pInputPort->phFence)      &&
                    (0    != *(pInputPort->phFence))   &&
                    (-1   != pInputPort->pImageBuffer->GetFileDescriptor()))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Input port %d: cslh=0x%x, ft=0x%x, buffer size %d,"
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

                    result = pIQPacket->AddIOConfig(pImageBuffer,
                                                    pInputPort->portId,
                                                    CSLIODirection::CSLIODirectionInput,
                                                    pInputPort->phFence,
                                                    1,
                                                    NULL,
                                                    NULL,
                                                    0);

                    if (CamxResultSuccess == result)
                    {
                        CAMX_LOG_INFO(CamxLogGroupPProc,
                                      "node %s reporting Input config,portId=%d,imgBuf=0x%x, wxh=%dx%d, hFence=%d,request=%llu",
                                      NodeIdentifierString(),
                                      pInputPort->portId,
                                      pInputPort->pImageBuffer,
                                      pInputPort->pImageBuffer->GetFormat()->width,
                                      pInputPort->pImageBuffer->GetFormat()->height,
                                      *(pInputPort->phFence),
                                      pNodeRequestData->pCaptureRequest->requestId);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to create IO config packet result = %d", result);
                    }

                    if (CamxResultSuccess == result)
                    {
                        result = FillInputFrameSetData(pIPECmdBuffer[CmdBufferFrameProcess],
                                                       pInputPort->portId,
                                                       pInputPort->pImageBuffer,
                                                       pNodeRequestData->pCaptureRequest->numBatchedFrames);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fill input buffer frame data result = %d", result);
                            break;
                        }
                    }

                    if (CamxResultSuccess == result)
                    {
                        const ImageFormat* pImageFormat = pInputPort->pImageBuffer->GetFormat();
                        if (NULL != pImageFormat)
                        {
                            if (IPEInputPortFull == pInputPort->portId)
                            {
                                parentNodeID                    = GetParentNodeType(pInputPort->portId);
                                moduleInput.pipelineIPEData.fullInputFormat = pImageFormat->format;
                            }
                            else if (IPEInputPortDS4 == pInputPort->portId)
                            {
                                moduleInput.pipelineIPEData.ds4InputWidth  = pImageFormat->width;
                                moduleInput.pipelineIPEData.ds4InputHeight = pImageFormat->height;
                            }
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input image format");
                            result = CamxResultEInvalidPointer;
                        }
                    }
                }
                else
                {
                    result = CamxResultEInvalidArg;
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Invalid IO Config inputPort %p ImageBuffer %p, fence %p:%d, fd %d, "
                                   "request %llu",
                                   NodeIdentifierString(),
                                   pInputPort,
                                   ((NULL != pInputPort) ? pInputPort->pImageBuffer : NULL),
                                   ((NULL != pInputPort) ? pInputPort->phFence : NULL),
                                   ((NULL != pInputPort->phFence) ? *(pInputPort->phFence) : 0),
                                   ((NULL != pInputPort) ? ((NULL != pInputPort->pImageBuffer) ?
                                       pInputPort->pImageBuffer->GetFileDescriptor() : 0) : NULL),
                                   pNodeRequestData->pCaptureRequest->requestId);
                    break;
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Input Port: Add IO config failed port idx: %d", i);
                    break;
                }
            }
        }

        UINT32 metaTag         = 0;
        INT32  disableZoomCrop = FALSE;

        if (CamxResultSuccess == result)
        {
            result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ref.cropsize",
                                                              "DisableZoomCrop",
                                                              &metaTag);
        }

        if (CamxResultSuccess == result)
        {
            metaTag |= InputMetadataSectionMask;

            static const UINT32 PropertiesIPE[] =
            {
                metaTag
            };
            UINT    length                   = CAMX_ARRAY_SIZE(PropertiesIPE);
            VOID*   pData[1]                 = { 0 };
            UINT64  propertyDataIPEOffset[1] = { 0 };

            result = GetDataList(PropertiesIPE, pData, propertyDataIPEOffset, length);
            if ((CamxResultSuccess == result) &&
                (NULL != pData[0]))
            {
                disableZoomCrop = *reinterpret_cast<INT32*>(pData[0]);
            }
        }

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s: zoom operaton from IPE", NodeIdentifierString());
            result = FillFrameZoomWindow(&moduleInput, parentNodeID, pIPECmdBuffer[CmdBufferGenericBlob],
                                         pNodeRequestData->pCaptureRequest->requestId, disableZoomCrop);
        }

        // update it one more time in case if m_numPasses is updated from FillFrameZoomWindow(...) in MFSR
        moduleInput.pipelineIPEData.numPasses = m_numPasses;

        CAMX_LOG_INFO(CamxLogGroupPProc, "%s: blacklevel %d, colorCorrectionAberrationMode %d, colorCorrectionMode %d"
                      "controlAEMode %d, controlAWBMode %d, controlAWBLock %d, controlAECLock %d, controlMode %d,"
                      "controlPostRawSensitivityBoost %d, hotPixelMode %d, noiseReductionMode %d, shadingMode %d,"
                      "statisticsHotPixelMapMode %d, statisticsLensShadingMapMode %d,  saturation %d, edgeMode %d,"
                      "controlVideoStabilizationMode %d, sharpness %f, contrastLevel %d, ltmDynamicContrastStrength %d,"
                      " ltmDarkBoostStrength %d, ltmBrightSupressStrength %d tonemapMode %d",
                      NodeIdentifierString(),
                      moduleInput.pHALTagsData->blackLevelLock,
                      moduleInput.pHALTagsData->colorCorrectionAberrationMode,
                      moduleInput.pHALTagsData->colorCorrectionMode,
                      moduleInput.pHALTagsData->controlAEMode,
                      moduleInput.pHALTagsData->controlAWBMode,
                      moduleInput.pHALTagsData->controlAWBLock,
                      moduleInput.pHALTagsData->controlAECLock,
                      moduleInput.pHALTagsData->controlMode,
                      moduleInput.pHALTagsData->controlPostRawSensitivityBoost,
                      moduleInput.pHALTagsData->hotPixelMode,
                      moduleInput.pHALTagsData->noiseReductionMode,
                      moduleInput.pHALTagsData->shadingMode,
                      moduleInput.pHALTagsData->statisticsHotPixelMapMode,
                      moduleInput.pHALTagsData->statisticsLensShadingMapMode,
                      moduleInput.pHALTagsData->saturation,
                      moduleInput.pHALTagsData->edgeMode,
                      moduleInput.pHALTagsData->controlVideoStabilizationMode,
                      moduleInput.pHALTagsData->sharpness,
                      moduleInput.pHALTagsData->contrastLevel,
                      moduleInput.pHALTagsData->ltmContrastStrength.ltmDynamicContrastStrength,
                      moduleInput.pHALTagsData->ltmContrastStrength.ltmDarkBoostStrength,
                      moduleInput.pHALTagsData->ltmContrastStrength.ltmBrightSupressStrength,
                      moduleInput.pHALTagsData->tonemapCurves.tonemapMode);


        if (CamxResultSuccess == result)
        {
            moduleInput.pAECUpdateData->exposureInfo[0].sensitivity
                = moduleInput.pAECUpdateData->exposureInfo[0].sensitivity * moduleInput.pAECUpdateData->predictiveGain;
            moduleInput.pAECUpdateData->exposureInfo[0].linearGain
                = moduleInput.pAECUpdateData->exposureInfo[0].linearGain * moduleInput.pAECUpdateData->predictiveGain;
            // program settings for IQ modules
            result = ProgramIQConfig(&moduleInput);
        }
        if (CamxResultSuccess == result)
        {
            pIQPacket->SetRequestId(GetCSLSyncId(requestId));
            pIQPacket->SetOpcode(CSLDeviceType::CSLDeviceTypeICP, CSLPacketOpcodesIPEUpdate);
        }
        if (CamxResultSuccess == result)
        {
            result = FillCDMProgramArrays(pFrameProcessData,
                                          pIPEIQsettings,
                                          pIPECmdBuffer,
                                          pNodeRequestData->pCaptureRequest->numBatchedFrames);
        }

        if (CamxResultSuccess == result)
        {
            for (UINT portIndex = 0; portIndex < pPerRequestPorts->numOutputPorts; portIndex++)
            {
                PerRequestOutputPortInfo* pOutputPort  = &pPerRequestPorts->pOutputPorts[portIndex];
                ImageBuffer*              pImageBuffer = pOutputPort->ppImageBuffer[0];

                // Even though there are 4 buffers on same port, AddIOConfig shall be called only once.
                if ((NULL != pImageBuffer)            &&
                    (NULL != pOutputPort)             &&
                    (NULL != pOutputPort->phFence)    &&
                    (0    != *(pOutputPort->phFence)) &&
                    (-1   != pImageBuffer->GetFileDescriptor()))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Output port %d: cslh=0x%x, ft=0x%x, buffer size %d,"
                        "w=%d, h=%d,(w=%d, h=%d, s=%d, sh=%d)",
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

                    result = pIQPacket->AddIOConfig(pImageBuffer,
                                                    pOutputPort->portId,
                                                    CSLIODirection::CSLIODirectionOutput,
                                                    pOutputPort->phFence,
                                                    1,
                                                    NULL,
                                                    NULL,
                                                    0);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Output Port: Add IO config failed port Idx: %d", portIndex);
                        break;
                    }
                }
                else
                {
                    result = CamxResultEInvalidArg;
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Invalid IO Config inputPort %p ImageBuffer %p, fence %p:%d, fd %d, "
                                   "request %llu",
                                   NodeIdentifierString(),
                                   pOutputPort,
                                   pImageBuffer,
                                   ((NULL != pOutputPort) ? pOutputPort->phFence : NULL),
                                   ((NULL != pOutputPort->phFence) ? *(pOutputPort->phFence) : 0),
                                   ((NULL != pImageBuffer) ? pImageBuffer->GetFileDescriptor() : 0),
                                   pNodeRequestData->pCaptureRequest->requestId);
                    break;
                }

                if (CamxResultSuccess == result)
                {
                    pFrameProcessData->numFrameSetsInBatch = pNodeRequestData->pCaptureRequest->numBatchedFrames;
                    result = UpdateOutputBufferConfig(pFrameProcessData,
                                                      pIPECmdBuffer[CmdBufferFrameProcess],
                                                      pOutputPort,
                                                      pNodeRequestData->pCaptureRequest->HALOutputBufferCombined,
                                                      pNodeRequestData->pCaptureRequest->requestId,
                                                      portIndex);
                }


                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Output Port: Add IO config failed %d", pOutputPort->portId);
                    break;
                }
            }
            if ((CamxResultSuccess == result) && (TRUE == m_createdDownscalebuffers))
            {
                result = FillDownscaleBufferManagers(
                    pNodeRequestData->pCaptureRequest->requestId,
                    pIPECmdBuffer[CmdBufferFrameProcess],
                    pIPEIQsettings,
                    &m_multipassBufferParams[0]);
            }
            if ((CamxResultSuccess == result) && (TRUE == IsLoopBackPortEnabled()))
            {
                result = FillLoopBackFrameBufferData(
                    pNodeRequestData->pCaptureRequest->requestId,
                    requestIdOffsetFromLastFlush,
                    pIPECmdBuffer[CmdBufferFrameProcess],
                    pNodeRequestData->pCaptureRequest->numBatchedFrames,
                    pPerRequestPorts);
            }
            if (CamxResultSuccess == result)
            {
                result == PatchDS4LENRBuffers(
                    pNodeRequestData->pCaptureRequest->requestId,
                    pIPECmdBuffer[CmdBufferFrameProcess],
                    pIPEIQsettings);
            }
            if (CamxResultSuccess == result)
            {
                if (FALSE == m_scratchBufferPortEnabled)
                {
                    result == PatchScratchBuffers(
                        pNodeRequestData->pCaptureRequest->requestId,
                        pIPECmdBuffer[CmdBufferFrameProcess]);
                }

                if (CamxResultSuccess == result)
                {
                    result = FillFramePerfParams(pFrameProcessData);
                }

                // Following code can only be called after FillFrameZoomWindow(...)
                if (NULL != m_pIPECmdBufferManager[CmdBufferStriping])
                {
                    pIPECmdBuffer[CmdBufferStriping] =
                        GetCmdBufferForRequest(requestId, m_pIPECmdBufferManager[CmdBufferStriping]);
                }

                if (NULL != m_pIPECmdBufferManager[CmdBufferBLMemory])
                {
                    pIPECmdBuffer[CmdBufferBLMemory] =
                        GetCmdBufferForRequest(requestId, m_pIPECmdBufferManager[CmdBufferBLMemory]);
                }

                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s: requestId %llu, Striping vaddr %p, BL memory vaddr %p",
                        NodeIdentifierString(),
                        requestId,
                        (NULL != pIPECmdBuffer[CmdBufferStriping]) ? pIPECmdBuffer[CmdBufferStriping]->GetHostAddr() : NULL,
                        (NULL != pIPECmdBuffer[CmdBufferBLMemory]) ? pIPECmdBuffer[CmdBufferBLMemory]->GetHostAddr() : NULL);

                if ((CamxResultSuccess == result) && (TRUE == m_capability.swStriping))
                {
                    result = FillStripingParams(pFrameProcessData, pIPEIQsettings, pIPECmdBuffer, &IPECbw);
                }

                if (CamxResultSuccess == result)
                {
                    result = PatchBLMemoryBuffer(pFrameProcessData, pIPECmdBuffer);
                }

                if (CamxResultSuccess == result)
                {
                    CheckAndUpdateClockBW(pIPECmdBuffer[CmdBufferGenericBlob], pExecuteProcessRequestData,
                                          pFrameProcessData,
                                          &IPECbw);
                }
                if (CamxResultSuccess == result)
                {
                    result = CommitAllCommandBuffers(pIPECmdBuffer);
                }

                if (CamxResultSuccess == result)
                {
                    result = pIQPacket->CommitPacket();
                }

                if (CamxResultSuccess == result)
                {
                    result = pIQPacket->AddCmdBufferReference(pIPECmdBuffer[CmdBufferFrameProcess], NULL);
                }
                if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->dumpIPEFirmwarePayload)
                {

                    // Dump all firmware payload components for debugging purpose only
                    DumpPayload(CmdBufferFrameProcess, pIPECmdBuffer[CmdBufferFrameProcess], requestId);
                    DumpPayload(CmdBufferStriping, pIPECmdBuffer[CmdBufferStriping], requestId);
                    DumpPayload(CmdBufferIQSettings, pIPECmdBuffer[CmdBufferIQSettings], requestId);
                    DumpPayload(CmdBufferPreLTM, pIPECmdBuffer[CmdBufferPreLTM], requestId);
                    DumpPayload(CmdBufferPostLTM, pIPECmdBuffer[CmdBufferPostLTM], requestId);
                    DumpPayload(CmdBufferDMIHeader, pIPECmdBuffer[CmdBufferDMIHeader], requestId);
                    DumpPayload(CmdBufferNPS, pIPECmdBuffer[CmdBufferNPS], requestId);
                }

                if (CamxResultSuccess == result)
                {

                    if (pIPECmdBuffer[CmdBufferGenericBlob]->GetResourceUsedDwords() > 0)
                    {
                        pIPECmdBuffer[CmdBufferGenericBlob]->SetMetadata(static_cast<UINT32>(CSLICPCmdBufferIdGenericBlob));
                        result = pIQPacket->AddCmdBufferReference(pIPECmdBuffer[CmdBufferGenericBlob], NULL);
                    }
                }

                // Post metadata from IQ modules to metadata
                if ((CamxResultSuccess == result) && (FALSE == IsSkipPostMetadata()))
                {
                    result = PostMetadata(&moduleInput);
                }

                if ((NULL != pTuningMetadata)                           &&
                    (FALSE == IsScalerOnlyIPE())                        &&
                    (TRUE == isMasterCamera)                            &&
                    (CamxResultSuccess == result))
                {
                    // Only use debug data on the master camera
                    // Don't dump debug data if in Scale mode
                    DumpTuningMetadata(&moduleInput);
                }

                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_INFO(CamxLogGroupPProc, "Submit packets for IPE:%d request %llu",
                                  InstanceID(),
                                  requestId);

                    result = GetHwContext()->Submit(GetCSLSession(), m_hDevice, pIQPacket);

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
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            m_currentrequestID  = requestId;
            CAMX_LOG_INFO(CamxLogGroupPProc, "[%s]: Submitted packet(s) with requestId = %llu successfully",
                          NodeIdentifierString(),
                          requestId);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "[%s]: Submitted packet(s) with requestId = %llu failed %d",
                           NodeIdentifierString(),
                           requestId,
                           result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateUBWCLossyParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::UpdateUBWCLossyParams(
    IpeConfigIoData*   pConfigIOData,
    const ImageFormat* pImageFormat,
    IPE_IO_IMAGES      firmwarePortId)
{
    pConfigIOData->images[firmwarePortId].info.ubwcLossyMode       =
        static_cast<UbwcLossyMode>(pImageFormat->ubwcVerInfo.lossy);
    pConfigIOData->images[firmwarePortId].info.ubwcBwLimit         =
        (static_cast<Titan17xContext *>(GetHwContext()))->GetUBWCBandwidthLimit(
                                         static_cast <UbwcVersion>(pImageFormat->ubwcVerInfo.version),
                                        (IsReferencePort(firmwarePortId) ? LossyPathTF : LossyPathIPE),
                                        0,
                                        pImageFormat);
    pConfigIOData->images[firmwarePortId].info.ubwcThreshLossy1    =
        (static_cast<Titan17xContext *>(GetHwContext()))->GetUBWCLossyThreshold1(
                                        static_cast <UbwcVersion>(pImageFormat->ubwcVerInfo.version),
                                        (IsReferencePort(firmwarePortId) ? LossyPathTF : LossyPathIPE),
                                        pImageFormat);
    pConfigIOData->images[firmwarePortId].info.ubwcThreshLossy0    =
        (static_cast<Titan17xContext *>(GetHwContext()))->GetUBWCLossyThreshold0(
                                        static_cast <UbwcVersion>(pImageFormat->ubwcVerInfo.version),
                                        (IsReferencePort(firmwarePortId) ? LossyPathTF : LossyPathIPE),
                                        pImageFormat);
    pConfigIOData->images[firmwarePortId].info.ubwcOffsetsVarLossy = UBWCv4OffsetsVarLossy;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                    "node %s firmwarePortId:%d BWLimit:0x%x LossyMode: %d Lossy0:0x%x Lossy1:0x%x",
                    NodeIdentifierString(),
                    firmwarePortId,
                    pConfigIOData->images[firmwarePortId].info.ubwcBwLimit,
                    pConfigIOData->images[firmwarePortId].info.ubwcLossyMode,
                    pConfigIOData->images[firmwarePortId].info.ubwcThreshLossy0,
                    pConfigIOData->images[firmwarePortId].info.ubwcThreshLossy1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::OverrideConfigIOData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::OverrideConfigIOData(
    IpeConfigIoData*   pConfigIOData,
    const ImageFormat* pImageFormat,
    IPE_IO_IMAGES      firmwarePortId,
    UINT               parentNodeId)
{
    CamxResult result = CamxResultSuccess;
    CHAR       gpu[]  = "com.qti.node.gpu";
    const UINT gpuAlignment = 512;

    m_titanVersion = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();

    if ((CSLCameraTitanVersion::CSLTitan170 == m_titanVersion) &&
            (TRUE == GetPipeline()->IsNodeExistByNodePropertyValue(gpu)) &&
            (parentNodeId == ChiExternalNode))
    {
        if (((firmwarePortId == IPE_INPUT_IMAGE_DS4) || (firmwarePortId == IPE_INPUT_IMAGE_DS16)) &&
            ((pImageFormat->format == Format::YUV420NV12) || (pImageFormat->format == Format::YUV420NV21)))
        {
            pConfigIOData->images[firmwarePortId].bufferLayout[0].bufferStride =
                    Utils::AlignGeneric32(pImageFormat->width, gpuAlignment);
            pConfigIOData->images[firmwarePortId].bufferLayout[0].bufferHeight =
                    Utils::AlignGeneric32(pImageFormat->height, gpuAlignment);

            pConfigIOData->images[firmwarePortId].bufferLayout[1].bufferStride =
                    Utils::AlignGeneric32(pImageFormat->width, gpuAlignment);
            pConfigIOData->images[firmwarePortId].bufferLayout[1].bufferHeight =
                    Utils::AlignGeneric32((pImageFormat->height + 1) >> 1, gpuAlignment >> 1);

            CAMX_LOG_CONFIG(CamxLogGroupPProc,
                             "Overriding SetConfigIOData firmwarePortId %d, "
                             "bufferStride[0] %d, bufferHeight[0] %d, bufferStride[1] %d, bufferHeight[1] %d",
                             firmwarePortId,
                             pConfigIOData->images[firmwarePortId].bufferLayout[0].bufferStride,
                             pConfigIOData->images[firmwarePortId].bufferLayout[0].bufferHeight,
                             pConfigIOData->images[firmwarePortId].bufferLayout[1].bufferStride,
                             pConfigIOData->images[firmwarePortId].bufferLayout[1].bufferHeight);
        }
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::SetConfigIOData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::SetConfigIOData(
    IpeConfigIoData*   pConfigIOData,
    const ImageFormat* pImageFormat,
    IPE_IO_IMAGES      firmwarePortId,
    const CHAR*        pPortType)
{
    CamxResult result = CamxResultSuccess;
    m_titanVersion = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();

    pConfigIOData->images[firmwarePortId].info.format                 =
        TranslateFormatToFirmwareImageFormat(pImageFormat->format);
    pConfigIOData->images[firmwarePortId].info.dimensions.widthPixels = pImageFormat->width;
    pConfigIOData->images[firmwarePortId].info.dimensions.heightLines = pImageFormat->height;
    pConfigIOData->images[firmwarePortId].info.enableByteSwap         =
        ((pImageFormat->format == CamX::Format::YUV420NV21) ||
        (pImageFormat->format == CamX::Format::YUV420NV21TP10)) ? 1 : 0;

    if ((0 == OsUtils::StrNICmp(pPortType, "DownscaleInput", sizeof("DownscaleInput"))) ||
       (0 == OsUtils::StrNICmp(pPortType, "DownscaleRefInput", sizeof("DownscaleRefInput"))))
        // when Ref i/p ports are also generated internally
    {
        UpdateFromDownscaleFlag(pConfigIOData, firmwarePortId);
    }

    if (TRUE == ImageFormatUtils::IsUBWC(pImageFormat->format))
    {
        pConfigIOData->images[firmwarePortId].info.ubwcVersion =
            static_cast <UbwcVersion>(pImageFormat->ubwcVerInfo.version);
        if (UBWC_VERSION_3 <= static_cast <UbwcVersion>(pImageFormat->ubwcVerInfo.version))
        {
            UpdateUBWCLossyParams(pConfigIOData, pImageFormat, firmwarePortId);
        }
    }

    if (TRUE == IsValidConfigIOData(pConfigIOData->images[firmwarePortId].info.format, firmwarePortId, m_titanVersion))
    {
        CAMX_LOG_CONFIG(CamxLogGroupPProc,
                      "node %s, %s, firmwarePortId %d format %d, width %d, height %d, ubwc version %d, lossy %d, "
                      "fromdownscale %d",
                      NodeIdentifierString(),
                      pPortType,
                      firmwarePortId,
                      pImageFormat->format,
                      pImageFormat->width,
                      pImageFormat->height,
                      pConfigIOData->images[firmwarePortId].info.ubwcVersion,
                      pConfigIOData->images[firmwarePortId].info.ubwcLossyMode,
                      pConfigIOData->images[firmwarePortId].info.fromDownscale);
    }
    else
    {
        result = CamxResultEUnsupported;

        CAMX_LOG_ERROR(CamxLogGroupPProc,
                       "node %s, %s, firmwarePortId %d format %d, width %d, height %d ubwc version %d, lossy %d",
                       NodeIdentifierString(),
                       pPortType,
                       firmwarePortId,
                       pImageFormat->format,
                       pImageFormat->width,
                       pImageFormat->height,
                       pConfigIOData->images[firmwarePortId].info.ubwcVersion,
                       pConfigIOData->images[firmwarePortId].info.ubwcLossyMode);
    }

    if (CamxResultSuccess == result)
    {
        const YUVFormat* pPlane = &pImageFormat->formatParams.yuvFormat[0];
        for (UINT plane = 0; plane < ImageFormatUtils::GetNumberOfPlanes(pImageFormat); plane++)
        {
            CAMX_ASSERT(plane <= MAX_NUM_OF_IMAGE_PLANES);
            pConfigIOData->images[firmwarePortId].bufferLayout[plane].bufferStride =
                pPlane[plane].planeStride;
            pConfigIOData->images[firmwarePortId].bufferLayout[plane].bufferHeight =
                pPlane[plane].sliceHeight;
            pConfigIOData->images[firmwarePortId].metadataBufferLayout[plane].bufferStride =
                pPlane[plane].metadataStride;
            pConfigIOData->images[firmwarePortId].metadataBufferLayout[plane].bufferHeight =
                pPlane[plane].metadataHeight;
            CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                             "%s, plane %d, stride %d, scanline %d, metastride %d, metascanline %d",
                             pPortType,
                             plane,
                             pPlane[plane].planeStride,
                             pPlane[plane].sliceHeight,
                             pPlane[plane].metadataStride,
                             pPlane[plane].metadataHeight);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetFPSAndBatchSize()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetFPSAndBatchSize()
{
    static const UINT UsecasePropertiesIPE[] = { PropertyIDUsecaseFPS, PropertyIDUsecaseBatch };
    const UINT        length = CAMX_ARRAY_SIZE(UsecasePropertiesIPE);
    VOID*             pData[length] = { 0 };
    UINT64            usecasePropertyDataIPEOffset[length] = { 0 };

    CamxResult result = CamxResultSuccess;

    GetDataList(UsecasePropertiesIPE, pData, usecasePropertyDataIPEOffset, length);

    // This is a soft dependency
    if ((NULL != pData[0]) && (NULL != pData[1]))
    {
        m_FPS = *reinterpret_cast<UINT*>(pData[0]);
        m_maxBatchSize = *reinterpret_cast<UINT*>(pData[1]);
    }
    else
    {
        m_FPS = 30;
        m_maxBatchSize = 1;
    }

    if (FALSE == IsValidBatchSize())
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Unsupported batch size %d for fps %d", m_maxBatchSize, m_FPS);
        result = CamxResultEUnsupported;
    }

    CAMX_LOG_INFO(CamxLogGroupPProc, "batch size %d for fps %d", m_maxBatchSize, m_FPS);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateIPEConfigIOTopology
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::UpdateIPEConfigIOTopology(
    IpeConfigIoData*    pConfigIOData,
    UINT32              numPasses,
    IPEProfileId        profileId,
    IPEProcessingType   processingType)
{
    switch(processingType)
    {
        case IPEMFNRPrefilter:
        case IPEMFSRPrefilter:
            pConfigIOData->topologyType = IpeTopologyType::TOPOLOGY_TYPE_MFSR_PREFILTER;
            break;
        case IPEMFNRBlend:
        case IPEMFSRBlend:
            pConfigIOData->topologyType = IpeTopologyType::TOPOLOGY_TYPE_MFSR_BLEND;
            break;
        case IPEMFNRPostfilter:
        case IPEMFSRPostfilter:
        default:
            if ((IPEProfileId::IPEProfileIdPPS == profileId) &&
                (1 < numPasses))
            {
                pConfigIOData->topologyType = IpeTopologyType::TOPOLOGY_TYPE_NO_NPS_LTM;
            }
            else if (IPEProfileIdIndications == profileId)
            {
                pConfigIOData->topologyType = TOPOLOGY_TYPE_NPS_ONLY;
            }
            else
            {
                pConfigIOData->topologyType = IpeTopologyType::TOPOLOGY_TYPE_DEFAULT;
            }
            break;
    }

    CAMX_LOG_INFO(CamxLogGroupPProc, "Config IO Topology=%d", pConfigIOData->topologyType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::SetupDeviceResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::SetupDeviceResource(
    CSLBufferInfo*     pConfigIOMem,
    CSLDeviceResource* pResource)
{
    CamxResult                  result          = CamxResultSuccess;
    CSLICPAcquireDeviceInfo*    pIcpResource    = NULL;
    CSLICPResourceInfo*         pIcpOutResource = NULL;
    UINT                        countResource   = 0;
    SIZE_T                      resourceSize    = 0;
    UINT                        clockBWResSize  = 0;
    UINT                        numOutputPort   = 0;
    UINT                        numInputPort    = 0;
    const ImageFormat*          pImageFormat    = NULL;
    UINT                        inputPortId[IPEMaxInput];
    UINT                        outputPortId[IPEMaxOutput];
    IPE_IO_IMAGES               firmwarePortId;
    UINT                        parentNodeID = IFE;

    // Get Input Port List
    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    // Get Output Port List
    GetAllOutputPortIds(&numOutputPort, &outputPortId[0]);

    if (numInputPort <= 0 || numInputPort > IPEMaxInput ||
        numOutputPort <= 0 || numOutputPort > IPEMaxOutput)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "invalid input (%u) or output port (%u)", numInputPort, numOutputPort);
        result = CamxResultEUnsupported;
    }

    result = GetFPSAndBatchSize();

    if (CamxResultSuccess == result)
    {
        clockBWResSize                    = Utils::MaxUINT32(sizeof(CSLICPClockBandwidthRequest),
                                                             (sizeof(CSLICPClockBandwidthRequestV2) +
                                                             (sizeof(CSLAXIperPathBWVote) * (CSLAXIPathDataIPEMaxNum - 1))));
        m_pIPEGenericClockAndBandwidthData = static_cast<BYTE*>(CAMX_CALLOC(clockBWResSize));
        if (NULL == m_pIPEGenericClockAndBandwidthData)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "m_pIPEGenericClockAndBandwidthData is NULL");
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            resourceSize = sizeof(CSLICPAcquireDeviceInfo) +
                (sizeof(CSLICPResourceInfo) * (numOutputPort - 1));
            pIcpResource = static_cast<CSLICPAcquireDeviceInfo*>(CAMX_CALLOC(resourceSize));

            if (NULL == pIcpResource)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "pIcpResource is NULL");
                result = CamxResultENoMemory;
            }
        }

        if (CamxResultSuccess == result)
        {
            pIcpResource->numOutputResource = numOutputPort;
            pIcpResource->secureMode        = IsSecureMode();

            IpeConfigIo*     pConfigIO;
            IpeConfigIoData* pConfigIOData;

            pConfigIO          = reinterpret_cast<IpeConfigIo*>(pConfigIOMem->pVirtualAddr);
            pConfigIO->userArg = 0;
            pConfigIOData      = &pConfigIO->cmdData;
            CamX::Utils::Memset(pConfigIOData, 0, sizeof(*pConfigIOData));
            pConfigIOData->secureMode = IsSecureMode();

            for (UINT inputPortIndex = 0; inputPortIndex < numInputPort; inputPortIndex++)
            {
                TranslateToFirmwarePortId(inputPortId[inputPortIndex], &firmwarePortId);

                pImageFormat                                                      =
                    GetInputPortImageFormat(InputPortIndex(inputPortId[inputPortIndex]));

                if (NULL == pImageFormat)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "pImageFormat is NULL");
                    result = CamxResultENoMemory;
                    break;
                }

                result = SetConfigIOData(pConfigIOData, pImageFormat, firmwarePortId, "Input");
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Error!! Invalid Input configuration");
                    break;
                }

                if (inputPortId[inputPortIndex] == IPEInputPortFull)
                {
                    pIcpResource->inputResource.format = static_cast <UINT32>(pImageFormat->format);
                    pIcpResource->inputResource.width  = pImageFormat->width;
                    pIcpResource->inputResource.height = pImageFormat->height;
                    pIcpResource->inputResource.FPS    = m_FPS;

                    m_fullInputWidth  = pImageFormat->width;
                    m_fullInputHeight = pImageFormat->height;
                    m_numPasses++;
                }

                if ((IPEInputPortDS4  == inputPortId[inputPortIndex]) ||
                    (IPEInputPortDS16 == inputPortId[inputPortIndex]) ||
                    (IPEInputPortDS64 == inputPortId[inputPortIndex]))
                {
                    m_numPasses++;
                }

                if ((inputPortId[inputPortIndex] >= IPEInputPortFullRef) &&
                    (inputPortId[inputPortIndex] <= IPEInputPortDS64Ref))
                {
                    if (FALSE == IsLoopBackPortEnabled())
                    {
                        m_numOutputRefPorts++;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "node %s, invalid configuration"
                           "loop back ports enabled from xml with MCTF stabilization type",
                            NodeIdentifierString());
                        result = CamxResultEInvalidArg;
                        break;
                    }
                }
                parentNodeID = GetParentNodeType(inputPortId[inputPortIndex]);

                /* In case of GPU as a parent node, override configIOData as per GPU output */
                OverrideConfigIOData(pConfigIOData, pImageFormat, firmwarePortId, parentNodeID);
            }

            if (CamxResultSuccess == result)
            {
                pIcpOutResource = pIcpResource->outputResource;
                m_fullOutputHeight = 0;
                m_fullOutputWidth = 0;

                for (UINT outputPortIndex = 0; outputPortIndex < numOutputPort; outputPortIndex++)
                {
                    TranslateToFirmwarePortId(outputPortId[outputPortIndex], &firmwarePortId);

                    pImageFormat =
                        GetOutputPortImageFormat(OutputPortIndex(firmwarePortId));

                    if (NULL == pImageFormat)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "pImageFormat is NULL");
                        result = CamxResultENoMemory;
                        break;
                    }

                    result = SetConfigIOData(pConfigIOData, pImageFormat, firmwarePortId, "Output");

                    pIcpOutResource->format = static_cast <UINT32>(pImageFormat->format);
                    pIcpOutResource->width = pImageFormat->width;
                    pIcpOutResource->height = pImageFormat->height;
                    pIcpOutResource->FPS = m_FPS;
                    pIcpOutResource++;

                    if ((outputPortId[outputPortIndex] == IPEOutputPortDisplay) ||
                        (outputPortId[outputPortIndex] == IPEOutputPortVideo))
                    {
                        if (m_fullOutputHeight < static_cast<INT32>(pImageFormat->height))
                        {
                            m_fullOutputHeight = pImageFormat->height;
                        }
                        if (m_fullOutputWidth < static_cast<INT32>(pImageFormat->width))
                        {
                            m_fullOutputWidth = pImageFormat->width;
                        }
                        if (TRUE == ImageFormatUtils::IsUBWC(pImageFormat->format))
                        {
                            m_compressiononOutput = TRUE;
                        }
                    }
                }
            }

            if (CamxResultSuccess == result)
            {
                if (TRUE == CheckIsIPERealtime(m_numPasses))
                {
                    m_realTimeIPE              = TRUE;
                    pIcpResource->resourceType =
                        (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType)) ?
                        CSLICPResourceIDIPESemiRealTime : CSLICPResourceIDIPERealTime;
                    m_frameOpMode              =
                        (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType)) ?
                        SEMI_REAL_TIME_STREAM : REAL_TIME_STREAM;
                    SetIPERTPipeline(TRUE);
                }
                else
                {
                    m_realTimeIPE              = FALSE;
                    pIcpResource->resourceType = CSLICPResourceIDIPENonRealTime;
                    m_frameOpMode              = NON_REAL_TIME_STREAM;
                }

                result = GetStabilizationMargins();
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to get stabilization margins %d", result);
                }

                result = UpdateNumberofPassesonDimension(m_fullInputWidth, m_fullInputHeight, &m_numPasses);

                pConfigIOData->maxBatchSize = m_maxBatchSize;

                if ((TRUE == IsBlendWithNPS()) || (TRUE == IsPostfilterWithNPS()))
                {
                    SetMuxMode(pConfigIOData);
                    CAMX_LOG_INFO(CamxLogGroupPProc, "IPE:%d, MUX Mode: %d", InstanceID(), pConfigIOData->muxMode);
                }

                pIcpResource->hIOConfigCmd  = pConfigIOMem->hHandle;
                pIcpResource->IOConfigLen   = sizeof(IpeConfigIo);
                countResource               = 1;

                // Add to the resource list
                pResource->resourceID              = pIcpResource->resourceType;
                pResource->pDeviceResourceParam    = static_cast<VOID*>(pIcpResource);
                pResource->deviceResourceParamSize = resourceSize;

                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "numResources %d", countResource);
                if ((CamxResultSuccess == result) && (TRUE == NeedInternalMultipass(m_numPasses, m_maxBatchSize)))
                {
                    // Added the m_multipassBufferParams argument - so that when Ref DS ports are to be allocated,
                    // CreateDownscaleBufferManagers function can be called with Downscale buffer parameters such
                    // as CreateDownscaleBufferManagers("Ref", m_multipassRefBufferParams);
                    result = CreateDownscaleBufferManagers("Input", &m_multipassBufferParams[0]);
                }

                if (CamxResultSuccess == result)
                {
                    if (TRUE == IsLoopBackPortEnabled())
                    {
                        result = CreateLoopBackBufferManagers();
                    }
                    else
                    {
                        result = AllocateDS4LENRBuffers();
                    }
                }

                // Update IPE configIO topology type based on profile and lower pass input
                UpdateIPEConfigIOTopology(pConfigIOData,
                                          m_numPasses,
                                          m_instanceProperty.profileId,
                                          m_instanceProperty.processingType);

                if (CamxResultSuccess == result)
                {
                    pConfigIOData->stabilizationMargins.widthPixels = m_stabilizationMargin.widthPixels;
                    pConfigIOData->stabilizationMargins.heightLines = m_stabilizationMargin.heightLines;
                    result = InitializeStripingParams(pConfigIOData);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Initialize Striping params failed %d", result);
                    }
                }
            }
        }
    }

    for (UINT pass = 0; pass < m_numPasses; pass++)
    {
        m_inputPortIPEPassesMask |= 1 << pass;
    }

    CAMX_LOG_INFO(CamxLogGroupPProc, "IPE: %d number of passes %d, refports %d, inputPortIPEPassesMask %d",
        InstanceID(), m_numPasses, m_numOutputRefPorts, m_inputPortIPEPassesMask);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::AcquireDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::AcquireDevice()
{
    CSLICPAcquireDeviceInfo* pDevInfo;
    CamxResult               result = CamxResultSuccess;

    /// @todo (CAMX-886) Add CSLMemFlagSharedAccess once available from memory team

    if (CSLInvalidHandle == m_configIOMem.hHandle)
    {
        result = CSLAlloc(NodeIdentifierString(),
                          &m_configIOMem,
                          GetFWBufferAlignedSize(sizeof(IpeConfigIo)),
                          1,
                          (CSLMemFlagUMDAccess | CSLMemFlagSharedAccess | CSLMemFlagHw | CSLMemFlagKMDAccess),
                          &DeviceIndices()[0],
                          1);
    }
    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "CSLAlloc returned configIOMem.fd=%d", m_configIOMem.fd);

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(CSLInvalidHandle != m_configIOMem.hHandle);
        CAMX_ASSERT(NULL != m_configIOMem.pVirtualAddr);

        if ((NULL != m_configIOMem.pVirtualAddr) && (CSLInvalidHandle != m_configIOMem.hHandle))
        {
            result = SetupDeviceResource(&m_configIOMem, &m_deviceResourceRequest);
        }
        if (CamxResultSuccess == result)
        {
            m_IPEClockEfficiency = (TRUE == m_realTimeIPE) ?
                m_capability.realtimeClockEfficiency :
                m_capability.nonrealtimeClockEfficiency;
            CAMX_LOG_INFO(CamxLogGroupISP, "%s: m_IPEClockEfficiency %f  numIPE %d", NodeIdentifierString(),
                          m_IPEClockEfficiency, m_capability.numIPE);
        }

        if (CamxResultSuccess == result)
        {
            // During acquire device, KMD will create firmware handle and also call config IO
            result = CSLAcquireDevice(GetCSLSession(),
                                      &m_hDevice,
                                      DeviceIndices()[0],
                                      &m_deviceResourceRequest,
                                      1,
                                      NULL,
                                      0,
                                      NodeIdentifierString());

            pDevInfo = reinterpret_cast<CSLICPAcquireDeviceInfo*>(m_deviceResourceRequest.pDeviceResourceParam);
            // Firmware will provide scratch buffer requirements during configIO
            m_firmwareScratchMemSize = pDevInfo->scratchMemSize;
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s: m_firmwareScratchMemSize %d",
                          NodeIdentifierString(),
                          m_firmwareScratchMemSize);

            if (CamxResultSuccess == result)
            {
                SetDeviceAcquired(TRUE);
                AddCSLDeviceHandle(m_hDevice);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%d Acquire IPE Device Failed", InstanceID());
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%d Out of memory", InstanceID());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::ReleaseDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::ReleaseDevice()
{
    CamxResult result = CamxResultSuccess;

    DeInitializeStripingParams();
    if (NULL != m_hHandle)
    {
        CloseStripingLibrary();
    }

    if ((NULL != GetHwContext()) && (0 != m_hDevice))
    {
        result = CSLReleaseDevice(GetCSLSession(), m_hDevice);

        if (CamxResultSuccess == result)
        {
            SetDeviceAcquired(FALSE);
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to release device");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::ConfigureIPECapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::ConfigureIPECapability()
{
    CamxResult  result = CamxResultSuccess;

    m_titanVersion = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();

    switch (m_titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
            m_pISPPipeline = CAMX_NEW IPEPipelineTitan150(GetHwContext());
            break;

        case CSLCameraTitanVersion::CSLTitan160:
            m_pISPPipeline = CAMX_NEW IPEPipelineTitan160(GetHwContext());
            break;

        case CSLCameraTitanVersion::CSLTitan170:
            m_pISPPipeline = CAMX_NEW IPEPipelineTitan170(GetHwContext());
            break;

        case CSLCameraTitanVersion::CSLTitan175:
            m_pISPPipeline = CAMX_NEW IPEPipelineTitan175(GetHwContext());
            break;

        case CSLCameraTitanVersion::CSLTitan480:
            m_pISPPipeline = CAMX_NEW IPEPipelineTitan480(GetHwContext());
            break;

        default:
            result = CamxResultEUnsupported;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unsupported Titan Version = %u",
                static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion());
            break;
    }

    if (NULL != m_pISPPipeline)
    {
        m_pISPPipeline->GetCapability(&m_capability);
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "No memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateIPEIOLimits
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::UpdateIPEIOLimits(
    BufferNegotiationData* pBufferNegotiationData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pBufferNegotiationData)
    {
        // If other ports in addition to full port is notified, then limits change.
        // This is because if Full port along with other ports, DSx are enabled then
        // the limits of the IPE input processing changes due to NPS limitation.
        if (1 < pBufferNegotiationData->numOutputPortsNotified)
        {
            m_capability.minInputWidth  = IPEMinInputWidthMultiPass;
            m_capability.minInputHeight = IPEMinInputHeightMultiPass;
        }
        else
        {
            m_capability.minInputWidth  = IPEMinInputWidth;
            m_capability.minInputHeight = IPEMinInputHeight;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%d Invalid buffer negotiation data pointer", InstanceID());
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetModuleProcessingSection
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEProcessingSection GetModuleProcessingSection(
    ISPIQModuleType ipeIQModule)
{
    IPEProcessingSection type = IPEProcessingSection::IPEInvalidSection;

    switch (ipeIQModule)
    {
        case ISPIQModuleType::IPEICA:
        case ISPIQModuleType::IPERaster22:
        case ISPIQModuleType::IPERasterPD:
        case ISPIQModuleType::IPEANR:
        case ISPIQModuleType::IPETF:
            type = IPEProcessingSection::IPENPS;
            break;
        case ISPIQModuleType::IPECAC:
        case ISPIQModuleType::IPECrop:
        case ISPIQModuleType::IPEChromaUpsample:
        case ISPIQModuleType::IPECST:
        case ISPIQModuleType::IPELTM:
        case ISPIQModuleType::IPEHNR:
        case ISPIQModuleType::IPELENR:
            type = IPEProcessingSection::IPEPPSPreLTM;
            break;

        case ISPIQModuleType::IPEColorCorrection:
        case ISPIQModuleType::IPEGamma:
        case ISPIQModuleType::IPE2DLUT:
        case ISPIQModuleType::IPEChromaEnhancement:
        case ISPIQModuleType::IPEChromaSuppression:
        case ISPIQModuleType::IPESCE:
        case ISPIQModuleType::IPEASF:
        case ISPIQModuleType::IPEUpscaler:
        case ISPIQModuleType::IPEGrainAdder:
        case ISPIQModuleType::IPEDownScaler:
        case ISPIQModuleType::IPEFOVCrop:
        case ISPIQModuleType::IPEClamp:
            type = IPEProcessingSection::IPEPPSPostLTM;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unsupported IQ module type");
            break;
    }
    return type;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetProcessingSectionForProfile
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEProcessingSection GetProcessingSectionForProfile(
    IPEProfileId propertyValue)
{
    IPEProcessingSection type = IPEProcessingSection::IPEAll;
    switch (propertyValue)
    {
        case IPEProfileId::IPEProfileIdNPS:
        case IPEProfileId::IPEProfileIdIndications:
        case IPEProfileId::IPEProfileIdICAWarpOnly:
            type = IPEProcessingSection::IPENPS;
            break;
        case IPEProfileId::IPEProfileIdPPS:
        case IPEProfileId::IPEProfileIdUpscale:
            type = IPEProcessingSection::IPEPPS;
            break;
        case IPEProfileId::IPEProfileIdScale:
            break;
        case IPEProfileId::IPEProfileIdNoZoomCrop:
        case IPEProfileId::IPEProfileIdDefault:
            type = IPEProcessingSection::IPEAll;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unsupported IQ module type");
            break;
    }
    return type;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::CreateIPEIQModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::CreateIPEIQModules()
{
    CamxResult              result                      = CamxResultSuccess;
    IPEIQModuleInfo*        pIQModule                   = m_capability.pIPEIQModuleList;
    IPEModuleCreateData     moduleInputData             = { 0 };
    IPEProcessingSection    instanceSection             = IPEProcessingSection::IPEAll;
    IPEProcessingSection    moduleSection               = IPEProcessingSection::IPEAll;
    BOOL                    moduleDependeciesEnabled    = TRUE;
    UINT32                  titanVersion                = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();

    instanceSection = GetProcessingSectionForProfile(m_instanceProperty.profileId);
    // This is a special case where we want all IQ blocks, but want to skip the fillzoomwindow
    if (IPEProcessingSection::IPENoZoomCrop == instanceSection)
    {
        m_nodePropDisableZoomCrop = TRUE;
    }
    moduleInputData.initializationData.pipelineIPEData.pDeviceIndex = &m_deviceIndex;
    moduleInputData.initializationData.pHwContext                   = GetHwContext();
    moduleInputData.initializationData.requestQueueDepth            = GetPipeline()->GetRequestQueueDepth();
    moduleInputData.pNodeIdentifier                                 = NodeIdentifierString();
    moduleInputData.titanVersion                                    = titanVersion;
    m_numIPEIQModulesEnabled                                        = 0;

    result = CreateIPEICAInputModule(&moduleInputData, pIQModule);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%d Failed to Create ICA1 Module, result = %d",
            InstanceID(), result);
    }

    if (CamxResultSuccess == result)
    {
        // create rest of the IQ modules from the list other than mandatory ICA1
        for (UINT count = m_numIPEIQModulesEnabled; count < m_capability.numIPEIQModules; count++)
        {
            if  ((TRUE == IsScalerOnlyIPE()) ||
                 (TRUE == IsICAOnlyIPE()))
            {
                // No IQ modules for Scale profile
                break;
            }
            moduleDependeciesEnabled = TRUE;

            if (instanceSection != IPEProcessingSection::IPEAll)
            {
                moduleSection = GetModuleProcessingSection(pIQModule[count].moduleType);
                if ((moduleSection == IPEProcessingSection::IPEPPSPreLTM) ||
                    (moduleSection == IPEProcessingSection::IPEPPSPostLTM))
                {
                    moduleSection = IPEProcessingSection::IPEPPS;
                }
                // In case of Invalid Processing section only moduleDependeciesEnabled should be FALSE
                if (instanceSection != moduleSection)
                {
                    moduleDependeciesEnabled = FALSE;
                }
            }

            /// @todo (CAMX-735) Link IPE IQ modules with new Chromatix adapter
            if ((TRUE == IsIQModuleInstalled(&pIQModule[count])) && (TRUE == moduleDependeciesEnabled))
            {
                moduleInputData.path = pIQModule[count].path;
                result               = pIQModule[count].IQCreate(&moduleInputData);
                if (CamxResultSuccess == result)
                {
                    m_pEnabledIPEIQModule[m_numIPEIQModulesEnabled] = moduleInputData.pModule;
                    m_numIPEIQModulesEnabled++;
                }
                else
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("%s: Failed to Create IQ Module, count = %d", __FUNCTION__, count);
                    break;
                }
            }
        }
    }

    if ((CamxResultSuccess == result) && (0 < m_numIPEIQModulesEnabled))
    {
        result = CreateIQModulesCmdBufferManager(&moduleInputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::CreateIQModulesCmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::CreateIQModulesCmdBufferManager(
    IPEModuleCreateData*     pModuleInputData)
{
    CamxResult              result                   = CamxResultSuccess;
    CmdBufferManagerParam*  pBufferManagerParam      = NULL;
    IQModuleCmdBufferParam  bufferManagerCreateParam = { 0 };

    pBufferManagerParam =
        static_cast<CmdBufferManagerParam*>(CAMX_CALLOC(sizeof(CmdBufferManagerParam) * m_capability.numIPEIQModules));

    if (NULL != pBufferManagerParam)
    {
        bufferManagerCreateParam.pCmdBufManagerParam    = pBufferManagerParam;
        bufferManagerCreateParam.numberOfCmdBufManagers = 0;

        for (UINT count = 0; count < m_numIPEIQModulesEnabled; count++)
        {
            pModuleInputData->pModule = m_pEnabledIPEIQModule[count];
            pModuleInputData->pModule->FillCmdBufferManagerParams(&pModuleInputData->initializationData,
                &bufferManagerCreateParam);
        }

        if (0 != bufferManagerCreateParam.numberOfCmdBufManagers)
        {
            // Create Cmd Buffer Managers for IQ Modules
            result = CmdBufferManager::CreateMultiManager(FALSE,
                                                          pBufferManagerParam,
                                                          bufferManagerCreateParam.numberOfCmdBufManagers);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "IQ Modules Cmd Buffer Manager Creation failed");
            }
        }

        // Free up the memory allocated bt IQ Blocks
        for (UINT index = 0; index < bufferManagerCreateParam.numberOfCmdBufManagers; index++)
        {
            if (NULL != pBufferManagerParam[index].pBufferManagerName)
            {
                CAMX_FREE (pBufferManagerParam[index].pBufferManagerName);
                pBufferManagerParam[index].pBufferManagerName = NULL;
            }

            if (NULL != pBufferManagerParam[index].pParams)
            {
                CAMX_FREE (pBufferManagerParam[index].pParams);
                pBufferManagerParam[index].pParams = NULL;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Out Of Memory");
        result = CamxResultENoMemory;
    }

    if (NULL != pBufferManagerParam)
    {
        CAMX_FREE (pBufferManagerParam);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::Cleanup()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::Cleanup()
{
    UINT           count            = 0;
    CamxResult     result           = CamxResultSuccess;
    UINT32         numberOfMappings = 0;
    CSLBufferInfo  bufferInfo       = { 0 };
    CSLBufferInfo* pBufferInfo[CSLICPMaxMemoryMapRegions];

    if (NULL != m_pIPECmdBufferManager[CmdBufferFrameProcess])
    {
        if (NULL != m_pIPECmdBufferManager[CmdBufferFrameProcess]->GetMergedCSLBufferInfo())
        {
            Utils::Memcpy(&bufferInfo,
                          m_pIPECmdBufferManager[CmdBufferFrameProcess]->GetMergedCSLBufferInfo(),
                          sizeof(CSLBufferInfo));
            pBufferInfo[numberOfMappings] = &bufferInfo;
            numberOfMappings++;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to get Merged Buffer Info");
            result = CamxResultEFailed;
        }
    }

    if (NULL != m_UBWCStatBufInfo.pUBWCStatsBuffer)
    {
        pBufferInfo[numberOfMappings] = m_UBWCStatBufInfo.pUBWCStatsBuffer;
        numberOfMappings++;

    }

    if (0 != numberOfMappings)
    {
        result = SendFWCmdRegionInfo(CSLICPGenericBlobCmdBufferUnMapFWMemRegion,
                                     pBufferInfo,
                                     numberOfMappings);
    }

    // De-allocate all of the IQ modules

    for (count = 0; count < m_numIPEIQModulesEnabled; count++)
    {
        if (NULL != m_pEnabledIPEIQModule[count])
        {
            m_pEnabledIPEIQModule[count]->Destroy();
            m_pEnabledIPEIQModule[count] = NULL;
        }
    }

    if (FALSE == m_scratchBufferPortEnabled)
    {
        for (count = 0; count < m_numScratchBuffers; count++)
        {
            if (NULL != m_pScratchMemoryBuffer[count])
            {
                if (CSLInvalidHandle != m_pScratchMemoryBuffer[count]->hHandle)
                {
                    CSLReleaseBuffer(m_pScratchMemoryBuffer[count]->hHandle);
                }
                CAMX_FREE(m_pScratchMemoryBuffer[count]);
                m_pScratchMemoryBuffer[count] = NULL;
            }
        }
    }

    // free ubwc buffer
    if (NULL != m_UBWCStatBufInfo.pUBWCStatsBuffer)
    {
        CSLReleaseBuffer(m_UBWCStatBufInfo.pUBWCStatsBuffer->hHandle);
        CAMX_FREE(m_UBWCStatBufInfo.pUBWCStatsBuffer);
        m_UBWCStatBufInfo.pUBWCStatsBuffer = NULL;
    }

    if (TRUE == m_createdDownscalebuffers)
    {
        result = DeleteDownscaleBufferManagers(&m_multipassBufferParams[0]);
    }

    if (TRUE == IsLoopBackPortEnabled())
    {
        DeleteLoopBackBufferManagers();
    }

    DeleteDS4LENRBuffers();

    m_numIPEIQModulesEnabled = 0;

    // Check if striping in UMD is enabled before destroying striping library context
    if (NULL != m_hStripingLib)
    {
        result          = m_funcPtrIPEDestroy(&m_hStripingLib);
        m_hStripingLib  = NULL;
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to destroy striping library, error = %d", result);
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

    if (NULL != m_pISPPipeline)
    {
        CAMX_DELETE m_pISPPipeline;
        m_pISPPipeline = NULL;
    }

    if (NULL != m_pIPEGenericClockAndBandwidthData)
    {
        CAMX_FREE(m_pIPEGenericClockAndBandwidthData);
        m_pIPEGenericClockAndBandwidthData = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::PopulateGeneralTuningMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::PopulateGeneralTuningMetadata(
    ISPInputData* pInputData)
{
    IpeIQSettings*          pIPEIQsettings          = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
    ChiTuningModeParameter* pTuningModeParameter    = pInputData->pTuningData;
    TuningModeDebugData*    pTuningDebugData        = &pInputData->pIPETuningMetadata->IPETuningModeDebugData;

    // Copy to a packed structure the IPE module configuration used by FW
    CAMX_STATIC_ASSERT(sizeof(IpeIQSettings) == sizeof(pInputData->pIPETuningMetadata->IPEFWHeader.IPEFirmwareSetting));
    // If these are not the same size, IpeIQSettings has new modules. camxtuningdump.h needs to be updated.
    // Update IPEFirmwareAPIVersion with the current Firmware Version
    // #include "icpdefs.h" "FW Version: %x", FW_API_VERSION
    // Update IPEModulesConfigFields with the correct field size
    // "Array Size: %d", sizeof(IpeIQSettings) / sizeof(UINT32)

    pInputData->pIPETuningMetadata->IPEFWHeader.IPEFWAPIVersion = IPEFirmwareAPIVersion;

    Utils::Memcpy(&pInputData->pIPETuningMetadata->IPEFWHeader.IPEFirmwareSetting,
                  pIPEIQsettings,
                  sizeof(pInputData->pIPETuningMetadata->IPEFWHeader.IPEFirmwareSetting));

    // Populate trigger data
    IPETuningTriggerData* pIPETuningTriggers       = &pInputData->pIPETuningMetadata->IPETuningTriggers;
    pIPETuningTriggers->predictiveGain             = pInputData->triggerData.predictiveGain;
    pIPETuningTriggers->AECYHistStretchClampOffset = pInputData->triggerData.AECYHistStretchClampOffset;
    pIPETuningTriggers->AECYHistStretchScaleFactor = pInputData->triggerData.AECYHistStretchScaleFactor;

    ChiTuningMode* pTuningMode = NULL;
    for (UINT32 paramNumber = 0; paramNumber <  pTuningModeParameter->noOfSelectionParameter; paramNumber++)
    {
        pTuningMode = &pTuningModeParameter->TuningMode[paramNumber];

        switch (pTuningMode->mode)
        {
            case ChiModeType::Default:
                pTuningDebugData->base = static_cast<UINT32>(pTuningMode->subMode.value);
                break;
            case ChiModeType::Sensor:
                pTuningDebugData->sensor = static_cast<UINT32>(pTuningMode->subMode.value);
                break;
            case ChiModeType::Usecase:
                pTuningDebugData->usecase = static_cast<UINT32>(pTuningMode->subMode.usecase);
                break;
            case ChiModeType::Feature1:
                pTuningDebugData->feature1 = static_cast<UINT32>(pTuningMode->subMode.feature1);
                break;
            case ChiModeType::Feature2:
                pTuningDebugData->feature2 = static_cast<UINT32>(pTuningMode->subMode.feature2);
                break;
            case ChiModeType::Scene:
                pTuningDebugData->scene = static_cast<UINT32>(pTuningMode->subMode.scene);
                break;
            case ChiModeType::Effect:
                pTuningDebugData->effect = static_cast<UINT32>(pTuningMode->subMode.effect);
                break;
            default:
                CAMX_LOG_WARN(CamxLogGroupPProc, "IPE: fail to set tuning mode type");
                break;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                     "TuningMode: ReqID: %llu: Default %u, Sensor %u usecase %u feature1 %u feature2 %u secne %u effect %u",
                     pInputData->frameNum,
                     pTuningDebugData->base,
                     pTuningDebugData->sensor,
                     pTuningDebugData->usecase,
                     pTuningDebugData->feature1,
                     pTuningDebugData->feature2,
                     pTuningDebugData->scene,
                     pTuningDebugData->effect);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::PrepareTuningMetadataDump()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::PrepareTuningMetadataDump(
    ISPInputData* pInputData,
    IPETuningMetadata** ppTuningMetadata)
{
    CamxResult      result                              = CamxResultSuccess;
    UINT32          debugDataPartition                  = 0;
    DebugData       debugData                           = { 0 };
    UINT            PropertiesTuning[]                  = { 0 };
    static UINT     metaTagDebugDataAll                 = 0;
    const UINT      length                              = CAMX_ARRAY_SIZE(PropertiesTuning);
    VOID*           pData[length]                       = { 0 };
    UINT64          propertyDataTuningOffset[length]    = { 0 };
    DebugData*      pDebugDataPartial                   = NULL;
    BOOL            isIPERealtime                       = m_realTimeIPE;

    if (TRUE == isIPERealtime)
    {
        PropertiesTuning[0] = PropertyIDTuningDataIPE;
    }
    else
    {
        VendorTagManager::QueryVendorTagLocation("org.quic.camera.debugdata", "DebugDataAll", &metaTagDebugDataAll);
        PropertiesTuning[0] = metaTagDebugDataAll | InputMetadataSectionMask;
    }

    GetDataList(PropertiesTuning, pData, propertyDataTuningOffset, length);

    pDebugDataPartial = reinterpret_cast<DebugData*>(pData[0]);
    if (NULL == pDebugDataPartial || NULL == pDebugDataPartial->pData)
    {
        // Debug-data buffer not available, session or pipeline has not assigned a buffer because is not needed.
        CAMX_LOG_VERBOSE(CamxLogGroupDebugData,
                         "SKIP no debug-data available/needed RT: %u: ReqId: %llu, InstanceID: %u, profId: %u, procType: %u",
                         isIPERealtime,
                         pInputData->frameNum,
                         InstanceID(),
                         m_instanceProperty.profileId,
                         m_instanceProperty.processingType);
        *ppTuningMetadata                               = NULL;
        pInputData->pIPETuningMetadata                  = NULL;
        pInputData->pipelineIPEData.pDebugDataWriter    = NULL;
    }
    else
    {
        if (TRUE == isIPERealtime)
        {
            debugData.pData = pDebugDataPartial->pData;
            // Use first partition for real time
            debugData.size  = pDebugDataPartial->size / DebugDataPartitionsIPE;
        }
        else if (DebugDataPartitionsIPE > debugDataPartition) // Using second and third partition for offline processing
        {
            SIZE_T instanceOffset = 0;

            if ((IPEProfileIdNPS    == m_instanceProperty.profileId)        &&
                (IPEMFNRPostfilter  == m_instanceProperty.processingType))
            {
                // MFNR NPS: Post-filter will go in third partition
                debugDataPartition  = 2;
            }
            else
            {
                // Offline IPE will start in partition second partition
                debugDataPartition  = 1;
            }

            // Only 2 offline IPE pass supported for debug data, scale not included
            debugData.size  = HAL3MetadataUtil::DebugDataSize(DebugDataType::IPETuning) / DebugDataPartitionsIPE;
            instanceOffset  = debugDataPartition * debugData.size;
            debugData.pData = Utils::VoidPtrInc(
                pDebugDataPartial->pData,
                (HAL3MetadataUtil::DebugDataOffset(DebugDataType::IPETuning) + instanceOffset));
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_WARN(
                CamxLogGroupDebugData,
                "RT: %u:  ERROR: debugDataPartition: %u: not enough partitions to save IPE data frameNum: %llu",
                IsRealTime(),
                debugDataPartition,
                pInputData->frameNum);
        }

        // Don't dump debug data if in Scale mode
        if ((CamxResultSuccess == result) &&
            (FALSE == IsScalerOnlyIPE()))
        {
            DebugDataTagID nodeInfoTagID;

            CAMX_LOG_VERBOSE(CamxLogGroupDebugData,
                             "Tuning-metadata:IPE:RT: %u, frameNum: %llu, InstanceID: %u, base: %p, start: %p, size: %u,"
                             " profId: %u, procType: %u, debugDataPartition: %u",
                             isIPERealtime,
                             pInputData->frameNum,
                             InstanceID(),
                             pDebugDataPartial->pData,
                             debugData.pData,
                             debugData.size,
                             m_instanceProperty.profileId,
                             m_instanceProperty.processingType,
                             debugDataPartition);

            // Set the buffer pointer
            m_pDebugDataWriter->SetBufferPointer(static_cast<BYTE*>(debugData.pData), debugData.size);

            // Populate node information
            pInputData->pIPETuningMetadata->IPENodeInformation.instanceId       = InstanceID();
            pInputData->pIPETuningMetadata->IPENodeInformation.requestId        = pInputData->frameNum;
            pInputData->pIPETuningMetadata->IPENodeInformation.isRealTime       = IsRealTime();
            pInputData->pIPETuningMetadata->IPENodeInformation.profileId        = m_instanceProperty.profileId;
            pInputData->pIPETuningMetadata->IPENodeInformation.processingType   = m_instanceProperty.processingType;

            if (TRUE == IsRealTime())
            {
                nodeInfoTagID = DebugDataTagID::TuningIPENodeInfo;
            }
            else
            {
                nodeInfoTagID = DebugDataTagID::TuningIPENodeInfoOffline;
            }

            result = m_pDebugDataWriter->AddDataTag(nodeInfoTagID,
                                                    DebugDataTagType::TuningIQNodeInfo,
                                                    1,
                                                    &pInputData->pIPETuningMetadata->IPENodeInformation,
                                                    sizeof(pInputData->pIPETuningMetadata->IPENodeInformation));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
                result = CamxResultSuccess; // Non-fatal error
            }

            // Provide debug writer to all modules
            pInputData->pipelineIPEData.pDebugDataWriter = m_pDebugDataWriter;
        }
        else
        {
            // Skip TuningData for IPEProfileIdScale
            *ppTuningMetadata                               = NULL;
            pInputData->pIPETuningMetadata                  = NULL;
            pInputData->pipelineIPEData.pDebugDataWriter    = NULL;

            CAMX_LOG_VERBOSE(CamxLogGroupDebugData,
                             "Tuning-metadata:IPE:RT: %u, SKIP: frameNum: %llu, InstanceID: %u, base: %p, start: %p, size: %u,"
                             " profId: %u, procType: %u, debugDataPartition: %u",
                             IsRealTime(),
                             pInputData->frameNum,
                             InstanceID(),
                             pDebugDataPartial->pData,
                             debugData.pData,
                             debugData.size,
                             m_instanceProperty.profileId,
                             m_instanceProperty.processingType,
                             debugDataPartition);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::DumpTuningMetadata()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::DumpTuningMetadata(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    // Populate any metadata obtained direclty from base IPE node
    PopulateGeneralTuningMetadata(pInputData);

    result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIPEChipsetVersion,
                                            DebugDataTagType::UInt32,
                                            1,
                                            &pInputData->titanVersion,
                                            sizeof(pInputData->titanVersion));

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
        result = CamxResultSuccess; // Non-fatal error
    }

    if (TRUE == m_realTimeIPE)
    {
        // Add IPE tuning metadata tags
        result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIPETuningMode,
                                                DebugDataTagType::TuningModeInfo,
                                                1,
                                                &pInputData->pIPETuningMetadata->IPETuningModeDebugData,
                                                sizeof(pInputData->pIPETuningMetadata->IPETuningModeDebugData));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }

        result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIPEFirmwareHeader,
                                                DebugDataTagType::TuningIPEFirmwareHeader,
                                                1,
                                                &pInputData->pIPETuningMetadata->IPEFWHeader,
                                                sizeof(pInputData->pIPETuningMetadata->IPEFWHeader));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }

        result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIPETriggerModulesData,
                                                DebugDataTagType::TuningIPETriggerData,
                                                1,
                                                &pInputData->pIPETuningMetadata->IPETuningTriggers,
                                                sizeof(pInputData->pIPETuningMetadata->IPETuningTriggers));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }

        // Make a copy in main metadata pool
        static const UINT PropertiesDebugData[] = { PropertyIDDebugDataAll };
        VOID* pSrcData[1] = { 0 };
        const UINT lengthAll = CAMX_ARRAY_SIZE(PropertiesDebugData);
        UINT64 propertyDataTuningAllOffset[lengthAll] = { 0 };
        static UINT metaTagDebugDataAll = 0;
        GetDataList(PropertiesDebugData, pSrcData, propertyDataTuningAllOffset, lengthAll);

        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.debugdata", "DebugDataAll", &metaTagDebugDataAll);

        if (CamxResultSuccess == result)
        {
            const UINT TuningVendorTag[] = { metaTagDebugDataAll };
            const VOID* pDstData[1] = { pSrcData[0] };
            UINT pDataCount[1] = { sizeof(DebugData) };

            WriteDataList(TuningVendorTag, pDstData, pDataCount, 1);
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Fail to get DebugDataAll tag location");
            result = CamxResultSuccess; // Non-fatal
        }
    }
    else
    {
        // Add IPE tuning metadata tags
        result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIPETuningModeOffline,
                                                DebugDataTagType::TuningModeInfo,
                                                1,
                                                &pInputData->pIPETuningMetadata->IPETuningModeDebugData,
                                                sizeof(pInputData->pIPETuningMetadata->IPETuningModeDebugData));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }

        result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIPEFirmwareHeaderOffline,
                                                DebugDataTagType::TuningIPEFirmwareHeader,
                                                1,
                                                &pInputData->pIPETuningMetadata->IPEFWHeader,
                                                sizeof(pInputData->pIPETuningMetadata->IPEFWHeader));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }

        result = m_pDebugDataWriter->AddDataTag(DebugDataTagID::TuningIPETriggerModulesDataOffline,
                                                DebugDataTagType::TuningIPETriggerData,
                                                1,
                                                &pInputData->pIPETuningMetadata->IPETuningTriggers,
                                                sizeof(pInputData->pIPETuningMetadata->IPETuningTriggers));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            result = CamxResultSuccess; // Non-fatal error
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::PostMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::PostMetadata(
    const ISPInputData* pInputData)
{
    CamxResult  result                               = CamxResultSuccess;
    const VOID* ppData[NumIPEMetadataOutputTags]     = { 0 };
    UINT        pDataCount[NumIPEMetadataOutputTags] = { 0 };
    UINT        index                                = 0;
    UINT        effectMap                            = 0;
    UINT        sceneMap                             = 0;
    UINT        modeMap                              = 0;

    ChiModeType              mode   = ChiModeType::Default;
    ChiModeEffectSubModeType effect = ChiModeEffectSubModeType::None;
    ChiModeSceneSubModeType  scene  = ChiModeSceneSubModeType::None;

    if (pInputData->pTuningData)
    {
        for (UINT i = 0; i < pInputData->pTuningData->noOfSelectionParameter; i++)
        {
            mode = pInputData->pTuningData->TuningMode[i].mode;
            if (ChiModeType::Effect == mode)
            {
                effect    = pInputData->pTuningData->TuningMode[i].subMode.effect;
                effectMap = IPEEffectMap[static_cast<UINT>(effect)].to;
            }
            if (ChiModeType::Scene == mode)
            {
                if (ChiModeSceneSubModeType::BestShot >= pInputData->pTuningData->TuningMode[i].subMode.scene)
                {
                    scene = pInputData->pTuningData->TuningMode[i].subMode.scene;
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupPProc, "Tuning data scene out of bound, Use default scene : None ");
                }
                sceneMap = IPESceneMap[static_cast<UINT>(scene)].to;
            }
        }
    }

    // Get Control mode, Scene mode and effects mode from HAL tag
    static const UINT vendorTagsControlMode[] = { InputControlMode, InputControlSceneMode, InputControlEffectMode };
    const SIZE_T numTags                      = CAMX_ARRAY_SIZE(vendorTagsControlMode);
    VOID*        pData[numTags]               = { 0 };
    UINT64       pDataModeOffset[numTags]     = { 0 };

    GetDataList(vendorTagsControlMode, pData, pDataModeOffset, numTags);

    if (NULL != pData[0])
    {
        Utils::Memcpy(&modeMap, pData[0], sizeof(modeMap));
    }

    if (NULL != pData[1])
    {
        Utils::Memcpy(&sceneMap, pData[1], sizeof(sceneMap));
    }

    if (NULL != pData[2])
    {
        Utils::Memcpy(&effectMap, pData[2], sizeof(effectMap));
    }

    pDataCount[index] = 1;
    ppData[index]     = &modeMap;
    index++;

    pDataCount[index] = 1;
    ppData[index]     = &effectMap;
    index++;

    pDataCount[index] = 1;
    ppData[index]     = &sceneMap;
    index++;

    pDataCount[index] = 1;
    ppData[index]     = &(pInputData->pCalculatedData->metadata.edgeMode);
    index++;

    pDataCount[index] = 1;
    ppData[index]     = &(pInputData->pHALTagsData->controlVideoStabilizationMode);
    index++;

    pDataCount[index] = 1;
    ppData[index]     = &(pInputData->pCalculatedData->metadata.colorCorrectionAberrationMode);
    index++;

    pDataCount[index] = 1;
    ppData[index]     = &(pInputData->pHALTagsData->noiseReductionMode);
    index++;

    pDataCount[index] = 1;
    ppData[index]     = &pInputData->pCalculatedData->toneMapData.tonemapMode;
    index++;

    pDataCount[index] = 1;
    ppData[index]     = &pInputData->pCalculatedData->colorCorrectionMode;
    index++;

    pDataCount[index] = 3 * 3;
    ppData[index]     = &pInputData->pCalculatedData->CCTransformMatrix;
    index++;

    if (pInputData->pCalculatedData->toneMapData.curvePoints > 0)
    {
        pDataCount[index] = pInputData->pCalculatedData->toneMapData.curvePoints;
        ppData[index]     = &pInputData->pCalculatedData->toneMapData.tonemapCurveBlue;
        index++;

        pDataCount[index] = pInputData->pCalculatedData->toneMapData.curvePoints;
        ppData[index]     = &pInputData->pCalculatedData->toneMapData.tonemapCurveGreen;
        index++;

        pDataCount[index] = pInputData->pCalculatedData->toneMapData.curvePoints;
        ppData[index]     = &pInputData->pCalculatedData->toneMapData.tonemapCurveRed;
        index++;

        WriteDataList(IPEMetadataOutputTags, ppData, pDataCount, NumIPEMetadataOutputTags);
    }
    else
    {
        WriteDataList(IPEMetadataOutputTags, ppData, pDataCount, NumIPEMetadataOutputTags - 3);
    }

    if (IPEProcessingType::IPEMFSRPrefilter == m_instanceProperty.processingType)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d dimension width=%d height=%d ratio=%f",
                        InstanceID(),
                        m_curIntermediateDimension.width,
                        m_curIntermediateDimension.height,
                        m_curIntermediateDimension.ratio);

        // write intermediate dimension
        static const UINT dimensionProps[]            = { PropertyIDIntermediateDimension };
        const UINT        length                      = CAMX_ARRAY_SIZE(dimensionProps);
        const VOID*       pDimensionData[length]      = { &m_curIntermediateDimension };
        UINT              pDimensionDataCount[length] = { sizeof(IntermediateDimensions) };

        result = WriteDataList(dimensionProps, pDimensionData, pDimensionDataCount, length);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc,
                          "IPE:%d failed to write intermediate dimension to property data list with error = %d",
                          InstanceID(), result);
        }

        UINT metaTag = 0;
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.intermediateDimension",
                                                          "IntermediateDimension",
                                                          &metaTag);
        if (CamxResultSuccess == result)
        {
            static const UINT dimensionInfoTag[]        = { metaTag };
            const UINT        dataLength                = CAMX_ARRAY_SIZE(dimensionInfoTag);
            const VOID*       pDimData[dataLength]      = { &m_curIntermediateDimension };
            UINT              pDimDataCount[dataLength] = { sizeof(IntermediateDimensions) };

            result = WriteDataList(dimensionInfoTag, pDimData, pDimDataCount, dataLength);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc,
                               "IPE:%d failed to write intermediate dimension to vendor data list error = %d",
                               InstanceID(), result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::ProgramIQConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::ProgramIQConfig(
    ISPInputData* pInputData)
{
    CamxResult      result        = CamxResultSuccess;
    UINT            count         = 0;
    UINT            path          = 0;
    UINT            index         = 0;

    IPEIQModuleData moduleData;

    // Call IQInterface to Set up the Trigger data
    Node* pBaseNode = this;
    IQInterface::IQSetupTriggerData(pInputData, pBaseNode, IsRealTime(), NULL);

    pInputData->triggerData.enabledTMCversion = static_cast<SWTMCVersion>(m_adrcInfo.version);
    pInputData->triggerData.pADRCData         = &m_adrcInfo;
    pInputData->triggerData.fullInputWidth    = pInputData->pipelineIPEData.fullInputDimension.widthPixels;
    pInputData->triggerData.fullInputHeight   = pInputData->pipelineIPEData.fullInputDimension.heightLines;
    pInputData->triggerData.ds4InputWidth     = pInputData->pipelineIPEData.ds4InputWidth;
    pInputData->triggerData.ds4InputHeight    = pInputData->pipelineIPEData.ds4InputHeight;

    if (TRUE == GetHwContext()->GetStaticSettings()->enableIPEIQLogging)
    {
        CAMX_LOG_INFO(CamxLogGroupPProc, "Begin of dumping IPE Trigger ------");
        IQInterface::s_interpolationTable.IQTriggerDataDump(&pInputData->triggerData);
        CAMX_LOG_INFO(CamxLogGroupPProc, "End of dumping IPE Trigger ------");
    }

    for (count = 0; count < m_numIPEIQModulesEnabled; count++)
    {
        if (TRUE == m_adrcInfo.enable)
        {
            // Update AEC Gain values for ADRC use cases, before GTM(includes) will be triggered by shortGain,
            // betweem GTM & LTM(includes) will be by shortGain*power(DRCGain, gtm_perc) and post LTM will be
            // by shortGain*DRCGain
            IQInterface::UpdateAECGain(m_pEnabledIPEIQModule[count]->GetIQType(), pInputData, m_adrcInfo.gtmPercentage);
        }

        result = m_pEnabledIPEIQModule[count]->Execute(pInputData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Failed to Run IQ Config, count %d, IQ type %d",
                           NodeIdentifierString(), count, m_pEnabledIPEIQModule[count]->GetIQType());
            break;
        }

        m_pEnabledIPEIQModule[count]->GetModuleData(&moduleData);

        switch (m_pEnabledIPEIQModule[count]->GetIQType())
        {
            case ISPIQModuleType::IPELTM:
                m_preLTMLUTOffset[ProgramIndexLTM]      = m_pEnabledIPEIQModule[count]->GetLUTOffset();
                break;
            case ISPIQModuleType::IPEGamma:
                m_postLTMLUTOffset[ProgramIndexGLUT]    = m_pEnabledIPEIQModule[count]->GetLUTOffset();
                break;
            case ISPIQModuleType::IPE2DLUT:
                m_postLTMLUTOffset[ProgramIndex2DLUT]   = m_pEnabledIPEIQModule[count]->GetLUTOffset();
                break;
            case ISPIQModuleType::IPEASF:
                m_postLTMLUTOffset[ProgramIndexASF]     = m_pEnabledIPEIQModule[count]->GetLUTOffset();
                break;
            case ISPIQModuleType::IPEUpscaler:
                m_postLTMLUTOffset[ProgramIndexUpscale] = m_pEnabledIPEIQModule[count]->GetLUTOffset();
                break;
            case ISPIQModuleType::IPEGrainAdder:
                m_postLTMLUTOffset[ProgramIndexGRA]     = m_pEnabledIPEIQModule[count]->GetLUTOffset();
                break;
            case ISPIQModuleType::IPEANR:
                for (UINT passNum = PASS_NAME_FULL; passNum < PASS_NAME_MAX; passNum++)
                {
                    m_ANRPassOffset[passNum] = moduleData.offsetPass[passNum];
                }
                m_ANRSinglePassCmdBufferSize = moduleData.singlePassCmdLength;
                break;
            case ISPIQModuleType::IPEICA:
                path                  = moduleData.IPEPath;
                if ((path == IPEPath::REFERENCE) || (path == IPEPath::INPUT) || (path == IPEPath::CVPICA))
                {
                    index                 =
                        (path == REFERENCE) ? ProgramIndexICA2 : ProgramIndexICA1;
                    m_ICALUTOffset[index] = m_pEnabledIPEIQModule[count]->GetLUTOffset();
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid ipe iq module path %d", path);
                }
                break;
            case ISPIQModuleType::IPETF:
                for (UINT passNum = PASS_NAME_FULL; passNum < PASS_NAME_MAX; passNum++)
                {
                    m_TFPassOffset[passNum] = moduleData.offsetPass[passNum];
                }
                m_TFSinglePassCmdBufferSize = moduleData.singlePassCmdLength;
                break;
            case ISPIQModuleType::IPEHNR:
                m_preLTMLUTOffset[ProgramIndexHNR] = m_pEnabledIPEIQModule[count]->GetLUTOffset();
                break;
            default:
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::SetIQModuleNumLUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IPENode::SetIQModuleNumLUT(
    ISPIQModuleType type,
    UINT            numLUTs,
    INT             path)
{
    UINT index = 0;
    switch (type)
    {
        case ISPIQModuleType::IPELTM:
            m_preLTMLUTCount[ProgramIndexLTM]      = numLUTs;
            break;
        case ISPIQModuleType::IPEGamma:
            m_postLTMLUTCount[ProgramIndexGLUT]    = numLUTs;
            break;
        case ISPIQModuleType::IPE2DLUT:
            m_postLTMLUTCount[ProgramIndex2DLUT]   = numLUTs;
            break;
        case ISPIQModuleType::IPEASF:
            m_postLTMLUTCount[ProgramIndexASF]     = numLUTs;
            break;
        case ISPIQModuleType::IPEUpscaler:
            m_postLTMLUTCount[ProgramIndexUpscale] = numLUTs;
            break;
        case ISPIQModuleType::IPEGrainAdder:
            m_postLTMLUTCount[ProgramIndexGRA]     = numLUTs;
            break;
        case ISPIQModuleType::IPEICA:
            index                =
                (path == (IPEPath::REFERENCE)) ? ProgramIndexICA2 : ProgramIndexICA1;
            m_ICALUTCount[index] = numLUTs;
            break;
        case ISPIQModuleType::IPEHNR:
            m_preLTMLUTCount[ProgramIndexHNR] = numLUTs;
            break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::UpdateIQCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IPENode::UpdateIQCmdSize()
{
    IPEProcessingSection section    = IPEProcessingSection::IPEInvalidSection;
    UINT                 numLUTs    = 0;
    ISPIQModuleType      type;
    INT                  path       = -1;
    IPEIQModuleData      moduleData;

    for (UINT count = 0; count < m_numIPEIQModulesEnabled; count++)
    {
        numLUTs = m_pEnabledIPEIQModule[count]->GetNumLUT();
        type    = m_pEnabledIPEIQModule[count]->GetIQType();
        section = GetModuleProcessingSection(type);
        switch (section)
        {
            case IPEProcessingSection::IPEPPSPreLTM:
                m_maxCmdBufferSizeBytes[CmdBufferPreLTM]      += m_pEnabledIPEIQModule[count]->GetIQCmdLength();
                m_maxCmdBufferSizeBytes[CmdBufferDMIHeader]   += numLUTs *
                    cdm_get_cmd_header_size(CDMCmdDMI) * RegisterWidthInBytes;
                break;
            case IPEProcessingSection::IPEPPSPostLTM:
                m_maxCmdBufferSizeBytes[CmdBufferPostLTM]     += m_pEnabledIPEIQModule[count]->GetIQCmdLength();
                m_maxCmdBufferSizeBytes[CmdBufferDMIHeader]   += numLUTs *
                    cdm_get_cmd_header_size(CDMCmdDMI) * RegisterWidthInBytes;
                break;
            case IPEProcessingSection::IPENPS:
                m_maxCmdBufferSizeBytes[CmdBufferNPS]         +=
                    m_pEnabledIPEIQModule[count]->GetIQCmdLength();
                m_maxCmdBufferSizeBytes[CmdBufferDMIHeader]   +=
                    numLUTs * cdm_get_cmd_header_size(CDMCmdDMI) * RegisterWidthInBytes;
                m_pEnabledIPEIQModule[count]->GetModuleData(&moduleData);
                path                                           = moduleData.IPEPath;
                if ((path != IPEPath::REFERENCE) && (path != IPEPath::INPUT) && (path != IPEPath::CVPICA))
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid ipe iq module path %d", path);
                }
                break;
            default:
                CAMX_LOG_WARN(CamxLogGroupPProc, "%s: invalid module type %d", __FUNCTION__, type);
                break;
        }
        SetIQModuleNumLUT(type, numLUTs, path);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::IsFDEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPENode::IsFDEnabled(
    VOID)
{
    BOOL bIsFDPostingResultsEnabled = FALSE;

    // Set offset to 1 to point to the previous request.
    GetFDPerFrameMetaDataSettings(m_fdDataOffset, &bIsFDPostingResultsEnabled, NULL);

    return bIsFDPostingResultsEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::SetDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    UINT                        parentNodeId)
{
    NodeProcessRequestData* pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    UINT32&                 rCount              = pNodeRequestData->dependencyInfo[0].propertyDependency.count;
    UINT32                  metaTagFDRoi        = 0;
    UINT32                  result              = CamxResultSuccess;
    BOOL                    isSWEISEnabled      = FALSE;

    if ((ISPHwTitan150 == static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion()) &&
        ((0 != (IPEStabilizationType::IPEStabilizationTypeSWEIS2 & m_instanceProperty.stabilizationType)) ||
         (0 != (IPEStabilizationType::IPEStabilizationTypeSWEIS3 & m_instanceProperty.stabilizationType))))
    {
        isSWEISEnabled = TRUE;
    }

    result = VendorTagManager::QueryVendorTagLocation(VendorTagSectionOEMFDResults,
           VendorTagNameOEMFDResults, &metaTagFDRoi);

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE: ProcessRequest: Setting dependency for Req#%llu",
                     pNodeRequestData->pCaptureRequest->requestId);

    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(pNodeRequestData->pCaptureRequest->requestId);
    if (FirstValidRequestId < requestIdOffsetFromLastFlush)
    {
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount] = GetNodeCompleteProperty();
        // Always point to the previous request. Should NOT be tied to the pipeline delay!
        pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[rCount] = 1;
        rCount++;
    }

    if (parentNodeId == IFE)
    {
        UINT32 metaTagAppliedCrop = 0;
        if (CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera.ifecropinfo",
                                "AppliedCrop", &metaTagAppliedCrop) &&
            (TRUE == IsTagPresentInPublishList(metaTagAppliedCrop)))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = metaTagAppliedCrop;
        }
    }

    if (TRUE == m_isStatsNodeAvailable)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Set dependency for real time pipeline");

        // 3A dependency
        if (TRUE == IsTagPresentInPublishList(PropertyIDAECFrameControl))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDAECFrameControl;

        }

        if (TRUE == IsTagPresentInPublishList(PropertyIDAWBFrameControl))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDAWBFrameControl;
        }

        if ((parentNodeId == IFE) || (TRUE == isSWEISEnabled))
        {
            // IFE dependency
            if (TRUE == IsTagPresentInPublishList(PropertyIDIFEDigitalZoom))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDIFEDigitalZoom;
            }
            if (TRUE == IsTagPresentInPublishList(PropertyIDIFEScaleOutput))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDIFEScaleOutput;
            }
            if (TRUE == IsTagPresentInPublishList(PropertyIDIFEGammaOutput))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDIFEGammaOutput;
            }
            if ((TRUE == m_instanceProperty.enableFOVC) && (TRUE == IsTagPresentInPublishList(PropertyIDFOVCFrameInfo)))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDFOVCFrameInfo;
            }

            if ((result == CamxResultSuccess)                              &&
                (TRUE == IsTagPresentInPublishList(metaTagFDRoi))          &&
                (TRUE == IsFDEnabled())                                    &&
                (1 == pNodeRequestData->pCaptureRequest->numBatchedFrames) &&
                ((requestIdOffsetFromLastFlush) > GetStaticSettings()->minReqFdDependency))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[rCount] = 1;
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = metaTagFDRoi;
            }

        }
        else if (TRUE == IsNodeInPipeline(BPS))
        {
            if (TRUE == IsTagPresentInPublishList(PropertyIDBPSGammaOutput))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDBPSGammaOutput;
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "parent Node is not IFE in real time pipeline id %d", parentNodeId);
        }
    }
    else
    {
        if (TRUE == m_OEMStatsSettingEnable)
        {
            if ((parentNodeId == IFE) || (TRUE == isSWEISEnabled))
            {
                if (TRUE == IsTagPresentInPublishList(PropertyIDIFEDigitalZoom))
                {
                    pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDIFEDigitalZoom;
                }
                if (TRUE == IsTagPresentInPublishList(PropertyIDIFEGammaOutput))
                {
                    pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDIFEGammaOutput;
                }
            }
            else if (parentNodeId == BPS)
            {
                if (TRUE == IsTagPresentInPublishList(PropertyIDBPSGammaOutput))
                {
                    pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDBPSGammaOutput;
                }
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "parent Node is not IFE/BPS in OEMSetting pipeline id %d", parentNodeId);
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Set dependency for none real time pipeline");

            if (TRUE == IsNodeInPipeline(BPS))
            {
                // BPS dependency
                if (TRUE == IsTagPresentInPublishList(PropertyIDBPSGammaOutput))
                {
                    pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = PropertyIDBPSGammaOutput;
                }
            }
        }
    }

    // Don't need to add parent check, because ipe just use this property when parent node is chinode.
    if (TRUE == m_instanceProperty.enableCHICropInfoPropertyDependency)
    {
        UINT32 metaTag = 0;
        if (CDKResultSuccess == VendorTagManager::QueryVendorTagLocation("com.qti.cropregions",
                                                                         "ChiNodeResidualCrop", &metaTag) &&
            (TRUE == IsTagPresentInPublishList(metaTag)))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] = metaTag;
        }
    }


    // Set dependency for GeoLib
    SetGeoLibStillFrameConfigurationDependency(pNodeRequestData);

    // Set Dependency for ADRC Info.
    SetADRCDependencies(pNodeRequestData);

    // ICA dependency needed for EIS, MCTF or offline pipeline in case of MFNR / MFSR uscases
    SetICADependencies(pNodeRequestData);

    // multi-camera
    SetMultiCameraMasterDependency(pNodeRequestData);

    if (0 < pNodeRequestData->dependencyInfo[0].propertyDependency.count)
    {
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
    }

    if (TRUE == GetStaticSettings()->enableImageBufferLateBinding)
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

        SetInputBuffersReadyDependency(pExecuteProcessRequestData, 0);
    }

    // ExecuteProcessRequest always requires sequenceId 1, purposefully reporting dep regardless of having a dependency or not
    pNodeRequestData->numDependencyLists                                                    = 1;
    pNodeRequestData->dependencyInfo[0].processSequenceId                                   = 1;
    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::CreateFWCommandBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::CreateFWCommandBufferManagers()
{
    ResourceParams               resourceParams[IPEMaxFWCmdBufferManagers];
    CHAR                         bufferManagerName[IPEMaxFWCmdBufferManagers][MaxStringLength256];
    struct CmdBufferManagerParam createParam[IPEMaxFWCmdBufferManagers];
    UINT32                       numberOfBufferManagers = 0;
    CamxResult                   result                 = CamxResultSuccess;

    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                        m_maxCmdBufferSizeBytes[CmdBufferStriping],
                        CmdType::FW,
                        CSLMemFlagUMDAccess,
                        0,
                        &m_deviceIndex,
                        m_IPECmdBlobCount);
    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                      sizeof(CHAR) * MaxStringLength256,
                      "CBM_%s_%s",
                      NodeIdentifierString(),
                      "CmdBufferStriping");
    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pIPECmdBufferManager[CmdBufferStriping];

    numberOfBufferManagers++;

    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                        m_maxCmdBufferSizeBytes[CmdBufferBLMemory],
                        CmdType::FW,
                        CSLMemFlagUMDAccess,
                        0,
                        &m_deviceIndex,
                        m_IPECmdBlobCount);
    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                      sizeof(CHAR) * MaxStringLength256,
                      "CBM_%s_%s",
                      NodeIdentifierString(),
                      "CmdBufferBLMemory");
    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pIPECmdBufferManager[CmdBufferBLMemory];

    numberOfBufferManagers++;

    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                        CmdBufferFrameProcessSizeBytes,
                        CmdType::FW,
                        CSLMemFlagUMDAccess,
                        IPEMaxTopCmdBufferPatchAddress,
                        &m_deviceIndex,
                        m_IPECmdBlobCount);
    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                      sizeof(CHAR) * MaxStringLength256,
                      "CBM_%s_%s",
                      NodeIdentifierString(),
                      "CmdBufferFrameProcess");
    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pIPECmdBufferManager[CmdBufferFrameProcess];

    numberOfBufferManagers++;

    FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                        sizeof(IpeIQSettings),
                        CmdType::FW,
                        CSLMemFlagUMDAccess,
                        0,
                        &m_deviceIndex,
                        m_IPECmdBlobCount);
    OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                      sizeof(CHAR) * MaxStringLength256,
                      "CBM_%s_%s",
                      NodeIdentifierString(),
                      "CmdBufferIQSettings");
    createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
    createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
    createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pIPECmdBufferManager[CmdBufferIQSettings];

    numberOfBufferManagers++;

    if (m_maxCmdBufferSizeBytes[CmdBufferNPS] > 0)
    {
        FillCmdBufferParams(&resourceParams[numberOfBufferManagers],
                            m_maxCmdBufferSizeBytes[CmdBufferNPS],
                            CmdType::FW,
                            CSLMemFlagUMDAccess,
                            IPEMaxNPSPatchAddress,
                            &m_deviceIndex,
                            m_IPECmdBlobCount);
        OsUtils::SNPrintF(bufferManagerName[numberOfBufferManagers],
                          sizeof(CHAR) * MaxStringLength256,
                          "CBM_%s_%s",
                          NodeIdentifierString(),
                          "CmdBufferNPS");
        createParam[numberOfBufferManagers].pBufferManagerName = bufferManagerName[numberOfBufferManagers];
        createParam[numberOfBufferManagers].pParams            = &resourceParams[numberOfBufferManagers];
        createParam[numberOfBufferManagers].ppCmdBufferManager = &m_pIPECmdBufferManager[CmdBufferNPS];

        numberOfBufferManagers++;
    }

    if (numberOfBufferManagers > IPEMaxFWCmdBufferManagers)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Maxmimum FW Cmd buffers reached");
        result = CamxResultEFailed;
    }

    if ((CamxResultSuccess == result) &&  (0 != numberOfBufferManagers))
    {
        result = CreateMultiCmdBufferManager(createParam, numberOfBufferManagers);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "FW Cmd Buffer Creation failed result %d", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::OpenStripingLibrary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::OpenStripingLibrary()
{
    CamxResult     result   = CamxResultSuccess;
    m_funcPtrIPECreate      = NULL;
    m_funcPtrIPEDestroy     = NULL;
    m_funcPtrIPEExecute     = NULL;
    m_funcPtrIPECalcScratchBufSize = NULL;

#if defined(_LP64)
    const CHAR libFilename[] = "/vendor/lib64/libipebpsstriping.so";
#else // _LP64
    const CHAR libFilename[] = "/vendor/lib/libipebpsstriping.so";
#endif // _LP64
    m_hHandle = CamX::OsUtils::LibMap(libFilename);

    if (NULL != m_hHandle)
    {
        m_funcPtrIPECreate = reinterpret_cast<IpeStripingLibraryContextCreate_t>
            (CamX::OsUtils::LibGetAddr(m_hHandle, "IPEStripingLibraryContextCreate"));
        if (NULL == m_funcPtrIPECreate)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "function pointer is NULL for IPEStripingLibraryContextCreate");
        }
        if (result == CamxResultSuccess)
        {
            m_funcPtrIPEExecute = reinterpret_cast<IpeStripingLibraryExecute_t>
                (CamX::OsUtils::LibGetAddr(m_hHandle, "IPEStripingLibraryExecute"));
        }
        if (NULL == m_funcPtrIPEExecute)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "function pointer is NULL for IPEStripingLibraryExecute");
        }
        if (result == CamxResultSuccess)
        {
            m_funcPtrIPEDestroy = reinterpret_cast<IpeStripingLibraryContextDestroy_t>
                (CamX::OsUtils::LibGetAddr(m_hHandle, "IPEStripingLibraryContextDestroy"));
        }
        if (NULL == m_funcPtrIPEDestroy)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "function pointer is NULL for IPEStripingLibraryContextDestroy");
        }
        if (result == CamxResultSuccess)
        {
            m_funcPtrIPECalcScratchBufSize = reinterpret_cast<IpeStripingLibraryCalculateScratchBufSize_t>
                (CamX::OsUtils::LibGetAddr(m_hHandle, "IPEStripingLibraryCalculateScratchBufSize"));
        }
        if (NULL == m_funcPtrIPECalcScratchBufSize)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "### function pointer is NULL for IPEStripingLibraryCalculateScratchBufSize");
        }
        if (result == CamxResultSuccess)
        {
            m_funcPtrIPEGetStripingLibVer = reinterpret_cast<GetStripingLibraryVersion_t>
                (CamX::OsUtils::LibGetAddr(m_hHandle, "GetStripingLibraryVersion"));

        }
        if (NULL == m_funcPtrIPEGetStripingLibVer)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "function pointer is NULL for GetStripingLibVersion");
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupPProc, "%s: StripingLibraryVersion %s",
                NodeIdentifierString(), m_funcPtrIPEGetStripingLibVer());
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Handle is NULL for Striping context create");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::CloseStripingLibrary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IPENode::CloseStripingLibrary()
{
    CamX::OsUtils::LibUnmap(m_hHandle);
    m_hHandle = NULL;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::InitializeStripingParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::InitializeStripingParams(
    IpeConfigIoData* pConfigIOData)
{
    CamxResult     result       = CamxResultSuccess;
    ResourceParams params       = { 0 };
    UINT32         titanVersion = static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion();
    UINT32         hwVersion    = static_cast<Titan17xContext *>(GetHwContext())->GetHwVersion();

    CAMX_ASSERT(NULL != pConfigIOData);

    if (NULL != pConfigIOData)
    {
        CAMX_LOG_INFO(CamxLogGroupPProc, "titanversion 0x%x, hwversion 0x%x", titanVersion, hwVersion);
        // Check if striping in UMD is enabled before creating striping library context
        result = m_funcPtrIPECreate(pConfigIOData,
                                    NULL,
                                    titanVersion,
                                    hwVersion,
                                    &m_hStripingLib,
                                    &m_maxCmdBufferSizeBytes[CmdBufferStriping],
                                    &m_maxCmdBufferSizeBytes[CmdBufferBLMemory]);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Stripinglib ctxt failed result %d, ConfigIO %p,titanversion 0x%x,hwversion 0x%x",
                result, pConfigIOData, titanVersion, hwVersion);
        }
        else
        {
            result = CreateFWCommandBufferManagers();
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::DeInitializeStripingParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::DeInitializeStripingParams()
{
    CamxResult result = CamxResultSuccess;
    if (NULL != m_hStripingLib)
    {
        result          = m_funcPtrIPEDestroy(&m_hStripingLib);
        m_hStripingLib  = NULL;
        if (result != 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "IPE:%d Cannot destroy Striping Library with result=%d", InstanceID(), result);
            result = CamxResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::FillStripingParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillStripingParams(
    IpeFrameProcessData*         pFrameProcessData,
    IpeIQSettings*               pIPEIQsettings,
    CmdBuffer**                  ppIPECmdBuffer,
    IPEClockAndBandwidth*        pICPClockAndBandWidth)
{
    CamxResult                     result           = CamxResultSuccess;
    IPEStripingLibExecuteParams    stripeParams     = { 0 };
    UINT32*                        pStripeMem       = NULL;
    UINT32                         offset;
    IPEStripingLibExecuteMetaData  metaDataBuffer   = { 0 };
    userIPEArgs                    userIPEArgs      = {0};
    CAMX_ASSERT(NULL != ppIPECmdBuffer[CmdBufferStriping]);
    pStripeMem = reinterpret_cast<UINT32*>(
        ppIPECmdBuffer[CmdBufferStriping]->BeginCommands(m_maxCmdBufferSizeBytes[CmdBufferStriping] / 4));


    if (NULL != pStripeMem)
    {
        stripeParams.iq                 = pIPEIQsettings;
        stripeParams.ica1               = &pIPEIQsettings->ica1Parameters;
        stripeParams.ica2               = &pIPEIQsettings->ica2Parameters;
        stripeParams.zoom               = &pIPEIQsettings->ica1Parameters.zoomWindow;
        stripeParams.prevZoom           = &pIPEIQsettings->ica2Parameters.zoomWindow;
        stripeParams.maxNumOfCoresToUse = pFrameProcessData->maxNumOfCoresToUse;

        CAMX_LOG_INFO(CamxLogGroupPProc, "## node %s,"
                      "rt %d blob %u processingType %d, profile %d, stab %d, ica1 [persp %u, grid %u], ica2 [persp %u, grid %u]"
                      ", anr [%d %d %d %d], tf [%d, %d, %d %d], hnr %d, cac %d, ltm %d, cc %d, glut %d, lut %d, chromaEn %d, "
                      "chromasup %d, skin %d, asf %d, gra %d, colortransform %d, refine [%d, %d, %d], lenr %d, lmc %d, "
                      "geolib %d, numcores %d, realtimeIPE %d, request %u",
                      NodeIdentifierString(),
                      m_realTimeIPE, m_IPECmdBlobCount, m_instanceProperty.processingType,
                      m_instanceProperty.profileId, m_instanceProperty.stabilizationType,
                      pIPEIQsettings->ica1Parameters.isPerspectiveEnable,
                      pIPEIQsettings->ica1Parameters.isGridEnable,
                      pIPEIQsettings->ica2Parameters.isPerspectiveEnable,
                      pIPEIQsettings->ica2Parameters.isGridEnable,
                      pIPEIQsettings->anrParameters.parameters[0].moduleCfg.EN,
                      pIPEIQsettings->anrParameters.parameters[1].moduleCfg.EN,
                      pIPEIQsettings->anrParameters.parameters[2].moduleCfg.EN,
                      pIPEIQsettings->anrParameters.parameters[3].moduleCfg.EN,
                      pIPEIQsettings->tfParameters.parameters[PASS_NAME_FULL].moduleCfg.EN,
                      pIPEIQsettings->tfParameters.parameters[PASS_NAME_DC_4].moduleCfg.EN,
                      pIPEIQsettings->tfParameters.parameters[PASS_NAME_DC_16].moduleCfg.EN,
                      pIPEIQsettings->tfParameters.parameters[PASS_NAME_DC_64].moduleCfg.EN,
                      pIPEIQsettings->hnrParameters.moduleCfg.EN,
                      pIPEIQsettings->cacParameters.moduleCfg.EN,
                      pIPEIQsettings->ltmParameters.moduleCfg.EN,
                      pIPEIQsettings->colorCorrectParameters.moduleCfg.EN,
                      pIPEIQsettings->glutParameters.moduleCfg.EN,
                      pIPEIQsettings->lut2dParameters.moduleCfg.EN,
                      pIPEIQsettings->chromaEnhancementParameters.moduleCfg.EN,
                      pIPEIQsettings->chromaSupressionParameters.moduleCfg.EN,
                      pIPEIQsettings->skinEnhancementParameters.moduleCfg.EN,
                      pIPEIQsettings->asfParameters.moduleCfg.EN,
                      pIPEIQsettings->graParameters.moduleCfg.EN,
                      pIPEIQsettings->colorTransformParameters.moduleCfg.EN,
                      pIPEIQsettings->refinementParameters.dc[0].refinementCfg.TRENABLE,
                      pIPEIQsettings->refinementParameters.dc[1].refinementCfg.TRENABLE,
                      pIPEIQsettings->refinementParameters.dc[2].refinementCfg.TRENABLE,
                      pIPEIQsettings->lenrParameters.moduleCfg.EN,
                      pIPEIQsettings->lmcParameters.enableLMC,
                      pIPEIQsettings->useGeoLibOutput,
                      stripeParams.maxNumOfCoresToUse,
                      m_realTimeIPE,
                      pFrameProcessData->requestId);

        userIPEArgs.dumpEnable          = 0;
        userIPEArgs.frameNumber         = pFrameProcessData->requestId;
        userIPEArgs.instance            = InstanceID();
        userIPEArgs.processingType      = m_instanceProperty.processingType;
        userIPEArgs.profileID           = m_instanceProperty.profileId;
        userIPEArgs.realTime            = m_realTimeIPE;
        userIPEArgs.FileDumpPath        = FileDumpPath;
        userIPEArgs.dumpEnable          = (m_enableIPEStripeDump == 1);
        result = m_funcPtrIPEExecute(m_hStripingLib, &stripeParams, pStripeMem,
                                     &metaDataBuffer, &userIPEArgs);

        /* Output stripe width should be atleast (2*tilewidth + 2) pixels
         * in UBWC formats. Very low output resolution and high input
         * resolution sometimes violates this condition, but stripping lib
         * output is acceptable. */
        if (STRIPING_LIB_IPE_INVALID_STRIPE_UBWC == result)
        {
            IpeConfigIo*     pConfigIO;
            IpeConfigIoData* pConfigIOData;

            pConfigIO     = reinterpret_cast<IpeConfigIo*>(m_configIOMem.pVirtualAddr);
            pConfigIOData = &pConfigIO->cmdData;

            CAMX_LOG_WARN(CamxLogGroupPProc, "Ignore invalid IPE UBWC Stripe for low resolution "
                    "Input (f,w,h) = (%d,%d,%d), "
                    "DISPLAY Output (f,w,h) = (%d,%d,%d), "
                    "VIDEO Output (f,w,h) = (%d,%d,%d)",
                    pConfigIOData->images[IPE_INPUT_IMAGE_FULL].info.format,
                    pConfigIOData->images[IPE_INPUT_IMAGE_FULL].info.dimensions.widthPixels,
                    pConfigIOData->images[IPE_INPUT_IMAGE_FULL].info.dimensions.heightLines,
                    pConfigIOData->images[IPE_OUTPUT_IMAGE_DISPLAY].info.format,
                    pConfigIOData->images[IPE_OUTPUT_IMAGE_DISPLAY].info.dimensions.widthPixels,
                    pConfigIOData->images[IPE_OUTPUT_IMAGE_DISPLAY].info.dimensions.heightLines,
                    pConfigIOData->images[IPE_OUTPUT_IMAGE_VIDEO].info.format,
                    pConfigIOData->images[IPE_OUTPUT_IMAGE_VIDEO].info.dimensions.widthPixels,
                    pConfigIOData->images[IPE_OUTPUT_IMAGE_VIDEO].info.dimensions.heightLines);

            result = CamxResultSuccess;
        }

        if (CamxResultSuccess == result)
        {
            offset =
                static_cast<UINT32>(offsetof(IpeFrameProcess, cmdData)) +
                static_cast<UINT32>(offsetof(IpeFrameProcessData, stripingLibOutAddr));
            result = ppIPECmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(
                offset, ppIPECmdBuffer[CmdBufferStriping], 0);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Patching failed for Striping result=%d", NodeIdentifierString(), result);
            }
            pICPClockAndBandWidth->frameCycles = metaDataBuffer.pixelCount;
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Num pixels = %d", metaDataBuffer.pixelCount);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Req[%d] - Failed result %d, strip mem %p, cmd buffr vaddr %p, rt %d, "
                           "blob %u, processingType %d, profile %d, stab %d, ica1[persp %u, grid %u], ica2[persp %u, grid %u], "
                           "anr[%d %d %d %d], tf[%d, %d, %d %d], hnr %d, cac %d, ltm %d, cc %d, glut %d, lut %d, chromaEn %d, "
                           "chromasup %d, skin %d, asf %d, gra %d, colortransform %d, refine[%d, %d, %d], lenr %d, lmc %d, "
                           "numcores %d",
                           NodeIdentifierString(), pFrameProcessData->requestId, result, pStripeMem,
                           (NULL != ppIPECmdBuffer[CmdBufferStriping]) ? ppIPECmdBuffer[CmdBufferStriping]->GetHostAddr() : NULL
                           , m_realTimeIPE, m_IPECmdBlobCount, m_instanceProperty.processingType,
                           m_instanceProperty.profileId, m_instanceProperty.stabilizationType,
                           pIPEIQsettings->ica1Parameters.isPerspectiveEnable,
                           pIPEIQsettings->ica1Parameters.isGridEnable,
                           pIPEIQsettings->ica2Parameters.isPerspectiveEnable,
                           pIPEIQsettings->ica2Parameters.isGridEnable,
                           pIPEIQsettings->anrParameters.parameters[0].moduleCfg.EN,
                           pIPEIQsettings->anrParameters.parameters[1].moduleCfg.EN,
                           pIPEIQsettings->anrParameters.parameters[2].moduleCfg.EN,
                           pIPEIQsettings->anrParameters.parameters[3].moduleCfg.EN,
                           pIPEIQsettings->tfParameters.parameters[PASS_NAME_FULL].moduleCfg.EN,
                           pIPEIQsettings->tfParameters.parameters[PASS_NAME_DC_4].moduleCfg.EN,
                           pIPEIQsettings->tfParameters.parameters[PASS_NAME_DC_16].moduleCfg.EN,
                           pIPEIQsettings->tfParameters.parameters[PASS_NAME_DC_64].moduleCfg.EN,
                           pIPEIQsettings->hnrParameters.moduleCfg.EN,
                           pIPEIQsettings->cacParameters.moduleCfg.EN,
                           pIPEIQsettings->ltmParameters.moduleCfg.EN,
                           pIPEIQsettings->colorCorrectParameters.moduleCfg.EN,
                           pIPEIQsettings->glutParameters.moduleCfg.EN,
                           pIPEIQsettings->lut2dParameters.moduleCfg.EN,
                           pIPEIQsettings->chromaEnhancementParameters.moduleCfg.EN,
                           pIPEIQsettings->chromaSupressionParameters.moduleCfg.EN,
                           pIPEIQsettings->skinEnhancementParameters.moduleCfg.EN,
                           pIPEIQsettings->asfParameters.moduleCfg.EN,
                           pIPEIQsettings->graParameters.moduleCfg.EN,
                           pIPEIQsettings->colorTransformParameters.moduleCfg.EN,
                           pIPEIQsettings->refinementParameters.dc[0].refinementCfg.TRENABLE,
                           pIPEIQsettings->refinementParameters.dc[1].refinementCfg.TRENABLE,
                           pIPEIQsettings->refinementParameters.dc[2].refinementCfg.TRENABLE,
                           pIPEIQsettings->lenrParameters.moduleCfg.EN,
                           pIPEIQsettings->lmcParameters.enableLMC,
                           stripeParams.maxNumOfCoresToUse);

            if (0x2 == m_enableIPEStripeDump)
            {
                userIPEArgs.dumpEnable = 1;
                pIPEIQsettings->anrParameters.parameters[0].cylpfParameters.ditheringSeedFrame++;
                pIPEIQsettings->anrParameters.parameters[1].cylpfParameters.ditheringSeedFrame++;
                pIPEIQsettings->anrParameters.parameters[2].cylpfParameters.ditheringSeedFrame++;
                pIPEIQsettings->anrParameters.parameters[3].cylpfParameters.ditheringSeedFrame++;

                m_funcPtrIPEExecute(m_hStripingLib, &stripeParams, pStripeMem,
                    &metaDataBuffer, &userIPEArgs);
                DumpConfigIOData();

                CAMX_LOG_CONFIG(CamxLogGroupPProc, "%s ICA1 Zoom Window [%d, %d, %d, %d] full %d x %d, requestId %d",
                                NodeIdentifierString(),
                                pIPEIQsettings->ica1Parameters.zoomWindow.windowLeft,
                                pIPEIQsettings->ica1Parameters.zoomWindow.windowTop,
                                pIPEIQsettings->ica1Parameters.zoomWindow.windowWidth,
                                pIPEIQsettings->ica1Parameters.zoomWindow.windowHeight,
                                pIPEIQsettings->ica1Parameters.zoomWindow.fullWidth,
                                pIPEIQsettings->ica1Parameters.zoomWindow.fullHeight,
                                pFrameProcessData->requestId);
                CAMX_LOG_CONFIG(CamxLogGroupPProc, "%s ICA1 IFE Zoom Window [%d, %d, %d, %d] full %d x %d, requestId %d",
                                NodeIdentifierString(),
                                pIPEIQsettings->ica1Parameters.ifeZoomWindow.windowLeft,
                                pIPEIQsettings->ica1Parameters.ifeZoomWindow.windowTop,
                                pIPEIQsettings->ica1Parameters.ifeZoomWindow.windowWidth,
                                pIPEIQsettings->ica1Parameters.ifeZoomWindow.windowHeight,
                                pIPEIQsettings->ica1Parameters.ifeZoomWindow.fullWidth,
                                pIPEIQsettings->ica1Parameters.ifeZoomWindow.fullHeight,
                                pFrameProcessData->requestId);
                CAMX_LOG_CONFIG(CamxLogGroupPProc, "%s ICA2 Zoom Window [%d, %d, %d, %d] full %d x %d, requestId %d",
                                NodeIdentifierString(),
                                pIPEIQsettings->ica2Parameters.zoomWindow.windowLeft,
                                pIPEIQsettings->ica2Parameters.zoomWindow.windowTop,
                                pIPEIQsettings->ica2Parameters.zoomWindow.windowWidth,
                                pIPEIQsettings->ica2Parameters.zoomWindow.windowHeight,
                                pIPEIQsettings->ica2Parameters.zoomWindow.fullWidth,
                                pIPEIQsettings->ica2Parameters.zoomWindow.fullHeight,
                                pFrameProcessData->requestId);
                CAMX_LOG_CONFIG(CamxLogGroupPProc, "%s ICA2 IFE Zoom Window [%d, %d, %d, %d] full %d x %d, requestId %d",
                                NodeIdentifierString(),
                                pIPEIQsettings->ica2Parameters.ifeZoomWindow.windowLeft,
                                pIPEIQsettings->ica2Parameters.ifeZoomWindow.windowTop,
                                pIPEIQsettings->ica2Parameters.ifeZoomWindow.windowWidth,
                                pIPEIQsettings->ica2Parameters.ifeZoomWindow.windowHeight,
                                pIPEIQsettings->ica2Parameters.ifeZoomWindow.fullWidth,
                                pIPEIQsettings->ica2Parameters.ifeZoomWindow.fullHeight,
                                pFrameProcessData->requestId);
                if (TRUE == pIPEIQsettings->useGeoLibOutput)
                {
                    CAMX_LOG_CONFIG(CamxLogGroupPProc, "%s: tfCropWindow offset-x,y %f, %f, size-x,y %f, %f",
                                    NodeIdentifierString(),
                                    pIPEIQsettings->tfCropWindow.offset.x,
                                    pIPEIQsettings->tfCropWindow.offset.y,
                                    pIPEIQsettings->tfCropWindow.size.x,
                                    pIPEIQsettings->tfCropWindow.size.y);
                    CAMX_LOG_CONFIG(CamxLogGroupPProc, "%s: videoOutFOV offset-x,y %f, %f, size-x,y %f, %f",
                                    NodeIdentifierString(),
                                    pIPEIQsettings->videoOutFOV.offset.x,
                                    pIPEIQsettings->videoOutFOV.offset.y,
                                    pIPEIQsettings->videoOutFOV.size.x,
                                    pIPEIQsettings->videoOutFOV.size.y);
                    CAMX_LOG_CONFIG(CamxLogGroupPProc, "%s: m_IPESIMOMode %d displayOutFOV offset-x,y %f, %f, size-x,y %f, %f",
                                    NodeIdentifierString(),
                                    m_IPESIMOMode,
                                    pIPEIQsettings->displayOutFOV.offset.x,
                                    pIPEIQsettings->displayOutFOV.offset.y,
                                    pIPEIQsettings->displayOutFOV.size.x,
                                    pIPEIQsettings->displayOutFOV.size.y);
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Striping memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::InitializeIPEIQSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE VOID IPENode::InitializeIPEIQSettings(
    IpeIQSettings*               pIPEIQsettings)
{
    CamxResult                     result = CamxResultSuccess;

    Utils::Memset(&pIPEIQsettings->ica1Parameters.zoomWindow, 0x0, sizeof(IpeZoomWindow));
    Utils::Memset(&pIPEIQsettings->ica1Parameters.ifeZoomWindow, 0x0, sizeof(IpeZoomWindow));
    Utils::Memset(&pIPEIQsettings->ica2Parameters.zoomWindow, 0x0, sizeof(IpeZoomWindow));
    Utils::Memset(&pIPEIQsettings->ica2Parameters.ifeZoomWindow, 0x0, sizeof(IpeZoomWindow));

    for (UINT16 i = 0; i < MFHDR_EXPOSURE_MAX; i++)
    {
        pIPEIQsettings->mfhdrParameters.colorTransformParameters[i].moduleCfg.EN = 0;
        pIPEIQsettings->mfhdrParameters.glutParameters[i].moduleCfg.EN           = 0;
    }
    pIPEIQsettings->mfhdrParameters.macParameters.moduleCfg.EN                   = 0;
    pIPEIQsettings->mfhdrParameters.gtmParameters.moduleCfg.EN                   = 0;
    pIPEIQsettings->mfhdrParameters.ltmParameters.moduleCfg.EN                   = 0;
    pIPEIQsettings->mfhdrParameters.postMacGlutParameters.moduleCfg.EN           = 0;
    pIPEIQsettings->mfhdrParameters.postMacColorTransformParameters.moduleCfg.EN = 0;

    for (UINT16 i = 0; i < PASS_NAME_MAX; i++)
    {
        pIPEIQsettings->anrParameters.parameters[i].moduleCfg.EN    = 0;
        pIPEIQsettings->tfParameters.parameters[i].moduleCfg.EN     = 0;
    }

    pIPEIQsettings->ica1Parameters.isGridEnable                 = 0;
    pIPEIQsettings->ica1Parameters.isPerspectiveEnable          = 0;
    pIPEIQsettings->ica2Parameters.isGridEnable                 = 0;
    pIPEIQsettings->ica2Parameters.isPerspectiveEnable          = 0;
    pIPEIQsettings->hnrParameters.moduleCfg.EN                  = 0;
    pIPEIQsettings->cacParameters.moduleCfg.EN                  = 0;
    pIPEIQsettings->ltmParameters.moduleCfg.EN                  = 0;
    pIPEIQsettings->colorCorrectParameters.moduleCfg.EN         = 0;
    pIPEIQsettings->glutParameters.moduleCfg.EN                 = 0;
    pIPEIQsettings->lut2dParameters.moduleCfg.EN                = 0;
    pIPEIQsettings->chromaEnhancementParameters.moduleCfg.EN    = 0;
    pIPEIQsettings->chromaSupressionParameters.moduleCfg.EN     = 0;
    pIPEIQsettings->skinEnhancementParameters.moduleCfg.EN      = 0;
    pIPEIQsettings->asfParameters.moduleCfg.EN                  = 0;
    pIPEIQsettings->graParameters.moduleCfg.EN                  = 0;
    pIPEIQsettings->colorTransformParameters.moduleCfg.EN       = 0;
    pIPEIQsettings->colorCorrectParameters.moduleCfg.EN         = 0;
    pIPEIQsettings->lenrParameters.moduleCfg.EN                 = 0;
    pIPEIQsettings->lmcParameters.enableLMC                     = 0;
    pIPEIQsettings->useGeoLibOutput                             = 0;

    pIPEIQsettings->mainInputSize.widthPixels                   = 0;
    pIPEIQsettings->mainInputSize.heightLines                   = 0;

    pIPEIQsettings->tfCropWindow.offset.x                       = 0.0f;
    pIPEIQsettings->tfCropWindow.offset.y                       = 0.0f;
    pIPEIQsettings->tfCropWindow.size.x                         = 0.0f;
    pIPEIQsettings->tfCropWindow.size.y                         = 0.0f;

    pIPEIQsettings->videoOutFOV.offset.x                        = 0.0f;
    pIPEIQsettings->videoOutFOV.offset.y                        = 0.0f;
    pIPEIQsettings->videoOutFOV.size.x                          = 0.0f;
    pIPEIQsettings->videoOutFOV.size.y                          = 0.0f;

    pIPEIQsettings->displayOutFOV.offset.x                      = 0.0f;
    pIPEIQsettings->displayOutFOV.offset.y                      = 0.0f;
    pIPEIQsettings->displayOutFOV.size.x                        = 0.0f;
    pIPEIQsettings->displayOutFOV.size.y                        = 0.0f;


    for (UINT16 i = 0; i < (PASS_NAME_MAX - 1); i++)
    {
        pIPEIQsettings->refinementParameters.dc[i].refinementCfg.TRENABLE = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::PatchBLMemoryBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::PatchBLMemoryBuffer(
    IpeFrameProcessData* pFrameProcessData,
    CmdBuffer**          ppIPECmdBuffer)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != ppIPECmdBuffer[CmdBufferBLMemory]) && (0 != m_maxCmdBufferSizeBytes[CmdBufferBLMemory]))
    {
        pFrameProcessData->cdmBufferSize = m_maxCmdBufferSizeBytes[CmdBufferBLMemory];

        UINT offset =
            static_cast<UINT32>(offsetof(IpeFrameProcess, cmdData)) +
            static_cast<UINT32>(offsetof(IpeFrameProcessData, cdmBufferAddress));

        result = ppIPECmdBuffer[CmdBufferFrameProcess]->AddNestedCmdBufferInfo(offset,
                                                                               ppIPECmdBuffer[CmdBufferBLMemory],
                                                                               0);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Patching failed for BL memory result=%d", NodeIdentifierString(), result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetFaceROI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetFaceROI(
    ISPInputData* pInputData,
    UINT          parentNodeId)
{
    CamxResult              result                      = CamxResultSuccess;
    FaceROIInformation      faceRoiData                 = {};
    RectangleCoordinate*    pRoiRect                    = NULL;
    CHIRectangle            roiRect                     = {};
    CHIRectangle            cropInfo                    = {};
    CHIDimension            currentFrameDimension       = {};
    CHIRectangle            currentFrameMapWrtReference = {};
    CHIRectangle            roiWrtReferenceFrame        = {};
    UINT32                  metaTagFDRoi                = 0;
    BOOL                    bIsFDPostingResultsEnabled  = FALSE;

    GetFDPerFrameMetaDataSettings(m_fdDataOffset, &bIsFDPostingResultsEnabled, NULL);
    if (TRUE == bIsFDPostingResultsEnabled)
    {
        result = VendorTagManager::QueryVendorTagLocation(VendorTagSectionOEMFDResults,
                                                          VendorTagNameOEMFDResults, &metaTagFDRoi);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "query FD vendor failed result %d", result);
            return CamxResultSuccess;
        }

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
            Utils::Memcpy(&faceRoiData, pData[0], sizeof(FaceROIInformation));

            // Translate face roi if BPS is not parent
            if (BPS != parentNodeId)
            {
                PortCropInfo portCropInfo = { { 0 } };
                if (CamxResultSuccess == Node::GetPortCrop(this, IPEInputPortFull, &portCropInfo, NULL))
                {
                    cropInfo                            = portCropInfo.appliedCrop;
                    currentFrameMapWrtReference.top     = cropInfo.top;
                    currentFrameMapWrtReference.left    = cropInfo.left;
                    currentFrameMapWrtReference.width   = cropInfo.width;
                    currentFrameMapWrtReference.height  = cropInfo.height;

                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "currentFrameWrtReference T:%d L:%d W:%d H:%d",
                                     currentFrameMapWrtReference.top, currentFrameMapWrtReference.left,
                                     currentFrameMapWrtReference.width, currentFrameMapWrtReference.height);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "no applied crop data");
                    result = CamxResultEFailed;
                }

                if (CamxResultSuccess == result)
                {
                    // Input width/height
                    currentFrameDimension.width  = m_fullInputWidth;
                    currentFrameDimension.height = m_fullInputHeight;
                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "current dim W:%d H:%d",
                                     currentFrameDimension.width, currentFrameDimension.height);
                }
            }

            if (CamxResultSuccess == result)
            {
                UINT16 faceROICount = static_cast<UINT16>(
                    (faceRoiData.ROICount > MAX_FACE_NUM) ? MAX_FACE_NUM : faceRoiData.ROICount);

                UINT16 validROICount = 0;
                for (UINT16 i = 0; i < faceROICount; i++)
                {
                    pRoiRect = &faceRoiData.unstabilizedROI[i].faceRect;

                    roiWrtReferenceFrame.left   = pRoiRect->left;
                    roiWrtReferenceFrame.top    = pRoiRect->top;
                    roiWrtReferenceFrame.width  = pRoiRect->width;
                    roiWrtReferenceFrame.height = pRoiRect->height;

                    // If BPS is parent, no conversion would be performed
                    roiRect = Translator::ConvertROIFromReferenceToCurrent(
                        &currentFrameDimension, &currentFrameMapWrtReference, &roiWrtReferenceFrame);

                    NodeUtils::CheckForROIBound(
                        &roiRect, currentFrameDimension.width, currentFrameDimension.height, NodeIdentifierString());

                    if ((roiRect.width > 0) && (roiRect.height > 0) && (validROICount < MAX_FACE_NUM))
                    {
                        pInputData->fDData.faceCenterX[validROICount] = static_cast<INT16>(roiRect.left + (roiRect.width / 2));
                        pInputData->fDData.faceCenterY[validROICount] = static_cast<INT16>(roiRect.top + (roiRect.height / 2));
                        pInputData->fDData.faceRadius[validROICount]  =
                            static_cast<INT16>(Utils::MinUINT32(roiRect.width, roiRect.height));

                        CAMX_LOG_VERBOSE(CamxLogGroupPProc, " center x:%d y:%d r:%d",
                                         pInputData->fDData.faceCenterX[validROICount],
                                         pInputData->fDData.faceCenterY[validROICount],
                                         pInputData->fDData.faceRadius[validROICount]);

                        validROICount++;
                    }
                    else
                    {
                        CAMX_LOG_WARN(CamxLogGroupPProc, "ROI out of bound! index = %d, validROICount %d, roi (%d, %d, %d, %d)",
                           i, validROICount, roiRect.left, roiRect.top, roiRect.width, roiRect.height);
                    }
                }

                pInputData->fDData.numberOfFace = validROICount;
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Face ROI num %d max %d", pInputData->fDData.numberOfFace, MAX_FACE_NUM);
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "Face ROI is not published");
        }
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::SetAAAInputData()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::SetAAAInputData(
    ISPInputData* pInputData,
    UINT          parentNodeID)
{
    VOID*   pData[IPEVendorTagMax]   = { 0 };
    UINT64  offsets[IPEVendorTagMax] = { 0 };

    if (TRUE == m_isStatsNodeAvailable)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Get 3A properties for realtime pipeline");

        // Note, the property order should matched what defined in the enum IPEVendorTagId
        static const UINT Properties3A[] =
        {
            PropertyIDAECFrameControl,
            PropertyIDAWBFrameControl,
            PropertyIDSensorNumberOfLEDs,
        };
        static const UINT PropertySize = CAMX_ARRAY_SIZE(Properties3A);
        GetDataList(Properties3A, pData, offsets, PropertySize);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Get 3A properties for non realtime pipeline");
        static const UINT Properties3A[] =
        {
            PropertyIDAECFrameControl    | InputMetadataSectionMask,
            PropertyIDAWBFrameControl    | InputMetadataSectionMask,
            PropertyIDSensorNumberOfLEDs | InputMetadataSectionMask,
        };
        static const UINT PropertySize = CAMX_ARRAY_SIZE(Properties3A);
        GetDataList(Properties3A, pData, offsets, PropertySize);
    }

    if (NULL != pData[IPEVendorTagAECFrame])
    {
        Utils::Memcpy(pInputData->pAECUpdateData, pData[IPEVendorTagAECFrame], sizeof(AECFrameControl));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid AEC Frame metadata");
    }

    if (NULL != pData[IPEVendorTagAWBFrame])
    {
        Utils::Memcpy(pInputData->pAWBUpdateData, pData[IPEVendorTagAWBFrame], sizeof(AWBFrameControl));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid AWB Frame metadata");
    }

    if (NULL != pData[IPEVendorTagFlashNumber])
    {
        pInputData->numberOfLED = *reinterpret_cast<UINT16*>(pData[IPEVendorTagFlashNumber]);
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Number of led %d", pInputData->numberOfLED);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Flash number metadata");
    }

    pInputData->parentNodeID = parentNodeID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::HardcodeSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::HardcodeSettings(
    ISPInputData* pInputData)
{
    pInputData->pipelineIPEData.hasTFRefInput             = 0;
    pInputData->pipelineIPEData.numOfFrames               = 2;
    pInputData->pipelineIPEData.isDigitalZoomEnabled      = 0;
    pInputData->pipelineIPEData.upscalingFactorMFSR       = 1.0f;
    pInputData->pipelineIPEData.digitalZoomStartX         = 0;
    pInputData->pipelineIPEData.digitalZoomStartY         = 0;
    pInputData->pipelineIPEData.pWarpGeometryData         = NULL;
    pInputData->lensPosition                              = 1.0f;
    pInputData->lensZoom                                  = 1.0f;
    pInputData->preScaleRatio                             = 1.0f;
    pInputData->postScaleRatio                            = 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::SetICADependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::SetICADependencies(
    NodeProcessRequestData*  pNodeRequestData)
{
    // @todo (CAMX-2690) Provide this input based on usecase local variable setting to zero for now
    UINT32& rCount = pNodeRequestData->dependencyInfo[0].propertyDependency.count;

    if ((TRUE == IsBlendWithNPS()) || (TRUE == IsPostfilterWithNPS()))
    {
        // "ICAInPerspectiveTransform"
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount] = m_IPEICATAGLocation[0];

        if (CSLCameraTitanVersion::CSLTitan150 == (static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion()))
        {
            // For Talos Perspective Transform should be performed on ICA2: 'ICARefPerspectiveTransform"'
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount]  = m_IPEICATAGLocation[4];
        }

        rCount++;
    }

    if (TRUE == IsLoopBackPortEnabled())
    {
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;

        if (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d, setdependency on referenceparams in metadata req %llu",
                           InstanceID(),
                           pNodeRequestData->pCaptureRequest->requestId);
            // "ICAReferenceParamsLookAhead"
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount]  = m_IPEICATAGLocation[8];
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d, setdependency on referenceparams in metadata req %llu",
                           InstanceID(),
                           pNodeRequestData->pCaptureRequest->requestId);
            // "ICAReferenceParams"
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount] = m_IPEICATAGLocation[7];
        }
        rCount++;
    }

    // Set SAT, EIS, LDC ICA transform depedency
    SetPortLinkICATransformDependency(pNodeRequestData);

    if (TRUE == m_OEMICASettingEnable)
    {
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
        // "ICAInGridOut2InTransform"
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount]  = m_IPEICATAGLocation[1];
        rCount++;

        // "ICARefGridTransform"
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount]  = m_IPEICATAGLocation[5];
        rCount++;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::SetADRCDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::SetADRCDependencies(
    NodeProcessRequestData*  pNodeRequestData)
{
    UINT32& rCount = pNodeRequestData->dependencyInfo[0].propertyDependency.count;

    if ((TRUE == IsNodeInPipeline(IFE))                                  &&
        (TRUE == IsTagPresentInPublishList(PropertyIDIFEADRCInfoOutput)) &&
        (TRUE == m_realTimeIPE))
    {
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] =
            PropertyIDIFEADRCInfoOutput;
    }
    else if (TRUE == IsNodeInPipeline(BPS) && (TRUE == IsTagPresentInPublishList(PropertyIDBPSADRCInfoOutput)))
    {
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] =
            PropertyIDBPSADRCInfoOutput;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Don't need Set ADRC Dependency");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::UpdateICADependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::UpdateICADependencies(
    ISPInputData* pInputData)
{
    CamxResult     result                                  = CamxResultSuccess;
    VOID*          pPropertyDataICA[IPEICATagNums]         = { 0 };
    UINT64         propertyDataICAOffset[IPEICATagNums]    = { 0 };
    UINT           length                                  = CAMX_ARRAY_SIZE(m_IPEICATAGLocation);
    VOID*          pAvailableICAData                       = NULL;
    UINT           psPropertiesICATag[1]                   = { 0 };
    VOID*          pPSPropertyDataICA[1]                   = { 0 };
    UINT64         psPropertyDataICAOffset[1]              = { 0 };

    GetDataList(m_IPEICATAGLocation, pPropertyDataICA, propertyDataICAOffset, length);

    if ((IPEProcessingType::IPEMFNRPostfilter == m_instanceProperty.processingType) &&
        (IPEProfileId::IPEProfileIdDefault == m_instanceProperty.profileId))
    {
        CAMX_LOG_INFO(CamxLogGroupPProc, "IPE:%d, Skip ICA transforms for MFNR  NPS Postfilter final stage", InstanceID());
    }
    else if ((IPEProcessingType::IPEMFSRPostfilter == m_instanceProperty.processingType) &&
            (IPEProfileId::IPEProfileIdDefault == m_instanceProperty.profileId))
    {
        CAMX_LOG_INFO(CamxLogGroupPProc, "IPE:%d, Skip ICA transforms for MFSR  NPS Postfilter final stage", InstanceID());
    }
    else if (TRUE == IsValidInputPrespectiveParams())
    {
        // In perspective transform tag
        psPropertiesICATag[0]      = m_IPEICATAGLocation[0];
        pPSPropertyDataICA[0]      = NULL;
        psPropertyDataICAOffset[0] = { 0 };

        // get port link based metadata
        GetPSDataList(IPEInputPortFull, psPropertiesICATag, pPSPropertyDataICA, psPropertyDataICAOffset, 1);

        pAvailableICAData = (NULL != pPSPropertyDataICA[0]) ? pPSPropertyDataICA[0] : pPropertyDataICA[0];
        if (NULL != pAvailableICAData)
        {
            Utils::Memcpy(&pInputData->ICAConfigData.ICAInPerspectiveParams,
                reinterpret_cast<IPEICAPerspectiveTransform*>(pAvailableICAData),
                sizeof(pInputData->ICAConfigData.ICAInPerspectiveParams));

            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d, perspective IN %d , w %d, h %d, c %d, r %d, frameNum %llu",
                             InstanceID(),
                             pInputData->ICAConfigData.ICAInPerspectiveParams.perspectiveTransformEnable,
                             pInputData->ICAConfigData.ICAInPerspectiveParams.transformDefinedOnWidth,
                             pInputData->ICAConfigData.ICAInPerspectiveParams.transformDefinedOnHeight,
                             pInputData->ICAConfigData.ICAInPerspectiveParams.perspetiveGeometryNumColumns,
                             pInputData->ICAConfigData.ICAInPerspectiveParams.perspectiveGeometryNumRows,
                             pInputData->frameNum);
        }
    }

    if (TRUE == static_cast<Titan17xContext*>(
             GetHwContext())->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableICAInGrid)
    {
        pPSPropertyDataICA[0]      = NULL;
        psPropertyDataICAOffset[0] = { 0 };

        if ((TRUE == IsStabilizationTypeEIS()) || (TRUE == CheckForNonEISLDCGridDependency()))
        {
            // In grid out2in transform tag
            psPropertiesICATag[0] = m_IPEICATAGLocation[1];
            // get port link based metadata
            GetPSDataList(IPEInputPortFull, psPropertiesICATag, pPSPropertyDataICA, psPropertyDataICAOffset, 1);
        }

        pAvailableICAData = (NULL != pPSPropertyDataICA[0]) ? pPSPropertyDataICA[0] : pPropertyDataICA[1];
        if (NULL != pAvailableICAData)
        {
            Utils::Memcpy(&pInputData->ICAConfigData.ICAInGridParams,
                          reinterpret_cast<IPEICAGridTransform*>(pAvailableICAData),
                          sizeof(pInputData->ICAConfigData.ICAInGridParams));
        }
        else if (TRUE == NeedICAGridFromInputPool())
        {
            // Query from Input data for offline pipeline
            static const UINT PropertiesOfflineICATag[] =
            {
                m_IPEICATAGLocation[1] | InputMetadataSectionMask
            };
            VOID*        pICAData[1]                   = { 0 };
            UINT64       propertyDataIPEOffset[1]      = { 0 };

            GetDataList(PropertiesOfflineICATag, pICAData, propertyDataIPEOffset, 1);
            if (NULL != pICAData[0])
            {
                Utils::Memcpy(&pInputData->ICAConfigData.ICAInGridParams,
                            reinterpret_cast<IPEICAGridTransform*>(pICAData[0]),
                            sizeof(pInputData->ICAConfigData.ICAInGridParams));
            }
        }
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d, grid IN %d , w %d, h %d, corner %d, frameNum %llu",
                         InstanceID(),
                         pInputData->ICAConfigData.ICAInGridParams.gridTransformEnable,
                         pInputData->ICAConfigData.ICAInGridParams.transformDefinedOnWidth,
                         pInputData->ICAConfigData.ICAInGridParams.transformDefinedOnHeight,
                         pInputData->ICAConfigData.ICAInGridParams.gridTransformArrayExtrapolatedCorners,
                         pInputData->frameNum);
    }

    if (NULL != pPropertyDataICA[3])
    {
        Utils::Memcpy(&pInputData->ICAConfigData.ICAInInterpolationParams,
            reinterpret_cast<IPEICAInterpolationParams*>(pPropertyDataICA[3]),
            sizeof(pInputData->ICAConfigData.ICAInInterpolationParams));
    }

    if (NULL != pPropertyDataICA[4])
    {
        if ((IPEProcessingType::IPEMFNRPostfilter == m_instanceProperty.processingType) &&
            (IPEProfileId::IPEProfileIdDefault == m_instanceProperty.profileId))
        {
            CAMX_LOG_INFO(CamxLogGroupPProc, "Skip ICA transforms for MFNR  NPS Postfilter final stage");
        }
        else
        {
            Utils::Memcpy(&pInputData->ICAConfigData.ICARefPerspectiveParams,
                reinterpret_cast<IPEICAPerspectiveTransform*>(pPropertyDataICA[4]),
                sizeof(pInputData->ICAConfigData.ICARefPerspectiveParams));

            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d, perspective REF %d , w %d, h %d, c %d, r %d, frameNum %llu",
                             InstanceID(),
                             pInputData->ICAConfigData.ICARefPerspectiveParams.perspectiveTransformEnable,
                             pInputData->ICAConfigData.ICARefPerspectiveParams.transformDefinedOnWidth,
                             pInputData->ICAConfigData.ICARefPerspectiveParams.transformDefinedOnHeight,
                             pInputData->ICAConfigData.ICARefPerspectiveParams.perspetiveGeometryNumColumns,
                             pInputData->ICAConfigData.ICARefPerspectiveParams.perspectiveGeometryNumRows,
                             pInputData->frameNum);
        }
    }

    if (NULL != pPropertyDataICA[5])
    {
        if (TRUE == static_cast<Titan17xContext*>(
            GetHwContext())->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableICARefGrid)
        {
            Utils::Memcpy(&pInputData->ICAConfigData.ICARefGridParams,
                reinterpret_cast<IPEICAGridTransform*>(pPropertyDataICA[5]),
                sizeof(pInputData->ICAConfigData.ICARefGridParams));
        }
    }

    if (NULL != pPropertyDataICA[6])
    {
        Utils::Memcpy(&pInputData->ICAConfigData.ICARefInterpolationParams,
            reinterpret_cast<IPEICAInterpolationParams*>(pPropertyDataICA[6]),
            sizeof(pInputData->ICAConfigData.ICARefInterpolationParams));
    }

    if (TRUE == IsLoopBackPortEnabled())
    {
        IPEICAPerspectiveTransform* pImageAlignmentReferenceParams    = NULL;
        IPEICAPerspectiveTransform* pGyroAlignmentReferenceParams     = NULL;
        IPEICAPerspectiveTransform* pSelectedAlignmentReferenceParams = NULL;

        if (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType))
        {
            if (NULL != pPropertyDataICA[8])
            {
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d, received image alignment refrence params req %llu",
                                 InstanceID(),
                                 pInputData->frameNum);
                // CVP MCTF image alignment for EISv3 lookahead instance
                pImageAlignmentReferenceParams = reinterpret_cast<IPEICAPerspectiveTransform*>(pPropertyDataICA[8]);
            }
        }
        else if (NULL != pPropertyDataICA[7])
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d, received image alignment refrence params req %llu",
                             InstanceID(),
                             pInputData->frameNum);
            // CVP MCTF image alignment for EISv2 realtime instance
            pImageAlignmentReferenceParams = reinterpret_cast<IPEICAPerspectiveTransform*>(pPropertyDataICA[7]);
        }

        if (((0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType)) ||
            (0 != (IPEStabilizationType::IPEStabilizationTypeEIS2 & m_instanceProperty.stabilizationType))) &&
            (TRUE == m_capability.referenceLookaheadTransformSupported))
        {
            // EIS MCTF gyro based alignemnt matrix
            psPropertiesICATag[0] = m_IPEICATAGLocation[7];
            GetPSDataList(IPEInputPortFull, psPropertiesICATag, pPSPropertyDataICA, psPropertyDataICAOffset, 1);
            if (NULL != pPSPropertyDataICA[0])
            {
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%d, received gyro alignment refrence params req %llu",
                                 InstanceID(),
                                 pInputData->frameNum);
                pGyroAlignmentReferenceParams = reinterpret_cast<IPEICAPerspectiveTransform*>(pPSPropertyDataICA[0]);
            }
        }

        pSelectedAlignmentReferenceParams = GetAlignmentFusion(pImageAlignmentReferenceParams,
                                                               pGyroAlignmentReferenceParams,
                                                               pInputData);

        if (NULL != pSelectedAlignmentReferenceParams)
        {
            Utils::Memcpy(&pInputData->ICAConfigData.ICAReferenceParams,
                          pSelectedAlignmentReferenceParams,
                          sizeof(pInputData->ICAConfigData.ICAReferenceParams));
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupPProc, "%s: CVP transform null %p, disabled %d or confidence low  %d"
                          "gyro transform null %p, disabled %d, or confidence from gyro low %d - fall back to unity",
                          NodeIdentifierString(),
                          pImageAlignmentReferenceParams,
                          ((NULL != pImageAlignmentReferenceParams) ?
                              pImageAlignmentReferenceParams->perspectiveTransformEnable : 0),
                          ((NULL != pImageAlignmentReferenceParams) ?
                              pImageAlignmentReferenceParams->perspectiveConfidence : 0),
                          pGyroAlignmentReferenceParams,
                          ((NULL != pGyroAlignmentReferenceParams) ?
                              pGyroAlignmentReferenceParams->perspectiveTransformEnable : 0),
                          ((NULL != pGyroAlignmentReferenceParams) ?
                              pGyroAlignmentReferenceParams->perspectiveConfidence : 0));
            Utils::Memcpy(pInputData->ICAConfigData.ICAReferenceParams.perspectiveTransformArray,
                           perspArray, sizeof(perspArray));
            pInputData->ICAConfigData.ICAReferenceParams.perspectiveTransformEnable       = 1;
            pInputData->ICAConfigData.ICAReferenceParams.ReusePerspectiveTransform        = 0;
            pInputData->ICAConfigData.ICAReferenceParams.perspetiveGeometryNumColumns     = 1;
            pInputData->ICAConfigData.ICAReferenceParams.perspectiveGeometryNumRows       = 1;
            pInputData->ICAConfigData.ICAReferenceParams.perspectiveConfidence            = 128;
            pInputData->ICAConfigData.ICAReferenceParams.byPassAlignmentMatrixAdjustement = 1;
            pInputData->ICAConfigData.ICAReferenceParams.transformDefinedOnWidth          = m_fullInputWidth;;
            pInputData->ICAConfigData.ICAReferenceParams.transformDefinedOnHeight         = m_fullInputHeight;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                         "IPE:%d Perspective ReferenceParams %d , w %d, h %d, c %d, r %d, conf %u, identity %u, frameNum %llu",
                         InstanceID(),
                         pInputData->ICAConfigData.ICAReferenceParams.perspectiveTransformEnable,
                         pInputData->ICAConfigData.ICAReferenceParams.transformDefinedOnWidth,
                         pInputData->ICAConfigData.ICAReferenceParams.transformDefinedOnHeight,
                         pInputData->ICAConfigData.ICAReferenceParams.perspetiveGeometryNumColumns,
                         pInputData->ICAConfigData.ICAReferenceParams.perspectiveGeometryNumRows,
                         pInputData->ICAConfigData.ICAReferenceParams.perspectiveConfidence,
                         pInputData->ICAConfigData.ICAReferenceParams.byPassAlignmentMatrixAdjustement,
                         pInputData->frameNum);
    }

    // Update margins in pixels
    pInputData->pipelineIPEData.marginDimension.widthPixels = m_stabilizationMargin.widthPixels;
    pInputData->pipelineIPEData.marginDimension.heightLines = m_stabilizationMargin.heightLines;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::PublishICADependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::PublishICADependencies(
    NodeProcessRequestData* pNodeRequestData)
{

    const VOID*       pData[2]      = { 0 };
    UINT              pDataCount[2] = { 0 };
    CamxResult        result        = CamxResultSuccess;
    UINT              i             = 0;


    // Example as to how perspective and grid are posted
    if ((IPEProcessingType::IPEMFNRBlend                == m_instanceProperty.processingType)          ||
        (IPEProcessingType::IPEMFNRPostfilter           == m_instanceProperty.processingType)          ||
        (IPEProcessingType::IPEMFSRBlend                == m_instanceProperty.processingType)          ||
        (IPEProcessingType::IPEMFSRPostfilter           == m_instanceProperty.processingType)          ||
        (0 != (IPEStabilizationType::IPEStabilizationTypeEIS2 & m_instanceProperty.stabilizationType)) ||
        (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType)))
    {
        CAMX_UNREFERENCED_PARAM(pNodeRequestData);
        IPEICAPerspectiveTransform transform;
        {
            transform.perspectiveConfidence        = 255;
            transform.perspectiveGeometryNumRows   = 9;
            transform.perspetiveGeometryNumColumns = 1;
            transform.perspectiveTransformEnable   = 1;
            transform.ReusePerspectiveTransform    = 0;
            transform.transformDefinedOnWidth      = m_fullInputWidth;
            transform.transformDefinedOnHeight     = m_fullInputHeight;
            Utils::Memcpy(&transform.perspectiveTransformArray,
                          perspArray, sizeof(perspArray));
        }
        pData[i]      = &transform;
        pDataCount[i] = sizeof(transform);
        i++;

    }

    if ((0 != (IPEStabilizationType::IPEStabilizationTypeEIS2 & m_instanceProperty.stabilizationType)) ||
        (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType)))
    {
        CAMX_STATIC_ASSERT(sizeof(gridArrayX20) == sizeof(gridArrayY20));
        CAMX_STATIC_ASSERT(sizeof(gridArrayX) == sizeof(gridArrayY));
        BOOL isICA20 = ((CSLCameraTitanVersion::CSLTitan175 ==
                        static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion()) ||
                        (CSLCameraTitanVersion::CSLTitan160 ==
                        static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion())) ? TRUE : FALSE;

        const FLOAT* gArrayX  = (TRUE == isICA20) ? gridArrayX20 : gridArrayX;
        const FLOAT* gArrayY  = (TRUE == isICA20) ? gridArrayY20 : gridArrayY;
        UINT32 gridTWidth     = (TRUE == isICA20) ? ICA20GridTransformWidth  : ICA10GridTransformWidth;
        UINT32 gridTHeight    = (TRUE == isICA20) ? ICA20GridTransformHeight : ICA10GridTransformHeight;

        IPEICAGridTransform gridTransform;
        {
            gridTransform.gridTransformEnable      = 1;
            gridTransform.reuseGridTransform       = 0;
            gridTransform.transformDefinedOnWidth  = GridTransformDefinedOnWidth;
            gridTransform.transformDefinedOnHeight = GridTransformDefinedOnHeight;

            for (UINT idx = 0; idx < (gridTWidth * gridTHeight); idx++)
            {
                gridTransform.gridTransformArray[idx].x = gArrayX[idx];
                gridTransform.gridTransformArray[idx].y = gArrayY[idx];
            }

            gridTransform.gridTransformArrayExtrapolatedCorners = 0;
        }
        pData[i] = &gridTransform;
        pDataCount[i] = sizeof(gridTransform);
        i++;
    }

    result = WriteDataList(&m_IPEICATAGLocation[0], pData, pDataCount, i);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "WriteDataList failed");
    }
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::HardcodeAAAInputData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::HardcodeAAAInputData(
    ISPInputData* pInputData,
    UINT          parentNodeID)
{
    pInputData->pAECUpdateData->luxIndex                       = 350.0f;
    for (UINT i = 0; i < ExposureIndexCount; i++)
    {
        pInputData->pAECUpdateData->exposureInfo[i].exposureTime    = 1;
        pInputData->pAECUpdateData->exposureInfo[i].linearGain  = 1.0f;
        pInputData->pAECUpdateData->exposureInfo[i].sensitivity = 1.0f;
    }
    pInputData->pAECUpdateData->predictiveGain                 = 1.0f;
    pInputData->pAWBUpdateData->AWBGains.rGain                 = 2.043310f;
    pInputData->pAWBUpdateData->AWBGains.gGain                 = 1.0f;
    pInputData->pAWBUpdateData->AWBGains.bGain                 = 1.493855f;
    pInputData->pAWBUpdateData->colorTemperature               = 2600;
    pInputData->pAWBUpdateData->numValidCCMs                   = 1;
    pInputData->pAWBUpdateData->AWBCCM[0].isCCMOverrideEnabled = FALSE;
    pInputData->parentNodeID                                   = parentNodeID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateNumberofPassesonDimension
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::UpdateNumberofPassesonDimension(
    UINT32  fullInputWidth,
    UINT32  fullInputHeight,
    UINT32* pNumPasses)
{
    CamxResult  result                     = CamxResultSuccess;
    UINT        numPasses                  = 1;
    UINT        downScale                  = 4;
    UINT32      inputWidthWithoutMargin    = 0;
    UINT32      inputHeightWithoutMargin   = 0;

    GetSizeWithoutStablizationMargin(fullInputWidth, fullInputHeight, &inputWidthWithoutMargin, &inputHeightWithoutMargin);
    // Full pass input port is always enabled
    for (UINT passNumber = PASS_NAME_DC_4; passNumber < *pNumPasses; passNumber++)
    {
        if (((inputWidthWithoutMargin / downScale) >= ICAMinWidthPixels) &&
            ((inputHeightWithoutMargin / downScale) >= ICAMinHeightPixels))
        {
            numPasses++;
        }
        downScale *= 4;
    }

    if (numPasses != *pNumPasses)
    {
        *pNumPasses = numPasses;
        CAMX_LOG_INFO(CamxLogGroupIQMod, " Update numberofpasses to %d  %d due to unsupported dimension",
                      *pNumPasses, numPasses);
    }

    if (PASS_NAME_MAX < *pNumPasses)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "IPE's number of passes is %u, %u, which is more than 4 (MAX) ",
                       *pNumPasses, numPasses);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetOEMStatsConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetOEMStatsConfig(
    ISPInputData* pInputData,
    UINT          parentNodeID)
{
    CamxResult result = CamxResultSuccess;

    UINT32 metadataAECFrameControl   = 0;
    UINT32 metadataAWBFrameControl   = 0;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AECFrameControl",
        &metadataAECFrameControl);

    result |= VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "AWBFrameControl",
        &metadataAWBFrameControl);


    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Fail to query metadata tag location AEC Frame control: %d, AWB Frame control: %d",
            metadataAECFrameControl, metadataAWBFrameControl);
    }

    if (CamxResultSuccess == result)
    {
        static const UINT vendorTagsControl3A[] =
        {
            metadataAECFrameControl | InputMetadataSectionMask,
            metadataAWBFrameControl | InputMetadataSectionMask,
        };

        const SIZE_T numTags = CAMX_ARRAY_SIZE(vendorTagsControl3A);
        VOID*        pVendorTagsControl3A[numTags] = { 0 };
        UINT64       vendorTagsControl3AOffset[numTags] = { 0 };

        GetDataList(vendorTagsControl3A, pVendorTagsControl3A, vendorTagsControl3AOffset, numTags);
        if (NULL != pVendorTagsControl3A[IPEVendorTagAECFrame] && NULL != pVendorTagsControl3A[IPEVendorTagAWBFrame])
        {
            // Pointers in pVendorTagsControl3A[] guaranteed to be non-NULL by GetDataList() for InputMetadataSectionMask
            Utils::Memcpy(pInputData->pAECUpdateData, pVendorTagsControl3A[IPEVendorTagAECFrame], sizeof(AECFrameControl));
            Utils::Memcpy(pInputData->pAWBUpdateData, pVendorTagsControl3A[IPEVendorTagAWBFrame], sizeof(AWBFrameControl));
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid OEM AEC frame control and AWB control");
            result = CamxResultEInvalidArg;
        }

    }
    pInputData->parentNodeID = parentNodeID;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetOEMIQConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetOEMIQConfig(
    ISPInputData* pInputData,
    UINT          parentNodeID)
{
    CamxResult result             = CamxResultSuccess;
    UINT32     metadataIPEIQParam = 0;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.iqsettings", "OEMIPEIQSetting",
        &metadataIPEIQParam);
    if (CamxResultSuccess == result)
    {
        static const UINT vendorTagsControlIQ[] =
        {
            metadataIPEIQParam | InputMetadataSectionMask,
        };

        const SIZE_T numTags                            = CAMX_ARRAY_SIZE(vendorTagsControlIQ);
        VOID*        pVendorTagsControlIQ[numTags]      = { 0 };
        UINT64       vendorTagsControlIQOffset[numTags] = { 0 };

        GetDataList(vendorTagsControlIQ, pVendorTagsControlIQ, vendorTagsControlIQOffset, numTags);

        pInputData->pOEMIQSetting = pVendorTagsControlIQ[IPEIQParam];
    }
    else
    {
        pInputData->pOEMIQSetting = NULL;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to query OEM IPEIQSetting metadata tag: %d", metadataIPEIQParam);
    }
    pInputData->parentNodeID = parentNodeID;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::NotifyRequestProcessingError()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::NotifyRequestProcessingError(
    NodeFenceHandlerData* pFenceHandlerData,
    UINT                  unSignaledFenceCount)
{
    CAMX_ASSERT(NULL != pFenceHandlerData);
    const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    OutputPort*     pOutputPort     = pFenceHandlerData->pOutputPort;
    CSLFenceResult  fenceResult     = pFenceHandlerData->fenceResult;
    BOOL            enableDump      = ((((0x2 == m_enableIPEHangDump) && (CSLFenceResultFailed == fenceResult)) ||
                                      ((0x1 == m_enableIPEHangDump) && (0 == unSignaledFenceCount))) ? TRUE : FALSE);

    BOOL      enableRefDump                 = FALSE;
    BOOL      enableIPEInternalDSBuffers    = FALSE;
    UINT32    enableOutputPortMask          = 0xFFFFFFFF;
    UINT32    enableInputPortMask           = 0xFFFFFFFF;
    UINT32    enableInstanceMask            = 0xFFFFFFFF;
    UINT32    enabledNodeMask               = pSettings->autoImageDumpMask;

    UINT errorCase = (CSLFenceResultFailed == fenceResult) ? TRUE : FALSE;
    if (CSLFenceResultSuccess != fenceResult)
    {
        if (CSLFenceResultFailed == fenceResult)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Fence failure for output port %d req %llu enableDump %d",
                           pOutputPort->portId, pFenceHandlerData->requestId, enableDump);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Fence failure during flush for output port %d req %llu enableDump %d",
                             pOutputPort->portId, pFenceHandlerData->requestId, enableDump);
        }
    }

    if ((TRUE == pSettings->autoImageDump) && (enabledNodeMask & ImageDumpIPE))
    {
        enableOutputPortMask        = pSettings->autoImageDumpIPEoutputPortMask;
        enableInstanceMask          = pSettings->autoImageDumpIPEInstanceMask;
        enableRefDump               = ((0 == unSignaledFenceCount) ? TRUE : FALSE);
        enableIPEInternalDSBuffers  = TRUE;
    }

    if (TRUE == enableRefDump)
    {
        DumpLoopBackReferenceBuffer(pFenceHandlerData->requestId, enableOutputPortMask,
            enableInstanceMask,
            GetBatchedHALOutputNum(pFenceHandlerData->pOutputPort->numBatchedFrames,
                                   pFenceHandlerData->pOutputPort->HALOutputBufferCombined));
    }

    if (TRUE == enableIPEInternalDSBuffers)
    {
        DumpInternalMultiPassBuffer(pFenceHandlerData->requestId, enableInputPortMask, enableInstanceMask, 1);
    }

    if (TRUE == enableDump)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "notify error fence back for request %llu", pFenceHandlerData->requestId);
        CmdBuffer* pBuffer = NULL;
        pBuffer =
            CheckCmdBufferWithRequest(pFenceHandlerData->requestId, m_pIPECmdBufferManager[CmdBufferBLMemory]);
        if (!pBuffer)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "cant find buffer");
            return;
        }

        DumpDebug(CmdBufferBLMemory, pBuffer, pFenceHandlerData->requestId,
            IsRealTime(), m_instanceProperty, NodeIdentifierString(), errorCase);
    }
    if (TRUE == GetStaticSettings()->enableIPEScratchBufferDump)
    {
        DumpScratchBuffer(m_pScratchMemoryBuffer, m_numScratchBuffers, pFenceHandlerData->requestId, InstanceID(),
                            IsRealTime(), m_instanceProperty, GetPipeline());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetStaticMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::GetStaticMetadata()
{
    HwCameraInfo    cameraInfo;

    HwEnvironment::GetInstance()->GetCameraInfo(GetPipeline()->GetCameraId(), &cameraInfo);

    // Initialize default metadata
    m_HALTagsData.saturation                        = 5;
    m_HALTagsData.colorCorrectionAberrationMode     = ColorCorrectionAberrationModeFast;
    m_HALTagsData.edgeMode                          = EdgeModeFast;
    m_HALTagsData.controlVideoStabilizationMode     = NoiseReductionModeFast;
    m_HALTagsData.sharpness                         = 1;
    // Initialize default metadata
    m_HALTagsData.blackLevelLock                    = BlackLevelLockOff;
    m_HALTagsData.colorCorrectionMode               = ColorCorrectionModeFast;
    m_HALTagsData.controlAEMode                     = ControlAEModeOn;
    m_HALTagsData.controlAWBMode                    = ControlAWBModeAuto;
    m_HALTagsData.controlMode                       = ControlModeAuto;

    m_HALTagsData.noiseReductionMode                = NoiseReductionModeFast;
    m_HALTagsData.shadingMode                       = ShadingModeFast;
    m_HALTagsData.statisticsHotPixelMapMode         = StatisticsHotPixelMapModeOff;
    m_HALTagsData.statisticsLensShadingMapMode      = StatisticsLensShadingMapModeOff;
    m_HALTagsData.tonemapCurves.tonemapMode         = TonemapModeFast;

    // Retrieve the static capabilities for this camera
    CAMX_ASSERT(MaxCurvePoints >= cameraInfo.pPlatformCaps->maxTonemapCurvePoints);
    m_HALTagsData.tonemapCurves.curvePoints = cameraInfo.pPlatformCaps->maxTonemapCurvePoints;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::GetStabilizationMargins()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetStabilizationMargins()
{
    CamxResult          result                     = CamxResultSuccess;
    UINT32              receivedMarginEISTag       = 0;
    UINT32              additionalCropOffsetEISTag = 0;
    StabilizationMargin receivedMargin             = { 0 };
    ImageDimensions     additionalCropOffset       = { 0 };

    if (0 != (IPEStabilizationType::IPEStabilizationTypeEIS2 & m_instanceProperty.stabilizationType))
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime",
                                                          "StabilizationMargins", &receivedMarginEISTag);
        CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: StabilizationMargins");

        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime",
                                                          "AdditionalCropOffset", &additionalCropOffsetEISTag);
        CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: AdditionalCropOffset");
    }
    else if (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType))
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eislookahead",
                                                          "StabilizationMargins", &receivedMarginEISTag);
        CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: StabilizationMargins");

        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eislookahead",
                                                          "AdditionalCropOffset", &additionalCropOffsetEISTag);
        CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: AdditionalCropOffset");
    }
    else if (0 != (IPEStabilizationType::IPEStabilizationTypeSWEIS2 & m_instanceProperty.stabilizationType))
    {
        // In case of SWEIS, read AdditionalCropOffset meta only, as stabilization margin is already reduced by
        // dewarp node.
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime",
                                                          "AdditionalCropOffset", &additionalCropOffsetEISTag);
        CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: AdditionalCropOffset");
    }
    else if (0 != (IPEStabilizationType::IPEStabilizationTypeSWEIS3 & m_instanceProperty.stabilizationType))
    {
        // In case of SWEIS, read AdditionalCropOffset meta only, as stabilization margin is already reduced by
        // dewarp node.
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eislookahead",
                                                          "AdditionalCropOffset", &additionalCropOffsetEISTag);
        CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fail to query: AdditionalCropOffset");
    }

    const UINT marginTags[] =
    {
        receivedMarginEISTag       | UsecaseMetadataSectionMask,
        additionalCropOffsetEISTag | UsecaseMetadataSectionMask,
    };

    const SIZE_T length         = CAMX_ARRAY_SIZE(marginTags);
    VOID*        pData[length]  = { 0 };
    UINT64       offset[length] = { 0 };

    result = GetDataList(marginTags, pData, offset, length);

    if (CamxResultSuccess == result)
    {
        if (NULL != pData[0])
        {
            receivedMargin                    = *static_cast<StabilizationMargin*>(pData[0]);
            m_stabilizationMargin.widthPixels = Utils::EvenFloorUINT32(receivedMargin.widthPixels);
            m_stabilizationMargin.heightLines = Utils::EvenFloorUINT32(receivedMargin.heightLines);
        }

        if (NULL != pData[1])
        {
            additionalCropOffset               = *static_cast<ImageDimensions*>(pData[1]);
            m_additionalCropOffset.widthPixels = Utils::EvenFloorUINT32(additionalCropOffset.widthPixels);
            m_additionalCropOffset.heightLines = Utils::EvenFloorUINT32(additionalCropOffset.heightLines);
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupCore,
                     "IPE stabilization margins for stabilization type %d set to %ux%u, additional Crop offset %ux%u",
                     m_instanceProperty.stabilizationType,
                     m_stabilizationMargin.widthPixels,
                     m_stabilizationMargin.heightLines,
                     m_additionalCropOffset.widthPixels,
                     m_additionalCropOffset.heightLines);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::SetScaleRatios
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPENode::SetScaleRatios(
    ISPInputData*     pInputData,
    UINT              parentNodeID,
    CHIRectangle*     pCropInfo,
    IFEScalerOutput*  pIFEScalerOutput,
    CmdBuffer*        pCmdBuffer,
    BOOL              upscaleonStream)
{
    CAMX_UNREFERENCED_PARAM(pCmdBuffer);

    // initialize with default 1.0f
    pInputData->preScaleRatio  = 1.0f;
    pInputData->postScaleRatio = 1.0f;

    if (NULL == pCropInfo)
    {
        CAMX_LOG_INFO(CamxLogGroupPProc, "pCropInfo is NULL");
        return FALSE;
    }

    CAMX_LOG_INFO(CamxLogGroupPProc,
                  "%s, parentNodeID=%d, input width = %d, height = %d, output width = %d, height = %d",
                  NodeIdentifierString(), parentNodeID,
                  m_fullInputWidth, m_fullInputHeight,
                  m_fullOutputWidth, m_fullOutputHeight);

    if (TRUE == IsSrEnabled())
    {

        GeoLibIcaMapping* pICAMappingFull      =
            &m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig.ica1Mapping.icaMappingFull;
        GeoLibCropAndScale* pPPSUsCropAndScale =
            &m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig.ppsUsCropAndScale;
        GeoLibCropAndScale* pBPSOutFullCrop    =
            &m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig.bpsOutFullCrop;

        FLOAT postScaleX = (GeoLibMultiFrameMode::GEOLIB_MF_FINAL ==
                            m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig.mfMode) ?
                            (static_cast<FLOAT>(pPPSUsCropAndScale->inputImageSize.widthPixels) /
                            static_cast<FLOAT>(pPPSUsCropAndScale->outputImageSize.widthPixels)) : 1.0f;
        FLOAT postScaleY = (GeoLibMultiFrameMode::GEOLIB_MF_FINAL ==
                            m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig.mfMode) ?
                            (static_cast<FLOAT>(pPPSUsCropAndScale->inputImageSize.heightLines) /
                            static_cast<FLOAT>(pPPSUsCropAndScale->outputImageSize.heightLines)) : 1.0f;

        FLOAT preScaleY = (GeoLibMultiFrameMode::GEOLIB_MF_FINAL ==
                           m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig.mfMode) ?
                           // work around to know ica upcale in previous stages as it is same as inverse of bps crop
                           (static_cast<FLOAT>(pBPSOutFullCrop->outputImageSize.heightLines) /
                           static_cast<FLOAT>(pBPSOutFullCrop->inputImageSize.heightLines)) :
                           (static_cast<FLOAT>(pICAMappingFull->inputImageSize.heightLines) /
                           static_cast<FLOAT>(pICAMappingFull->outputImageSize.heightLines));


        FLOAT preScaleX = (GeoLibMultiFrameMode::GEOLIB_MF_FINAL ==
                           m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig.mfMode) ?
                           // work around to know ica upcale in previous stages as it is same as inverse of bps crop
                           (static_cast<FLOAT>(pBPSOutFullCrop->outputImageSize.widthPixels) /
                           static_cast<FLOAT>(pBPSOutFullCrop->inputImageSize.widthPixels)) :
                           (static_cast<FLOAT>(pICAMappingFull->inputImageSize.widthPixels) /
                           static_cast<FLOAT>(pICAMappingFull->outputImageSize.widthPixels));

        pInputData->postScaleRatio = postScaleX > postScaleY ? postScaleY : postScaleX;


        pInputData->preScaleRatio = preScaleX > preScaleY ? preScaleY : preScaleX;

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s:, ICA1 mapping inputroi x %f * y %f, output roi x %f, y %f,"
                         "input size %d, %d output size %d %d",
                         NodeIdentifierString(),
                         pICAMappingFull->inputImageRoi.size.x,
                         pICAMappingFull->inputImageRoi.size.y,
                         pICAMappingFull->outputImageRoi.size.x,
                         pICAMappingFull->outputImageRoi.size.y,
                         pICAMappingFull->inputImageSize.widthPixels,
                         pICAMappingFull->inputImageSize.heightLines,
                         pICAMappingFull->outputImageSize.widthPixels,
                         pICAMappingFull->outputImageSize.heightLines);

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s:, pps upscale inputroi x %f * y %f, output roi x %f, y %f,"
                         "input size %d, %d output size %d %d",
                         NodeIdentifierString(),
                         pPPSUsCropAndScale->inputImageRoi.size.x,
                         pPPSUsCropAndScale->inputImageRoi.size.y,
                         pPPSUsCropAndScale->outputImageRoi.size.x,
                         pPPSUsCropAndScale->outputImageRoi.size.y,
                         pPPSUsCropAndScale->inputImageSize.widthPixels,
                         pPPSUsCropAndScale->inputImageSize.heightLines,
                         pPPSUsCropAndScale->outputImageSize.widthPixels,
                         pPPSUsCropAndScale->outputImageSize.heightLines);

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s:, bps crop inputroi x %f * y %f, output roi x %f, y %f,"
                         "input size %d, %d output size %d, %d",
                         NodeIdentifierString(),
                         pBPSOutFullCrop->inputImageRoi.size.x,
                         pBPSOutFullCrop->inputImageRoi.size.y,
                         pBPSOutFullCrop->outputImageRoi.size.x,
                         pBPSOutFullCrop->outputImageRoi.size.y,
                         pBPSOutFullCrop->inputImageSize.widthPixels,
                         pBPSOutFullCrop->inputImageSize.heightLines,
                         pBPSOutFullCrop->outputImageSize.widthPixels,
                         pBPSOutFullCrop->outputImageSize.heightLines);
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s: mode %d,mfMode %d,prescaleX %f,preScaleY %f,postscaleX %f,postScaleY %f",
                         NodeIdentifierString(),
                         m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig.flowMode,
                         m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig.mfMode,
                         preScaleX, preScaleY, postScaleX, postScaleY);
        CAMX_LOG_INFO(CamxLogGroupPProc,
                      "%s: parent %d fwidth= %d,fheight= %d,preScaleRatio= %f,postScaleRatio= %f",
                      NodeIdentifierString(),
                      parentNodeID,
                      m_fullOutputWidth,
                      m_fullOutputHeight,
                      pInputData->preScaleRatio,
                      pInputData->postScaleRatio);
    }
    else
    {
        if (BPS == parentNodeID)
        {
            if (TRUE == upscaleonStream)
            {
                FLOAT ratio1 = static_cast<FLOAT>(pCropInfo->width) / static_cast<FLOAT>(m_fullOutputWidth);
                FLOAT ratio2 = static_cast<FLOAT>(pCropInfo->height) / static_cast<FLOAT>(m_fullOutputHeight);
                pInputData->postScaleRatio = (ratio1 > ratio2) ? ratio2 : ratio1;
            }
        }
        else if ((IFE == parentNodeID) || (ChiExternalNode == parentNodeID))
        {
            FLOAT preScaleRatio = 1.0f;

            // Update with previous scaler output
            if ((NULL != pIFEScalerOutput) && (FALSE == Utils::FEqual(0.0f, pIFEScalerOutput->scalingFactor)))
            {
                preScaleRatio = pIFEScalerOutput->scalingFactor;
                CAMX_LOG_INFO(CamxLogGroupPProc, "IFE scaling factor %f", pIFEScalerOutput->scalingFactor);
            }

            pInputData->preScaleRatio = preScaleRatio;

            // calculate post Scale Ratio
            if (TRUE == upscaleonStream)
            {
                FLOAT ratio1 = static_cast<FLOAT>(pCropInfo->width) / static_cast<FLOAT>(m_fullOutputWidth);
                FLOAT ratio2 = static_cast<FLOAT>(pCropInfo->height) / static_cast<FLOAT>(m_fullOutputHeight);
                pInputData->postScaleRatio = (ratio1 > ratio2) ? ratio2 : ratio1;
            }

            CAMX_LOG_INFO(CamxLogGroupPProc,
                          "%s: parent %d cropwidth= %d,height= %d,fwidth= %d,fheight= %d,preScaleRatio= %f,postScaleRatio= %f",
                          NodeIdentifierString(),
                          parentNodeID,
                          pCropInfo->width,
                          pCropInfo->height,
                          m_fullOutputWidth,
                          m_fullOutputHeight,
                          pInputData->preScaleRatio,
                          pInputData->postScaleRatio);

        }
        else
        {
            if (TRUE == upscaleonStream)
            {
                FLOAT ratio1 = static_cast<FLOAT>(pCropInfo->width) / static_cast<FLOAT>(m_fullOutputWidth);
                FLOAT ratio2 = static_cast<FLOAT>(pCropInfo->height) / static_cast<FLOAT>(m_fullOutputHeight);
                pInputData->postScaleRatio = (ratio1 > ratio2) ? ratio2 : ratio1;
            }
        }
    }


    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::CheckIsIPERealtime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPENode::CheckIsIPERealtime(
    UINT numPasses)
{
    // if IPE is part of realtime pipeline
    BOOL isRealTime = FALSE;

    switch (m_instanceProperty.processingType)
    {
        case IPEMFNRPrefilter:
        case IPEMFNRBlend:
        case IPEMFNRPostfilter:
        case IPEMFSRPrefilter:
        case IPEMFSRBlend:
        case IPEMFSRPostfilter:
            break;
        default:
        {
            if (TRUE == IsRealTime())
            {
                isRealTime = TRUE;
                if (numPasses == 4)
                {
                    isRealTime = FALSE;
                }
            }
            else if (FALSE == IsScalerOnlyIPE())
            {
                // Preview / video part of offline pipeline , check for processing type so does not fall into MFNR category.
                if (0 != (IPEStabilizationType::IPEStabilizationMCTF & m_instanceProperty.stabilizationType))
                {
                    isRealTime = TRUE;
                }
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "[%s]: has bps node=%d, m_numPasses=%d", NodeIdentifierString(),
                    IsNodeInPipeline(BPS), numPasses);
                // if node does not have BPS in pipeline and number of passess less than 3
                // then this is realtime IPE part of an offline pipeline
                if ((numPasses <= 3) && (FALSE == IsNodeInPipeline(BPS)))
                {
                    isRealTime = TRUE;
                }
            }
            else
            {
                if (IPEProfileIdUpscale == m_instanceProperty.profileId)
                {
                    // Call node API here
                    CAMX_LOG_INFO(CamxLogGroupPProc, "[%s]: upscale profile stream type %d", NodeIdentifierString(),
                                  GetStreamProcessType());
                    isRealTime = (0 != (GetStreamProcessType() & RealtimeStream)) ? TRUE : FALSE;
                }
            }
            break;
        }
    }

    return isRealTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetUBWCStatBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetUBWCStatBuffer(
    CSLBufferInfo* pBuf,
    UINT64         requestId)
{
    CamxResult result = CamxResultSuccess;
    UINT       bufIdx = requestId % MaxUBWCStatsBufferSize;

    if ((NULL != pBuf) &&
        (NULL != m_UBWCStatBufInfo.pUBWCStatsBuffer))
    {
        m_UBWCStatBufInfo.requestId[bufIdx] = requestId;
        Utils::Memcpy(pBuf, m_UBWCStatBufInfo.pUBWCStatsBuffer, sizeof(CSLBufferInfo));
        pBuf->offset = m_UBWCStatBufInfo.offset[bufIdx];
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Fail to memcpy, ptrs should be valid: pBuf: %p, pUBWCStatsBuffer: %p",
                       pBuf, m_UBWCStatBufInfo.pUBWCStatsBuffer);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::ProcessFrameDataCallBack
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::ProcessFrameDataCallBack(
    UINT64                requestId,
    UINT                  portId,
    ChiBufferInfo*        pBufferInfo,
    const ImageFormat*    pImageFormat)
{
#if !defined(LE_CAMERA) // qdmetadata not present
    UbwcStatsBuffer* pUBWCStats;
    UINT             ubwcStatsYBufIndx;
    UINT             ubwcStatsCBufIndx;
    UBWCStats        ubwcStatsMetadata[2];
    CSLBufferInfo*   pBuf;
    UINT             bufIdx = requestId % MaxUBWCStatsBufferSize;

    CAMX_ASSERT(NULL != pBufferInfo);

    ubwcStatsMetadata[0].bDataValid = 0;
    ubwcStatsMetadata[1].bDataValid = 0;
    //  needed because qdmetadata has additions
    ubwcStatsMetadata[0].version =
        // UBWC 2.0
        (UBWCVersion2 == pImageFormat->ubwcVerInfo.version) ? static_cast<enum UBWC_Version>(2) :
        // UBWC 3.0
        (UBWCVersion3 == pImageFormat->ubwcVerInfo.version) ? static_cast<enum UBWC_Version>(3) :
        // UBWC 4.0
        (UBWCVersion4 == pImageFormat->ubwcVerInfo.version) ? static_cast<enum UBWC_Version>(4) :
        // UBWC unused
        static_cast<enum UBWC_Version>(0);

    if ((TRUE                  == IsRealTime())                                &&
        ((IPEOutputPortDisplay == portId) || (IPEOutputPortVideo   == portId)) &&
        (TRUE                  == CamX::IsGrallocBuffer(pBufferInfo))          &&
        (FALSE                 == IsSecureMode()))
    {
        if (requestId == m_UBWCStatBufInfo.requestId[bufIdx])
        {
            ubwcStatsMetadata[0].bDataValid = 1;

            pBuf = m_UBWCStatBufInfo.pUBWCStatsBuffer;
            pUBWCStats = reinterpret_cast<UbwcStatsBuffer*>(
               static_cast<BYTE*>(pBuf->pVirtualAddr) + m_UBWCStatBufInfo.offset[bufIdx]);

            if (IPEOutputPortDisplay == portId)
            {
                ubwcStatsYBufIndx = IPE_UBWC_STATS_DISPLAY_Y;
                ubwcStatsCBufIndx = IPE_UBWC_STATS_DISPLAY_C;
            }
            else
            {
                ubwcStatsYBufIndx = IPE_UBWC_STATS_VIDEO_Y;
                ubwcStatsCBufIndx = IPE_UBWC_STATS_VIDEO_C;
            }

            if (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_32B <
                UINT_MAX - pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_32B)
            {
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile32 =
                    (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_32B +
                     pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_32B);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "nCRStatsTile32 ADD overflow, YBuf: %u, CBuf: %u",
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_32B,
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_32B);
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile32 = UINT_MAX;
            }

            if (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_64B <
                UINT_MAX - pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_64B)
            {
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile64 =
                    (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_64B +
                     pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_64B);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "nCRStatsTile64 ADD overflow, YBuf: %u, CBuf: %u",
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_64B,
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_64B);
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile64 = UINT_MAX;
            }

            if (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_96B <
                UINT_MAX - pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_96B)
            {
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile96 =
                    (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_96B +
                     pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_96B);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "nCRStatsTile96 ADD overflow, YBuf: %u, CBuf: %u",
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_96B,
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_96B);
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile96 = UINT_MAX;
            }

            if (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_128B <
                UINT_MAX - pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_128B)
            {
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile128 =
                    (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_128B +
                     pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_128B);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "nCRStatsTile128 ADD overflow, YBuf: %u, CBuf: %u",
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_128B,
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_128B);
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile128 = UINT_MAX;
            }

            if (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_160B <
                UINT_MAX - pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_160B)
            {
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile160 =
                    (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_160B +
                     pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_160B);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "nCRStatsTile160 ADD overflow, YBuf: %u, CBuf: %u",
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_160B,
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_160B);
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile160 = UINT_MAX;
            }

            if (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_192B <
                UINT_MAX - pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_192B)
            {
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile192 =
                    (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_192B +
                     pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_192B);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "nCRStatsTile192 ADD overflow, YBuf: %u, CBuf: %u",
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_192B,
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_192B);
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile192 = UINT_MAX;
            }

            if (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_256B <
                UINT_MAX - pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_256B)
            {
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile256 =
                    (pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_256B +
                     pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_256B);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "nCRStatsTile256 ADD overflow, YBuf: %u, CBuf: %u",
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].UBWC_COMPRESSED_256B,
                               pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].UBWC_COMPRESSED_256B);
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile256 = UINT_MAX;
            }

#if DUMP_UBWC_STATS_INFO
            /* Weights are set to 1 for now, might change later*/
            const UINT weight32 = 1;
            const UINT weight64 = 1;
            const UINT weight96 = 1;
            const UINT weight128 = 1;
            const UINT weight160 = 1;
            const UINT weight192 = 1;
            const UINT weight256 = 1;

            const auto totalTiles = pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx].totalTiles +
                pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx].totalTiles;
            float frameCR =
                (static_cast<float>(ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile32) / totalTiles * 256 / 32) * weight32 +
                (static_cast<float>(ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile64) / totalTiles * 256 / 64) * weight64 +
                (static_cast<float>(ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile96) / totalTiles * 256 / 96) * weight96 +
                (static_cast<float>(ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile128) / totalTiles * 256 / 128) * weight128 +
                (static_cast<float>(ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile160) / totalTiles * 256 / 160) * weight160 +
                (static_cast<float>(ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile192) / totalTiles * 256 / 192) * weight192 +
                (static_cast<float>(ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile256) / totalTiles * 256 / 256) * weight256;

            const auto pYBuf = &(pUBWCStats->UbwcStatsBuffer[ubwcStatsYBufIndx]);
            const auto yBuftotalTiles = pYBuf->totalTiles;
            float yBufCR =
                (static_cast<float>(pYBuf->UBWC_COMPRESSED_32B) / yBuftotalTiles * 256 / 32) * weight32 +
                (static_cast<float>(pYBuf->UBWC_COMPRESSED_64B) / yBuftotalTiles * 256 / 64) * weight64 +
                (static_cast<float>(pYBuf->UBWC_COMPRESSED_96B) / yBuftotalTiles * 256 / 96) * weight96 +
                (static_cast<float>(pYBuf->UBWC_COMPRESSED_128B) / yBuftotalTiles * 256 / 128) * weight128 +
                (static_cast<float>(pYBuf->UBWC_COMPRESSED_160B) / yBuftotalTiles * 256 / 160) * weight160 +
                (static_cast<float>(pYBuf->UBWC_COMPRESSED_192B) / yBuftotalTiles * 256 / 192) * weight192 +
                (static_cast<float>(pYBuf->UBWC_COMPRESSED_256B) / yBuftotalTiles * 256 / 256) * weight256;

            const auto pCBuf = &(pUBWCStats->UbwcStatsBuffer[ubwcStatsCBufIndx]);
            const auto cBufTotalTiles = pCBuf->totalTiles;
            float cBufCR =
                (static_cast<float>(pCBuf->UBWC_COMPRESSED_32B) / cBufTotalTiles * 256 / 32) * weight32 +
                (static_cast<float>(pCBuf->UBWC_COMPRESSED_64B) / cBufTotalTiles * 256 / 64) * weight64 +
                (static_cast<float>(pCBuf->UBWC_COMPRESSED_96B) / cBufTotalTiles * 256 / 96) * weight96 +
                (static_cast<float>(pCBuf->UBWC_COMPRESSED_128B) / cBufTotalTiles * 256 / 128) * weight128 +
                (static_cast<float>(pCBuf->UBWC_COMPRESSED_160B) / cBufTotalTiles * 256 / 160) * weight160 +
                (static_cast<float>(pCBuf->UBWC_COMPRESSED_192B) / cBufTotalTiles * 256 / 192) * weight192 +
                (static_cast<float>(pCBuf->UBWC_COMPRESSED_256B) / cBufTotalTiles * 256 / 256) * weight256;

            CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                "Update UBWC stats for port %u, buf %u, request %llu\n"
                "UBWC version %d\n"
                "Frame UBWC CR = %f, Y buffer UBWC CR=%f, C buffer UBWC CR=%f\n"
                "Frame: 32B = %u 64B = %u 96B = %u 128B = %u 160B = %u "
                "192B = %u 256B = %u total tiles = %u\n"
                "Y buffer idx[%u]: 32B = %u 64B = %u 96B = %u 128B = %u 160B = %u "
                "192B = %u 256B = %u total tiles = %u\n"
                "C buffer idx[%u]: 32B = %u 64B = %u 96B = %u 128B = %u 160B = %u "
                "192B = %u 256B = %u total tiles = %u",
                portId, bufIdx, requestId,
                ubwcStatsMetadata[0].version,
                frameCR, yBufCR, cBufCR,
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile32,
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile64,
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile96,
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile128,
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile160,
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile192,
                ubwcStatsMetadata[0].ubwc_stats.nCRStatsTile256,
                totalTiles,
                ubwcStatsYBufIndx,
                pYBuf->UBWC_COMPRESSED_32B,
                pYBuf->UBWC_COMPRESSED_64B,
                pYBuf->UBWC_COMPRESSED_96B,
                pYBuf->UBWC_COMPRESSED_128B,
                pYBuf->UBWC_COMPRESSED_160B,
                pYBuf->UBWC_COMPRESSED_192B,
                pYBuf->UBWC_COMPRESSED_256B,
                pYBuf->totalTiles,
                ubwcStatsCBufIndx,
                pCBuf->UBWC_COMPRESSED_32B,
                pCBuf->UBWC_COMPRESSED_64B,
                pCBuf->UBWC_COMPRESSED_96B,
                pCBuf->UBWC_COMPRESSED_128B,
                pCBuf->UBWC_COMPRESSED_160B,
                pCBuf->UBWC_COMPRESSED_192B,
                pCBuf->UBWC_COMPRESSED_256B,
                pCBuf->totalTiles);

#endif // DUMP_UBWC_STATS_INFO
        }
        else
        {
            if (FALSE == GetFlushStatus())
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid UBWC stats buffer: expected request=%lld,"
                    "actual request=%lld",
                    m_UBWCStatBufInfo.requestId[bufIdx],
                    requestId);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupPProc, "Invalid UBWC stats buffer during flush: expected request=%lld,"
                    "actual request=%lld",
                    m_UBWCStatBufInfo.requestId[bufIdx],
                    requestId);
            }
        }

        BufferHandle* phNativeHandle = CamX::GetBufferHandleFromBufferInfo(pBufferInfo);

        if (NULL != phNativeHandle)
        {
            OsUtils::SetGrallocMetaData(*phNativeHandle, SET_UBWC_CR_STATS_INFO, &ubwcStatsMetadata[0]);
        }
    }
#endif // LE_CAMERA
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::CalculateIntermediateSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT IPENode::CalculateIntermediateSize(
    ISPInputData*     pInputData,
    CHIRectangle*     pCropInfo)
{
    m_curIntermediateDimension.ratio  = 1.0f / pInputData->postScaleRatio;
    m_curIntermediateDimension.width  = m_fullOutputWidth;
    m_curIntermediateDimension.height = m_fullOutputHeight;

    if (m_curIntermediateDimension.ratio >= m_maxICAUpscaleLimit)
    {
        m_curIntermediateDimension.ratio  = m_maxICAUpscaleLimit;
        m_curIntermediateDimension.width  = Utils::EvenCeilingUINT32(
            static_cast<UINT32>(m_curIntermediateDimension.ratio * pCropInfo->width));
        m_curIntermediateDimension.height = Utils::EvenCeilingUINT32(
            static_cast<UINT32>(m_curIntermediateDimension.ratio * pCropInfo->height));

        CAMX_LOG_INFO(CamxLogGroupPProc, "IPE:%d intermediate width=%d, height=%d, ratio=%f",
                       InstanceID(),
                       m_curIntermediateDimension.width,
                       m_curIntermediateDimension.height,
                       m_curIntermediateDimension.ratio);
    }

    if (m_curIntermediateDimension.height > static_cast<UINT32>(m_fullOutputHeight))
    {
        m_curIntermediateDimension.height = static_cast<UINT32>(m_fullOutputHeight);
    }
    if (m_curIntermediateDimension.width > static_cast<UINT32>(m_fullOutputWidth))
    {
        m_curIntermediateDimension.width = static_cast<UINT32>(m_fullOutputWidth);
    }

    return m_curIntermediateDimension.ratio;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::DumpConfigIOData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::DumpConfigIOData()
{
    IpeConfigIo*     pConfigIO;
    IpeConfigIoData* pConfigIOData;

    pConfigIO     = reinterpret_cast<IpeConfigIo*>(m_configIOMem.pVirtualAddr);
    pConfigIOData = &pConfigIO->cmdData;

    CAMX_LOG_CONFIG(CamxLogGroupPProc, ":%s:icaupscale %d,topology %d,batchsize %d,muxmode %d,sescure %d, margin w %d, h %d",
                    NodeIdentifierString(),
                    pConfigIOData->icaMaxUpscalingQ16,
                    pConfigIOData->topologyType,
                    pConfigIOData->maxBatchSize,
                    pConfigIOData->muxMode,
                    pConfigIOData->secureMode,
                    pConfigIOData->stabilizationMargins.widthPixels,
                    pConfigIOData->stabilizationMargins.heightLines);

    for (UINT i = 0; i < IPE_IO_IMAGES_MAX; i++)
    {
        if (IMAGE_FORMAT_INVALID != pConfigIOData->images[i].info.format)
        {
            CAMX_LOG_CONFIG(CamxLogGroupPProc, "%s port %d,format %d,width %d,height %d,bayerordr %d,pixelPack %d,yuv422ordr %d"
                            "enableByteSwap %d, ubwcVersion %d, lossy %d, bwlimit %d, lossy thresh0 %d, lossy thresh1 %d,"
                            "argbAlpha %d, ubwcOffsetsVarLossy %d, fromDownscale %d",
                            NodeIdentifierString(), i,
                            pConfigIOData->images[i].info.format,
                            pConfigIOData->images[i].info.dimensions.widthPixels,
                            pConfigIOData->images[i].info.dimensions.heightLines,
                            pConfigIOData->images[i].info.bayerOrder,
                            pConfigIOData->images[i].info.pixelPackingAlignment,
                            pConfigIOData->images[i].info.yuv422Order,
                            pConfigIOData->images[i].info.enableByteSwap,
                            pConfigIOData->images[i].info.ubwcVersion,
                            pConfigIOData->images[i].info.ubwcLossyMode,
                            pConfigIOData->images[i].info.ubwcBwLimit,
                            pConfigIOData->images[i].info.ubwcThreshLossy0,
                            pConfigIOData->images[i].info.ubwcThreshLossy1,
                            pConfigIOData->images[i].info.argbAlpha,
                            pConfigIOData->images[i].info.ubwcOffsetsVarLossy,
                            pConfigIOData->images[i].info.fromDownscale);
            CAMX_LOG_CONFIG(CamxLogGroupPProc, "%s:, plane0 :bufferstride %d, height %d meta stride %d, meta height %d"
                            "plane1 :buffer stride %d, height %d meta stride %d, meta height %d",
                            NodeIdentifierString(),
                            pConfigIOData->images[i].bufferLayout[0].bufferStride,
                            pConfigIOData->images[i].bufferLayout[0].bufferHeight,
                            pConfigIOData->images[i].metadataBufferLayout[0].bufferStride,
                            pConfigIOData->images[i].metadataBufferLayout[0].bufferHeight,
                            pConfigIOData->images[i].bufferLayout[1].bufferStride,
                            pConfigIOData->images[i].bufferLayout[1].bufferHeight,
                            pConfigIOData->images[i].metadataBufferLayout[1].bufferStride,
                            pConfigIOData->images[i].metadataBufferLayout[1].bufferHeight);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::DumpTuningModeData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::DumpTuningModeData()
{
    // print out tuning mode information
    CAMX_LOG_INFO(CamxLogGroupPProc,
                     "IPE:%s "
                     "mode : default %d, sensor %d, usecase %d, feature1 %d feature2 %d, scene %d, effect %d",
                     NodeIdentifierString(),
                     static_cast<UINT32>(m_tuningData.TuningMode[0].subMode.value),
                     static_cast<UINT32>(m_tuningData.TuningMode[1].subMode.value),
                     static_cast<UINT32>(m_tuningData.TuningMode[2].subMode.usecase),
                     static_cast<UINT32>(m_tuningData.TuningMode[3].subMode.feature1),
                     static_cast<UINT32>(m_tuningData.TuningMode[4].subMode.feature2),
                     static_cast<UINT32>(m_tuningData.TuningMode[5].subMode.scene),
                     static_cast<UINT32>(m_tuningData.TuningMode[6].subMode.effect));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::IsSupportedPortConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPENode::IsSupportedPortConfiguration(
    UINT portId)
{
    CamxResult result             = CamxResultSuccess;
    BOOL       supportedConfig    = TRUE;

    const ImageFormat* pImageFormat    = NULL;
    UINT32             width           = 0;
    UINT32             height          = 0;
    UINT               factor          = 1;

    switch (portId)
    {
        case IPEInputPortFull:
        case IPEInputPortFullRef:
            factor = 1;
            break;
        case IPEInputPortDS4:
        case IPEInputPortDS4Ref:
            factor = 4;
            break;
        case IPEInputPortDS16:
        case IPEInputPortDS16Ref:
            factor = 16;
            break;
        case IPEInputPortDS64:
        case IPEInputPortDS64Ref:
            factor = 64;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupPProc, "%s: Invalid port %d", NodeIdentifierString(), portId);
            supportedConfig = FALSE;
            result = CamxResultEInvalidArg;
            break;
    }

    if (CamxResultSuccess == result)
    {
        pImageFormat = GetInputPortImageFormat(InputPortIndex(portId));
        if (NULL == pImageFormat)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "pImageFormat is NULL");
            result = CamxResultEInvalidPointer;
        }
    }

    if (CamxResultSuccess == result)
    {
        width = (0 != m_fullInputWidth) ?
            Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
            (m_fullInputWidth - m_stabilizationMargin.widthPixels), factor) / factor) :
            pImageFormat->width;
        height = (0 != m_fullInputHeight) ? Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
            (m_fullInputHeight - m_stabilizationMargin.heightLines), factor) / factor) :
            pImageFormat->height;

        if ((IPEInputPortFull != portId) && (IPEInputPortFullRef != portId))
        {
            if ((height < ICAMinHeightPixels) || (width < ICAMinWidthPixels))
            {
                supportedConfig = FALSE;
            }
            else
            {
                supportedConfig = TRUE;
            }
        }
        else
        {
            CAMX_ASSERT((height >= ICAMinHeightPixels) && (width >= ICAMinWidthPixels));
        }
    }


    return supportedConfig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateNodeParamsOnPortStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::UpdateNodeParamsOnPortStatus()
{
    // Update number of passes
    m_numPasses = Utils::CountSetBits((GetInputPortDisableMask() & m_inputPortIPEPassesMask)
        ^m_inputPortIPEPassesMask);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    UINT32 finalNumIPEOutputTags = 0;

    if (FALSE == IsSkipPostMetadata())
    {
        finalNumIPEOutputTags = NumIPEMetadataOutputTags;

        for (UINT32 tagIndex = 0; tagIndex < finalNumIPEOutputTags; ++tagIndex)
        {
            pPublistTagList->tagArray[tagIndex] = IPEMetadataOutputTags[tagIndex];
        }
    }

    pPublistTagList->tagCount = finalNumIPEOutputTags;
    CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%u tags will be published", finalNumIPEOutputTags);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::ApplyStatsFOVCCrop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::ApplyStatsFOVCCrop(
    CHIRectangle* pCropInfo,
    UINT          parentNodeId,
    UINT32        fullInputWidth,
    UINT32        fullInputHeight,
    UINT64        requestId)
{
    if (TRUE == m_instanceProperty.enableFOVC)
    {
        // Get FOVC compensation factor from metadata
        FLOAT   FOVCCompensationFactor  = 0.0f;
        UINT    FOVCProp[]              = { PropertyIDFOVCFrameInfo, };
        VOID*   pFOVCData[1]            = { 0 };
        UINT64  pFOVCOffset[1]          = { 0 };

        GetDataList(FOVCProp, pFOVCData, pFOVCOffset, 1);
        FOVCOutput* pFOVCOutput = reinterpret_cast<FOVCOutput*>(pFOVCData[0]);
        if (NULL != pFOVCOutput)
        {
            FOVCCompensationFactor = pFOVCOutput->fieldOfViewCompensationFactor;
        }
        else
        {
            FOVCProp[0] |= InputMetadataSectionMask;
            GetDataList(FOVCProp, pFOVCData, pFOVCOffset, 1);
            pFOVCOutput = reinterpret_cast<FOVCOutput*>(pFOVCData[0]);
            if (NULL != pFOVCOutput)
            {
                FOVCCompensationFactor = pFOVCOutput->fieldOfViewCompensationFactor;
            }
        }

        UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);
        if (0.0f < FOVCCompensationFactor)
        {
            m_prevFOVC = FOVCCompensationFactor;
        }

        if ((BPS != parentNodeId) && (NULL != pCropInfo) && (0.0f < m_prevFOVC))
        {
            FLOAT fovcCropFactor = 1.0f;
            fovcCropFactor       = m_prevFOVC;

            int32_t adjustedWidth  = pCropInfo->width;
            int32_t adjustedHeight = pCropInfo->height;

            // Calculate total change in width or height
            adjustedWidth   -= static_cast<int32_t>(adjustedWidth  * (1 - fovcCropFactor));
            adjustedHeight  -= static_cast<int32_t>(adjustedHeight * (1 - fovcCropFactor));

            // (change in height)/2 is change in top, (change in width)/2 is change in left
            int32_t width   = adjustedWidth  / 2;
            int32_t height  = adjustedHeight / 2;

            pCropInfo->left     += width;
            pCropInfo->top      += height;
            pCropInfo->width    -= width * 2;
            pCropInfo->height   -= height * 2;

            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "ZDBG: After IPE ICA1 Zoom Window: [%d, %d, %d, %d] full %d x %d,"
                             "FOVC crop_factor %f, requestId %llu ",
                             pCropInfo->left, pCropInfo->top, pCropInfo->width, pCropInfo->height,
                             fullInputWidth, fullInputHeight, fovcCropFactor, requestId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::FillLoopBackFrameBufferData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillLoopBackFrameBufferData(
    UINT64                  requestId,
    UINT                    firstValidRequest,
    CmdBuffer*              pFrameProcessCmdBuffer,
    UINT32                  numFramesInBuffer,
    PerRequestActivePorts*  pPerRequestPorts)
{
    CamxResult   result            = CamxResultSuccess;
    ImageBuffer* pPingBuffer       = NULL;
    ImageBuffer* pPongBuffer       = NULL;
    INT32        outputBufferIndex = 0;
    INT32        inputBufferIndex  = 0;
    UINT64       requestDelta      = (requestId - m_currentrequestID);

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "pipeline %s IPE:%d Input for setting loop-back"
                     "-requestDelta %llu, numFramesInBuffer %d, firstValidRequest %d",
                     NodeIdentifierString(),
                     InstanceID(),
                     requestDelta,
                     numFramesInBuffer,
                     firstValidRequest);

    // if IPE Reference output dumps are not enabled m_referenceBufferCount remains 2;
    // then output and input buffer indices will circularly switch across {0, 1}
    // if IPE Reference output dumps are ENABLED, m_referenceBufferCount is made 32;
    // considering HFR Max Batch size 16, for 480fps video @30fps preview then
    // output and input buffer indices will circularly switch across {0,1... ,31}
    outputBufferIndex = GetLoopBackBufferBatchIndex(requestId, numFramesInBuffer);
    inputBufferIndex  = ((outputBufferIndex - 1) < 0) ? (m_referenceBufferCount - 1) : (outputBufferIndex - 1);

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "refbufcnt: %d requestId: %llu o/pidx: %d I/pIdx: %d ",
        m_referenceBufferCount, requestId, outputBufferIndex, inputBufferIndex);

    for (UINT pass = 0; pass < m_numPasses; pass++)
    {
        // Patch buffers only if the port is enabled. Could be disabled by dimension limitations
        if (TRUE == m_loopBackBufferParams[pass].portEnable)
        {
            // Non batch mode case ping/pong between each request.
            if (1 >= numFramesInBuffer)
            {
                pPingBuffer = m_loopBackBufferParams[pass].pReferenceBuffer[outputBufferIndex];
                pPongBuffer = m_loopBackBufferParams[pass].pReferenceBuffer[inputBufferIndex];

                if ((NULL == pPingBuffer) || (NULL == pPongBuffer))
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid reference I/O buffers %p : %p",
                        pPingBuffer, pPongBuffer);
                    result = CamxResultEInvalidPointer;

                    break;
                }

                result = FillFrameBufferData(pFrameProcessCmdBuffer,
                                             pPingBuffer,
                                             0,
                                             0,
                                             m_loopBackBufferParams[pass].outputPortID);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fill o/p ref buffer frame data result = %d", result);
                    break;
                }

                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "node %s reporting o/p reference"
                                 "port %d, request %llu, buffer %p",
                                 NodeIdentifierString(),
                                 m_loopBackBufferParams[pass].outputPortID,
                                 requestId,
                                 pPingBuffer->GetPlaneVirtualAddr(0, 0));

                // reset reference is true for first request to a node and
                // also if difference between requests in greater than 1.
                if (FALSE == IsSkipReferenceInput(requestDelta, firstValidRequest))
                {
                    result = FillFrameBufferData(pFrameProcessCmdBuffer,
                                                 pPongBuffer,
                                                 0,
                                                 0,
                                                 m_loopBackBufferParams[pass].inputPortID);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fill i/p ref buffer frame data result = %d", result);
                        break;
                    }

                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "node %s reporting i/p reference"
                                     "port %d,request %llu,buffer %p",
                                     NodeIdentifierString(),
                                     m_loopBackBufferParams[pass].inputPortID,
                                     requestId,
                                     pPongBuffer->GetPlaneVirtualAddr(0, 0));
                }
            }
            else
            {
                /* In order to accomodate HFR dump frames we use certain slots of buffer indices for
                 * m_loopBackBufferParams[pass].pReferenceBuffer[] of size "numFramesInBuffer" or batch size.
                 * Every request gets an unique slot of buffer index and those slots are rotated circularly.
                 * For example: if Batch size is 8 then we'll have 4 slots of buffer indices mapped to each
                 * requestID.
                 * {
                 *     RequestID: 1 ==> slot1 (8  to 15) ,
                 *     RequestID: 2 ==> slot2 (16 to 23) ,
                 *     RequestID: 3 ==> slot3 (24 to 31) ,
                 *     RequestID: 4 ==> slot0 (0  to 7)  ,
                 *     .......
                 * }
                 * */
                INT32 bufferBatchOffset = GetLoopBackBufferBatchIndex(requestId, numFramesInBuffer);
                INT32 maxBufferIndex    =
                    ((m_referenceBufferCount > numFramesInBuffer) ? (numFramesInBuffer - 1) : (m_referenceBufferCount - 1));
                for (UINT batchedFrameIndex = 0; batchedFrameIndex < numFramesInBuffer; batchedFrameIndex++)
                {
                    if (m_referenceBufferCount >= numFramesInBuffer)
                    {
                        outputBufferIndex = bufferBatchOffset + batchedFrameIndex;
                        inputBufferIndex  = bufferBatchOffset +
                            ((batchedFrameIndex == 0U) ? maxBufferIndex : (batchedFrameIndex - 1U));
                    }
                    else
                    {
                        outputBufferIndex = (batchedFrameIndex % m_referenceBufferCount);
                        inputBufferIndex  = ((outputBufferIndex - 1) < 0) ?
                            m_referenceBufferCount - 1 : outputBufferIndex - 1;
                    }

                    pPingBuffer = m_loopBackBufferParams[pass].pReferenceBuffer[outputBufferIndex];
                    pPongBuffer = m_loopBackBufferParams[pass].pReferenceBuffer[inputBufferIndex];

                    if ((NULL == pPingBuffer) || (NULL == pPongBuffer))
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid reference I/O buffers %p : %p",
                            pPingBuffer, pPongBuffer);
                        result = CamxResultEInvalidPointer;

                        break;
                    }

                    result = FillFrameBufferData(pFrameProcessCmdBuffer,
                                                 pPingBuffer,
                                                 batchedFrameIndex,
                                                 0,
                                                 m_loopBackBufferParams[pass].outputPortID);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fill output buffer frame data result = %d", result);
                        break;
                    }

                    CAMX_LOG_INFO(CamxLogGroupPProc, "node %s reporting o/p reference-"
                                  "batchedFrameIndex %d port %d, buffer %p, wxh %dx%d, request %llu",
                                  NodeIdentifierString(),
                                  batchedFrameIndex,
                                  m_loopBackBufferParams[pass].outputPortID,
                                  pPingBuffer->GetPlaneVirtualAddr(0, 0),
                                  (NULL != pPingBuffer->GetFormat()) ? pPingBuffer->GetFormat()->width : 0,
                                  (NULL != pPingBuffer->GetFormat()) ? pPingBuffer->GetFormat()->height : 0,
                                  requestId);

                    if (TRUE == IsSkipReferenceInput(requestDelta, firstValidRequest))
                    {
                        UINT portId = GetInputPortIdFromInputReferencePortId(m_loopBackBufferParams[pass].inputPortID);
                        ImageBuffer* pInputImageBuffer = GetInputPortImageBuffer(portId, pPerRequestPorts);

                        if ((NULL != pInputImageBuffer) && (NULL != pInputImageBuffer->GetFormat())      &&
                            (NULL != pPongBuffer->GetFormat())                                           &&
                            (pInputImageBuffer->GetFormat()->width  == pPongBuffer->GetFormat()->width)  &&
                            (pInputImageBuffer->GetFormat()->height == pPongBuffer->GetFormat()->height) &&
                            (0x1 == GetStaticSettings()->isFirstFrameLoopbackInputImageEnabled))
                        {
                            if (0 == batchedFrameIndex)
                            {
                                CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                                                 "node %s IsSkipReferenceInput inputBuff wxh %dx%d "
                                                 "refBuf wxh %dx%d for request %llu, batchedFrameIndex %d, pass %d"
                                                 " InputPortId %d, refPortId %d, featureenable %d",
                                                 NodeIdentifierString(),
                                                 pInputImageBuffer->GetFormat()->width,
                                                 pInputImageBuffer->GetFormat()->height,
                                                 pPongBuffer->GetFormat()->width,
                                                 pPongBuffer->GetFormat()->height,
                                                 requestId,
                                                 batchedFrameIndex,
                                                 pass,
                                                 portId,
                                                 m_loopBackBufferParams[pass].inputPortID,
                                                 GetStaticSettings()->isFirstFrameLoopbackInputImageEnabled);

                                pPongBuffer = pInputImageBuffer;
                            }
                        }
                        else
                        {
                            continue;
                        }
                    }

                    {
                        result = FillFrameBufferData(pFrameProcessCmdBuffer,
                                                     pPongBuffer,
                                                     batchedFrameIndex,
                                                     0,
                                                     m_loopBackBufferParams[pass].inputPortID);
                    }

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fill input buffer frame data result = %d", result);
                        break;
                    }

                    CAMX_LOG_INFO(CamxLogGroupPProc, "node %s reporting i/p reference"
                                  "batchedFrameIndex %d port %d, buffer %p, wxh %dx%d, request %llu",
                                  NodeIdentifierString(),
                                  batchedFrameIndex,
                                  m_loopBackBufferParams[pass].inputPortID,
                                  pPongBuffer->GetPlaneVirtualAddr(0, 0),
                                  (NULL != pPongBuffer->GetFormat()) ? pPongBuffer->GetFormat()->width : 0,
                                  (NULL != pPongBuffer->GetFormat()) ? pPongBuffer->GetFormat()->height : 0,
                                  requestId);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::DeleteLoopBackBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::DeleteLoopBackBufferManagers()
{
    CamxResult result         = CamxResultSuccess;
    UINT       referenceCount = 0;
    for (UINT pass = 0; pass < PASS_NAME_MAX; pass++)
    {
        if (TRUE == m_loopBackBufferParams[pass].portEnable)
        {
            for (UINT bufferIndex = 0; bufferIndex < m_referenceBufferCount; bufferIndex++)
            {
                referenceCount =
                    m_loopBackBufferParams[pass].pReferenceBuffer[bufferIndex]->ReleaseBufferManagerImageReference();
                if (0 == referenceCount)
                {
                    CAMX_ASSERT(TRUE == m_loopBackBufferParams[pass].pReferenceBuffer[bufferIndex]->HasBackingBuffer());
                    m_loopBackBufferParams[pass].pReferenceBuffer[bufferIndex]->Release(FALSE);
                    CAMX_ASSERT(FALSE == m_loopBackBufferParams[pass].pReferenceBuffer[bufferIndex]->HasBackingBuffer());
                    m_loopBackBufferParams[pass].pReferenceBuffer[bufferIndex] = NULL;
                }
            }

            if (NULL != m_loopBackBufferParams[pass].pReferenceBufferManager)
            {
                m_loopBackBufferParams[pass].pReferenceBufferManager->Deactivate(FALSE);
                m_loopBackBufferParams[pass].pReferenceBufferManager->Destroy();
                m_loopBackBufferParams[pass].pReferenceBufferManager = NULL;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::DeleteDS4LENRBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::DeleteDS4LENRBuffers()
{
    CamxResult result = CamxResultSuccess;
    if ((FALSE == m_realTimeIPE)                     &&
        (FALSE == m_capability.disableLENRDS4Buffer) &&
        (TRUE  == m_capability.LENRSupport))
    {
        for (UINT count = 0; count < m_numLENRScratchBuffers; count++)
        {
            if (NULL != m_pLENRScratchMemoryBuffer[count])
            {
                if (CSLInvalidHandle != m_pLENRScratchMemoryBuffer[count]->hHandle)
                {
                    CSLReleaseBuffer(m_pLENRScratchMemoryBuffer[count]->hHandle);
                }
                CAMX_FREE(m_pLENRScratchMemoryBuffer[count]);
                m_pLENRScratchMemoryBuffer[count] = NULL;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::AllocateScratchBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::AllocateScratchBuffers()
{

    CamxResult  result                  = CamxResultSuccess;
    UINT32      memFlags                = CSLMemFlagHw;
    ImageFormat         iFormat         = { 0 };
    FormatParamInfo     formatParamInfo = { 0 };

    iFormat.format                      = CamX::Format::Blob;
    iFormat.width                       = m_firmwareScratchMemSize;
    iFormat.height                      = 1;
    iFormat.alignment                   = 1;
    iFormat.colorSpace                  = CamX::ColorSpace::Unknown;
    iFormat.rotation                    = CamX::Rotation::CW0Degrees;
    formatParamInfo.yuvPlaneAlign       = GetStaticSettings()->yuvPlaneAlignment;

    ImageFormatUtils::InitializeFormatParams(&iFormat, &formatParamInfo);

    m_numScratchBuffers = MaxScratchBuffer;

    for (UINT count = 0; count < m_numScratchBuffers; count++)
    {
        m_pScratchMemoryBuffer[count] = static_cast<CSLBufferInfo*>(CAMX_CALLOC(sizeof(CSLBufferInfo)));

        if (NULL != m_pScratchMemoryBuffer[count])
        {
            if (TRUE == IsSecureMode())
            {
                memFlags = (CSLMemFlagProtected | CSLMemFlagHw);
            }

            result = CSLAlloc(NodeIdentifierString(),
                m_pScratchMemoryBuffer[count],
                m_firmwareScratchMemSize,
                CamxCommandBufferAlignmentInBytes,
                memFlags,
                &DeviceIndices()[0],
                1);

            if ((CamxResultSuccess != result) ||
                (CSLInvalidHandle == m_pScratchMemoryBuffer[count]->hHandle))
            {
                result = CamxResultENoMemory;
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to Alloc scratch %d, result %d, vaddr %p, handle %x",
                    count, result, m_pScratchMemoryBuffer[count]->hHandle,
                    m_pScratchMemoryBuffer[count]->pVirtualAddr);
                break;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                    "CSLAlloc returned m_pScratchMemoryBuffer[%d].fd=%d",
                    count,
                    m_pScratchMemoryBuffer[count]->fd);

                // uncomment after we have routing for scratch buffer size and also moving the call to SetupDeviceResource.
                // result = SetConfigIOData(pConfigIOData,
                //     &iFormat,
                //     IPE_INPUT_OUTPUT_SCRATCHBUFFER,
                //     "Scratch Buffer Output");
            }
        }
        else
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::AllocateDS4LENRBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::AllocateDS4LENRBuffers()
{
    UINT32              memFlags        = (CSLMemFlagUMDAccess | CSLMemFlagHw);
    CamxResult          result          = CamxResultSuccess;
    FormatParamInfo     formatParamInfo = { 0 };
    ImageFormat         iFormat         = { 0 };
    IpeConfigIo*        pConfigIO       = reinterpret_cast<IpeConfigIo*>(m_configIOMem.pVirtualAddr);
    IpeConfigIoData*    pConfigIOData   = &pConfigIO->cmdData;

    if ((FALSE == m_realTimeIPE)                     &&
        (TRUE  == m_capability.LENRSupport)          &&
        (FALSE == m_capability.disableLENRDS4Buffer) &&
        (1 < m_numPasses)                            &&
        (IPEProfileId::IPEProfileIdDefault == m_instanceProperty.profileId))
    {
        if ((TRUE == IsLoopBackPortEnabled()))
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "node %s LENR scratch buffer allocation for invalid config"
                "profile %d, processingType %d, stabilizationtype %d, loopbackport %d",
                NodeIdentifierString(), m_instanceProperty.profileId, m_instanceProperty.processingType,
                m_instanceProperty.stabilizationType, IsLoopBackPortEnabled());
            result = CamxResultEUnsupported;
        }

        if (CamxResultSuccess == result)
        {
            m_numLENRScratchBuffers = MaxScratchBuffer;
            iFormat.format          = CamX::Format::PD10;
            iFormat.width           = Utils::EvenCeilingUINT32(Utils::AlignGeneric32(m_fullInputWidth, 4) / 4);
            iFormat.height          = Utils::EvenCeilingUINT32(Utils::AlignGeneric32(m_fullInputHeight, 4) / 4);
            // Minimum pass dimension not met
            if ((iFormat.width < ICAMinWidthPixels) ||
                (iFormat.height < ICAMinHeightPixels))
            {
                CAMX_LOG_INFO(CamxLogGroupPProc, "node %s, DS4 LENR, dimension w %d, h %d < than 30 * 26",
                              NodeIdentifierString(),
                              iFormat.width,
                              iFormat.height);
                result = CamxResultEUnsupported;
            }
        }

        if (CamxResultSuccess == result)
        {
            iFormat.alignment               = 1;
            iFormat.colorSpace              = CamX::ColorSpace::Unknown;
            iFormat.rotation                = CamX::Rotation::CW0Degrees;
            formatParamInfo.yuvPlaneAlign   = GetStaticSettings()->yuvPlaneAlignment;

            ImageFormatUtils::InitializeFormatParams(&iFormat, &formatParamInfo);

            for (UINT count = 0; count < m_numLENRScratchBuffers; count++)
            {
                m_pLENRScratchMemoryBuffer[count] = static_cast<CSLBufferInfo*>(CAMX_CALLOC(sizeof(CSLBufferInfo)));

                if (NULL != m_pLENRScratchMemoryBuffer[count])
                {
                    if (TRUE == IsSecureMode())
                    {
                        memFlags = (CSLMemFlagProtected | CSLMemFlagHw);
                    }

                    result = CSLAlloc(NodeIdentifierString(),
                                      m_pLENRScratchMemoryBuffer[count],
                                      ImageFormatUtils::GetTotalSize(&iFormat),
                                      CamxCommandBufferAlignmentInBytes,
                                      memFlags,
                                      &DeviceIndices()[0],
                                      1);

                    if ((CamxResultSuccess != result)                                    ||
                        (CSLInvalidHandle == m_pLENRScratchMemoryBuffer[count]->hHandle) ||
                        (NULL == m_pLENRScratchMemoryBuffer[count]->pVirtualAddr))
                    {
                        result = CamxResultENoMemory;
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to Alloc scratch %d, result %d, vaddr %p, handle %x",
                                       count, result, m_pLENRScratchMemoryBuffer[count]->hHandle,
                                       m_pLENRScratchMemoryBuffer[count]->pVirtualAddr);
                        break;
                    }
                    else
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupPProc,
                                         "CSLAlloc returned m_pLENRScratchMemoryBuffer[%d].fd=%d",
                                         count,
                                         m_pLENRScratchMemoryBuffer[count]->fd);
                        result = SetConfigIOData(pConfigIOData,
                                                 &iFormat,
                                                 IPE_OUTPUT_IMAGE_DS4_REF,
                                                 "ReferenceOutput");
                    }
                }
                else
                {
                    result = CamxResultENoMemory;
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::PatchDS4LENRBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::PatchDS4LENRBuffers(
    UINT64                   requestId,
    CmdBuffer*               pFrameProcessCmdBuffer,
    IpeIQSettings*           pIPEIQsettings)
{
    CamxResult result = CamxResultSuccess;

    if ((TRUE  == pIPEIQsettings->lenrParameters.moduleCfg.EN)              &&
        (TRUE  == m_capability.LENRSupport)                                 &&
        (IPEProfileId::IPEProfileIdDefault == m_instanceProperty.profileId) &&
        (FALSE == m_capability.disableLENRDS4Buffer))
    {
        if ((TRUE == m_realTimeIPE)           ||
            (TRUE == IsLoopBackPortEnabled()) ||
            (1 >= m_numPasses))
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "node %s trying to patch LENR buffer in invalid config"
                "profile %d, processingType %d, stabilizationtype %d, loopbackport %d, LENR supported %d",
                NodeIdentifierString(), m_instanceProperty.profileId, m_instanceProperty.processingType,
                m_instanceProperty.stabilizationType, IsLoopBackPortEnabled(), m_capability.disableLENRDS4Buffer);
            result = CamxResultEUnsupported;
        }

        if (CamxResultSuccess == result)
        {
            result = pFrameProcessCmdBuffer->AddNestedBufferInfo(
                s_frameBufferOffset[0][IPE_OUTPUT_IMAGE_DS4_REF].bufferPtr[0],
                m_pLENRScratchMemoryBuffer[(requestId % m_numLENRScratchBuffers)]->hHandle,
                0);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::PatchScratchBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::PatchScratchBuffers(
    UINT64                   requestId,
    CmdBuffer*               pFrameProcessCmdBuffer)
{
    CamxResult result = CamxResultSuccess;

    // 1 plane. width is size, height is 1

    result = pFrameProcessCmdBuffer->AddNestedBufferInfo(
        s_frameBufferOffset[0][IPE_INPUT_OUTPUT_SCRATCHBUFFER].bufferPtr[0],
        m_pScratchMemoryBuffer[(requestId % m_numScratchBuffers)]->hHandle,
        0);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "paTch Scratch buffer failed %d", result);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::CreateLoopBackBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::CreateLoopBackBufferManagers()
{
    CamxResult result                = CamxResultSuccess;
    UINT32     inputReferenceOffset  = IPEInputPortFullRef;
    UINT32     outputReferenceOffset = IPEOutputPortFullRef;
    UINT32     width                 = m_fullInputWidth;
    UINT32     height                = m_fullInputHeight;

    if ((0 != m_stabilizationMargin.widthPixels) && (0 != m_stabilizationMargin.heightLines))
    {
        width  = Utils::AlignGeneric32((m_fullInputWidth - m_stabilizationMargin.widthPixels), 32);
        width  = Utils::MinUINT32(m_fullInputWidth, width);

        height = Utils::AlignGeneric32((m_fullInputHeight - m_stabilizationMargin.heightLines), 32);
        height = Utils::MinUINT32(m_fullInputHeight, height);
    }



    BufferManagerCreateData createData          = {};
    FormatParamInfo         formatParamInfo     = { 0 };
    IpeConfigIo*            pConfigIO           = reinterpret_cast<IpeConfigIo*>(m_configIOMem.pVirtualAddr);
    IpeConfigIoData*        pConfigIOData       = &pConfigIO->cmdData;
    const CamX::StaticSettings* pStaticSettings = CamX::HwEnvironment::GetInstance()->GetStaticSettings();
    CamX::Format camxFormat                     = Format::UBWCTP10;


    // Set Full ref port as per the override setting. (Default UBWCTP10)
    IsvalidRefFormat(pStaticSettings->refoutputFormatType, camxFormat);

    // Initialize buffer manager name for each passess
    OsUtils::SNPrintF(m_loopBackBufferParams[0].bufferManagerName, sizeof(m_loopBackBufferParams[0].bufferManagerName),
        "%s_%s%d_OutputPortId_FullRef", GetPipelineName(), Name(), InstanceID());

    OsUtils::SNPrintF(m_loopBackBufferParams[1].bufferManagerName, sizeof(m_loopBackBufferParams[1].bufferManagerName),
        "%s_%s%d_OutputPortId_Ds4Ref", GetPipelineName(), Name(), InstanceID());

    OsUtils::SNPrintF(m_loopBackBufferParams[2].bufferManagerName, sizeof(m_loopBackBufferParams[2].bufferManagerName),
        "%s_%s%d_OutputPortId_DS16Ref", GetPipelineName(), Name(), InstanceID());

    // update each pass specific parameters
    // preview/ video only 3 passess.
    for (UINT pass = 0; (pass < m_numPasses) && (m_numPasses < PASS_NAME_MAX); pass++)
    {
        //  Intialize loop back portIDs
        m_loopBackBufferParams[pass].outputPortID = static_cast<IPE_IO_IMAGES>(pass + IPE_OUTPUT_IMAGE_FULL_REF);
        m_loopBackBufferParams[pass].inputPortID  = static_cast<IPE_IO_IMAGES>(pass + IPE_INPUT_IMAGE_FULL_REF);
        UINT factor = static_cast<UINT>(pow(4, pass));

        if (IPEOutputPortFullRef == m_loopBackBufferParams[pass].outputPortID)
        {
            if (TRUE == ImageFormatUtils::IsUBWC(camxFormat))
            {
                formatParamInfo.UBWCVersionConsumerUsage   = 0xFFFF;
                formatParamInfo.UBWCVersionProducerUsage   = 0xFFFF;
                formatParamInfo.LossyUBWCProducerUsage     = UBWCLossy;
                formatParamInfo.LossyUBWCConsumerUsage     = UBWCLossy;
                m_loopBackBufferParams[pass].format.format = camxFormat;
                m_loopBackBufferParams[pass].format.width  = width;
                m_loopBackBufferParams[pass].format.height = height;
                SetProducerFormatParameters(
                    &m_loopBackBufferParams[pass].format,
                    &formatParamInfo);
                SetConsumerFormatParameters(
                    &m_loopBackBufferParams[pass].format,
                    &formatParamInfo);
            }
            else
            {
                m_loopBackBufferParams[pass].format.format = camxFormat;
                m_loopBackBufferParams[pass].format.width  = width;
                m_loopBackBufferParams[pass].format.height = height;
                CAMX_LOG_INFO(CamxLogGroupPProc, "Set format %d for full ref port of node %s, with dimension w %d, h %d",
                    m_loopBackBufferParams[pass].format.format, NodeIdentifierString(),
                    m_loopBackBufferParams[pass].format.width, m_loopBackBufferParams[pass].format.height);
            }
        }
        else
        {
            m_loopBackBufferParams[pass].format.format = CamX::Format::PD10;
            m_loopBackBufferParams[pass].format.width  =
                Utils::EvenCeilingUINT32(Utils::AlignGeneric32(width, factor) / factor);
            m_loopBackBufferParams[pass].format.height =
                Utils::EvenCeilingUINT32(Utils::AlignGeneric32(height, factor) / factor);
        }

        // Minimum pass dimension not met
        if ((Utils::EvenCeilingUINT32(Utils::AlignGeneric32(width, factor) / factor)
                < ICAMinWidthPixels) ||
            (Utils::EvenCeilingUINT32(Utils::AlignGeneric32(height, factor) / factor)
                < ICAMinHeightPixels))
        {
            CAMX_LOG_INFO(CamxLogGroupPProc, "node %s, pass %d, dim w %d, h %d,  < than 30 * 26",
                          NodeIdentifierString(),
                          pass,
                          m_loopBackBufferParams[pass].format.width,
                          m_loopBackBufferParams[pass].format.height);
            break;
        }

        m_loopBackBufferParams[pass].format.alignment  = 1;
        m_loopBackBufferParams[pass].format.colorSpace = CamX::ColorSpace::Unknown;
        m_loopBackBufferParams[pass].format.rotation   = CamX::Rotation::CW0Degrees;
        formatParamInfo.yuvPlaneAlign                  = GetStaticSettings()->yuvPlaneAlignment;

        ImageFormatUtils::InitializeFormatParams(&m_loopBackBufferParams[pass].format, &formatParamInfo);

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "node %s, pass %d, port %d-%d, w %d, h %d,format %d,buffersize %d",
                         NodeIdentifierString(),
                         pass,
                         m_loopBackBufferParams[pass].inputPortID,
                         m_loopBackBufferParams[pass].outputPortID,
                         m_loopBackBufferParams[pass].format.width,
                         m_loopBackBufferParams[pass].format.height,
                         m_loopBackBufferParams[pass].format.format,
                         ImageFormatUtils::GetTotalSize(&m_loopBackBufferParams[pass].format));

        createData.bufferProperties.immediateAllocImageBuffers = m_referenceBufferCount;
        createData.bufferProperties.maxImageBuffers            = m_referenceBufferCount;
        createData.bufferProperties.consumerFlags              = 0;
        createData.bufferProperties.producerFlags              = 0;
        createData.bufferProperties.grallocFormat              = 0;
        createData.bufferProperties.bufferHeap                 = CSLBufferHeapIon;
        createData.bufferProperties.memFlags                   = CSLMemFlagHw;
        createData.bufferProperties.imageFormat                = m_loopBackBufferParams[pass].format;

        if (TRUE == IsSecureMode())
        {
            createData.bufferProperties.memFlags |= CSLMemFlagProtected;
        }
        else
        {
            createData.bufferProperties.memFlags |= CSLMemFlagUMDAccess;
        }

        createData.bNeedDedicatedBuffers      = TRUE;
        createData.bDisableSelfShrinking      = TRUE;
        createData.deviceIndices[0]           = m_deviceIndex;
        createData.deviceCount                = 1;
        createData.allocateBufferMemory       = TRUE;
        createData.bEnableLateBinding         = 0;
        createData.maxBufferCount             = m_referenceBufferCount;
        createData.immediateAllocBufferCount  = m_referenceBufferCount;
        createData.bufferManagerType          = BufferManagerType::CamxBufferManager;
        createData.linkProperties.pNode       = this;
        createData.numBatchedFrames           = 1;

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "node %s, pass %d, buffer manager name:%s, allocate buffer:%d",
                         NodeIdentifierString(),
                         pass,
                         m_loopBackBufferParams[pass].bufferManagerName,
                         createData.allocateBufferMemory);

        result = Node::CreateImageBufferManager(m_loopBackBufferParams[pass].bufferManagerName, &createData,
                                          &m_loopBackBufferParams[pass].pReferenceBufferManager);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "node %s, pass %d, Create buffer manager failed %s",
                           NodeIdentifierString(),
                           pass,
                           m_loopBackBufferParams[pass].bufferManagerName);

            break;
        }
        else
        {
            result = m_loopBackBufferParams[pass].pReferenceBufferManager->Activate();
            if (CamxResultSuccess == result)
            {
                for (UINT bufferIndex = 0; bufferIndex < m_referenceBufferCount; bufferIndex++)
                {
                    m_loopBackBufferParams[pass].pReferenceBuffer[bufferIndex] =
                        m_loopBackBufferParams[pass].pReferenceBufferManager->GetImageBuffer();

                    if (NULL == m_loopBackBufferParams[pass].pReferenceBuffer[bufferIndex])
                    {
                        CAMX_LOG_ERROR(CamxLogGroupPProc, "Node %s, pass %d, buffer not available failed %s",
                                       NodeIdentifierString(),
                                       pass,
                                       m_loopBackBufferParams[pass].bufferManagerName);
                        result = CamxResultENoMemory;
                        break;
                    }
                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "node %s, pass %d,"
                                     "get buffer success %s, bufferindex %d , buffer %p, vaddr %p",
                                     NodeIdentifierString(),
                                     pass,
                                     m_loopBackBufferParams[pass].bufferManagerName,
                                     bufferIndex,
                                     m_loopBackBufferParams[pass].pReferenceBuffer[bufferIndex],
                                     m_loopBackBufferParams[pass].pReferenceBuffer[bufferIndex]->GetPlaneVirtualAddr(0, 0));
                }
            }
            if (CamxResultSuccess != result)
            {
                break;
            }
            else
            {
                m_loopBackBufferParams[pass].portEnable = TRUE;
                m_numOutputRefPorts++;
                result = SetConfigIOData(pConfigIOData,
                                         &m_loopBackBufferParams[pass].format,
                                         m_loopBackBufferParams[pass].outputPortID,
                                         "ReferenceOutput");

                if (CamxResultSuccess == result)
                {
                    result = SetConfigIOData(pConfigIOData,
                                             &m_loopBackBufferParams[pass].format,
                                             m_loopBackBufferParams[pass].inputPortID,
                                             "ReferenceInput");
                }
                else
                {
                    break;
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::UpdateTuningModeData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::UpdateTuningModeData(
    ChiTuningModeParameter* pTuningModeData,
    ISPInputData*           pModuleInput)
{
    CamxResult result = CamxResultSuccess;
    if ((NULL != pModuleInput) && (NULL != pTuningModeData))
    {
        if (m_pPreTuningDataManager == pModuleInput->pTuningDataManager)
        {
            pModuleInput->tuningModeChanged = ISPIQModule::IsTuningModeDataChanged(
                pTuningModeData,
                &m_tuningData);
        }
        else
        {
            pModuleInput->tuningModeChanged = TRUE;
            m_pPreTuningDataManager = pModuleInput->pTuningDataManager;
        }

        // if camera ID changed, it should set tuningModeChanged TRUE to trigger all IQ
        // module to update tuning parameters
        if (m_camIdChanged == TRUE)
        {
            pModuleInput->tuningModeChanged = TRUE;
        }
        // Needed to have different tuning data for different instances of a node within same pipeline
        // Also, cache tuning mode selector data for comparison for next frame, to help
        // optimize tuning data (tree) search in the IQ modules.
        // And, force update the tuning mode if camera id is updated, IPE node in SAT preview offline pipeline,
        // the active camera will switch based on the zoom level, we also need to update tuning data even
        // tuning mode not changed.
        if (TRUE == pModuleInput->tuningModeChanged)
        {
            Utils::Memcpy(&m_tuningData, pTuningModeData, sizeof(ChiTuningModeParameter));
            // remove this processing type checking target and HDR vendor tag
            if (IPEProcessingType::IPEProcessingPreview == m_instanceProperty.processingType)
            {
                // This node instance is preview instance.. Overwrite the tuning param to preview
                m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase =
                    ChiModeUsecaseSubModeType::Preview;
            }
            // Validate the usecase mode
            if (TRUE == m_realTimeIPE)
            {
                // Realtime stream should be  always enabled with  either preview or video tuning. validate and correct if wrong
                if ((ChiModeUsecaseSubModeType::Preview !=
                    m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase) &&
                    (ChiModeUsecaseSubModeType::Video !=
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase))
                {
                    CAMX_LOG_WARN(CamxLogGroupPProc, "Usecase Tuning data input is  %d for an realtime stream",
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase);
                    m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase =
                        ChiModeUsecaseSubModeType::Preview;
                }
            }
            else
            {
                if ((ChiModeUsecaseSubModeType::ZSL !=
                    m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase) &&
                    (ChiModeUsecaseSubModeType::Snapshot !=
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase) &&
                        (ChiModeUsecaseSubModeType::Liveshot !=
                            m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase))
                {
                    CAMX_LOG_WARN(CamxLogGroupPProc, "Usecase Tuning data input is  %d for an offline stream",
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase);
                    m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Usecase)].subMode.usecase =
                        ChiModeUsecaseSubModeType::Snapshot;
                }
            }

            // Validate Feature
            if (TRUE == IsMFProcessingType())
            {
                if (TRUE == IsPostfilterWithDefault())
                {
                    if (ChiModeFeature2SubModeType::MFNRPostFilter !=
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "feature not valid for MFNR postfilter default ipe %d",
                            m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2);
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2 =
                            ChiModeFeature2SubModeType::MFNRPostFilter;
                    }
                }
                else if (TRUE == IsPostfilterWithNPS())
                {
                    if (ChiModeFeature2SubModeType::MFNRBlend !=
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2)
                    {
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2 =
                            ChiModeFeature2SubModeType::MFNRBlend;
                        CAMX_LOG_WARN(CamxLogGroupPProc, " Upadte feature %d for processing type %d, profile %d",
                            m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2,
                            m_instanceProperty.processingType, m_instanceProperty.profileId);
                    }
                }
                else
                {
                    if (ChiModeFeature2SubModeType::MFNRBlend !=
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2)
                    {
                        CAMX_LOG_WARN(CamxLogGroupPProc, "feature %d not valid  processing type %d, profile %d",
                            m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2,
                            m_instanceProperty.processingType, m_instanceProperty.profileId);
                        m_tuningData.TuningMode[static_cast<UINT32>(ModeType::Feature2)].subMode.feature2 =
                            ChiModeFeature2SubModeType::MFNRBlend;
                    }
                }
            }
        }

        // Now refer to the updated tuning mode selector data
        pModuleInput->pTuningData = &m_tuningData;

        DumpTuningModeData();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, " Invalid tuning mode data %p, or module input %d",
            pTuningModeData, pModuleInput);
        result = CamxResultEInvalidArg;
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::CreateDownscaleBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::CreateDownscaleBufferManagers(
    const CHAR*             pPortIpOrRef,
    MultipassBufferParams*  pMultipassBufferParams)
{
    CamxResult              result                = CamxResultSuccess;
    BufferManagerCreateData createData            = {};
    FormatParamInfo         formatParamInfo       = { 0 };
    IpeConfigIo*            pConfigIO             = reinterpret_cast<IpeConfigIo*>(m_configIOMem.pVirtualAddr);
    IpeConfigIoData*        pConfigIOData         = &pConfigIO->cmdData;
    UINT32                  passMax               = 0;
    const CHAR*             pPortType             = "DownscaleInput";

    passMax = (FALSE == m_realTimeIPE) ? PASS_NAME_MAX : PASS_NAME_MAX - 1;
    passMax -= m_numPasses;
    // update each pass specific parameters
    for (UINT pass = 0; pass < passMax; pass++)
    {
        // Intialize multipass buffer portIDs
        if (0 == OsUtils::StrNICmp(pPortIpOrRef, "Input", sizeof("Input")))
        {
            pMultipassBufferParams[pass].inputPortID = static_cast<IPE_IO_IMAGES>(IPE_INPUT_IMAGE_FULL + m_numPasses);
        }
        else if (0 == OsUtils::StrNICmp(pPortIpOrRef, "Reference", sizeof("Reference")))
        {
            pMultipassBufferParams[pass].inputPortID =
                static_cast<IPE_IO_IMAGES>(IPE_INPUT_IMAGE_FULL_REF + m_numPasses);
            pPortType                                = "DownscaleRefInput";
        }

        OsUtils::SNPrintF(pMultipassBufferParams[pass].bufferManagerName,
            sizeof(pMultipassBufferParams[pass].bufferManagerName),
            "%s_%s%d_%sPortId_%d", GetPipelineName(), Name(), InstanceID(), pPortIpOrRef,
            pMultipassBufferParams[pass].inputPortID);

        pMultipassBufferParams[pass].passIdx = m_numPasses;
        UINT factor = static_cast<UINT>(pow(4, (m_numPasses)));

        if ((IPEInputPortFull != pMultipassBufferParams[pass].inputPortID) ||
            (IPEInputPortFullRef != pMultipassBufferParams[pass].inputPortID))
        {
            pMultipassBufferParams[pass].format.format = CamX::Format::PD10;
            pMultipassBufferParams[pass].format.width  =
                Utils::EvenCeilingUINT32(Utils::AlignGeneric32(m_fullInputWidth, factor) / factor);
            pMultipassBufferParams[pass].format.height =
                Utils::EvenCeilingUINT32(Utils::AlignGeneric32(m_fullInputHeight, factor) / factor);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Port is IPE full input or Reference full input, no need to create buffers");
        }
        // Minimum pass dimension not met
        if ((Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
            (m_fullInputWidth - m_stabilizationMargin.widthPixels), factor) / factor) < ICAMinWidthPixels) ||
            (Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
            (m_fullInputHeight - m_stabilizationMargin.heightLines), factor) / factor) < ICAMinHeightPixels))
        {
            CAMX_LOG_INFO(CamxLogGroupPProc, "node %s, pass %d, dim w %d, h %d, w/o margin w %d, h %d < than 30 * 26",
                          NodeIdentifierString(),
                          pass,
                          pMultipassBufferParams[pass].format.width,
                          pMultipassBufferParams[pass].format.height,
                          Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
                          (m_fullInputWidth - m_stabilizationMargin.widthPixels), factor) / factor),
                          Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
                          (m_fullInputHeight - m_stabilizationMargin.heightLines), factor) / factor));
            break;
        }

        pMultipassBufferParams[pass].format.alignment  = 1;
        pMultipassBufferParams[pass].format.colorSpace = CamX::ColorSpace::Unknown;
        pMultipassBufferParams[pass].format.rotation   = CamX::Rotation::CW0Degrees;
        formatParamInfo.yuvPlaneAlign                  = GetStaticSettings()->yuvPlaneAlignment;

        ImageFormatUtils::InitializeFormatParams(&pMultipassBufferParams[pass].format, &formatParamInfo);

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s, pass %d, idx %d, port %d, w %d, h %d,format %d,buffersize %d",
                         NodeIdentifierString(),
                         pass,
                         pMultipassBufferParams[pass].passIdx,
                         pMultipassBufferParams[pass].inputPortID,
                         pMultipassBufferParams[pass].format.width,
                         pMultipassBufferParams[pass].format.height,
                         pMultipassBufferParams[pass].format.format,
                         ImageFormatUtils::GetTotalSize(&pMultipassBufferParams[pass].format));

        createData.bufferProperties.immediateAllocImageBuffers = MaxMultipassBufferCount;
        createData.bufferProperties.maxImageBuffers            = MaxMultipassBufferCount;
        createData.bufferProperties.consumerFlags              = 0;
        createData.bufferProperties.producerFlags              = 0;
        createData.bufferProperties.grallocFormat              = 0;
        createData.bufferProperties.bufferHeap                 = CSLBufferHeapIon;
        createData.bufferProperties.memFlags                   = CSLMemFlagHw;
        createData.bufferProperties.imageFormat                = pMultipassBufferParams[pass].format;

        if (TRUE == IsSecureMode())
        {
            createData.bufferProperties.memFlags |= CSLMemFlagProtected;
        }
        else
        {
            createData.bufferProperties.memFlags |= CSLMemFlagUMDAccess;
        }

        createData.bNeedDedicatedBuffers      = TRUE;
        createData.bDisableSelfShrinking      = TRUE;
        createData.deviceIndices[0]           = m_deviceIndex;
        createData.deviceCount                = 1;
        createData.allocateBufferMemory       = TRUE;
        createData.bEnableLateBinding         = 0;
        createData.maxBufferCount             = MaxMultipassBufferCount;
        createData.immediateAllocBufferCount  = MaxMultipassBufferCount;
        createData.bufferManagerType          = BufferManagerType::CamxBufferManager;
        createData.linkProperties.pNode       = this;
        createData.numBatchedFrames           = 1;

        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "node %s, pass %d, buffer manager name:%s, allocate buffer:%d",
                         NodeIdentifierString(),
                         pass,
                         pMultipassBufferParams[pass].bufferManagerName,
                         createData.allocateBufferMemory);

        result = Node::CreateImageBufferManager(pMultipassBufferParams[pass].bufferManagerName, &createData,
                                          &pMultipassBufferParams[pass].pMultipassBufferManager);

        if (CamxResultSuccess == result)
        {
            result = pMultipassBufferParams[pass].pMultipassBufferManager->Activate();
            if (CamxResultSuccess == result)
            {
                pMultipassBufferParams[pass].pMultipassBuffer =
                    pMultipassBufferParams[pass].pMultipassBufferManager->GetImageBuffer();

                if (NULL == pMultipassBufferParams[pass].pMultipassBuffer)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Node %s, pass %d, buffer not available failed %s",
                                   NodeIdentifierString(),
                                   pass,
                                   pMultipassBufferParams[pass].bufferManagerName);
                    result = CamxResultENoMemory;
                    break;
                }
                m_createdDownscalebuffers = TRUE;
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "node %s, pass %d,"
                                 "get buffer success %s, buffer %p, vaddr %p",
                                 NodeIdentifierString(),
                                 pass,
                                 pMultipassBufferParams[pass].bufferManagerName,
                                 pMultipassBufferParams[pass].pMultipassBuffer,
                                 pMultipassBufferParams[pass].pMultipassBuffer->GetPlaneVirtualAddr(0, 0));
            }
            else
            {
                break;
            }
            pMultipassBufferParams[pass].portEnable = TRUE;
            m_numPasses++; // Update num of passes - creating the ports virtually with setconfigIOData
            result = SetConfigIOData(pConfigIOData,
                                     &pMultipassBufferParams[pass].format,
                                     pMultipassBufferParams[pass].inputPortID,
                                     pPortType);
            if (CamxResultSuccess != result)
            {
                break;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "node %s, pass %d, Create buffer manager failed %s",
                           NodeIdentifierString(),
                           pass,
                           pMultipassBufferParams[pass].bufferManagerName);

            break;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::FillDownscaleBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::FillDownscaleBufferManagers(
    UINT64                  requestId,
    CmdBuffer*              pFrameProcessCmdBuffer,
    IpeIQSettings*          pIPEIQsettings,
    MultipassBufferParams*  pMultipassBufferParams)
{
    CamxResult   result             = CamxResultSuccess;
    ImageBuffer* pDownscaleBuffer   = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "pipeline %s IPE:%d Input for setting downscale buffers",
                     NodeIdentifierString(), InstanceID());

    for (UINT pass = 0; pass < (m_numPasses - 1); pass++)
    {
        UINT32 passIdx = pMultipassBufferParams[pass].passIdx;
        // Patch buffers only if the port is enabled. Could be disabled by dimension limitations
        if ((TRUE  == pMultipassBufferParams[pass].portEnable) &&
            ((TRUE == pIPEIQsettings->anrParameters.parameters[passIdx].moduleCfg.EN) ||
             (TRUE == pIPEIQsettings->tfParameters.parameters[passIdx].moduleCfg.EN)))

        {
            pDownscaleBuffer = pMultipassBufferParams[pass].pMultipassBuffer;
            if (NULL == pDownscaleBuffer)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid downscale input buffer %p",
                    pDownscaleBuffer);
                result = CamxResultEInvalidPointer;

                break;
            }

            result = FillFrameBufferData(pFrameProcessCmdBuffer,
                                         pDownscaleBuffer,
                                         0,
                                         0,
                                         pMultipassBufferParams[pass].inputPortID);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to fill i/p downscale buffer frame data result = %d", result);
                break;
            }

            CAMX_LOG_INFO(CamxLogGroupPProc, "%s passidx %d reporting i/p downscale buffer port %d, request %llu, buffer %p",
                NodeIdentifierString(),
                pMultipassBufferParams[pass].passIdx,
                pMultipassBufferParams[pass].inputPortID,
                requestId,
                pDownscaleBuffer->GetPlaneVirtualAddr(0, 0));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::GetInSensorSeamlessControltState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetInSensorSeamlessControltState(
    ISPInputData*   pInputData)
{
    CamxResult  result  = CamxResultSuccess;
    UINT        metaTag = 0;

    result = VendorTagManager::QueryVendorTagLocation("com.qti.insensor_control", "seamless_insensor_state", &metaTag);

    if (CamxResultSuccess == result)
    {
        UINT              propertiesIPE[]   = { metaTag | InputMetadataSectionMask };
        static const UINT Length            = CAMX_ARRAY_SIZE(propertiesIPE);
        VOID*             pData[Length]     = { 0 };
        UINT64            offsets[Length]   = { 0 };

        result = GetDataList(propertiesIPE, pData, offsets, Length);

        if (CamxResultSuccess == result)
        {
            if (NULL != pData[0])
            {
                pInputData->seamlessInSensorState = *reinterpret_cast<SeamlessInSensorState*>(pData[0]);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%s Can't get in-sensor seamless control state!!!, pData is NULL",
                                 NodeIdentifierString());
            }
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%s Can't query vendor tag: seamless_insensor_state",
                         NodeIdentifierString());
    }

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE:%s Get tag: seamless_insensor_state = %u",
                     NodeIdentifierString(),
                     pInputData->seamlessInSensorState);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::GetAlignmentFusion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEICAPerspectiveTransform* IPENode::GetAlignmentFusion(
    IPEICAPerspectiveTransform* pImageAlignmentReferenceParams,
    IPEICAPerspectiveTransform* pGyroAlignmentReferenceParams,
    ISPInputData*               pInputData)
{
    IPEICAPerspectiveTransform* pSelectedAlignmentReferenceParams = NULL;

    NcLibAlignmentFusionIn    alignementFusionIn;
    NcLibAlignmentFusionOut   alignmentFusionOut;
    NcLibWarp                 imageAlign;
    NcLibWarp                 gyroAlign;
    alignmentFusionOut.alignmentTransform = NULL;
    alignmentFusionOut.alignmentType = NCLIB_NO_ALIGNMENT;
    alignmentFusionOut.age = 0;

    if (NULL != pImageAlignmentReferenceParams)
    {
        imageAlign.matrices.enable                         = pImageAlignmentReferenceParams->perspectiveTransformEnable;
        imageAlign.matrices.transformDefinedOn.widthPixels = pImageAlignmentReferenceParams->transformDefinedOnWidth;
        imageAlign.matrices.transformDefinedOn.heightLines = pImageAlignmentReferenceParams->transformDefinedOnHeight;
        imageAlign.matrices.centerType                     = CENTERED;
        imageAlign.matrices.confidence                     = pImageAlignmentReferenceParams->perspectiveConfidence;
        imageAlign.matrices.numRows                        = pImageAlignmentReferenceParams->perspectiveGeometryNumRows;
        imageAlign.matrices.numColumns                     = pImageAlignmentReferenceParams->perspetiveGeometryNumColumns;
        // Setting perspMatrices to NULL as NcLibAlignmentFusion does not use the matrix, this is to avoid memcpy
        imageAlign.matrices.perspMatrices                  = NULL;
        imageAlign.grid.enable                             = FALSE;
        alignementFusionIn.imageAlign                      = &imageAlign;
        alignementFusionIn.imageAlignForcedToIdentity      = pImageAlignmentReferenceParams->byPassAlignmentMatrixAdjustement;
    }
    else
    {
        alignementFusionIn.imageAlign                 = NULL;
        alignementFusionIn.imageAlignForcedToIdentity = 0;
    }

    if (NULL != pGyroAlignmentReferenceParams)
    {
        gyroAlign.matrices.enable                         = pGyroAlignmentReferenceParams->perspectiveTransformEnable;
        gyroAlign.matrices.transformDefinedOn.widthPixels = pGyroAlignmentReferenceParams->transformDefinedOnWidth;
        gyroAlign.matrices.transformDefinedOn.heightLines = pGyroAlignmentReferenceParams->transformDefinedOnHeight;
        gyroAlign.matrices.centerType                     = CENTERED;
        gyroAlign.matrices.confidence                     = pGyroAlignmentReferenceParams->perspectiveConfidence;
        gyroAlign.matrices.numRows                        = pGyroAlignmentReferenceParams->perspectiveGeometryNumRows;
        gyroAlign.matrices.numColumns                     = pGyroAlignmentReferenceParams->perspetiveGeometryNumColumns;
        // Setting perspMatrices to NULL as NcLibAlignmentFusion does not use the matrix, this is to avoid memcpy
        gyroAlign.matrices.perspMatrices                  = NULL;
        gyroAlign.grid.enable                             = FALSE;

        alignementFusionIn.gyroAlign                      = &gyroAlign;
    }
    else
    {
        alignementFusionIn.gyroAlign = NULL;
    }

    alignementFusionIn.mode = static_cast<NcLibAlignmentMode>(
        static_cast<Titan17xContext*>(GetHwContext())->
        GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->
        alignmentFusionMode);

    video_1_0_0::chromatix_video10Type* pChromatix = NULL;
    TuningDataManager*                  pTuningManager = pInputData->pTuningDataManager;
    if ((NULL != pTuningManager) && (TRUE == pTuningManager->IsValidChromatix()))
    {
        TuningMode    tuningMode[1];

        tuningMode[0].mode = ModeType::Default;
        tuningMode[0].subMode = { 0 };

        pChromatix = pTuningManager->GetChromatix()->GetModule_video10_sw(
            reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
            1);

        if (NULL != pChromatix)
        {
            video_1_0_0::chromatix_video10_reserveType* pReserveData = &pChromatix->chromatix_video10_reserve;
            alignementFusionIn.mode = (alignementFusionIn.mode == NCLIB_ALIGN_AUTO) ?
                static_cast<NcLibAlignmentMode>(pReserveData->fusion.mode) :
                alignementFusionIn.mode;

            video_1_0_0::chromatix_video10_coreType* pCoreData           = &pChromatix->chromatix_video10_core;
            video_1_0_0::video10_rgn_dataType::fusionStruct* pCoreFusion = &pCoreData->mod_video10_lens_posn_data->
                lens_posn_data.mod_video10_lens_zoom_data->lens_zoom_data.mod_video10_hdr_aec_data->
                hdr_aec_data.mod_video10_pre_scale_ratio_data->pre_scale_ratio_data.mod_video10_aec_data->
                video10_rgn_data.fusion;

            alignementFusionIn.imageConfHighThreshold            = pCoreFusion->image_conf_high_threshold;
            alignementFusionIn.imageConfLowThreshold             = pCoreFusion->image_conf_low_threshold;
            alignementFusionIn.minAge                            = pCoreFusion->min_age;
            alignementFusionIn.absoluteMotionThreshold           = pCoreFusion->absolute_motion_threshold;
            alignementFusionIn.absoluteMotionConfidenceThreshold = pCoreFusion->absolute_motion_confidence_threshold;
        }
    }

    if (NULL == pChromatix)
    {
        CAMX_LOG_WARN(CamxLogGroupPProc, "IPE:%d MCTF fusion chromatix not found use defaults", InstanceID());
        alignementFusionIn.imageConfHighThreshold            = IPEAlignmentFusionImgConfHighThreshold;
        alignementFusionIn.imageConfLowThreshold             = IPEAlignmentFusionImgConfLowThreshold;
        alignementFusionIn.minAge                            = IPEAlignmentFusionMinAge;
        alignementFusionIn.absoluteMotionThreshold           = IPEAlignmentFusionAbsMotionThreshold;
        alignementFusionIn.absoluteMotionConfidenceThreshold = IPEAlignmentFusionAbsMotionConfThreshold;
    }

    alignementFusionIn.absoluteMotion           = IPEAlignmentFusionAbsMotionThreshold;
    alignementFusionIn.absoluteMotionConfidence = IPEAlignmentFusionAbsMotionConfThreshold;
    alignementFusionIn.prevAlignment            = m_currentAlignmentType;
    alignementFusionIn.prevAge                  = m_currentAlignmentAge;

    NcLibAlignmentFusion(&alignementFusionIn, &alignmentFusionOut);

    m_currentAlignmentType = alignmentFusionOut.alignmentType;
    m_currentAlignmentAge  = alignmentFusionOut.age;

    if (NCLIB_ALIGN_IMAGE_BASED == m_currentAlignmentType)
    {
        pSelectedAlignmentReferenceParams = pImageAlignmentReferenceParams;
    }
    else if (NCLIB_ALIGN_GYRO_BASED == m_currentAlignmentType)
    {
        pSelectedAlignmentReferenceParams = pGyroAlignmentReferenceParams;
    }

    return pSelectedAlignmentReferenceParams;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::DeleteDownscaleBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::DeleteDownscaleBufferManagers(
    MultipassBufferParams*   pMultipassBufferParams)
{
    CamxResult result           = CamxResultSuccess;
    UINT       referenceCount   = 0;

    for (UINT pass = 0; pass < PASS_NAME_MAX-1; pass++)
    {
        if (TRUE == pMultipassBufferParams[pass].portEnable)
        {
            referenceCount =
                pMultipassBufferParams[pass].pMultipassBuffer->ReleaseBufferManagerImageReference();
            if (0 == referenceCount)
            {
                if (TRUE == pMultipassBufferParams[pass].pMultipassBuffer->HasBackingBuffer())
                {
                    pMultipassBufferParams[pass].pMultipassBuffer->Release(FALSE);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Image buffer is not valid to Release");
                }
                if (FALSE == pMultipassBufferParams[pass].pMultipassBuffer->HasBackingBuffer())
                {
                    pMultipassBufferParams[pass].pMultipassBuffer = NULL;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Image buffer is still valid after Release");
                }
            }

            if (NULL != pMultipassBufferParams[pass].pMultipassBufferManager)
            {
                pMultipassBufferParams[pass].pMultipassBufferManager->Deactivate(FALSE);
                pMultipassBufferParams[pass].pMultipassBufferManager->Destroy();
                pMultipassBufferParams[pass].pMultipassBufferManager = NULL;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Buffer Manager is Set to NULL");
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::NeedInternalMultipass
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPENode::NeedInternalMultipass(
    UINT32 numPasses,
    UINT32 batchsize)
{
    BOOL   needInternalMultipass = FALSE;
    UINT32 maxSupportedPassess   = (TRUE == m_realTimeIPE) ? (PASS_NAME_MAX - 1) : PASS_NAME_MAX;


    if ((maxSupportedPassess > numPasses)                           &&
        (1     == batchsize)                                        &&
        (FALSE == IsScalerOnlyIPE())                                &&
        (FALSE == IsICAOnlyIPE())                                   &&
        (FALSE == GetStaticSettings()->disableIPEInternalDownscale) &&
        (IPEProfileId::IPEProfileIdIndications != m_instanceProperty.profileId))
    {
        needInternalMultipass = TRUE;
    }

    return  needInternalMultipass;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::UpdateFromDownscaleFlag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::UpdateFromDownscaleFlag(
    IpeConfigIoData*   pConfigIOData,
    IPE_IO_IMAGES      firmwarePortId)
{
    pConfigIOData->images[firmwarePortId].info.fromDownscale = 0;
    switch (firmwarePortId)
    {
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS4:
            pConfigIOData->images[firmwarePortId].info.fromDownscale = 1;
            break;
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS16:
            pConfigIOData->images[firmwarePortId].info.fromDownscale = 1; // enable ipe internal downscaling for DS4/DS16
            break;
        case IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS64:
            if (FALSE == CheckIsIPERealtime(m_numPasses))
            {
                pConfigIOData->images[firmwarePortId].info.fromDownscale = 1; // enable ipe internal downscaling for DS64
            }
            else
            {
                pConfigIOData->images[firmwarePortId].info.fromDownscale = 0; // disable ipe internal downscaling for DS64
            }
            break;
        default:
            pConfigIOData->images[firmwarePortId].info.fromDownscale = 0;
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::DumpGeolibResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPENode::DumpGeolibResult(
    UINT64 requestId)
{

#if defined (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // NOWHINE PR002 <- Win32 definition
    CHAR dataPath[]        = "/data/vendor/camera";
#else
    CHAR dataPath[]        = "/data/misc/camera";
#endif // Android-P or later

    CHAR dumpFilename[256] = { 0 };

    OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/%s_reequest_%llu_%s.log", dataPath, NodeIdentifierString(),
        requestId, "geolib_ipe_still_frame_config");
    FILE* pFile = OsUtils::FOpen(dumpFilename, "wb");
    if (NULL != pFile)
    {
        fprintf(pFile, "==================== Still Output ====================\n");
        Dump_GeoLibStillFrameConfig(&m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig, pFile);
        OsUtils::FClose(pFile);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Can not create GeoLib output information dump file");
    }
 }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::GetGeoLibStillFrameConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetGeoLibStillFrameConfiguration(
    UINT64 requestId)
{
    CamxResult result = CamxResultSuccess;
    UINT32     tag;

    CAMX_LOG_INFO(CamxLogGroupPProc, "[%s]: issrenabled=%d, IPEMode=%d",
        NodeIdentifierString(), IsSrEnabled(), m_geolibData.IPEModeRealtime);
    if (TRUE == IsSrEnabled() && (FALSE == m_geolibData.IPEModeRealtime))
    {
        result = GetCurrentStillFrameConfigurationTagId(&tag);
        if (CamxResultSuccess == result)
        {
            UINT       geoLibFrameconfigTag[]    = { tag };
            const UINT length                    = CAMX_ARRAY_SIZE(geoLibFrameconfigTag);
            VOID*      pData[length]             = { 0 };
            UINT64     pDataOffset[length]       = { 0 };

            result = GetDataList(geoLibFrameconfigTag, pData, pDataOffset, length);
            if (CamxResultSuccess == result && (NULL != pData[0]))
            {
                // need to replace this with IPE node data
                CAMX_LOG_INFO(CamxLogGroupPProc, "Copy still frame configuration......");
                Utils::Memcpy(&m_geolibData.geoLibStreamData.nonRTgeolibData.stillFrameConfig,
                    pData[0], sizeof(GeoLibStillFrameConfig));
                if (TRUE == m_dumpGeolibResult)
                {
                    DumpGeolibResult(requestId);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "[%s]: Can not get Geo lib frame configuration: result=%d, data=%p",
                    NodeIdentifierString(), result, pData[0]);
                result = CamxResultENoSuch;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "[%s]: Can not get geolib tagId", NodeIdentifierString());
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::SetGeoLibStillFrameConfigurationDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::SetGeoLibStillFrameConfigurationDependency(
    NodeProcessRequestData*  pNodeRequestData)
{
    CamxResult result = CamxResultSuccess;
    UINT32     count  = pNodeRequestData->dependencyInfo[0].propertyDependency.count;

    if (TRUE == IsSrEnabled())
    {
        if (IPEMFSRPrefilter == m_instanceProperty.processingType)
        {
            if (TRUE == IsTagPresentInPublishList(m_stillFrameConfigurationTagIdPrefilter))
            {
                CAMX_LOG_INFO(CamxLogGroupPProc, "[%s]: No need to add Geolib frame configuration dependency",
                    NodeIdentifierString());
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupPProc, "[%s]: Add Geolib frame configuration dependency for tag=0x%x",
                    NodeIdentifierString(), m_stillFrameConfigurationTagIdPrefilter);
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count++] =
                    m_stillFrameConfigurationTagIdPrefilter;
                pNodeRequestData->dependencyInfo[0].propertyDependency.count = count;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::ComputeAndSetConfigIOData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::ComputeAndSetConfigIOData(
    IpeConfigIoData*    pConfigIOData)
{
    CamxResult  result                                     = CamxResultSuccess;
    UINT                        numOutputPort              = 0;
    UINT                        numInputPort               = 0;
    const ImageFormat*          pImageFormat               = NULL;
    UINT32                      fullInputPortWidth         = 0;
    UINT32                      fullInputPortHeight        = 0;
    UINT32                      numPasses                  = 0;
    UINT32                      inputReferenceOffset       = IPEInputPortFullRef;
    UINT32                      outputReferenceOffset      = IPEOutputPortFullRef;
    FormatParamInfo             formatParamInfo            = { 0 };
    UINT                        inputPortId[IPEMaxInput]   = { 0 };
    UINT                        outputPortId[IPEMaxOutput] = { 0 };
    UINT32                      numOutputRefPorts          = 0;
    UINT32                      passMax                    = 0;
    LoopBackBufferParams        loopBackBufferParams[PASS_NAME_MAX];
    MultipassBufferParams       multipassBufferParams[PASS_NAME_MAX - 1];
    IPE_IO_IMAGES               firmwarePortId;

    CamX::Utils::Memset(&loopBackBufferParams[0], 0, sizeof(loopBackBufferParams));
    CamX::Utils::Memset(&multipassBufferParams[0], 0, sizeof(multipassBufferParams));

    GetFPSAndBatchSize();

    pConfigIOData->maxBatchSize = m_maxBatchSize;

    if ((TRUE == IsBlendWithNPS()) || (TRUE == IsPostfilterWithNPS()))
    {
        SetMuxMode(pConfigIOData);
    }

    pConfigIOData->secureMode = IsSecureMode();

    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s:, MUX Mode: %d, secure mode %d, batch size %d",
                     NodeIdentifierString(), pConfigIOData->muxMode,
                     pConfigIOData->secureMode, pConfigIOData->maxBatchSize);

    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    // Get Output Port List
    GetAllOutputPortIds(&numOutputPort, &outputPortId[0]);

    if (numInputPort <= 0 || numInputPort > IPEMaxInput ||
        numOutputPort <= 0 || numOutputPort > IPEMaxOutput)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "invalid input (%u) or output port (%u)", numInputPort, numOutputPort);
        result = CamxResultEUnsupported;
    }

    // Add regualr  input ports
    for (UINT inputPortIndex = 0; inputPortIndex < numInputPort; inputPortIndex++)
    {
        TranslateToFirmwarePortId(inputPortId[inputPortIndex], &firmwarePortId);

        pImageFormat =
            GetInputPortImageFormat(InputPortIndex(inputPortId[inputPortIndex]));

        if (NULL == pImageFormat)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "pImageFormat is NULL");
            result = CamxResultENoMemory;
            break;
        }

        if (inputPortId[inputPortIndex] == IPEInputPortFull)
        {
            fullInputPortWidth  = pImageFormat->width;
            fullInputPortHeight = pImageFormat->height;
            numPasses++;
        }

        if ((IPEInputPortDS4  == inputPortId[inputPortIndex]) ||
            (IPEInputPortDS16 == inputPortId[inputPortIndex]) ||
            (IPEInputPortDS64 == inputPortId[inputPortIndex]))
        {
            numPasses++;
        }

        result = SetConfigIOData(pConfigIOData, pImageFormat, firmwarePortId, "Input");
    }

    // Internal downscale ports
    if (CamxResultSuccess == result)
    {
        if (TRUE == NeedInternalMultipass(numPasses, m_maxBatchSize))
        {
            passMax = (FALSE == CheckIsIPERealtime(numPasses)) ? PASS_NAME_MAX : PASS_NAME_MAX - 1;
            passMax -= numPasses;
            for (UINT pass = 0; pass < passMax; pass++)
            {
                multipassBufferParams[pass].inputPortID = static_cast<IPE_IO_IMAGES>(numPasses);

                UINT factor = static_cast<UINT>(pow(4, (numPasses)));
                multipassBufferParams[pass].format.format = CamX::Format::PD10;
                multipassBufferParams[pass].format.width  =
                    Utils::EvenCeilingUINT32(Utils::AlignGeneric32(fullInputPortWidth, factor) / factor);
                multipassBufferParams[pass].format.height =
                    Utils::EvenCeilingUINT32(Utils::AlignGeneric32(fullInputPortHeight, factor) / factor);
                // Minimum pass dimension not met
                if ((Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
                    (fullInputPortHeight - m_stabilizationMargin.heightLines), factor) / factor) < ICAMinWidthPixels) ||
                    (Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
                    (fullInputPortHeight - m_stabilizationMargin.heightLines), factor) / factor) < ICAMinHeightPixels))
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "node %s, pass %d, dim %d, h %d , w/o margin w %d , h %d < than 30 * 26",
                                   NodeIdentifierString(),
                                   pass,
                                   multipassBufferParams[pass].format.width,
                                   multipassBufferParams[pass].format.height,
                                   Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
                                   (fullInputPortWidth - m_stabilizationMargin.widthPixels), factor) / factor),
                                   Utils::EvenCeilingUINT32(Utils::AlignGeneric32(
                                   (fullInputPortHeight - m_stabilizationMargin.heightLines), factor) / factor));
                    break;
                }

                multipassBufferParams[pass].format.alignment  = 1;
                multipassBufferParams[pass].format.colorSpace = CamX::ColorSpace::Unknown;
                multipassBufferParams[pass].format.rotation   = CamX::Rotation::CW0Degrees;

                ImageFormatUtils::InitializeFormatParams(&multipassBufferParams[pass].format, &formatParamInfo);

                multipassBufferParams[pass].portEnable = TRUE;
                numPasses++; // Update num of passes - creating the ports virtually with setconfigIOData
                result = SetConfigIOData(pConfigIOData,
                                         &multipassBufferParams[pass].format,
                                         multipassBufferParams[pass].inputPortID,
                                         "Input");
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "internal multipass config IO failed");
                    break;
                }
            }
        }
    }

    // Add regualr  output ports
    if (CamxResultSuccess == result)
    {
        for (UINT outputPortIndex = 0; outputPortIndex < numOutputPort; outputPortIndex++)
        {
            TranslateToFirmwarePortId(outputPortId[outputPortIndex], &firmwarePortId);

            pImageFormat =
                GetOutputPortImageFormat(OutputPortIndex(firmwarePortId));

            if (NULL == pImageFormat)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "pImageFormat is NULL");
                result = CamxResultENoMemory;
                break;
            }
            result = SetConfigIOData(pConfigIOData, pImageFormat, firmwarePortId, "Output");
        }
    }

    // get margin and update number of passes
    if (CamxResultSuccess == result)
    {
        result = GetStabilizationMargins();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to get stabilization margins %d", result);
        }
        result = UpdateNumberofPassesonDimension(fullInputPortWidth, fullInputPortHeight, &numPasses);
    }

    // Loopback ports
    if (CamxResultSuccess == result)
    {
        CamX::Utils::Memset(&formatParamInfo, 0, sizeof(formatParamInfo));
        if (TRUE == IsLoopBackPortEnabled())
        {
            for (UINT pass = 0; (pass < numPasses) && (numPasses < PASS_NAME_MAX); pass++)
            {
                //  Intialize loop back portIDs
                loopBackBufferParams[pass].outputPortID = static_cast<IPE_IO_IMAGES>(pass + IPE_OUTPUT_IMAGE_FULL_REF);
                loopBackBufferParams[pass].inputPortID  = static_cast<IPE_IO_IMAGES>(pass + IPE_INPUT_IMAGE_FULL_REF);
                UINT factor = static_cast<UINT>(pow(4, pass));

                if (IPEOutputPortFullRef == loopBackBufferParams[pass].outputPortID)
                {
                    formatParamInfo.UBWCVersionConsumerUsage = 0xFFFF;
                    formatParamInfo.UBWCVersionProducerUsage = 0xFFFF;
                    formatParamInfo.LossyUBWCProducerUsage   = UBWCLossy;
                    formatParamInfo.LossyUBWCConsumerUsage   = UBWCLossy;
                    loopBackBufferParams[pass].format.format = CamX::Format::UBWCTP10;
                    loopBackBufferParams[pass].format.width  = fullInputPortWidth;
                    loopBackBufferParams[pass].format.height = fullInputPortHeight;
                    SetProducerFormatParameters(
                        &loopBackBufferParams[pass].format,
                        &formatParamInfo);
                    SetConsumerFormatParameters(
                        &loopBackBufferParams[pass].format,
                        &formatParamInfo);
                }
                else
                {
                    loopBackBufferParams[pass].format.format = CamX::Format::PD10;
                    loopBackBufferParams[pass].format.width  =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(fullInputPortWidth, factor) / factor);
                    loopBackBufferParams[pass].format.height =
                        Utils::EvenCeilingUINT32(Utils::AlignGeneric32(fullInputPortHeight, factor) / factor);
                }

                // Minimum pass dimension not met
                if ((Utils::EvenCeilingUINT32(Utils::AlignGeneric32((
                        fullInputPortWidth - m_stabilizationMargin.widthPixels), factor) / factor) < ICAMinWidthPixels) ||
                    (Utils::EvenCeilingUINT32(Utils::AlignGeneric32((
                        fullInputPortHeight - m_stabilizationMargin.heightLines), factor) / factor) < ICAMinHeightPixels))
                {
                    CAMX_LOG_INFO(CamxLogGroupPProc, "node %s, pass %d, dim w %d, h %d , w/o margin w %d, h %d < than 30 * 26",
                                  NodeIdentifierString(),
                                  pass,
                                  loopBackBufferParams[pass].format.width,
                                  loopBackBufferParams[pass].format.height,
                                  Utils::EvenCeilingUINT32(Utils::AlignGeneric32((
                                      fullInputPortWidth - m_stabilizationMargin.widthPixels), factor) / factor),
                                  Utils::EvenCeilingUINT32(Utils::AlignGeneric32((
                                      fullInputPortHeight - m_stabilizationMargin.heightLines), factor) / factor));
                    break;
                }

                loopBackBufferParams[pass].format.alignment  = 1;
                loopBackBufferParams[pass].format.colorSpace = CamX::ColorSpace::Unknown;
                loopBackBufferParams[pass].format.rotation   = CamX::Rotation::CW0Degrees;

                ImageFormatUtils::InitializeFormatParams(&loopBackBufferParams[pass].format, &formatParamInfo);
                loopBackBufferParams[pass].portEnable = TRUE;
                numOutputRefPorts++;
                result = SetConfigIOData(pConfigIOData,
                                         &loopBackBufferParams[pass].format,
                                         loopBackBufferParams[pass].outputPortID,
                                         "ReferenceOutput");

                if (CamxResultSuccess == result)
                {
                    result = SetConfigIOData(pConfigIOData,
                                             &loopBackBufferParams[pass].format,
                                             loopBackBufferParams[pass].inputPortID,
                                             "ReferenceInput");
                }
            }
        }
    }


    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE: %s number of passes %d",
                     NodeIdentifierString(), numPasses);
    if (CamxResultSuccess == result)
    {
        // Update IPE configIO topology type based on profile and lower pass input
        UpdateIPEConfigIOTopology(pConfigIOData,
                                  numPasses,
                                  m_instanceProperty.profileId,
                                  m_instanceProperty.processingType);

        pConfigIOData->stabilizationMargins.widthPixels = m_stabilizationMargin.widthPixels;
        pConfigIOData->stabilizationMargins.heightLines = m_stabilizationMargin.heightLines;
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "node %s, margin width %d, height %d",
                         NodeIdentifierString(),
                         pConfigIOData->stabilizationMargins.widthPixels,
                         pConfigIOData->stabilizationMargins.heightLines);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::GetScratchBufferSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetScratchBufferSize(
    ScratchBufferData* pScratchBufferData)
{
    CamxResult        result = CamxResultSuccess;
    IpeConfigIoData   configIOData;

    CamX::Utils::Memset(&configIOData, 0, sizeof(configIOData));

    result = ComputeAndSetConfigIOData(&configIOData);

    if (CamxResultSuccess == result)
    {
        result = m_funcPtrIPECalcScratchBufSize(&configIOData, reinterpret_cast<IPEScratchBufferData*>(pScratchBufferData));

        for (UINT pass = 0; pass < PASS_NAME_MAX; pass++)
        {
            CAMX_LOG_INFO(CamxLogGroupPProc, "%s:pass %d,size %d, [offset,size]:pdi %d, %d, tf %d, %d, tfr %d, %d, lmc %d, %d",
                          NodeIdentifierString(), pass, pScratchBufferData->scratchBufferSize,
                          pScratchBufferData->pdiOffset[pass], pScratchBufferData->pdiSize[pass],
                          pScratchBufferData->tfiOffset[pass], pScratchBufferData->tfiSize[pass],
                          pScratchBufferData->tfrOffset[pass], pScratchBufferData->tfrSize[pass],
                          pScratchBufferData->lmciOffset[pass], pScratchBufferData->lmciSize[pass]);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "%s ComputeAndSetConfigIOData failed %d",
                       NodeIdentifierString(), result);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::PublishScratchBufferData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::PublishScratchBufferData()
{
    CamxResult               result        = CamxResultSuccess;
    const VOID*              pData[1]      = { 0 };
    UINT                     pDataCount[1] = { 0 };
    pData[0] = &m_scratchBufferData;
    pDataCount[0] = sizeof(m_scratchBufferData);

    result = WriteDataList(&m_scratchBufferDataLocation, pData, pDataCount, 1);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "WriteDataList failed for scratch buffer data");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::GetCurrentStillFrameConfigurationTagId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetCurrentStillFrameConfigurationTagId(
    UINT32* pTagId)
{
    CamxResult result = CamxResultSuccess;

    switch (m_instanceProperty.processingType)
    {
        case IPEMFSRPrefilter:
        {
            *pTagId = m_stillFrameConfigurationTagIdPrefilter;
            CAMX_LOG_INFO(CamxLogGroupPProc, "Retrieve GelibMetadata for prefilter: tagId=0x%x",
                *pTagId);
            break;
        }
        case IPEMFSRBlend:
        {
            *pTagId = m_stillFrameConfigurationTagIdBlending | InputMetadataSectionMask;
            CAMX_LOG_INFO(CamxLogGroupPProc, "Retrieve GelibMetadata for blending: tagId=0x%x",
                *pTagId);
            break;
        }
        case IPEMFSRPostfilter:
        {
            *pTagId = m_stillFrameConfigurationTagIdPostfilter | InputMetadataSectionMask;
            CAMX_LOG_INFO(CamxLogGroupPProc, "Retrieve GelibMetadata for postfilter: tagId=0x%x",
                *pTagId);
            break;
        }
        default:
        {
            *pTagId = 0;
            result = CamxResultEFailed;
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPENode::GetScratchBufferData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPENode::GetScratchBufferData()
{
    CamxResult               result        = CamxResultSuccess;
    VOID*                    pData[1]      = { 0 };
    UINT64                   pDataCount[1] = { 0 };

    ScratchBufferData*       pScratchBufferData = NULL;

    result = GetDataList(&m_scratchBufferDataLocation, pData, pDataCount, 1);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "GetDataList failed for scratch buffer data");
    }
    else
    {
        pScratchBufferData = reinterpret_cast<ScratchBufferData*>(pData[0]);
        if (NULL != pScratchBufferData)
        {
            for (UINT pass = 0; pass < PASS_NAME_MAX; pass++)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "%s:pass %d,size %d, [offset,size]:"
                    "pdi %d, %d, tf %d, %d, tfr %d, %d, lmc %d, %d",
                    NodeIdentifierString(), pass , pScratchBufferData->scratchBufferSize,
                    pScratchBufferData->pdiOffset[pass], pScratchBufferData->pdiSize[pass],
                    pScratchBufferData->tfiOffset[pass], pScratchBufferData->tfiSize[pass],
                    pScratchBufferData->tfrOffset[pass], pScratchBufferData->tfrSize[pass],
                    pScratchBufferData->lmciOffset[pass], pScratchBufferData->lmciSize[pass]);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPENode::GetSensorModeData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const SensorMode* IPENode::GetSensorModeData()
{
    CamxResult        result      = CamxResultSuccess;
    const SensorMode* pSensorMode = NULL;
    BOOL sensorConnected          = (TRUE == IsRealTime()) ? TRUE : FALSE;

    if (FALSE == sensorConnected)
    {
        UINT       metaTag   = 0;
        UINT       modeIndex = 0;

        result = VendorTagManager::QueryVendorTagLocation("com.qti.sensorbps", "mode_index", &metaTag);

        metaTag |= InputMetadataSectionMask;

        /// @todo (CAMX-1015) Can optimize by keeping an array of SensorMode* per requestId
        UINT              propertiesIPE[] = { PropertyIDSensorCurrentMode, PropertyIDUsecaseSensorModes, metaTag };
        static const UINT Length          = CAMX_ARRAY_SIZE(propertiesIPE);
        VOID*             pData[Length]   = { 0 };
        UINT64            offsets[Length] = { 0, 0, 0 };

        GetDataList(propertiesIPE, pData, offsets, Length);

        if (NULL != pData[2])
        {
            // Sensor not connected so pull from the input metadata
            modeIndex = *reinterpret_cast<UINT*>(pData[2]);
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "IPE using vendor tag com.qti.sensorbps mode index %d", modeIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Sensor mode index vendor tag not provided! Using default 0!");
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
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Realtime ,IPE using pipeline mode index");
    }

    return pSensorMode;
}
CAMX_NAMESPACE_END
