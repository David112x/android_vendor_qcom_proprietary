////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecaseutils.cpp
/// @brief Usecase utils class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if (!defined(LE_CAMERA)) // ANDROID
#include <errno.h>
#include "gr_priv_handle.h"
#include "qdMetaData.h"
#endif // ANDROID
#include "chxadvancedcamerausecase.h"
#include "chxsensorselectmode.h"
#include "chxusecase.h"
#include "chxusecasedefault.h"
#include "chxusecasemc.h"
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) //Android-P or better
#include "chxusecasedual.h"
#endif
#include "chxusecasetorch.h"
#include "chxusecasevrmc.h"
#if (!defined(LE_CAMERA)) // ANDROID
#include "chxusecasesuperslowmotionfrc.h"
#endif // ANDROID

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

extern CHIBUFFERMANAGEROPS g_chiBufferManagerOps;

UINT        UsecaseSelector::NumImplDefinedFormats  = 4;
ChiStream   UsecaseSelector::FDStream               = m_DefaultFDStream;

ChiBufferFormat UsecaseSelector::AllowedImplDefinedFormats[] =
{
    ChiFormatYUV420NV12, ChiFormatYUV420NV21,  ChiFormatUBWCTP10,  ChiFormatUBWCNV12
};

BOOL UsecaseSelector::GPURotationUsecase           = FALSE;
BOOL UsecaseSelector::HFRNo3AUsecase               = FALSE;
UINT UsecaseSelector::VideoEISV2Usecase            = 0;
UINT UsecaseSelector::VideoEISV3Usecase            = 0;
BOOL UsecaseSelector::GPUDownscaleUsecase          = FALSE;
static const UINT RTYUVOutputCap                   = 3840 * 2160;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::~UsecaseSelector
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseSelector::~UsecaseSelector()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseSelector* UsecaseSelector::Create(
    const ExtensionModule* pExtModule)
{
    CDKResult        result           = CDKResultSuccess;
    UsecaseSelector* pUsecaseSelector = new UsecaseSelector;

    pUsecaseSelector->m_pExtModule = pExtModule;

    result = pUsecaseSelector->Initialize();

    if (CDKResultSuccess != result)
    {
        pUsecaseSelector->Destroy();
        pUsecaseSelector = NULL;
    }

    return pUsecaseSelector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSelector::Destroy()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSelector::Initialize()
{
    CDKResult result = CDKResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsPreviewStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsPreviewStream(
    const camera3_stream_t* pStream)
{
    CHX_ASSERT(NULL != pStream);

    BOOL isPreviewStream = FALSE;

    if ((CAMERA3_STREAM_OUTPUT     == pStream->stream_type) &&
        (GRALLOC_USAGE_HW_COMPOSER == (GRALLOC_USAGE_HW_COMPOSER & pStream->usage)))
    {
        isPreviewStream = TRUE;
    }

    return isPreviewStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsVideoStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsVideoStream(
    const camera3_stream_t* pStream)
{
    CHX_ASSERT(NULL != pStream);

    BOOL isVideoStream = FALSE;

    if ((NULL != pStream ) &&
        (0 != (GRALLOC_USAGE_HW_VIDEO_ENCODER & pStream->usage)) &&
        (DataspaceHEIF != static_cast<ChiDataSpace>(pStream->data_space)))
    {
        isVideoStream = TRUE;
    }

    return isVideoStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsHEIFStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsHEIFStream(
    const camera3_stream_t* pStream)
{
    BOOL isHEIFStream = FALSE;

    if (NULL != pStream)
    {
        ChiDataSpace dataSpace = static_cast<ChiDataSpace>(pStream->data_space);
        if ((0 != (GrallocUsageHwImageEncoder & pStream->usage)) ||
            (DataspaceHEIF == dataSpace))
        {
            isHEIFStream = TRUE;
        }
    }

    return isHEIFStream;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsYUVSnapshotStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsYUVSnapshotStream(
    const camera3_stream_t* pStream)
{
    BOOL isYUVSnapshotStream = FALSE;

    if ((NULL                           != pStream)                &&
        (CAMERA3_STREAM_OUTPUT          == pStream->stream_type)   &&
        (HAL_PIXEL_FORMAT_YCbCr_420_888 == pStream->format)        &&
        (FALSE                          == IsVideoStream(pStream)) &&
        (FALSE                          == IsPreviewStream(pStream)))
    {
        isYUVSnapshotStream = TRUE;
    }

    return isYUVSnapshotStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsYUVOutThreshold
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsYUVOutThreshold(
    const camera3_stream_t* pStream)
{
    BOOL IsYUVOutThreshold = FALSE;

    if (IsYUVSnapshotStream(pStream))
    {
        if (RTYUVOutputCap <= (pStream->width * pStream->height))
        {
           IsYUVOutThreshold = TRUE;
        }
    }

    return IsYUVOutThreshold;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsYUVSnapshotStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsJPEGSnapshotStream(
    const camera3_stream_t* pStream)
{
    BOOL isJPEGSnapshotStream = FALSE;

    if ((NULL                        != pStream)              &&
        (CAMERA3_STREAM_OUTPUT       == pStream->stream_type) &&
        (HAL_PIXEL_FORMAT_BLOB       == pStream->format)      &&
        ((HAL_DATASPACE_JFIF         == pStream->data_space)  ||
         (HAL_DATASPACE_V0_JFIF      == pStream->data_space)))
    {
        isJPEGSnapshotStream = TRUE;
    }

    return isJPEGSnapshotStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsRawStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsRawStream(
    const camera3_stream_t* pStream)
{
    BOOL isRawStream = FALSE;

    if ((NULL                    != pStream)                &&
        (CAMERA3_STREAM_OUTPUT   == pStream->stream_type)   &&
        ((HAL_PIXEL_FORMAT_RAW10 == pStream->format) ||
        ((HAL_PIXEL_FORMAT_RAW16 == pStream->format))))
    {
        isRawStream = TRUE;
    }

    return isRawStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsRawInputStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsRawInputStream(
    const camera3_stream_t* pStream)
{
    BOOL isRawStream = FALSE;

    if ((NULL != pStream) &&
        (CAMERA3_STREAM_INPUT == pStream->stream_type) &&
        ((HAL_PIXEL_FORMAT_RAW10 == pStream->format) ||
        ((HAL_PIXEL_FORMAT_RAW16 == pStream->format))))
    {
        isRawStream = TRUE;
    }

    return isRawStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::HasHeicSnapshotStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::HasHeicSnapshotStream(
    const CHISTREAMCONFIGINFO* pStreamConfig)
{
    BOOL                           result = FALSE;
    camera3_stream_configuration_t streamConfig;

    CHX_STATIC_ASSERT(offsetof(camera3_stream_t, data_space) == offsetof(CHISTREAM, dataspace));

    if (NULL != pStreamConfig)
    {
        streamConfig.num_streams = pStreamConfig->numStreams;
        streamConfig.streams     = reinterpret_cast<camera3_stream_t**>(pStreamConfig->pChiStreams);
        result                   = HasHeicSnapshotStream(&streamConfig);
    }
    else
    {
        CHX_LOG_ERROR("Cannot use NULL stream config!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::HasHeicSnapshotStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::HasHeicSnapshotStream(
    const camera3_stream_configuration_t* pStreamConfig)
{
    BOOL result = FALSE;
    for (UINT i = 0; i < pStreamConfig->num_streams; i++)
    {
        if (static_cast<UINT>(DataspaceHEIF) == pStreamConfig->streams[i]->data_space)
        {
            result = TRUE;
            break;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsYUVOutStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsYUVOutStream(
    const camera3_stream_t* pStream)
{
    CHX_ASSERT(NULL != pStream);

    BOOL bIsYUVOutStream = FALSE;

    if ((CAMERA3_STREAM_OUTPUT == pStream->stream_type) &&
        ((HAL_PIXEL_FORMAT_YCbCr_420_888 == pStream->format)))
    {
        bIsYUVOutStream = TRUE;
    }

    return bIsYUVOutStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::GetSnapshotStreamConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSelector::GetSnapshotStreamConfiguration(
    UINT                  numStreams,
    CHISTREAM**           ppChiStreams,
    SnapshotStreamConfig& rSnapshotStreamConfig)
{
    CDKResult result = CDKResultSuccess;

    rSnapshotStreamConfig.type = SnapshotStreamType::UNKNOWN;

    if (NULL == ppChiStreams)
    {
        result = CDKResultEInvalidArg;
        CHX_LOG_ERROR("ppChiStreams is NULL!");
    }
    else
    {
        rSnapshotStreamConfig.pSnapshotStream  = NULL;
        rSnapshotStreamConfig.pThumbnailStream = NULL;
        rSnapshotStreamConfig.pRawStream       = NULL;

        for (UINT streamIdx = 0; streamIdx < numStreams; streamIdx++)
        {
            CHISTREAM* pStream = ppChiStreams[streamIdx];

            if (ChiStreamTypeInput == pStream->streamType)
            {
                continue; // Skip inputs
            }
            switch (pStream->format)
            {
                case ChiStreamFormatBlob:
                    if (DataspaceJPEGAPPSegments == pStream->dataspace)
                    {
                        rSnapshotStreamConfig.pThumbnailStream = pStream;
                    }
                    else if (NULL == rSnapshotStreamConfig.pSnapshotStream) // JPEG Snapshot Case
                    {
                        rSnapshotStreamConfig.pSnapshotStream = pStream;
                        rSnapshotStreamConfig.type            = SnapshotStreamType::JPEG;
                    }
                    break;
                case ChiStreamFormatImplDefined:
                    if (DataspaceHEIF == pStream->dataspace)             // HEIC Snapshot Case
                    {
                        rSnapshotStreamConfig.pSnapshotStream = pStream;
                        rSnapshotStreamConfig.type            = SnapshotStreamType::HEIC;
                    }
                    break;
                case ChiStreamFormatRaw10:
                case ChiStreamFormatRaw16:
                    {
                        rSnapshotStreamConfig.pRawStream = pStream;
                    }
                    break;
                default:
                    break;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsYUVInStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsYUVInStream(
    const camera3_stream_t* pStream)
{
    CHX_ASSERT(NULL != pStream);

    BOOL bIsYUVInStream = FALSE;

    if ((CAMERA3_STREAM_INPUT  == pStream->stream_type) &&
        ((HAL_PIXEL_FORMAT_YCbCr_420_888 == pStream->format)))
    {
        bIsYUVInStream = TRUE;
    }

    return bIsYUVInStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsPreviewZSLYUVStreamConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsPreviewZSLStreamConfig(
    const camera3_stream_configuration_t* pStreamConfig)
{
    BOOL                    isMatch        = FALSE;
    const camera3_stream_t* pPreviewStream = NULL;
    const camera3_stream_t* pOtherStream   = NULL;

    for (UINT idx = 0; idx < pStreamConfig->num_streams; idx++)
    {
        const camera3_stream_t* pStream = pStreamConfig->streams[idx];
        if (IsPreviewStream(pStream))
        {
            pPreviewStream = pStream;
        }
        else if (NULL == pOtherStream)
        {
            pOtherStream = pStream;
        }
    }

    if (NULL != pPreviewStream)
    {
        /// if YUV stream + Preview stream
        /// if JPEG stream + Preview stream
        /// if HEIF snapshot stream + Preview stream
        ///  - stream[0]: preview,  format: implemenation_defined
        ///  - stream[1]: snapshot, format: implemenation_defined, usage: GrallocUsagePrivateHEIF
        if (FALSE == HasHeicSnapshotStream(pStreamConfig))
        {
            // Non-HEIF streams matching logic is specific for 2 stream cases.
            isMatch = (2 == pStreamConfig->num_streams) &&
                      ((TRUE == IsYUVSnapshotStream(pOtherStream))  ||
                      (TRUE == IsJPEGSnapshotStream(pOtherStream)));
        }
        else
        {
            isMatch = TRUE;
        }
    }

    return isMatch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsRawJPEGStreamConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsRawJPEGStreamConfig(
    const camera3_stream_configuration_t* pStreamConfig)
{
    BOOL isRaw   = FALSE;
    BOOL isJPEG  = FALSE;

    for (UINT i = 0; i < pStreamConfig->num_streams; i++)
    {
        if (TRUE == IsRawStream(pStreamConfig->streams[i]))
        {
            isRaw = TRUE;
        }
        else if (TRUE == IsJPEGSnapshotStream(pStreamConfig->streams[i]))
        {
            isJPEG = TRUE;
        }
    }

    return (isRaw && isJPEG);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsYUVInBlobOutConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsYUVInBlobOutConfig(
    const camera3_stream_configuration_t* pStreamConfig)
{
    BOOL bInYUV     = FALSE;
    BOOL bBlobOut   = FALSE;
    UINT yuvOutCnt  = 0;

    for (UINT i = 0; i < pStreamConfig->num_streams; i++)
    {
        if (TRUE == IsYUVInStream(pStreamConfig->streams[i]))
        {
            bInYUV = TRUE;
        }
        else if (TRUE == IsJPEGSnapshotStream(pStreamConfig->streams[i]))
        {
            bBlobOut = TRUE;
        }
        else if (TRUE == IsYUVOutStream(pStreamConfig->streams[i]))
        {
            yuvOutCnt++;
        }
    }
    if (1 < yuvOutCnt)
    {
        bInYUV     = FALSE;
    }

    return ((bInYUV && bBlobOut) ? TRUE : FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsVideoLiveShotConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsVideoLiveShotConfig(
    const camera3_stream_configuration_t* pStreamConfig)
{
    BOOL bVideoStream   = FALSE;
    BOOL bBlobOut       = FALSE;
    BOOL bCntMatch      = FALSE;
    BOOL bHEIF          = FALSE;

    if (3 == pStreamConfig->num_streams)
    {
        bCntMatch = TRUE;

        for (UINT i = 0; i < pStreamConfig->num_streams; i++)
        {
            if (TRUE == IsJPEGSnapshotStream(pStreamConfig->streams[i]))
            {
                bBlobOut = TRUE;
            }
            else if (TRUE == IsVideoStream(pStreamConfig->streams[i]))
            {
                bVideoStream = TRUE;
                if(TRUE == IsHEIFStream(pStreamConfig->streams[i]))
                {
                    bHEIF = TRUE;
                }
            }
        }
    }

    return (((TRUE == bVideoStream) && (TRUE == bBlobOut) && (TRUE == bCntMatch) && (FALSE == bHEIF)) ? TRUE : FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsVideoEISV2Enabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsVideoEISV2Enabled(
    camera3_stream_configuration_t* pStreamConfig)
{
    BOOL bEISV2Enabled = FALSE;

    if (0 != VideoEISV2Usecase)
    {
        if (2 == VideoEISV2Usecase)
        {
            // Force EISv2 usecase
            pStreamConfig->operation_mode = pStreamConfig->operation_mode | StreamConfigModeQTIEISRealTime;
        }

        if ((pStreamConfig->operation_mode & StreamConfigModeQTIEISRealTime) == StreamConfigModeQTIEISRealTime)
        {
            bEISV2Enabled = TRUE;
        }
    }

    return bEISV2Enabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsVideoEISV3Enabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsVideoEISV3Enabled(
    camera3_stream_configuration_t* pStreamConfig)
{
    BOOL bEISV3Enabled = FALSE;

    if (0 != VideoEISV3Usecase)
    {
        if (2 == VideoEISV3Usecase)
        {
            // Force EISv3 usecase
            pStreamConfig->operation_mode = pStreamConfig->operation_mode | StreamConfigModeQTIEISLookAhead;
        }

        if ((pStreamConfig->operation_mode & StreamConfigModeQTIEISLookAhead) == StreamConfigModeQTIEISLookAhead)
        {
            bEISV3Enabled = TRUE;
        }
    }

    return bEISV3Enabled;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::MFNRMatchingUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::MFNRMatchingUsecase(
    const camera3_stream_configuration_t* pStreamConfig)
{
    CHX_ASSERT(2 == pStreamConfig->num_streams);

    BOOL isMatch                = FALSE;
    BOOL advanceProcessingMFNR  = TRUE;

    /// @todo add check for vendor tag or hint to enable MFNR flow
    if (TRUE == advanceProcessingMFNR)
    {
        isMatch = TRUE;
    }

    return isMatch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::MFSRMatchingUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::MFSRMatchingUsecase(
    const camera3_stream_configuration_t* pStreamConfig)
{
    CHX_ASSERT(2 == pStreamConfig->num_streams);

    BOOL isMatch = FALSE;
    BOOL advanceProcessingMFSR = TRUE;

    /// @todo add check for vendor tag or hint to enable MFNR flow
    if (TRUE == advanceProcessingMFSR)
    {
        isMatch = TRUE;
    }

    return isMatch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::QuadCFAMatchingUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::QuadCFAMatchingUsecase(
    const LogicalCameraInfo*              pCamInfo,
    const camera3_stream_configuration_t* pStreamConfig)
{
    BOOL isMatch         = FALSE;
    CHIRECT binning_size = { 0 };

    CHX_ASSERT(2 == pStreamConfig->num_streams);

    for (UINT i = 0; i < pCamInfo->m_cameraCaps.numSensorModes; i++)
    {
        if (1 == pCamInfo->pSensorModeInfo[i].sensorModeCaps.u.QuadCFA)
        {
            binning_size.width  = pCamInfo->pSensorModeInfo[i].frameDimension.width  >> 1;
            binning_size.height = pCamInfo->pSensorModeInfo[i].frameDimension.height >> 1;
            break;
        }
    }

    for (UINT stream = 0; stream < pStreamConfig->num_streams; stream++)
    {
        if (IsYUVSnapshotStream(pStreamConfig->streams[stream])  ||
            IsJPEGSnapshotStream(pStreamConfig->streams[stream]) ||
            IsHEIFStream(pStreamConfig->streams[stream]))
        {
            if (pStreamConfig->streams[stream]->width  > binning_size.width ||
                pStreamConfig->streams[stream]->height > binning_size.height)
            {
                // if jpge/yuv snapshot stream is larger than binning size, then select Quad CFA usecase
                // otherwise, treat it as a normal one.
                isMatch = TRUE;
                CHX_LOG("QuadCFA usecase, sensor binning size:%dx%d", binning_size.width, binning_size.height);
            }
            break;
        }
    }

    return isMatch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::IsQuadCFASensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsQuadCFASensor(
    const LogicalCameraInfo* pCamInfo,
    CHIREMOSAICTYPE*         pRemosaicType)
{
    BOOL            isQuadCFASensor = FALSE;
    CHIREMOSAICTYPE remosaicType    = CHIREMOSAICTYPE::UnKnown;

    for (UINT i = 0; i < pCamInfo->m_cameraCaps.numSensorModes; i++)
    {
        if (pCamInfo->pSensorModeInfo[i].sensorModeCaps.u.QuadCFA == 1)
        {
            isQuadCFASensor = TRUE;
            remosaicType    = pCamInfo->pSensorModeInfo[i].remosaictype;

            if (CHIREMOSAICTYPE::UnKnown == remosaicType)
            {
                // fallback to use SWRemosaic type
                remosaicType = CHIREMOSAICTYPE::SWRemosaic;
            }

            break;
        }
    }

    if ((TRUE == isQuadCFASensor) && (NULL != pRemosaicType))
    {
        *pRemosaicType = remosaicType;
    }

    return isQuadCFASensor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::GetMatchingUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseId UsecaseSelector::GetMatchingUsecase(
    const LogicalCameraInfo*        pCamInfo,
    camera3_stream_configuration_t* pStreamConfig)
{
    UsecaseId usecaseId = UsecaseId::Default;
    UINT32 VRDCEnable = ExtensionModule::GetInstance()->GetDCVRMode();
    if ((pStreamConfig->num_streams == 2) && IsQuadCFASensor(pCamInfo, NULL) &&
        (LogicalCameraType_Default == pCamInfo->logicalCameraType))
    {
        // need to validate preview size <= binning size, otherwise return error

        /// If snapshot size is less than sensor binning size, select defaut zsl usecase.
        /// Only if snapshot size is larger than sensor binning size, select QuadCFA usecase.
        /// Which means for snapshot in QuadCFA usecase,
        ///   - either do upscale from sensor binning size,
        ///   - or change sensor mode to full size quadra mode.
        if (TRUE == QuadCFAMatchingUsecase(pCamInfo, pStreamConfig))
        {
            usecaseId = UsecaseId::QuadCFA;
            CHX_LOG_CONFIG("Quad CFA usecase selected");
            return usecaseId;
        }
    }

    if (pStreamConfig->operation_mode == StreamConfigModeSuperSlowMotionFRC)
    {
        usecaseId = UsecaseId::SuperSlowMotionFRC;
        CHX_LOG_CONFIG("SuperSlowMotionFRC usecase selected");
        return usecaseId;
    }

    /// Reset the usecase flags
    VideoEISV2Usecase   = 0;
    VideoEISV3Usecase   = 0;
    GPURotationUsecase  = FALSE;
    GPUDownscaleUsecase = FALSE;

    if ((NULL != pCamInfo) && (pCamInfo->numPhysicalCameras > 1) && VRDCEnable)
    {
        CHX_LOG_CONFIG("MultiCameraVR usecase selected");
        usecaseId = UsecaseId::MultiCameraVR;
    }
    else if ((NULL != pCamInfo) && (pCamInfo->numPhysicalCameras > 1) && (pStreamConfig->num_streams > 1))
    {
        CHX_LOG_CONFIG("MultiCamera usecase selected");
        usecaseId = UsecaseId::MultiCamera;
    }
    else
    {
        SnapshotStreamConfig snapshotStreamConfig;
        CHISTREAM**          ppChiStreams = reinterpret_cast<CHISTREAM**>(pStreamConfig->streams);
        switch (pStreamConfig->num_streams)
        {
            case 2:
                if (TRUE == IsRawJPEGStreamConfig(pStreamConfig))
                {
                    CHX_LOG_CONFIG("Raw + JPEG usecase selected");
                    usecaseId = UsecaseId::RawJPEG;
                    break;
                }

                /// @todo Enable ZSL by setting overrideDisableZSL to FALSE
                if (FALSE == m_pExtModule->DisableZSL())
                {
                    if (TRUE == IsPreviewZSLStreamConfig(pStreamConfig))
                    {
                        usecaseId = UsecaseId::PreviewZSL;
                        CHX_LOG_CONFIG("ZSL usecase selected");
                    }
                }

                if(TRUE == m_pExtModule->UseGPURotationUsecase())
                {
                    CHX_LOG_CONFIG("GPU Rotation usecase flag set");
                    GPURotationUsecase = TRUE;
                }

                if (TRUE == m_pExtModule->UseGPUDownscaleUsecase())
                {
                    CHX_LOG_CONFIG("GPU Downscale usecase flag set");
                    GPUDownscaleUsecase = TRUE;
                }

                if (TRUE == m_pExtModule->EnableMFNRUsecase())
                {
                    if (TRUE == MFNRMatchingUsecase(pStreamConfig))
                    {
                        usecaseId = UsecaseId::MFNR;
                        CHX_LOG_CONFIG("MFNR usecase selected");
                    }
                }

                if (TRUE == m_pExtModule->EnableHFRNo3AUsecas())
                {
                    CHX_LOG_CONFIG("HFR without 3A usecase flag set");
                    HFRNo3AUsecase = TRUE;
                }

                break;

            case 3:
                VideoEISV2Usecase = m_pExtModule->EnableEISV2Usecase();
                VideoEISV3Usecase = m_pExtModule->EnableEISV3Usecase();
                if (FALSE == m_pExtModule->DisableZSL() && (TRUE == IsPreviewZSLStreamConfig(pStreamConfig)))
                {
                    usecaseId = UsecaseId::PreviewZSL;
                    CHX_LOG_CONFIG("ZSL usecase selected");
                }
                else if(TRUE == IsRawJPEGStreamConfig(pStreamConfig))
                {
                    CHX_LOG_CONFIG("Raw + JPEG usecase selected");
                    usecaseId = UsecaseId::RawJPEG;
                }
                else if((FALSE == IsVideoEISV2Enabled(pStreamConfig)) && (FALSE == IsVideoEISV3Enabled(pStreamConfig)) &&
                    (TRUE == IsVideoLiveShotConfig(pStreamConfig)) && (FALSE == m_pExtModule->DisableZSL()))
                {
                    CHX_LOG_CONFIG("Video With Liveshot, ZSL usecase selected");
                    usecaseId = UsecaseId::VideoLiveShot;
                }

                break;

            case 4:
                GetSnapshotStreamConfiguration(pStreamConfig->num_streams, ppChiStreams, snapshotStreamConfig);
                if ((SnapshotStreamType::HEIC == snapshotStreamConfig.type) && (NULL != snapshotStreamConfig.pRawStream))
                {
                    CHX_LOG_CONFIG("Raw + HEIC usecase selected");
                    usecaseId = UsecaseId::RawJPEG;
                }
                break;

            default:
                CHX_LOG_CONFIG("Default usecase selected");
                break;

        }
    }

    if (TRUE == ExtensionModule::GetInstance()->IsTorchWidgetUsecase())
    {
        CHX_LOG_CONFIG("Torch widget usecase selected");
        usecaseId = UsecaseId::Torch;
    }

    CHX_LOG_INFO("usecase ID:%d",usecaseId);
    return usecaseId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StringMapToIndex
///
/// @brief  Given a sorted list of strings and a search string, return the index + 1 of that string if it exists or 0
///
/// @param  ppStringMap     [IN]  The list of strings
/// @param  stringMapLength [IN]  The number of strings in ppStringMap
/// @param  pString         [IN]  The search string
/// @param  rIndex          [OUT] Reference to the output
///
/// @return CDKResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CDKResult StringMapToIndex(const CHAR** const ppStringMap,
                                  const UINT         stringMapLength,
                                  const CHAR*  const pString,
                                  UINT&              rIndex)
{
    auto cmp = [](const VOID* p1, const VOID* p2) -> INT
    {
        return strcmp(static_cast<const CHAR*>(p1), *static_cast<const CHAR* const *>(p2));
    };
    CDKResult result = CDKResultSuccess;

    if (NULL == ppStringMap || NULL == pString)
    {
        result = CDKResultEInvalidArg;
    }
    else
    {
        VOID* pSearchResult = bsearch(pString, ppStringMap, stringMapLength, sizeof(CHAR*), cmp);
        if (NULL == pSearchResult)
        {
            result = CDKResultENoSuch;
        }
        else
        {
            // The InvalidEnum has value of 0 so that 0 initialized structs are marked as invalid.
            // Add 1 to the index to get the correct enum value
            rIndex = (static_cast<const CHAR * const *>(pSearchResult) - ppStringMap) + 1;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::GetVariantGroup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VariantGroup UsecaseSelector::GetVariantGroup(const CHAR* pVariantName)
{
    CDKResult    result;
    VariantGroup group;

    result = StringMapToIndex(g_stringMapVariantGroup, CHX_ARRAY_SIZE(g_stringMapVariantGroup), pVariantName, group);
    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("No VariantGroup %s exists [Error Code: %u]",
                      ((NULL == pVariantName) ? "NULL" : pVariantName),
                      result);
        group = EVariantGroup::InvalidVariantGroup;
    }
    return group;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::GetVariantType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VariantType UsecaseSelector::GetVariantType(const CHAR* pVariantName)
{
    CDKResult   result;
    VariantType type;

    result = StringMapToIndex(g_stringMapVariantType, CHX_ARRAY_SIZE(g_stringMapVariantType), pVariantName, type);
    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("No VariantType %s exists [Error Code: %u]",
                      ((NULL == pVariantName) ? "NULL" : pVariantName),
                      result);
        type = EVariantType::InvalidVariantType;
    }
    return type;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::FreeUsecaseDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSelector::FreeUsecaseDescriptor(
    ChiUsecase* pUsecase)
{
    CDKResult result = CDKResultSuccess;
    if (NULL == pUsecase)
    {
        result = CDKResultEInvalidArg;
    }
    else if(FALSE == pUsecase->isOriginalDescriptor)
    {
        for (UINT j = 0; j < pUsecase->numPipelines; j++)
        {
            ChiPipelineTargetCreateDescriptor* pPipelineTargetDesc = &pUsecase->pPipelineTargetCreateDesc[j];
            ChiPipelineCreateDescriptor&       rPipelineCreateDesc = pPipelineTargetDesc->pipelineCreateDesc;
            for (UINT nodeIdx = 0; nodeIdx < rPipelineCreateDesc.numNodes; nodeIdx++)
            {
                CHINODE& rNode = rPipelineCreateDesc.pNodes[nodeIdx];
                CHX_FREE(rNode.nodeAllPorts.pInputPorts);
                CHX_FREE(rNode.nodeAllPorts.pOutputPorts);
            }
            for (UINT linkIdx = 0; linkIdx < rPipelineCreateDesc.numLinks; linkIdx++)
            {
                CHX_FREE(rPipelineCreateDesc.pLinks[linkIdx].pDestNodes);
            }
            CHX_FREE(rPipelineCreateDesc.pNodes);
            CHX_FREE(rPipelineCreateDesc.pLinks);
            CHX_FREE(pPipelineTargetDesc->sinkTarget.pTargetPortDesc);
            CHX_FREE(pPipelineTargetDesc->sourceTarget.pTargetPortDesc);
        }
        CHX_FREE(pUsecase->ppChiTargets);
        CHX_FREE(pUsecase->pPipelineTargetCreateDesc);
        CHX_FREE(pUsecase);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ShouldPrune
///
/// @brief  Utility function to see if the prune properties match the prune settings
///
/// @param  pPruneSettings   A list of unsorted prune settings
/// @param  pPruneProperties A list of prune properties sorted by ascending VariantGroup
///
/// @return False if all of the PruneVariants contained by pPruneProperties match any of the PruneVariants contained by
///         pPruneSettings.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ShouldPrune(const PruneSettings* pPruneSettings,
                 const PruneSettings* pPruneProperties)
{
    BOOL shouldPrune = FALSE;

    if ((NULL == pPruneProperties) || (NULL == pPruneProperties->pVariants) || // Properties is more likely NULL for
        (NULL == pPruneSettings)   || (NULL == pPruneSettings->pVariants))     // short-circutting
    {
        shouldPrune = FALSE;
    }
    else
    {
        VariantGroup        currGroup         = EVariantGroup::InvalidVariantGroup;
        BOOL                hasFoundMatch     = TRUE;
        const PruneVariant* pPrunableElements = pPruneProperties->pVariants;
        for (UINT i = 0; i < pPruneProperties->numSettings; i++)
        {
            const PruneVariant* const pPruneableElement = &pPrunableElements[i];
            if (pPruneableElement->group != currGroup)
            {
                if (FALSE == hasFoundMatch)
                {
                    break; // If nothing matches the previous group, then we should be pruned
                }
                else
                {
                    currGroup     = pPruneableElement->group;
                    hasFoundMatch = FALSE;
                }
            }
            else if (TRUE == hasFoundMatch)
            {
                continue; // Continue scanning for next group
            }

            for (UINT j = 0; j < pPruneSettings->numSettings; j++)
            {
                const PruneVariant* const pPruneSetting = &pPruneSettings->pVariants[j];
                if (pPruneSetting->group != pPruneableElement->group)
                {
                    continue; // Don't consider elements not belonging to the same group
                }
                else if (pPruneSetting->type == pPruneableElement->type)
                {
                    hasFoundMatch = TRUE;
                    break;
                }
            }
        }
        // After considering all the variant for this setting, if no match is found, then prune this element
        if (FALSE == hasFoundMatch)
        {
            shouldPrune = TRUE;
        }

    }
    return shouldPrune;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::PruneUsecaseByStreamConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSelector::PruneUsecaseByStreamConfig(
    const camera3_stream_configuration* pStreamConfig,
    const ChiUsecase*                   pUsecaseInputDescriptor,
    ChiUsecase**                        ppUsecaseOutputDescriptor)
{
    PruneVariant  variants[3];
    const UINT    numVariants = CHX_ARRAY_SIZE(variants);
    const UINT32& rOpMode = pStreamConfig->operation_mode;

    variants[0].group = UsecaseSelector::GetVariantGroup("EIS");
    variants[1].group = GetVariantGroup("Snapshot");
    variants[2].group = GetVariantGroup("ScreenGrab");
    variants[1].type  = GetVariantType(HasHeicSnapshotStream(pStreamConfig) ? "HEIC" : "JPEG");
    variants[0].type  = UsecaseSelector::GetVariantType(
        ((rOpMode & StreamConfigModeQTIEISLookAhead) == StreamConfigModeQTIEISLookAhead) ? "EISv3" :
        ((rOpMode & StreamConfigModeQTIEISRealTime) == StreamConfigModeQTIEISRealTime)  ? "EISv2" :
        "Disabled");
    variants[2].type  = UsecaseSelector::GetVariantType(
        ExtensionModule::GetInstance()->GetScreenGrabLiveShotScene() ? "SAT2IPECapture" : "Disabled");

    CDKResult result = PruneUsecaseDescriptor(pUsecaseInputDescriptor, numVariants, variants, ppUsecaseOutputDescriptor);


    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("Error pruning usecase: %s Code: %u", pUsecaseInputDescriptor->pUsecaseName, result);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::PruneUsecaseDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSelector::PruneUsecaseDescriptor(const ChiUsecase* const   pUsecase,
                                                  const UINT                numPruneVariants,
                                                  const PruneVariant* const pPruneVariants,
                                                  ChiUsecase**              ppPrunedUsecase)
{
    PruneSettings pruneSettings;
    pruneSettings.numSettings    = numPruneVariants;
    pruneSettings.pVariants      = pPruneVariants;
    CDKResult     result         = CDKResultSuccess;
    ChiUsecase*   pPrunedUsecase = NULL;

    auto AddTargetInfo = [&pPrunedUsecase](const ChiTargetPortDescriptor* pTargetCreateDesc,
                                           ChiTargetPortDescriptorInfo&   rTargetPrunedInfo)
    {
        BOOL                     isTargetInList    = FALSE;
        ChiTargetPortDescriptor& rTargetPrunedDesc = rTargetPrunedInfo.pTargetPortDesc[rTargetPrunedInfo.numTargets++];
        rTargetPrunedDesc                          = *pTargetCreateDesc;
        CHX_LOG_VERBOSE("Adding target info: %s [%u] <- %u:%u:%u",
                        rTargetPrunedDesc.pTargetName,
                        rTargetPrunedInfo.numTargets - 1,
                        pTargetCreateDesc->pNodePort[0].nodeId,
                        pTargetCreateDesc->pNodePort[0].nodeInstanceId,
                        pTargetCreateDesc->pNodePort[0].nodePortId);

        for (UINT targetListIdx = 0; targetListIdx < pPrunedUsecase->numTargets; targetListIdx++)
        {
            if (pPrunedUsecase->ppChiTargets[targetListIdx] == rTargetPrunedDesc.pTarget)
            {
                isTargetInList = TRUE;
                break;
            }
        }
        if (FALSE == isTargetInList)
        {
            pPrunedUsecase->ppChiTargets[pPrunedUsecase->numTargets++] = rTargetPrunedDesc.pTarget;
        }
    };

    auto UpdateTargetInfo = [&AddTargetInfo, &pUsecase, &result, &pruneSettings] (
        const ChiLinkNodeDescriptor&       rLinkNodeDesc,
        const ChiTargetPortDescriptorInfo& rTargetCreateInfo,
        ChiTargetPortDescriptorInfo&       rTargetPrunedInfo,
        const PruneSettings*               pPruneProperties)
    {
        if (CDKResultSuccess != result)
        {
            return;
        }
        auto IsSameDescriptor = [](const ChiLinkNodeDescriptor& rDescA,
                                   const ChiLinkNodeDescriptor& rDescB)
        {
            return ((rDescA.nodeId         == rDescB.nodeId) &&
                    (rDescA.nodeInstanceId == rDescB.nodeInstanceId) &&
                    (rDescA.nodePortId     == rDescB.nodePortId));
        };
        const ChiTargetPortDescriptor* pTargetCreateDesc = NULL;
        for (UINT i = 0; i < rTargetCreateInfo.numTargets; i++)
        {
            const ChiTargetPortDescriptor& rTargetCreateDesc = rTargetCreateInfo.pTargetPortDesc[i];
            if (TRUE == IsSameDescriptor(rTargetCreateDesc.pNodePort[0], rLinkNodeDesc))
            {
                BOOL acceptTarget = TRUE;
                if (NULL == pPruneProperties->pVariants)
                {
                    CHX_LOG_VERBOSE("%s no target prune properties found on link - Accept: %s",
                                    pUsecase->pUsecaseName,
                                    rTargetCreateDesc.pTargetName);
                }
                else if (NULL == pUsecase->pTargetPruneSettings)
                {
                    // If no prune settings exist for the target, then blindly take it
                    CHX_LOG_VERBOSE("%s has no target prune settings - Accept: %s",
                                    pUsecase->pUsecaseName,
                                    rTargetCreateDesc.pTargetName);
                }
                else
                {
                    acceptTarget = FALSE;
                    for (UINT j = 0; j < pUsecase->numTargets; j++)
                    {
                        if (rTargetCreateDesc.pTarget == pUsecase->ppChiTargets[j])
                        {
                            if (FALSE == ShouldPrune(&pruneSettings, &pUsecase->pTargetPruneSettings[j]))
                            {
                                acceptTarget = TRUE;
                                CHX_LOG_VERBOSE("Accepting target %u - %s", j, rTargetCreateDesc.pTargetName);
                            }
                            else
                            {
                                CHX_LOG_INFO("Rejecting target %u - %s", j, rTargetCreateDesc.pTargetName);
                            }
                            break;
                        }
                    }
                }
                if (TRUE == acceptTarget)
                {
                    pTargetCreateDesc = &rTargetCreateDesc;
                    break;
                }
            }
            else
            {
                CHX_LOG_VERBOSE("%s - %u:%u:%u does not match CreateDesc %u:%u:%u",
                                rTargetCreateDesc.pTargetName,
                                rTargetCreateDesc.pNodePort[0].nodeId,
                                rTargetCreateDesc.pNodePort[0].nodeInstanceId,
                                rTargetCreateDesc.pNodePort[0].nodePortId,
                                rLinkNodeDesc.nodeId,
                                rLinkNodeDesc.nodeInstanceId,
                                rLinkNodeDesc.nodePortId);
            }
        }

        if (NULL == pTargetCreateDesc)
        {
            result = CDKResultEInvalidState;
            CHX_LOG_ERROR("pTargetCreateDesc is null for link connecting to: %u:%u:%u",
                          rLinkNodeDesc.nodeId,
                          rLinkNodeDesc.nodeInstanceId,
                          rLinkNodeDesc.nodePortId);
        }
        else
        {
            AddTargetInfo(pTargetCreateDesc, rTargetPrunedInfo);
        }
    };

    if ((NULL == pUsecase) || ((NULL == pPruneVariants) && (numPruneVariants > 0)) || (NULL == ppPrunedUsecase))
    {
        result = CDKResultEInvalidArg;
        CHX_LOG_ERROR("Invalid arguments: pUsecase: %p pPruneVariants: %p ppPrunedUsecase: %p",
                      pUsecase, pPruneVariants, ppPrunedUsecase);
    }
    else
    {
        ChiPipelineTargetCreateDescriptor* pPrunedTargetCreateDesc;
        ChiTarget**                        ppPrunedChiTargets;
        pPrunedUsecase          = static_cast<ChiUsecase*>(CHX_CALLOC(1 * sizeof(ChiUsecase)));
        pPrunedTargetCreateDesc = static_cast<ChiPipelineTargetCreateDescriptor*>(
            CHX_CALLOC(pUsecase->numPipelines * sizeof(ChiPipelineTargetCreateDescriptor)));
        ppPrunedChiTargets      = static_cast<ChiTarget**>(CHX_CALLOC(pUsecase->numTargets * sizeof(ChiTarget*)));
        // Shallow Copy Usecase Information
        if ((NULL != pPrunedUsecase) && (NULL != pPrunedTargetCreateDesc) && (NULL != ppPrunedChiTargets))
        {
            pPrunedUsecase->pUsecaseName              = pUsecase->pUsecaseName;
            pPrunedUsecase->streamConfigMode          = pUsecase->streamConfigMode;
            pPrunedUsecase->numPipelines              = pUsecase->numPipelines;
            pPrunedUsecase->pPipelineTargetCreateDesc = pPrunedTargetCreateDesc;
            pPrunedUsecase->ppChiTargets              = ppPrunedChiTargets;
            *ppPrunedUsecase                          = pPrunedUsecase;
        }
        else
        {
            CHX_LOG_ERROR("Out of memory");
            result = CDKResultENoMemory;
        }
    }

    if (NULL != pUsecase)
    {
        // Deep copy prunable information
        for (UINT j = 0; j < pUsecase->numPipelines; j++)
        {
            if (CDKResultSuccess != result)
            {
                break; // Since allocation can fail at anypoint, check the result before continuing
            }

            constexpr UINT NODE_ID_SOURCE_BUFFER  = 4;
            constexpr UINT NODE_ID_SINK_BUFFER    = 2;
            constexpr UINT NODE_ID_SINK_NO_BUFFER = 3;

            const ChiPipelineTargetCreateDescriptor& rTargetCreateDesc = pUsecase->pPipelineTargetCreateDesc[j];
            if (NULL == pPrunedUsecase)
            {
                CHX_LOG("ERROR: no memory!");
                return  CDKResultEInvalidPointer;
            }

            ChiPipelineTargetCreateDescriptor&       rTargetPrunedDesc = pPrunedUsecase->pPipelineTargetCreateDesc[j];
            const ChiPipelineCreateDescriptor&       rCreateDesc       = rTargetCreateDesc.pipelineCreateDesc;
            ChiPipelineCreateDescriptor&             rPrunedDesc       = rTargetPrunedDesc.pipelineCreateDesc;

            auto GetNodeDesc = [](const ChiLinkNodeDescriptor& rCreateDesc, const ChiPipelineCreateDescriptor& rDesc) -> ChiNode*
            {
                ChiNode* pChiNode = NULL;
                for (UINT nodeIdx = 0; nodeIdx < rDesc.numNodes; nodeIdx++)
                {
                    ChiNode& rNodeDesc = rDesc.pNodes[nodeIdx];
                    if ((rCreateDesc.nodeId == rNodeDesc.nodeId) && (rCreateDesc.nodeInstanceId == rNodeDesc.nodeInstanceId))
                    {
                        pChiNode = &rNodeDesc;
                        break;
                    }
                }
                return pChiNode;
            };

            auto AddInputPortInfo = [] (CHINODEPORTS&                rDestPortInfo,
                                        const ChiLinkNodeDescriptor& rLinkDesc,
                                        const BOOL                   isInputStream) -> VOID
            {
                CHIINPUTPORTDESCRIPTOR& inputPortInfo = rDestPortInfo.pInputPorts[rDestPortInfo.numInputPorts++];
                inputPortInfo.portId                  = rLinkDesc.nodePortId;
                inputPortInfo.isInputStreamBuffer     = isInputStream;
                inputPortInfo.portSourceTypeId        = rLinkDesc.portSourceTypeId;
            };

            rPrunedDesc.isRealTime = rCreateDesc.isRealTime;
            if ((rCreateDesc.numNodes < 0) || (rCreateDesc.numLinks < 0))
            {
                CHX_LOG_ERROR("ERROR: numNodes or numLinks are less than zero");
                return CDKResultEInvalidArg;
            }
            rPrunedDesc.pNodes     = static_cast<CHINODE*>(CHX_CALLOC(rCreateDesc.numNodes * sizeof(CHINODE)));
            rPrunedDesc.pLinks     = static_cast<CHINODELINK*>(CHX_CALLOC(rCreateDesc.numLinks * sizeof(CHINODELINK)));

            rTargetPrunedDesc.pPipelineName              = rTargetCreateDesc.pPipelineName;
            rTargetPrunedDesc.sinkTarget.pTargetPortDesc = static_cast<ChiTargetPortDescriptor*>(
                CHX_CALLOC(rTargetCreateDesc.sinkTarget.numTargets * sizeof(ChiTargetPortDescriptor)));
            rTargetPrunedDesc.sourceTarget.pTargetPortDesc = static_cast<ChiTargetPortDescriptor*>(
                CHX_CALLOC(rTargetCreateDesc.sourceTarget.numTargets * sizeof(ChiTargetPortDescriptor)));

            if ((NULL == rPrunedDesc.pNodes) ||
                (NULL == rPrunedDesc.pLinks) ||
                (NULL == rTargetPrunedDesc.sinkTarget.pTargetPortDesc) ||
                (NULL == rTargetPrunedDesc.sourceTarget.pTargetPortDesc))
            {
                CHX_LOG_ERROR("Out of memory");
                result = CDKResultENoMemory;
                break;
            }
            // Find non-pruned nodes
            for (UINT nodeIdx = 0; nodeIdx < rCreateDesc.numNodes; nodeIdx++)
            {
                const CHINODE& rNode = rCreateDesc.pNodes[nodeIdx];
                // Add this node to the final descriptor only if we shouldn't prune it
                if (TRUE == ShouldPrune(&pruneSettings, &rNode.pruneProperties))
                {
                    CHX_LOG_INFO("%s - Pruning Node: %u:%u", rTargetCreateDesc.pPipelineName, rNode.nodeId, rNode.nodeInstanceId);
                    continue;
                }
                // Append this node's info to the end of the pNodes
                CHINODE&      rNewNode  = rPrunedDesc.pNodes[rPrunedDesc.numNodes++];
                CHINODEPORTS& rNewPorts = rNewNode.nodeAllPorts;

                // Shallow copy simple node information
                rNewNode.nodeId          = rNode.nodeId;
                rNewNode.nodeInstanceId  = rNode.nodeInstanceId;
                rNewNode.pNodeProperties = rNode.pNodeProperties;
                rNewNode.numProperties   = rNode.numProperties;

                // Allocate space for prunable-port information

                rNewPorts.pInputPorts  = static_cast<ChiInputPortDescriptor*>(
                    CHX_CALLOC(rNode.nodeAllPorts.numInputPorts * sizeof(ChiInputPortDescriptor)));
                rNewPorts.pOutputPorts = static_cast<ChiOutputPortDescriptor*>(
                    CHX_CALLOC(rNode.nodeAllPorts.numOutputPorts * sizeof(ChiOutputPortDescriptor)));
                if ((NULL == rNewPorts.pInputPorts) || (NULL == rNewPorts.pOutputPorts))
                {
                    CHX_LOG_ERROR("Out of memory");
                    result = CDKResultENoMemory;
                    break;
                }
            }
            if (CDKResultSuccess != result)
            {
                break; // Since allocation can fail at anypoint, check the result before continuing
            }
            // Find non-pruned links
            for (UINT linkIdx = 0; linkIdx < rCreateDesc.numLinks; linkIdx++)
            {
                const CHINODELINK&           rLinkCreateDesc    = rCreateDesc.pLinks[linkIdx];
                CHINODELINK&                 rLinkPrunedDesc    = rPrunedDesc.pLinks[rPrunedDesc.numLinks];
                const ChiLinkNodeDescriptor& rSrcNodeCreateDesc = rLinkCreateDesc.srcNode;
                ChiNode*                     pSrcNode           = NULL;
                CHIOUTPUTPORTDESCRIPTOR*     pSrcOutputDesc     = NULL;
                // Check whether the SrcNode has been pruned.
                // Source Buffers are assumed to be non-prunable
                if (NODE_ID_SOURCE_BUFFER != rSrcNodeCreateDesc.nodeId)
                {
                    pSrcNode = GetNodeDesc(rSrcNodeCreateDesc, rPrunedDesc);
                    if (NULL == pSrcNode)
                    {
                        CHX_LOG_VERBOSE("%s - Source Has been pruned: Src[%u:%u:%u]",
                                        rTargetCreateDesc.pPipelineName,
                                        rSrcNodeCreateDesc.nodeId,
                                        rSrcNodeCreateDesc.nodeInstanceId,
                                        rSrcNodeCreateDesc.nodePortId);
                        continue; // Skip links with pruned sources
                    }
                    pSrcOutputDesc                   = &pSrcNode->nodeAllPorts.pOutputPorts[pSrcNode->nodeAllPorts.numOutputPorts];
                    pSrcOutputDesc->portId           = rSrcNodeCreateDesc.nodePortId;
                    pSrcOutputDesc->portSourceTypeId = rSrcNodeCreateDesc.portSourceTypeId;
                }
                // For each of the link's destination nodes
                // Update the non-pruned Destination Node's Info and update target destination information
                for (UINT destIdx = 0; destIdx < rLinkCreateDesc.numDestNodes; destIdx++)
                {
                    const ChiLinkNodeDescriptor& rDestNodeCreateDesc = rLinkCreateDesc.pDestNodes[destIdx];
                    ChiNode*                     pDestNode           = NULL;

                    if (TRUE == ShouldPrune(&pruneSettings, &rDestNodeCreateDesc.pruneProperties))
                    {
                        CHX_LOG_INFO("%s - Pruning Dst Port: Src[%u:%u:%u] -> Dst[%u:%u:%u]",
                                    rTargetCreateDesc.pPipelineName,
                                    rSrcNodeCreateDesc.nodeId,
                                    rSrcNodeCreateDesc.nodeInstanceId,
                                    rSrcNodeCreateDesc.nodePortId,
                                    rDestNodeCreateDesc.nodeId,
                                    rDestNodeCreateDesc.nodeInstanceId,
                                    rDestNodeCreateDesc.nodePortId);
                        continue; // This destination will be pruned. Check the next dest
                    }
                    else
                    {
                        switch (rDestNodeCreateDesc.nodeId)
                        {
                            case NODE_ID_SINK_BUFFER:
                            case NODE_ID_SINK_NO_BUFFER:
                                break;
                            default:
                                // Check if the destination node was pruned
                                pDestNode = GetNodeDesc(rDestNodeCreateDesc, rPrunedDesc);
                                if (NULL == pDestNode)
                                {
                                    CHX_LOG_VERBOSE("%s - Cannot Link: %u:%u:%u -> %u:%u:%u (DstNode was pruned)",
                                                    rTargetCreateDesc.pPipelineName,
                                                    rSrcNodeCreateDesc.nodeId,
                                                    rSrcNodeCreateDesc.nodeInstanceId,
                                                    rSrcNodeCreateDesc.nodePortId,
                                                    rDestNodeCreateDesc.nodeId,
                                                    rDestNodeCreateDesc.nodeInstanceId,
                                                    rDestNodeCreateDesc.nodePortId);
                                    continue; // This destination has been pruned. Check the next dest
                                }
                                break;
                        }
                    }

                    // If control has reached this point, we assume that this destination will not be pruned
                    if (0 == rLinkPrunedDesc.numDestNodes)
                    {
                        rLinkPrunedDesc.pDestNodes = static_cast<ChiLinkNodeDescriptor*>(
                            CHX_CALLOC(rLinkCreateDesc.numDestNodes * sizeof(ChiLinkNodeDescriptor)));
                        if (NULL == rLinkPrunedDesc.pDestNodes)
                        {
                            CHX_LOG_ERROR("Out of memory");
                            result = CDKResultENoMemory;
                            break;
                        }
                    }

                    // Copy destination information for Link Descriptor
                    rLinkPrunedDesc.pDestNodes[rLinkPrunedDesc.numDestNodes++] = rDestNodeCreateDesc;

                    // Update the Dest Node's descriptor
                    switch (rDestNodeCreateDesc.nodeId)
                    {
                        case NODE_ID_SINK_BUFFER:
                            // Add to target list if it doesnt already exist
                            UpdateTargetInfo(rSrcNodeCreateDesc,
                                            rTargetCreateDesc.sinkTarget,
                                            rTargetPrunedDesc.sinkTarget,
                                            &rDestNodeCreateDesc.pruneProperties);
                            break;
                        case NODE_ID_SINK_NO_BUFFER:
                            break; // Nothing to do for sink no buffer destinations
                        default:
                            AddInputPortInfo(pDestNode->nodeAllPorts, rDestNodeCreateDesc, FALSE);
                            break;
                    }
                    // Update the Src Node's descriptor
                    // Since pruning can affect how these flags are assigned, we need to rederive them here
                    if (NULL != pSrcOutputDesc)
                    {
                        switch (rDestNodeCreateDesc.nodeId)
                        {
                            case NODE_ID_SINK_BUFFER:
                                pSrcOutputDesc->isOutputStreamBuffer = TRUE;
                                pSrcOutputDesc->isSinkPort           = TRUE;
                                break;
                            case NODE_ID_SINK_NO_BUFFER:
                                pSrcOutputDesc->isSinkPort = TRUE;
                                break;
                            default:
                                break;
                        }
                    }
                } // end of for (UINT destIdx = 0; destIdx < rLinkCreateDesc.numDestNodes; destIdx++)

                // If this link has any destinations, then it hasn't been pruned
                if ((CDKResultSuccess == result) && (rLinkPrunedDesc.numDestNodes > 0))
                {
                    rLinkPrunedDesc.srcNode          = rSrcNodeCreateDesc;
                    rLinkPrunedDesc.bufferProperties = rLinkCreateDesc.bufferProperties;
                    rLinkPrunedDesc.linkProperties   = rLinkCreateDesc.linkProperties;
                    rPrunedDesc.numLinks++;
                    if (NULL != pSrcNode)
                    {
                        // If this Node is a bypassable node, then copy the bypass mapping
                        for (UINT i = 0; i < pSrcNode->numProperties; i++)
                        {
                            const CHINODEPROPERTY& rProp = pSrcNode->pNodeProperties[i];
                            // If NodeClass is Bypass
                            if ((11 == rProp.id) && ('1' == *(static_cast<const CHAR*>(rProp.pValue))))
                            {
                                // The pointer returned by GetNodeDesc can never be NULL b/c pSrcNode != NULL
                                const CHINODEPORTS& rCreatePorts = GetNodeDesc(rSrcNodeCreateDesc, rCreateDesc)->nodeAllPorts;

                                if (NULL == pSrcOutputDesc)
                                {
                                CHX_LOG_ERROR("ERROR: no memory!");
                                return CDKResultEInvalidPointer;
                                }

                                for (UINT portIdx = 0; portIdx < rCreatePorts.numOutputPorts; portIdx++)
                                {
                                    const CHIOUTPUTPORTDESCRIPTOR& rOutputPortDesc = rCreatePorts.pOutputPorts[portIdx];
                                    if (rOutputPortDesc.portId == pSrcOutputDesc->portId)
                                    {
                                    pSrcOutputDesc->numSourceIdsMapped = rOutputPortDesc.numSourceIdsMapped;
                                    pSrcOutputDesc->pMappedSourceIds   = rOutputPortDesc.pMappedSourceIds;
                                    break; // Exit output port loop
                                    }
                                }
                                break; // Exit property loop
                            }
                        }
                        pSrcNode->nodeAllPorts.numOutputPorts++;
                    }
                }
            } // end of for (UINT linkIdx = 0; linkIdx < rPipelineCreateDesc.numLinks; linkIdx++)

            // Find source buffers
            for (UINT sourceIdx = 0; sourceIdx < rTargetCreateDesc.sourceTarget.numTargets; sourceIdx++)
            {
                const ChiTargetPortDescriptor& rSrcBufferDesc = rTargetCreateDesc.sourceTarget.pTargetPortDesc[sourceIdx];
                ChiNode* const                 pDestNode      = GetNodeDesc(rSrcBufferDesc.pNodePort[0], rPrunedDesc);

                if (NULL != pDestNode)
                {
                    AddTargetInfo(&rSrcBufferDesc, rTargetPrunedDesc.sourceTarget);
                    AddInputPortInfo(pDestNode->nodeAllPorts, rSrcBufferDesc.pNodePort[0], TRUE);
                }
                else
                {
                    result = CDKResultEInvalidState; // This condition should be unreachable
                }
            } // end of for (UINT sourceIdx = 0; sourceIdx < rPipelineTargetCreateDesc.sourceTarget.numTargets; sourceIdx++)
            CHX_LOG_VERBOSE("%s - Create [Src: %u Sink: %u] Pruned [Src: %u Sink: %u]",
                            rTargetCreateDesc.pPipelineName,
                            rTargetCreateDesc.sourceTarget.numTargets,
                            rTargetCreateDesc.sinkTarget.numTargets,
                            rTargetPrunedDesc.sourceTarget.numTargets,
                            rTargetPrunedDesc.sinkTarget.numTargets);
        } // endof for (UINT j = 0; j < pUsecase->numPipelines; j++)
    }
    else
    {
        result = CDKResultEInvalidArg;
        CHX_LOG_ERROR("Invalid arguments: pUsecase: %p", pUsecase);
    }


    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("Error: %u", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::DefaultMatchingUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiUsecase* UsecaseSelector::DefaultMatchingUsecase(
    camera3_stream_configuration_t* pStreamConfig)
{
    ChiUsecase*        pSelectedUsecase   = NULL;
    ChiTargetUsecases* pChiTargetUsecases = NULL;
    BOOL               secureMode         = FALSE;
    UINT32             disableBit         = 0;  // Disables FOVC/VideoHDR bit from operation_mode for UseCase selection in EIS usecase.
    UINT32             videoWidth         = 0;
    UINT32             videoHeight        = 0;
    BOOL               hasHeicStream      = HasHeicSnapshotStream(pStreamConfig);

    PruneSettings pruneSettings;
    PruneVariant  variants[2];
    pruneSettings.numSettings = 0;
    pruneSettings.pVariants   = variants;

    auto AddSetting = [&pruneSettings, &variants](const CHAR* pGroup, const CHAR* pType) -> VOID
    {
        VariantGroup group = GetVariantGroup(pGroup);
        VariantType  type  = GetVariantType(pType);
        if ((EVariantGroup::InvalidVariantGroup != group) && (EVariantType::InvalidVariantType != type))
        {
            CHX_LOG_INFO("Adding prune setting #%u - %s = %s", pruneSettings.numSettings, pGroup, pType);
            PruneVariant* pVariant = &variants[pruneSettings.numSettings++];
            pVariant->group        = group;
            pVariant->type         = type;
        }
        else
        {
            CHX_LOG_WARN("Invalid Prune Setting - Group: %s(%u) Setting: %s(%u)", pGroup, group, pType, type);
        }
    };

    auto UsecaseMatches = [&pStreamConfig, &pruneSettings](const ChiUsecase* const pUsecase) -> BOOL
    {
        return IsMatchingUsecase(pStreamConfig, pUsecase, &pruneSettings);
    };


    for (UINT32 stream = 0; stream < pStreamConfig->num_streams; stream++)
    {
        const camera3_stream_t* pStream = pStreamConfig->streams[stream];
        if (0 != (pStream->usage & GRALLOC_USAGE_PROTECTED))
        {
            secureMode = TRUE;
            break;
        }
        if (TRUE == IsVideoStream(pStreamConfig->streams[stream]))
        {
            videoWidth  = pStreamConfig->streams[stream]->width;
            videoHeight = pStreamConfig->streams[stream]->height;
        }
    }

    AddSetting("Snapshot", (TRUE == hasHeicStream) ? "HEIC" : "JPEG");

    UINT numTargetsExpected = ChiMaxNumTargets + 1;

    if (pStreamConfig->num_streams <= ChiMaxNumTargets)
    {
        if (((TRUE == GPURotationUsecase) || (TRUE == GPUDownscaleUsecase)) &&
            (StreamConfigModeNormal == pStreamConfig->operation_mode))
        {
            CHX_LOG("Only selecting GPU usecases");
            switch (pStreamConfig->num_streams)
            {
            case 2:
                if (TRUE == UsecaseMatches(&Usecases2Target[UsecaseJPEGEncodePreviewSnapshotGPUId]))
                {
                    pSelectedUsecase = &Usecases2Target[UsecaseJPEGEncodePreviewSnapshotGPUId];
                }
                break;
            case 3:
                if (TRUE == UsecaseMatches(&Usecases3Target[UsecaseJPEGEncodeLiveSnapshotGPUId]))
                {
                    pSelectedUsecase = &Usecases3Target[UsecaseJPEGEncodeLiveSnapshotGPUId];
                }
                break;
            default:
                break;
            }
            if (NULL == pSelectedUsecase)
            {
                CHX_LOG("Failed to match with a GPU usecase");
            }
        }
        if ((TRUE == HFRNo3AUsecase)
                && (StreamConfigModeConstrainedHighSpeed == pStreamConfig->operation_mode) && (NULL == pSelectedUsecase))
        {
            if (TRUE == UsecaseMatches(&Usecases2Target[UsecaseVideoHFRNo3AId]))
            {
                pSelectedUsecase = &Usecases2Target[UsecaseVideoHFRNo3AId];

                CHX_LOG("UsecaseVideoHFRNo3A is selected for HFR");
            }
        }
        else if ((TRUE == secureMode) && (NULL == pSelectedUsecase))
        {
            CHX_LOG("Only selecting Secure usecases %d", pStreamConfig->num_streams);
            switch (pStreamConfig->num_streams)
            {
            case 1:
                if (TRUE == UsecaseMatches(&Usecases1Target[UsecaseSecurePreviewId]))
                {
                    pSelectedUsecase = &Usecases1Target[UsecaseSecurePreviewId];
                }
                else if (TRUE == UsecaseMatches(&Usecases1Target[UsecaseRawId]))
                {
                    pSelectedUsecase = &Usecases1Target[UsecaseRawId];
                }
                else if (TRUE == UsecaseMatches(&Usecases1Target[UsecaseDepth1Id]))
                {
                    pSelectedUsecase = &Usecases1Target[UsecaseDepth1Id];
                }
                break;
            case 2:
                if (TRUE == UsecaseMatches(&Usecases2Target[UsecaseSecurePreviewVideoId]))
                {
                    pSelectedUsecase = &Usecases2Target[UsecaseSecurePreviewVideoId];
                }
                else if (TRUE == UsecaseMatches(&Usecases2Target[UsecaseDepth2Id]))
                {
                    pSelectedUsecase = &Usecases2Target[UsecaseDepth2Id];
                }
                break;
            default:
                break;
            }
            if (NULL == pSelectedUsecase)
            {
                CHX_LOG("Failed to match with a Secure usecase");
            }
        }
        else if (NULL == pSelectedUsecase)
        {
            if ((pStreamConfig->operation_mode & StreamConfigModeQTIFOVC) == StreamConfigModeQTIFOVC)
            {
                disableBit |= StreamConfigModeQTIFOVC ^ StreamConfigModeQTIStart;
                pStreamConfig->operation_mode &= ~(disableBit); // Disable FOVC bit for proper usecase selection
            }
            if ((pStreamConfig->operation_mode & StreamConfigModeVideoHdr) == StreamConfigModeVideoHdr)
            {
                disableBit |= StreamConfigModeVideoHdr ^ StreamConfigModeQTIStart;
                pStreamConfig->operation_mode &= ~(disableBit); // Disable VideoHDR bit for proper usecase selection
            }

            if (TRUE == IsVideoEISV3Enabled(pStreamConfig))
            {
                UINT32 usecaseEIS3Id = UsecaseVideoEIS3PreviewEIS2Id; // Default EIS3 usecase
                if ((UHDResolutionWidth <= videoWidth) && (UHDResolutionHeight <= videoHeight))
                {
                    usecaseEIS3Id = UsecaseVideo4kEIS3PreviewEIS2Id; // If video resolution is UHD, use 4k EIS usecase
                }

                if (TRUE == UsecaseMatches(&Usecases3Target[usecaseEIS3Id]))
                {
                    CHX_LOG("Selected EISv3 usecase");
                    pSelectedUsecase = &Usecases3Target[usecaseEIS3Id];
                }
            }

            if ((TRUE == IsVideoEISV2Enabled(pStreamConfig)) && (NULL == pSelectedUsecase) &&
                (TRUE == UsecaseMatches(&Usecases3Target[UsecaseVideoEIS2PreviewEIS2Id])))
            {
                CHX_LOG("Selected EISv2 usecase");
                pSelectedUsecase = &Usecases3Target[UsecaseVideoEIS2PreviewEIS2Id];
            }

            if (NULL == pSelectedUsecase)
            {
                if ((StreamConfigModeQTIEISLookAhead == pStreamConfig->operation_mode) ||
                    (StreamConfigModeQTIEISRealTime == pStreamConfig->operation_mode))
                {
                    // EIS is disabled, ensure that operation_mode is also set accordingly
                    pStreamConfig->operation_mode = 0;
                }
                numTargetsExpected = pStreamConfig->num_streams - 1;
            }

            if (disableBit != 0)
            {
                pStreamConfig->operation_mode |= disableBit; // reset the operation_mode bit, if disable bit set
            }
        }
    }

    for (/*Initialized outside*/; numTargetsExpected < ChiMaxNumTargets; numTargetsExpected++)
    {
        pChiTargetUsecases = &PerNumTargetUsecases[numTargetsExpected];
        if (0 == pChiTargetUsecases->numUsecases)
        {
            continue;
        }
        // this check is introduced to mask FOVC/VideoHDR operation mode before selecting a Usecase, as alone FOVC/VideoHDR usecase is not
        // introduced in xml yet. And makes sure that if FOVC/VideoHDR is set in operation mode it will continue for all video
        // recording cases.
        if ((pStreamConfig->operation_mode & StreamConfigModeQTIFOVC) == StreamConfigModeQTIFOVC)
        {
            disableBit |= StreamConfigModeQTIFOVC;
        }
        if ((pStreamConfig->operation_mode & StreamConfigModeVideoHdr) == StreamConfigModeVideoHdr)
        {
            disableBit |= StreamConfigModeVideoHdr;
        }
        pStreamConfig->operation_mode = pStreamConfig->operation_mode ^ disableBit;

        CHX_LOG_INFO("Considering %u usecases that have %u targets", pChiTargetUsecases->numUsecases, numTargetsExpected + 1);
        for (UINT i = 0; i < pChiTargetUsecases->numUsecases; i++)
        {
            const ChiUsecase* pChiUsecaseDescriptor = &pChiTargetUsecases->pChiUsecases[i];
            if ((pChiUsecaseDescriptor == g_pUsecaseJPEGEncodeLiveSnapshotGPU) ||
                (pChiUsecaseDescriptor == g_pUsecaseJPEGEncodePreviewSnapshotGPU))
            {
                if ((FALSE == GPURotationUsecase) && (FALSE == GPUDownscaleUsecase))
                {
                    CHX_LOG_VERBOSE("Will not select GPU Usecase: %s", pChiUsecaseDescriptor->pUsecaseName);
                    continue; // Prevent selecting GPU usecase if above flags are disabled
                }
            }
            if (TRUE == UsecaseMatches(pChiUsecaseDescriptor))
            {
                CDKResult res = PruneUsecaseDescriptor(pChiUsecaseDescriptor,
                                                       pruneSettings.numSettings,
                                                       pruneSettings.pVariants,
                                                       &pSelectedUsecase);
                CHX_LOG("selected use case index:%d", i);
                break;
            }
        }
        if (disableBit != 0)
        {
            pStreamConfig->operation_mode |= disableBit; // reset the operation_mode bit, if disable bit set
        }

        if (NULL != pSelectedUsecase)
        {
            break;
        }

    }

    if (NULL != pSelectedUsecase)
    {
        CHX_LOG_INFO("usecase %s, pipelineName %s",
            pSelectedUsecase->pUsecaseName, pSelectedUsecase->pPipelineTargetCreateDesc->pPipelineName);
    }
    else
    {
        CHX_LOG_ERROR("Failed to match usecase. pSelectedUsecase is NULL");
    }

    return pSelectedUsecase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSelector::IsMatchingUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsMatchingUsecase(
    const camera3_stream_configuration_t* pStreamConfig,
    const ChiUsecase*                     pUsecase,
    const PruneSettings*                  pPruneSettings)
{
    CHX_ASSERT(NULL != pStreamConfig);
    CHX_ASSERT(NULL != pUsecase);

    UINT       numStreams       = pStreamConfig->num_streams;
    BOOL       isMatching       = FALSE;
    UINT       streamConfigMode = pUsecase->streamConfigMode;

    // The usecase structure generated from the parser contains an array of "HAL Target Info" that describes the HAL
    // buffers allowed for the usecase. We match the currently set stream config with the target info to find a matching
    // usecase from the pipeline XML. The first stream tries to find a match with any one of the 'n' target's in the
    // usecase. If it finds a match, the matching target from the usecase needs to be removed from the search list of
    // the next stream. So this variable keeps track of the targets that we need to compare the stream with. After
    // every successful stream match the matching target will be removed from the array below to prevent the next stream
    // from comparing with the same matching target for a previous stream.
    // We need to search all the targets in the usecase for the first stream
    UINT compareTargetIndexMask = ((1 << pUsecase->numTargets) - 1);
    UINT compareStreamIndexMask = ((1 << numStreams) - 1);

    if (streamConfigMode == static_cast<UINT>(pStreamConfig->operation_mode))
    {
        // For each stream, compare the stream parameters with the pipeline usecase hal target parameters
        for (UINT targetInfoIdx = 0; targetInfoIdx < pUsecase->numTargets; targetInfoIdx++)
        {
            ChiTarget* pTargetInfo = pUsecase->ppChiTargets[targetInfoIdx];
            // Current stream search begins as not matching any targets in the usecase being compared with
            if ((NULL != pUsecase->pTargetPruneSettings) &&
                (TRUE == ShouldPrune(pPruneSettings, &pUsecase->pTargetPruneSettings[targetInfoIdx])))
            {
                CHX_LOG_INFO("Ignoring Target Info because of prune settings: "
                             "format[0]: %u targetType = %d streamWidth = %d streamHeight = %d",
                             pTargetInfo->pBufferFormats[0],
                             pTargetInfo->direction,
                             pTargetInfo->dimension.maxWidth,
                             pTargetInfo->dimension.maxHeight);
                compareTargetIndexMask = ChxUtils::BitReset(compareTargetIndexMask, targetInfoIdx);
                continue; // Don't consider targets that will be pruned for Matching
            }
            isMatching = FALSE;

            for (UINT streamId = 0; streamId < numStreams; streamId++)
            {
                if (FALSE == ChxUtils::IsBitSet(compareStreamIndexMask, streamId))
                {
                    continue;
                }
                ChiStream* pStream = reinterpret_cast<ChiStream*>(pStreamConfig->streams[streamId]);

                CHX_ASSERT(pStream != NULL);

                if (NULL != pStream)
                {
                    INT    streamFormat = pStream->format;
                    UINT   streamType   = pStream->streamType;
                    UINT32 streamWidth  = pStream->width;
                    UINT32 streamHeight = pStream->height;

                    CHX_LOG("streamType = %d streamFormat = %d streamWidth = %d streamHeight = %d",
                            streamType, streamFormat, streamWidth, streamHeight);

                    // For the current stream, try to find a match with one of the targets defined in the usecase
                    // If the stream matches with one of the targets, move onto trying to find a match for the next stream.
                    // Before trying to find a match for the next stream, remove the currently matching usecase target from
                    // the next search list. The usecase targets to consider for matching are given in
                    // "compareWithTargetIndex[]"

                    isMatching = IsMatchingFormat(reinterpret_cast<ChiStream*>(pStream),
                                                  pTargetInfo->numFormats,
                                                  pTargetInfo->pBufferFormats);

                    if (TRUE == isMatching)
                    {
                        isMatching = ((streamType == static_cast<UINT>(pTargetInfo->direction)) ? TRUE : FALSE);
                    }

                    if (TRUE == isMatching)
                    {
                        BufferDimension* pRange = &pTargetInfo->dimension;

                        if ((streamWidth  >= pRange->minWidth)  && (streamWidth  <= pRange->maxWidth) &&
                            (streamHeight >= pRange->minHeight) && (streamHeight <= pRange->maxHeight))
                        {
                            isMatching = TRUE;
                        }
                        else
                        {
                            isMatching = FALSE;
                        }
                    }

                    // Current stream (streamId) matches with the current target (targetInfoIdx) in the usecase - so remove
                    // that target from the search list of the next stream
                    if (TRUE == isMatching)
                    {
                        pTargetInfo->pChiStream = pStream;
                        // Remove the target entry from the search list of the next stream
                        compareTargetIndexMask = ChxUtils::BitReset(compareTargetIndexMask, targetInfoIdx);
                        compareStreamIndexMask = ChxUtils::BitReset(compareStreamIndexMask, streamId);
                        break; // Move onto the next stream because we found a match for the current stream
                    }
                }
            }

            // Current stream did not find any matching target entry in the currently selected usecase. So bail out of the
            // current search (so that we can move onto the next usecase for matching)
            if (FALSE == isMatching)
            {
                break;
            }
        }
    }

    if (TRUE == isMatching)
    {
        isMatching = (0 == compareStreamIndexMask) ? TRUE : FALSE;
    }
    CHX_LOG_VERBOSE("Target Mask: %x Stream Mask: %x - %s",
                    compareTargetIndexMask,
                    compareStreamIndexMask,
                    pUsecase->pUsecaseName);

    return isMatching;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSelector::IsAllowedImplDefinedFormat
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsAllowedImplDefinedFormat(
    ChiBufferFormat format,
    GrallocUsage    grallocUsage)
{
    BOOL isAllowed = FALSE;

    for (UINT i = 0; i < NumImplDefinedFormats; i++)
    {
        if (AllowedImplDefinedFormats[i] == format)
        {
            if ((ChiFormatRawPlain16 == AllowedImplDefinedFormats[i]) ||
                (ChiFormatRawPlain64 == AllowedImplDefinedFormats[i]))
            {
                // If it is a display buffer we cannot allow Raw buffer
                if ((0 != (grallocUsage & GRALLOC_USAGE_HW_COMPOSER)) && (0 == (grallocUsage & GRALLOC_USAGE_HW_TEXTURE)))
                {
                    isAllowed = TRUE;
                    break;
                }
            }
            else
            {
                isAllowed = TRUE;
                break;
            }
        }
    }

    return isAllowed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSelector::IsMatchingFormat
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSelector::IsMatchingFormat(
    ChiStream*             pStream,
    UINT32                 numFormats,
    const ChiBufferFormat* pMatchingFormats)
{
    CHX_ASSERT(pStream != NULL);

    BOOL                isMatching         = FALSE;
    ChiStreamFormat     streamFormat       = pStream->format;
    GrallocUsage        streamGrallocUsage = pStream->grallocUsage;
    ChiStreamType       streamType         = static_cast<ChiStreamType>(pStream->streamType);

    for (UINT32 i = 0; i < numFormats; i++)
    {
        if (((ChiStreamFormatRaw16 == streamFormat) && (DataspaceDepth != pStream->dataspace)) &&
                  (ChiFormatRawPlain16 == pMatchingFormats[i]))
        {
            isMatching = TRUE;
        }
        else if ((ChiStreamFormatRaw64 == streamFormat) && (ChiFormatRawPlain64 == pMatchingFormats[i]))
        {
            isMatching = TRUE;
        }
        else if ((ChiStreamFormatY8 == streamFormat) && (ChiFormatRawMIPI8 == pMatchingFormats[i]))
        {
            isMatching = TRUE;
        }
        else if (((ChiStreamFormatRawOpaque == streamFormat) || (ChiStreamFormatRaw10 == streamFormat)) &&
                 (ChiFormatRawMIPI == pMatchingFormats[i]))
        {
            isMatching = TRUE;
        }
        else if (((ChiStreamFormatYCbCr420_888                    == streamFormat)               ||
                  ((ChiStreamFormatImplDefined                    == streamFormat)               &&
                  ((ChiStreamTypeBidirectional                    == streamType)                 ||
                   (ChiStreamTypeInput                            == streamType)                 ||
                  ((streamGrallocUsage & GrallocUsageHwCameraZSL) == GrallocUsageHwCameraZSL)))) &&
                  ((ChiFormatYUV420NV12 == pMatchingFormats[i])  || (ChiFormatYUV420NV21 == pMatchingFormats[i])))
        {
            isMatching = TRUE;
        }
        else if (ChiStreamFormatImplDefined == streamFormat)
        {
            isMatching = IsAllowedImplDefinedFormat(pMatchingFormats[i], streamGrallocUsage);
        }
        else if ((ChiStreamFormatBlob == streamFormat) &&
                 ((ChiFormatJpeg == pMatchingFormats[i]) || (ChiFormatBlob == pMatchingFormats[i])))
        {
            isMatching = TRUE;
        }
        else if ((ChiStreamFormatY16 == streamFormat) && (ChiFormatY16 == pMatchingFormats[i]))
        {
            isMatching = TRUE;
        }
        else if ((ChiStreamFormatRaw16 == streamFormat) && (ChiFormatRawDepth == pMatchingFormats[i]))
        {
            isMatching = TRUE;
        }
    }
    return isMatching;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::FindMaxResolutionCameraID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 UsecaseSelector::FindMaxResolutionCameraID(
    LogicalCameraInfo*     pCameraInfo)
{
    camera_metadata_t*  pMetadata[MaxDevicePerLogicalCamera] = { 0 };
    CDKResult           result                               = CDKResultSuccess;
    INT                 maxJpegSizeMetaPresent               = 1;

    // Default assignment of camera ID
    UINT32 cameraID = pCameraInfo->ppDeviceInfo[0]->cameraId;

    // Find camera corresponding to maximum resolution in multi camera use case
    if (1 < pCameraInfo->numPhysicalCameras)
    {
        for (UINT32 i = 0; i < pCameraInfo->numPhysicalCameras; i++)
        {
            camera_info_t* pCamInfo = static_cast<camera_info_t*>(pCameraInfo->ppDeviceInfo[i]->m_pDeviceCaps->pLegacy);

            pMetadata[i] = const_cast<camera_metadata_t *>(pCamInfo->static_camera_characteristics);

            if (NULL == pMetadata[i])
            {
                result = CDKResultEFailed;
            }
        }

        if (CDKResultSuccess == result)
        {
            camera_metadata_entry_t metaEntry[MaxDevicePerLogicalCamera];

            for (UINT32 i = 0; i < pCameraInfo->numPhysicalCameras; i++)
            {
                maxJpegSizeMetaPresent = find_camera_metadata_entry(pMetadata[i], ANDROID_JPEG_MAX_SIZE, &metaEntry[i]);

                if (0 != maxJpegSizeMetaPresent)
                {
                    break;
                }
            }

            if (0 == maxJpegSizeMetaPresent)
            {
                UINT32 maxJpegSizeMetaCamIdx = 0;

                for (UINT32 i = 0; i < (pCameraInfo->numPhysicalCameras - 1); i++)
                {
                    maxJpegSizeMetaCamIdx = (*metaEntry[maxJpegSizeMetaCamIdx].data.i32 > *metaEntry[i + 1].data.i32) ?
                                              maxJpegSizeMetaCamIdx :
                                              (i + 1);
                }

                cameraID = pCameraInfo->ppDeviceInfo[maxJpegSizeMetaCamIdx]->cameraId;
            }
            else
            {
                CHX_LOG_ERROR("One of the max jpeg size meta is Null and so set to default camera ID %d", cameraID);
            }
        }
        else
        {
            CHX_LOG_ERROR("One of the metadata is Null and so set to default camera ID %d", cameraID);
        }
    }
    return cameraID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::getSensorDimension
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSelector::getSensorDimension(
    const UINT32                          cameraID,
    const camera3_stream_configuration_t* pStreamConfig,
    UINT32                                *pSensorw,
    UINT32                                *pSensorh,
    UINT32                                downscaleFactor,
    UINT32                                activeAspectRatio)
{
    CHISENSORMODEINFO* sensorMode = GetSensorModeInfo(cameraID, pStreamConfig, downscaleFactor,
        activeAspectRatio);

    *pSensorw = sensorMode->frameDimension.width;
    *pSensorh = sensorMode->frameDimension.height;

    CHX_LOG("Sensor Output for cameraID %d, RAW: %dX%d", cameraID, *pSensorw, *pSensorh);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseSelector::GetSensorModeInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHISENSORMODEINFO* UsecaseSelector::GetSensorModeInfo(
    const UINT32                          cameraID,
    const camera3_stream_configuration_t* pStreamConfig,
    UINT32                                downscaleFactor,
    UINT32                                activeAspectRatio)
{
    UINT32 maxWidth  = 0;
    UINT32 maxHeight = 0;
    DesiredSensorMode desiredSensorMode = { 0 };
    desiredSensorMode.frameRate = ExtensionModule::GetInstance()->GetUsecaseMaxFPS();
    desiredSensorMode.forceMode = ExtensionModule::GetInstance()->GetForceSensorMode();

    // Select the highest width/height from all the input buffer requirements
    for (UINT32 stream = 0; stream < pStreamConfig->num_streams; stream++)
    {
        if ((pStreamConfig->streams[stream]->width/downscaleFactor) > maxWidth)
        {
            maxWidth  = pStreamConfig->streams[stream]->width/downscaleFactor;
        }
        if (pStreamConfig->streams[stream]->height > maxHeight)
        {
            maxHeight = pStreamConfig->streams[stream]->height;
        }
    }

    if (0 != activeAspectRatio)
    {
        CHIRECT            activeRect         = { 0 };
        FLOAT              maxAspectRatio     = (static_cast<FLOAT>(maxWidth) /
            static_cast<FLOAT>(maxHeight));
        FLOAT              aspectRatioDiff    = 0;
        static const FLOAT MaxAspectRatioDiff = 0.1f;

        ExtensionModule::GetInstance()->GetActiveArray(cameraID, &activeRect);

        if (0 != (activeRect.width * activeRect.height))
        {
            FLOAT activeAspectRatio = (static_cast<FLOAT>(activeRect.width) /
                static_cast<FLOAT>(activeRect.height));

            if (activeAspectRatio > maxAspectRatio)
            {
                aspectRatioDiff = static_cast<FLOAT>(
                    ChxUtils::AbsoluteFLOAT(activeAspectRatio - maxAspectRatio));
            }
            else
            {
                aspectRatioDiff = static_cast<FLOAT>(
                    ChxUtils::AbsoluteFLOAT(maxAspectRatio - activeAspectRatio));
            }

            if (aspectRatioDiff > MaxAspectRatioDiff)
            {
                maxHeight = ChxUtils::EvenCeilingUINT32(static_cast<UINT32>(
                    (static_cast<FLOAT>(maxWidth)/activeAspectRatio)));
            }

            CHX_LOG_INFO("Camera = %d Active Array = %dX%d AAR = %f MAR = %f Optimal = %dX%d",
                cameraID, activeRect.width, activeRect.height,
                activeAspectRatio, maxAspectRatio,
                maxWidth, maxHeight);
        }
    }

    desiredSensorMode.optimalWidth            = maxWidth;
    desiredSensorMode.optimalHeight           = maxHeight;
    desiredSensorMode.maxWidth                = maxWidth;
    desiredSensorMode.maxHeight               = maxHeight;
    desiredSensorMode.minWidth                = maxWidth;
    desiredSensorMode.minHeight               = maxHeight;
   if (SelectInSensorHDR3ExpUsecase::InSensorHDR3ExpPreview ==
    ExtensionModule::GetInstance()->SelectInSensorHDR3ExpUsecase())
   {
       desiredSensorMode.sensorModeCaps.u.IHDR = 1;
   }
   else
   {
       desiredSensorMode.sensorModeCaps.u.Normal = TRUE;
   }

    CHISENSORMODEINFO* sensorMode = ChxSensorModeSelect::FindBestSensorMode(cameraID,
                                     &desiredSensorMode);

    return sensorMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSelector::ClonePipelineDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiTarget* UsecaseSelector::ClonePipelineDesc(
    const ChiTarget *pSrcTarget)
{
    ChiTarget* pDstTarget = NULL;

    if (NULL != pSrcTarget)
    {
        pDstTarget = static_cast<ChiTarget*>(ChxUtils::Calloc(sizeof(ChiTarget)));
        if (NULL != pDstTarget)
        {
            ChxUtils::Memcpy(pDstTarget, pSrcTarget, sizeof(ChiTarget));

            pDstTarget->pBufferFormats = static_cast<ChiBufferFormat*>(
                ChxUtils::Calloc(sizeof(ChiBufferFormat) * pSrcTarget->numFormats));

            if (NULL != pDstTarget->pBufferFormats)
            {
                ChxUtils::Memcpy(pDstTarget->pBufferFormats, pSrcTarget->pBufferFormats,
                    (sizeof(ChiBufferFormat) * pSrcTarget->numFormats));
            }
            else
            {
                CHX_LOG_ERROR("pDstTarget->pBufferFormats allocation failed!");
                ChxUtils::Free(pDstTarget);
                pDstTarget = NULL;
            }
        }
    }

    return pDstTarget;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSelector::CloneUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiUsecase* UsecaseSelector::CloneUsecase(
    const ChiUsecase *pSrcUsecase, UINT32 numDesc, UINT32 *pDescIndex)
{
    ChiUsecase* pDstUsecase = NULL;
    INT32      result       = 0;
    CHX_ASSERT(NULL != pSrcUsecase);

    if (NULL != pSrcUsecase)
    {
        pDstUsecase = static_cast<ChiUsecase*>(ChxUtils::Calloc(sizeof(ChiUsecase)));

        if (NULL != pDstUsecase)
        {
            pDstUsecase->pUsecaseName      = pSrcUsecase->pUsecaseName;
            pDstUsecase->streamConfigMode  = pSrcUsecase->streamConfigMode;
            pDstUsecase->numPipelines      = 0;
            pDstUsecase->numTargets        = 0;

            if ((0 != numDesc) && (NULL != pDescIndex))
            {
                /// Clone Pipeline Desc
                pDstUsecase->pPipelineTargetCreateDesc = static_cast<ChiPipelineTargetCreateDescriptor*>(ChxUtils::Calloc(
                    sizeof(ChiPipelineTargetCreateDescriptor) * numDesc));

                if (NULL != pDstUsecase->pPipelineTargetCreateDesc)
                {
                    const ChiPipelineTargetCreateDescriptor* pSrcDesc = pSrcUsecase->pPipelineTargetCreateDesc;
                    ChiPipelineTargetCreateDescriptor*       pDstDesc = pDstUsecase->pPipelineTargetCreateDesc;

                    if (NULL != pDstDesc)
                    {
                        UINT32 numTargets = 0;

                        for (UINT32 descIndex = 0; descIndex < numDesc; descIndex++)
                        {
                            UINT32 currentIndex = pDescIndex[descIndex];
                            if (currentIndex < pSrcUsecase->numPipelines)
                            {
                                result = ClonePipelineDesc(&pSrcDesc[currentIndex],
                                                           &pDstDesc[pDstUsecase->numPipelines]);

                                if (CDKResultSuccess != result)
                                {
                                    CHX_LOG_ERROR("Pipeline descriptor clone failed, descIndex=%d, currentIndex=%d",
                                        descIndex, currentIndex);
                                    break;
                                }

                                numTargets += pDstDesc[pDstUsecase->numPipelines].sinkTarget.numTargets;
                                numTargets += pDstDesc[pDstUsecase->numPipelines].sourceTarget.numTargets;
                                pDstUsecase->numPipelines++;
                            }
                        }

                        if (CDKResultSuccess == result)
                        {
                            if (NULL != pSrcUsecase->pTargetPruneSettings)
                            {
                                pDstUsecase->pTargetPruneSettings = static_cast<const PruneSettings*>(ChxUtils::Calloc(
                                    numTargets * sizeof(PruneSettings)));
                                if (NULL == pDstUsecase->pTargetPruneSettings)
                                {
                                    CHX_LOG_ERROR("Out of memory!");
                                    result = CDKResultENoMemory;
                                }
                            }
                        }
                        if (CDKResultSuccess == result)
                        {
                            pDstUsecase->numTargets   = 0;
                            pDstUsecase->ppChiTargets = static_cast<ChiTarget**>(ChxUtils::Calloc(
                                sizeof(ChiTarget*) * numTargets));

                            if (NULL != pDstUsecase->ppChiTargets)
                            {
                                for (UINT32 descIndex = 0; descIndex < pDstUsecase->numPipelines; descIndex++)
                                {
                                    const ChiTargetPortDescriptorInfo* pSrcInfo  = &pSrcDesc[pDescIndex[descIndex]].sinkTarget;
                                    ChiTargetPortDescriptorInfo*       pDescInfo = &pDstDesc[descIndex].sinkTarget;
                                    for (UINT32 i = 0; i < pDescInfo->numTargets; i++)
                                    {
                                        if (NULL != pDescInfo->pTargetPortDesc[i].pTarget)
                                        {
                                            if ((NULL != pSrcUsecase->pTargetPruneSettings) &&
                                                (NULL != pDstUsecase->pTargetPruneSettings))
                                            {
                                                PruneSettings* pDstSettings = const_cast<PruneSettings*>(
                                                    &pDstUsecase->pTargetPruneSettings[pDstUsecase->numTargets]);
                                                // Find the original instance of this target in ppChiTargets
                                                // The index where that is located will map to index of the target prune
                                                // settings we should copy. A shallow copy is sufficient
                                                for (UINT j = 0; j < pSrcUsecase->numTargets; j++)
                                                {
                                                    if (pSrcInfo->pTargetPortDesc[i].pTarget == pSrcUsecase->ppChiTargets[j])
                                                    {
                                                        *pDstSettings = pSrcUsecase->pTargetPruneSettings[j];
                                                        break;
                                                    }
                                                }
                                            }
                                            pDstUsecase->ppChiTargets[pDstUsecase->numTargets++] =
                                                pDescInfo->pTargetPortDesc[i].pTarget;
                                        }
                                    }

                                    pDescInfo = &pDstDesc[descIndex].sourceTarget;
                                    for (UINT32 i = 0; i < pDescInfo->numTargets; i++)
                                    {
                                        if (NULL != pDescInfo->pTargetPortDesc[i].pTarget)
                                        {
                                            pDstUsecase->ppChiTargets[pDstUsecase->numTargets++] =
                                                pDescInfo->pTargetPortDesc[i].pTarget;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                CHX_LOG_ERROR("pDstUsecase->ppChiTargets allocation failed! numTargets=%d", numTargets);
                            }
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("pDstUsecase->pPipelineTargetCreateDesc allocation failed! numDesc=%d", numDesc);
                }
            }
            else
            {
                CHX_LOG_WARN("No pipeline descriptor need to be cloned!");
            }
        }
        else
        {
            CHX_LOG_ERROR("pDstUsecase allocation failed!");
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid arguments! pSrcUsecase=%p, numDesc=%d, pDescIndex=%p",
            pSrcUsecase, numDesc, pDescIndex);
    }
    return pDstUsecase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSelector::ClonePipelineDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSelector::ClonePipelineDesc(
    const ChiPipelineTargetCreateDescriptor* pSrcDesc,
    ChiPipelineTargetCreateDescriptor*       pDstDesc)
{
    CDKResult result = CDKResultSuccess;

    CHX_ASSERT(NULL != pSrcDesc);
    CHX_ASSERT(NULL != pDstDesc);

    if ((NULL != pDstDesc) && (NULL != pSrcDesc))
    {
        ChxUtils::Memcpy(pDstDesc, pSrcDesc, sizeof(ChiPipelineTargetCreateDescriptor));

        /// Clone Sink Targets
        const ChiTargetPortDescriptorInfo* pSrcTargetDescInfo  = &pSrcDesc->sinkTarget;
        ChiTargetPortDescriptorInfo* pDstTargetDescInfo        = &pDstDesc->sinkTarget;

        if ((0 != pSrcTargetDescInfo->numTargets) && (NULL != pSrcTargetDescInfo->pTargetPortDesc))
        {
            pDstTargetDescInfo->pTargetPortDesc = NULL;
            pDstTargetDescInfo->pTargetPortDesc = static_cast<ChiTargetPortDescriptor*>(
                    ChxUtils::Calloc(sizeof(ChiTargetPortDescriptor) * pSrcTargetDescInfo->numTargets));
            if (NULL != pDstTargetDescInfo->pTargetPortDesc)
            {
                ChxUtils::Memcpy(pDstTargetDescInfo->pTargetPortDesc, pSrcTargetDescInfo->pTargetPortDesc,
                        sizeof(ChiTargetPortDescriptor) * pSrcTargetDescInfo->numTargets);
                for(UINT32 i = 0; i < pSrcTargetDescInfo->numTargets; i++)
                {
                    pDstTargetDescInfo->pTargetPortDesc[i].pTarget = ClonePipelineDesc(
                        pSrcTargetDescInfo->pTargetPortDesc[i].pTarget);

                    if (NULL == pDstTargetDescInfo->pTargetPortDesc[i].pTarget)
                    {
                        CHX_LOG_ERROR("Failed to Clone pipeline = %s numSinkTargets = %d sinkTarget = %d",
                                pSrcDesc->pPipelineName, pSrcTargetDescInfo->numTargets, i);
                        result = CDKResultEFailed;
                        break;
                    }
                }
            }
            else
            {
                CHX_LOG_ERROR("Failed to Clone pipeline = %s numSinkTargets = %d",
                                pSrcDesc->pPipelineName, pSrcTargetDescInfo->numTargets);
                result = CDKResultENoMemory;
            }
        }

        /// Clone Source Targets
        if (CDKResultSuccess == result)
        {
            pSrcTargetDescInfo  = &pSrcDesc->sourceTarget;
            pDstTargetDescInfo  = &pDstDesc->sourceTarget;
            if ((pSrcTargetDescInfo->numTargets != 0) && (NULL != pSrcTargetDescInfo->pTargetPortDesc))
            {
                pDstTargetDescInfo->pTargetPortDesc = NULL;
                pDstTargetDescInfo->pTargetPortDesc = static_cast<ChiTargetPortDescriptor*>(
                        ChxUtils::Calloc(sizeof(ChiTargetPortDescriptor) * pSrcTargetDescInfo->numTargets));
                if (NULL != pDstTargetDescInfo->pTargetPortDesc)
                {
                    ChxUtils::Memcpy(pDstTargetDescInfo->pTargetPortDesc, pSrcTargetDescInfo->pTargetPortDesc,
                            sizeof(ChiTargetPortDescriptor) * pSrcTargetDescInfo->numTargets);
                    for(UINT32 i = 0; i < pSrcTargetDescInfo->numTargets; i++)
                    {
                        pDstTargetDescInfo->pTargetPortDesc[i].pTarget = ClonePipelineDesc(
                            pSrcTargetDescInfo->pTargetPortDesc[i].pTarget);
                        if (NULL == pDstTargetDescInfo->pTargetPortDesc[i].pTarget)
                        {
                            CHX_LOG_ERROR("Failed to Clone pipeline = %s numSourceTargets = %d sourceTarget = %d",
                                    pSrcDesc->pPipelineName, pSrcTargetDescInfo->numTargets, i);
                            result = CDKResultEFailed;
                            break;
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Failed to Clone pipeline = %s numSinkTargets = %d",
                                    pSrcDesc->pPipelineName, pSrcTargetDescInfo->numTargets);
                    result = CDKResultENoMemory;
                }
            }
        }

        /// Clone Pipeline Create Desc
        if (CDKResultSuccess == result)
        {
            const ChiPipelineCreateDescriptor* pSrcCreateDesc = &pSrcDesc->pipelineCreateDesc;
            ChiPipelineCreateDescriptor* pDstCreateDesc       = &pDstDesc->pipelineCreateDesc;
            CHX_ASSERT(NULL != pSrcCreateDesc->pNodes);
            if ((pSrcCreateDesc->numNodes != 0) && (NULL != pSrcCreateDesc->pNodes))
            {
                pDstCreateDesc->pNodes = NULL;
                pDstCreateDesc->pNodes = static_cast<CHINODE*>(ChxUtils::Calloc(sizeof(CHINODE) *
                    pSrcCreateDesc->numNodes));
                if (NULL != pDstCreateDesc->pNodes)
                {
                    ChxUtils::Memcpy(pDstCreateDesc->pNodes, pSrcCreateDesc->pNodes,
                        sizeof(CHINODE) * pSrcCreateDesc->numNodes);

                    for (UINT32 i = 0; i < pSrcCreateDesc->numNodes; i++)
                    {
                        CHINODEPORTS* pSrcPortDesc = &pSrcCreateDesc->pNodes[i].nodeAllPorts;
                        CHINODEPORTS* pDstPortDesc = &pDstCreateDesc->pNodes[i].nodeAllPorts;
                        if ((pSrcPortDesc->numInputPorts != 0) && (NULL != pSrcPortDesc->pInputPorts))
                        {
                            pDstPortDesc->pInputPorts = NULL;
                            pDstPortDesc->pInputPorts = static_cast<CHIINPUTPORTDESCRIPTOR*>(ChxUtils::Calloc(
                                sizeof(CHIINPUTPORTDESCRIPTOR) * pSrcPortDesc->numInputPorts));

                            if (NULL != pDstPortDesc->pInputPorts)
                            {
                                ChxUtils::Memcpy(pDstPortDesc->pInputPorts, pSrcPortDesc->pInputPorts,
                                    sizeof(CHIINPUTPORTDESCRIPTOR) * pSrcPortDesc->numInputPorts);
                            }
                            else
                            {
                                CHX_LOG_ERROR("Failed to Clone pipeline = %s Node = %d numInputPorts = %d",
                                    pSrcDesc->pPipelineName, i, pSrcPortDesc->numInputPorts);
                                result = CDKResultENoMemory;
                                break;
                            }
                        }

                        if ((pSrcPortDesc->numOutputPorts != 0) && (NULL != pSrcPortDesc->pOutputPorts))
                        {
                            pDstPortDesc->pOutputPorts = NULL;
                            pDstPortDesc->pOutputPorts = static_cast<CHIOUTPUTPORTDESCRIPTOR*>(ChxUtils::Calloc(
                                sizeof(CHIOUTPUTPORTDESCRIPTOR) * pSrcPortDesc->numOutputPorts));
                            if (NULL != pDstPortDesc->pOutputPorts)
                            {
                                ChxUtils::Memcpy(pDstPortDesc->pOutputPorts, pSrcPortDesc->pOutputPorts,
                                    sizeof(CHIOUTPUTPORTDESCRIPTOR) * pSrcPortDesc->numOutputPorts);
                            }
                            else
                            {
                                CHX_LOG_ERROR("Failed to Clone pipeline = %s Node = %d numOutputPorts = %d",
                                    pSrcDesc->pPipelineName, i, pSrcPortDesc->numOutputPorts);
                                result = CDKResultENoMemory;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Failed to Clone pipeline = %s numNodes = %d",
                        pSrcDesc->pPipelineName, pSrcCreateDesc->numNodes);
                    result = CDKResultENoMemory;
                }
            }

            if (CDKResultSuccess == result)
            {
                CHX_ASSERT(NULL != pSrcCreateDesc->pLinks);
                if ((pSrcCreateDesc->numLinks != 0) && (NULL != pSrcCreateDesc->pLinks))
                {
                    pDstCreateDesc->pLinks = NULL;
                    pDstCreateDesc->pLinks = static_cast<CHINODELINK*>(ChxUtils::Calloc(sizeof(CHINODELINK) *
                        pSrcCreateDesc->numLinks));
                    if (NULL != pDstCreateDesc->pLinks)
                    {
                        ChxUtils::Memcpy(pDstCreateDesc->pLinks, pSrcCreateDesc->pLinks,
                            sizeof(CHINODELINK) * pDstCreateDesc->numLinks);
                    }
                    else
                    {
                        CHX_LOG_ERROR("Failed to allocate and clone links pipeline = %s numLinks = %d",
                            pSrcDesc->pPipelineName, pSrcCreateDesc->numLinks);
                        result = CDKResultENoMemory;
                    }
                }
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Failed to Clone PipelineDesc = %p", pSrcDesc);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSelector::DestroyUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSelector::DestroyUsecase(ChiUsecase *pUsecase)
{
    CHX_ASSERT(NULL != pUsecase);

    if (NULL != pUsecase)
    {
        if (NULL != (pUsecase->ppChiTargets))
        {
            ChxUtils::Free(pUsecase->ppChiTargets);
            pUsecase->ppChiTargets = NULL;
        }
        if (NULL != pUsecase->pTargetPruneSettings)
        {
            ChxUtils::Free(const_cast<PruneSettings*>(pUsecase->pTargetPruneSettings));
            pUsecase->pTargetPruneSettings = NULL;
        }
        if (NULL != pUsecase->pPipelineTargetCreateDesc)
        {
            for (UINT32 descIndex = 0; descIndex < pUsecase->numPipelines; descIndex++)
            {
                DestroyPipelineDesc(&pUsecase->pPipelineTargetCreateDesc[descIndex]);
            }
            ChxUtils::Free(pUsecase->pPipelineTargetCreateDesc);
            pUsecase->pPipelineTargetCreateDesc = NULL;
        }

        ChxUtils::Free(pUsecase);
        pUsecase = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSelector::DestroyPipelineDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSelector::DestroyPipelineDesc(ChiTarget *pTarget)
{
    if (NULL != pTarget)
    {
        if (NULL != pTarget->pBufferFormats)
        {
            ChxUtils::Free(pTarget->pBufferFormats);
            pTarget->pBufferFormats = NULL;
        }
        ChxUtils::Free(pTarget);
        pTarget = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSelector::DestroyPipelineDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSelector::DestroyPipelineDesc(ChiPipelineTargetCreateDescriptor *pDesc)
{
    CHX_ASSERT(NULL != pDesc);

    if (NULL != pDesc)
    {
        /// Sink Targets
        ChiTargetPortDescriptorInfo* pTargetDescInfo  = &pDesc->sinkTarget;

        if ((NULL != pTargetDescInfo) && (NULL != pTargetDescInfo->pTargetPortDesc))
        {
            for(UINT32 i = 0; i < pTargetDescInfo->numTargets; i++)
            {
                DestroyPipelineDesc(pTargetDescInfo->pTargetPortDesc[i].pTarget);
            }

            pTargetDescInfo->numTargets = 0;
            ChxUtils::Free(pTargetDescInfo->pTargetPortDesc);
            pTargetDescInfo->pTargetPortDesc = NULL;
        }

        ///Source Targets
        pTargetDescInfo  = &pDesc->sourceTarget;
        if ((NULL != pTargetDescInfo) && (NULL != pTargetDescInfo->pTargetPortDesc))
        {
            for(UINT32 i = 0; i < pTargetDescInfo->numTargets; i++)
            {
                DestroyPipelineDesc(pTargetDescInfo->pTargetPortDesc[i].pTarget);
            }

            pTargetDescInfo->numTargets = 0;
            ChxUtils::Free(pTargetDescInfo->pTargetPortDesc);
            pTargetDescInfo->pTargetPortDesc = NULL;
        }

        /// Pipeline Create Desc
        ChiPipelineCreateDescriptor* pCreateDesc = &pDesc->pipelineCreateDesc;
        if ((NULL != pCreateDesc) && (NULL != pCreateDesc->pNodes))
        {
            for (UINT32 i = 0; i < pCreateDesc->numNodes; i++)
            {
                if (NULL != pCreateDesc->pNodes[i].nodeAllPorts.pInputPorts)
                {
                    ChxUtils::Free(pCreateDesc->pNodes[i].nodeAllPorts.pInputPorts);
                    pCreateDesc->pNodes[i].nodeAllPorts.pInputPorts = NULL;
                }

                if (NULL != pCreateDesc->pNodes[i].nodeAllPorts.pOutputPorts)
                {
                    ChxUtils::Free(pCreateDesc->pNodes[i].nodeAllPorts.pOutputPorts);
                    pCreateDesc->pNodes[i].nodeAllPorts.pOutputPorts = NULL;
                }
            }
            ChxUtils::Free(pCreateDesc->pNodes);
            pCreateDesc->pNodes = NULL;
        }

        if (NULL != pCreateDesc->pLinks)
        {
            ChxUtils::Free(pCreateDesc->pLinks);
            pCreateDesc->pLinks = NULL;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSelector::UpdateFDStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSelector::UpdateFDStream(FLOAT  referenceAspectRatio)
{
    FLOAT FDAspectRatio = static_cast<FLOAT>(m_DefaultFDStream.width) / static_cast<FLOAT>(m_DefaultFDStream.height);

    FDStream     = m_DefaultFDStream;

    if(0.0F != referenceAspectRatio)
    {
        // Default FD stream dimensions
        UINT32  fdwidth  = m_DefaultFDStream.width;
        UINT32  fdheight = m_DefaultFDStream.height;

        if (FDAspectRatio < referenceAspectRatio)
        {
            fdheight = static_cast<UINT32>(static_cast<FLOAT>(fdwidth) / referenceAspectRatio);
        }
        else if (FDAspectRatio > referenceAspectRatio)
        {
            fdwidth = static_cast<UINT32>(static_cast<FLOAT>(fdheight) * referenceAspectRatio);
        }

        fdwidth         = ChxUtils::EvenCeilingUINT32(fdwidth);
        fdheight        = ChxUtils::EvenCeilingUINT32(fdheight);

        FDStream.width  = fdwidth;
        FDStream.height = fdheight;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseFactory::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseFactory* UsecaseFactory::Create(
    const ExtensionModule* pExtModule)
{
    UsecaseFactory* pFactory = new UsecaseFactory;

    if (NULL != pFactory)
    {
        pFactory->m_pExtModule = pExtModule;
    }
    return pFactory;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseFactory::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseFactory::Destroy()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseFactory::~UsecaseFactory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseFactory::~UsecaseFactory()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseFactory::CreateUsecaseObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Usecase* UsecaseFactory::CreateUsecaseObject(
    LogicalCameraInfo*              pLogicalCameraInfo,     ///< camera info
    UsecaseId                       usecaseId,              ///< Usecase Id
    camera3_stream_configuration_t* pStreamConfig)          ///< Stream config
{
    Usecase* pUsecase  = NULL;
    UINT     camera0Id = pLogicalCameraInfo->ppDeviceInfo[0]->cameraId;

    switch (usecaseId)
    {
        case UsecaseId::PreviewZSL:
        case UsecaseId::VideoLiveShot:
            pUsecase = AdvancedCameraUsecase::Create(pLogicalCameraInfo, pStreamConfig, usecaseId);
            break;
        case UsecaseId::MultiCamera:
            {
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) //Android-P or better

                LogicalCameraType logicalCameraType = m_pExtModule->GetCameraType(pLogicalCameraInfo->cameraId);

                if (LogicalCameraType_DualApp == logicalCameraType)
                {
                    pUsecase = UsecaseDualCamera::Create(pLogicalCameraInfo, pStreamConfig);
                }
                else
#endif
                {
                    pUsecase = UsecaseMultiCamera::Create(pLogicalCameraInfo, pStreamConfig);
                }
                break;
            }
        case UsecaseId::MultiCameraVR:
            //pUsecase = UsecaseMultiVRCamera::Create(pLogicalCameraInfo, pStreamConfig);
            break;
        case UsecaseId::QuadCFA:
            pUsecase = AdvancedCameraUsecase::Create(pLogicalCameraInfo, pStreamConfig, usecaseId);
            break;
        case UsecaseId::Torch:
            pUsecase = UsecaseTorch::Create(pLogicalCameraInfo, pStreamConfig);
            break;
#if (!defined(LE_CAMERA)) // SuperSlowMotion not supported in LE
        case UsecaseId::SuperSlowMotionFRC:
            pUsecase = UsecaseSuperSlowMotionFRC::Create(pLogicalCameraInfo, pStreamConfig);
            break;
#endif
        default:
            pUsecase = AdvancedCameraUsecase::Create(pLogicalCameraInfo, pStreamConfig, usecaseId);
            break;
    }

    return pUsecase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::ImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageBuffer::ImageBuffer()
    : pGrallocBuffer(NULL),
    m_aReferenceCount(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::~ImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageBuffer::~ImageBuffer()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::ImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageBuffer* ImageBuffer::Create(
    Gralloc1Interface*  pGrallocInterface,
    gralloc1_device_t*  pGralloc1Device,
    UINT32              width,
    UINT32              height,
    UINT32              format,
    UINT64              producerUsageFlags,
    UINT64              consumerUsageFlags,
    UINT32*             pStride)
{
    CHX_ASSERT(NULL != pGrallocInterface);
    CHX_ASSERT(NULL != pGralloc1Device);
    CHX_ASSERT(NULL != pStride);

    CDKResult       result          = CDKResultSuccess;
    ImageBuffer*    pImageBuffer    = NULL;

    if ((NULL == pGrallocInterface) ||
        (NULL == pGralloc1Device) ||
        (NULL == pStride))
    {
        CHX_LOG_ERROR("Invalid arguments, creating ImageBuffer failed.");
    }
    else
    {
        pImageBuffer = CHX_NEW ImageBuffer;

        if (NULL != pImageBuffer)
        {
            result = pImageBuffer->AllocateGrallocBuffer(pGrallocInterface,
                                                         pGralloc1Device,
                                                         width,
                                                         height,
                                                         format,
                                                         producerUsageFlags,
                                                         consumerUsageFlags,
                                                         pStride);

            if (CDKResultSuccess != result)
            {
                pImageBuffer->Destroy(pGrallocInterface, pGralloc1Device);
                pImageBuffer = NULL;
            }
        }
    }
    return pImageBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBuffer::Destroy(Gralloc1Interface*  pGrallocInterface,
                          gralloc1_device_t*  pGralloc1Device)
{
    CHX_ASSERT(NULL != pGrallocInterface);
    CHX_ASSERT(NULL != pGralloc1Device);

    if ((NULL != pGrallocInterface) &&
        (NULL != pGralloc1Device))
    {
        UINT currentRefCount = GetReferenceCount();

        if (0 != currentRefCount)
        {
            CHX_LOG_ERROR("ImageBuffer %p is destroyed with reference count = %d", this, currentRefCount);
        }

        pGrallocInterface->Release(pGralloc1Device, pGrallocBuffer);
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::AllocateGrallocBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ImageBuffer::AllocateGrallocBuffer(
    Gralloc1Interface*  pGrallocInterface,
    gralloc1_device_t*  pGralloc1Device,
    UINT32              width,
    UINT32              height,
    UINT32              format,
    UINT64              producerUsageFlags,
    UINT64              consumerUsageFlags,
    UINT32*             pStride)
{
    INT32 result = GRALLOC1_ERROR_NONE;

    gralloc1_buffer_descriptor_t gralloc1BufferDescriptor;

    result = pGrallocInterface->CreateDescriptor(pGralloc1Device, &gralloc1BufferDescriptor);

    if (GRALLOC1_ERROR_NONE == result)
    {
        result = pGrallocInterface->SetDimensions(pGralloc1Device, gralloc1BufferDescriptor, width, height);
    }

    if (GRALLOC1_ERROR_NONE == result)
    {
        result = pGrallocInterface->SetFormat(pGralloc1Device, gralloc1BufferDescriptor, format);
    }

    if (GRALLOC1_ERROR_NONE == result)
    {
        result = pGrallocInterface->SetProducerUsage(pGralloc1Device, gralloc1BufferDescriptor, producerUsageFlags);
    }

    if (GRALLOC1_ERROR_NONE == result)
    {
        result = pGrallocInterface->SetConsumerUsage(pGralloc1Device, gralloc1BufferDescriptor, consumerUsageFlags);
    }

    if (GRALLOC1_ERROR_NONE == result)
    {
        result = pGrallocInterface->Allocate(pGralloc1Device, 1, &gralloc1BufferDescriptor, &pGrallocBuffer);
    }

    if (GRALLOC1_ERROR_NONE == result)
    {
        result = pGrallocInterface->GetStride(pGralloc1Device, pGrallocBuffer, pStride);
    }

    result = pGrallocInterface->DestroyDescriptor(pGralloc1Device, gralloc1BufferDescriptor);

    if (GRALLOC1_ERROR_NONE != result)
    {
        CHX_LOG_ERROR("AllocateGrallocBuffer failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::GetReferenceCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ImageBuffer::GetReferenceCount()
{
    return ChxUtils::AtomicLoadU32(&m_aReferenceCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::AddReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBuffer::AddReference()
{
    UINT32 currentReferenceCount = GetReferenceCount();

    CHX_LOG("ReferenceCount for ImageBuffer %p is incremented to %d", this, (currentReferenceCount + 1));

    ChxUtils::AtomicStoreU32(&m_aReferenceCount, currentReferenceCount + 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::ReleaseReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBuffer::ReleaseReference()
{
    UINT32 currentReferenceCount = GetReferenceCount();

    CHX_ASSERT(0 < currentReferenceCount);

    if (0 < currentReferenceCount)
    {
        ChxUtils::AtomicStoreU32(&m_aReferenceCount, (currentReferenceCount - 1));

        CHX_LOG("ReferenceCount for ImageBuffer %p is decremented to %d",
                this,
                ChxUtils::AtomicLoadU32(&m_aReferenceCount));
    }
    else
    {
        CHX_LOG_ERROR("ReleaseReference failed, current reference of ImageBuffer %p is 0", this);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::BufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIBufferManager::CHIBufferManager()
    : m_pUnifiedBufferManager(NULL),
    m_pHwModule(NULL),
    m_pGralloc1Device(NULL),
    m_pLock(NULL),
    m_pFreeBufferList(NULL),
    m_pBusyBufferList(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::~BufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIBufferManager::~CHIBufferManager()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIBufferManager* CHIBufferManager::Create(
    const CHAR*                 pBufferManagerName,
    CHIBufferManagerCreateData* pCreateData)
{
    CDKResult           result          = CDKResultSuccess;
    CHIBufferManager*   pBufferManager  = CHX_NEW CHIBufferManager;

    CHX_LOG("Creating BufferManager %s", pBufferManagerName);
    if (NULL != pBufferManager)
    {
        result = pBufferManager->Initialize(pBufferManagerName, pCreateData);

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Can't initialize BufferManager %s result = %d", pBufferManagerName, result);
            pBufferManager->Destroy();
            pBufferManager  = NULL;
            result          = CDKResultEFailed;
        }
    }
    else
    {
        CHX_LOG_ERROR("Can't create BufferManager %s", pBufferManagerName);
    }

    return pBufferManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHIBufferManager::Destroy()
{
    if (TRUE == m_bIsUnifiedBufferManagerEnabled)
    {
        g_chiBufferManagerOps.pDestroy(m_pUnifiedBufferManager);
    }
    else
    {
        FreeBuffers(FALSE);
#if !defined(LE_CAMERA)
        gralloc1_close(m_pGralloc1Device);
#endif // LE_CAMERA
        if (NULL != m_pWaitFreeBuffer)
        {
            m_pWaitFreeBuffer->Destroy();
            m_pWaitFreeBuffer = NULL;
        }

        if (NULL != m_pLock)
        {
            m_pLock->Destroy();
            m_pLock = NULL;
        }

        CHX_DELETE m_pFreeBufferList;
        m_pFreeBufferList = NULL;

        CHX_DELETE m_pBusyBufferList;
        m_pBusyBufferList = NULL;
    }
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::Activate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIBufferManager::Activate()
{
    CDKResult result = CDKResultSuccess;

    if (TRUE == m_bIsUnifiedBufferManagerEnabled)
    {
        result = g_chiBufferManagerOps.pActivate(m_pUnifiedBufferManager);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::Deactivate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIBufferManager::Deactivate(
    BOOL isPartialRelease)
{
    CDKResult result = CDKResultSuccess;

    if (TRUE == m_bIsUnifiedBufferManagerEnabled)
    {
        result = g_chiBufferManagerOps.pDeactivate(m_pUnifiedBufferManager, isPartialRelease);
    }
    else
    {
        FreeBuffers(isPartialRelease);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::FreeBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHIBufferManager::FreeBuffers(
    BOOL isPartialFree)
{
    ImageBuffer*                     pImageBuffer = NULL;
    LightweightDoublyLinkedListNode* pNode        = NULL;

    m_pLock->Lock();

    if ((NULL != m_pFreeBufferList) && (NULL != m_pBusyBufferList))
    {
        while (NULL != m_pFreeBufferList->Head())
        {
            if (TRUE == isPartialFree)
            {
                if ( (m_pFreeBufferList->NumNodes() + m_pBusyBufferList->NumNodes()) <= m_pBufferManagerData.immediateBufferCount)
                {
                    break;
                }
            }

            pNode        = m_pFreeBufferList->RemoveFromHead();
            pImageBuffer = reinterpret_cast<ImageBuffer*>(pNode->pData);

            pImageBuffer->Destroy(&m_grallocInterface, m_pGralloc1Device);
            pImageBuffer = NULL;

            CHX_DELETE pNode;
            pNode = NULL;
        }

        while (NULL != m_pBusyBufferList->Head())
        {
            if (TRUE == isPartialFree)
            {
                if ( (m_pFreeBufferList->NumNodes() + m_pBusyBufferList->NumNodes()) <= m_pBufferManagerData.immediateBufferCount)
                {
                    break;
                }
            }

            pNode        = m_pBusyBufferList->RemoveFromHead();
            pImageBuffer = reinterpret_cast<ImageBuffer*>(pNode->pData);

            pImageBuffer->Destroy(&m_grallocInterface, m_pGralloc1Device);
            pImageBuffer = NULL;

            CHX_DELETE pNode;
            pNode = NULL;
        }
    }
    else
    {
        CHX_LOG_ERROR("FreeBuffers failed! NULL buffer list");
    }

    m_pLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIBufferManager::Initialize(
    const CHAR*                 pBufferManagerName,
    CHIBufferManagerCreateData* pCreateData)
{
    CDKResult result = CDKResultSuccess;

    CHX_ASSERT(NULL != pBufferManagerName);
    CHX_ASSERT(NULL != pCreateData);

    if ((NULL == pBufferManagerName) || (NULL == pCreateData))
    {
        CHX_LOG_ERROR("Invalid input! pBufferManagerName=%p, pCreateData=%p", pBufferManagerName, pCreateData);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        CdkUtils::StrLCpy(m_pBufferManagerName, pBufferManagerName, sizeof(m_pBufferManagerName));
        m_pBufferManagerData = *pCreateData;

        if (TRUE == ExtensionModule::GetInstance()->EnableUnifiedBufferManager())
        {
            m_bIsUnifiedBufferManagerEnabled = TRUE;
        }
        else
        {
            m_bIsUnifiedBufferManagerEnabled = FALSE;
        }

        if (TRUE == m_bIsUnifiedBufferManagerEnabled)
        {
            m_pUnifiedBufferManager = g_chiBufferManagerOps.pCreate(m_pBufferManagerName, &m_pBufferManagerData);

            if (NULL == m_pUnifiedBufferManager)
            {
                CHX_LOG_ERROR("[%s]: Create BufferManager failed : width=%d, height=%d, format=0x%x, Flags=(0x%llx, 0x%llx), "
                              "stride=%d, BufferCount(%d, %d), lateBinding=%d, heap=%d, chiStream=%p, StreamFormat=%d, type=%d",
                              m_pBufferManagerName,
                              m_pBufferManagerData.width,
                              m_pBufferManagerData.height,
                              m_pBufferManagerData.format,
                              (unsigned long long) m_pBufferManagerData.producerFlags,
                              (unsigned long long) m_pBufferManagerData.consumerFlags,
                              m_pBufferManagerData.bufferStride,
                              m_pBufferManagerData.maxBufferCount,
                              m_pBufferManagerData.immediateBufferCount,
                              m_pBufferManagerData.bEnableLateBinding,
                              m_pBufferManagerData.bufferHeap,
                              m_pBufferManagerData.pChiStream,
                              (NULL != m_pBufferManagerData.pChiStream) ? m_pBufferManagerData.pChiStream->format : -1,
                              (NULL != m_pBufferManagerData.pChiStream) ? m_pBufferManagerData.pChiStream->streamType : -1);

                result = CDKResultEFailed;
            }
        }
        else
        {
            result = SetupGralloc1Interface();

            if (CDKResultSuccess == result)
            {
                m_pLock           = Mutex::Create();
                m_pWaitFreeBuffer = Condition::Create();
                m_pFreeBufferList = CHX_NEW LightweightDoublyLinkedList();
                m_pBusyBufferList = CHX_NEW LightweightDoublyLinkedList();

                CHX_ASSERT(NULL != m_pLock);
                CHX_ASSERT(NULL != m_pFreeBufferList);
                CHX_ASSERT(NULL != m_pBusyBufferList);

                if ((NULL != m_pLock) &&
                    (NULL != m_pFreeBufferList) &&
                    (NULL != m_pBusyBufferList))
                {
                    ImageBuffer*                     pImageBuffer = NULL;
                    LightweightDoublyLinkedListNode* pNode        = NULL;

                    // Allocate a small number of buffers as bare minimum initialization
                    for (UINT i = 0; (i < m_pBufferManagerData.immediateBufferCount) &&
                                     (i < m_pBufferManagerData.maxBufferCount); i++)
                    {
                        pNode = reinterpret_cast<LightweightDoublyLinkedListNode*>(
                                    CHX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

                        CHX_ASSERT(NULL != pNode);
                        if (NULL != pNode)
                        {
                            pImageBuffer = ImageBuffer::Create(&m_grallocInterface,
                                                               m_pGralloc1Device,
                                                               m_pBufferManagerData.width,
                                                               m_pBufferManagerData.height,
                                                               m_pBufferManagerData.format,
                                                               m_pBufferManagerData.producerFlags,
                                                               m_pBufferManagerData.consumerFlags,
                                                               &m_pBufferManagerData.bufferStride);
                            CHX_ASSERT(NULL != pImageBuffer);

                            if (NULL != pImageBuffer)
                            {
                                CHX_LOG("[%s] ImageBuffer created = %p", m_pBufferManagerName, pImageBuffer);

                                pNode->pData = pImageBuffer;
                                m_pFreeBufferList->InsertToTail(pNode);
                            }
                            else
                            {
                                CHX_LOG_ERROR("[%s] ImageBuffer allocated failed = %p", m_pBufferManagerName, pImageBuffer);

                                CHX_FREE(pNode);
                                pNode = NULL;

                                result = CDKResultEFailed;
                            }
                        }
                        else
                        {
                            CHX_LOG_ERROR("[%s] ImageBuffer allocation failed = %p", m_pBufferManagerName, pImageBuffer);

                            result = CDKResultENoMemory;
                        }
                    }
                }
            }
            else
            {
                CHX_LOG_ERROR("[%s] SetupGralloc1Interface failed!", m_pBufferManagerName);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::SetupGralloc1Interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIBufferManager::SetupGralloc1Interface()
{
    CDKResult result = CDKResultSuccess;
#if !defined(LE_CAMERA)
#if defined (_LINUX)
    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, const_cast<const hw_module_t**>(&m_pHwModule));
#endif

    if (NULL != m_pHwModule)
    {
        gralloc1_open(m_pHwModule, &m_pGralloc1Device);
    }
    else
    {
        result = CDKResultEFailed;
    }

    if (NULL != m_pGralloc1Device)
    {
        m_grallocInterface.CreateDescriptor = reinterpret_cast<GRALLOC1_PFN_CREATE_DESCRIPTOR>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_CREATE_DESCRIPTOR));
        m_grallocInterface.DestroyDescriptor = reinterpret_cast<GRALLOC1_PFN_DESTROY_DESCRIPTOR>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_DESTROY_DESCRIPTOR));
        m_grallocInterface.SetDimensions = reinterpret_cast<GRALLOC1_PFN_SET_DIMENSIONS>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_SET_DIMENSIONS));
        m_grallocInterface.SetFormat = reinterpret_cast<GRALLOC1_PFN_SET_FORMAT>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_SET_FORMAT));
        m_grallocInterface.SetProducerUsage = reinterpret_cast<GRALLOC1_PFN_SET_PRODUCER_USAGE>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_SET_PRODUCER_USAGE));
        m_grallocInterface.SetConsumerUsage = reinterpret_cast<GRALLOC1_PFN_SET_CONSUMER_USAGE>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_SET_CONSUMER_USAGE));
        m_grallocInterface.Allocate = reinterpret_cast<GRALLOC1_PFN_ALLOCATE>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_ALLOCATE));
        m_grallocInterface.GetStride = reinterpret_cast<GRALLOC1_PFN_GET_STRIDE>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_GET_STRIDE));
        m_grallocInterface.Release = reinterpret_cast<GRALLOC1_PFN_RELEASE>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_RELEASE));
        m_grallocInterface.Lock = reinterpret_cast<GRALLOC1_PFN_LOCK>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_LOCK));
        m_grallocInterface.Perform = reinterpret_cast<GRALLOC1_PFN_PERFORM>(
            m_pGralloc1Device->getFunction(m_pGralloc1Device,
                                           GRALLOC1_FUNCTION_PERFORM));
    }
    else
    {
        result = CDKResultEFailed;
    }
#endif // LE_CAMERA
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::GetImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageBuffer* CHIBufferManager::GetImageBuffer()
{
    LightweightDoublyLinkedListNode* pNode   = NULL;
    ImageBuffer*                     pBuffer = NULL;

    if (FALSE == m_bIsUnifiedBufferManagerEnabled)
    {
        m_pLock->Lock();

        // Check the free List for an available buffer
        if (NULL != m_pFreeBufferList->Head())
        {
            pNode = m_pFreeBufferList->RemoveFromHead();
        }

        // If no free buffers were found in the free list,
        // we check to see if an additional buffer can be allocated immediately.
        if (NULL == pNode)
        {
            UINT numOfFreeBuffers = m_pFreeBufferList->NumNodes();
            UINT numOfBusyBuffers = m_pBusyBufferList->NumNodes();
            if ((numOfFreeBuffers + numOfBusyBuffers) < m_pBufferManagerData.maxBufferCount)
            {
                ImageBuffer* pNewImageBuffer = ImageBuffer::Create(&m_grallocInterface,
                                                                   m_pGralloc1Device,
                                                                   m_pBufferManagerData.width,
                                                                   m_pBufferManagerData.height,
                                                                   m_pBufferManagerData.format,
                                                                   m_pBufferManagerData.producerFlags,
                                                                   m_pBufferManagerData.consumerFlags,
                                                                   &m_pBufferManagerData.bufferStride);
                CHX_ASSERT(NULL != pNewImageBuffer);

                if (NULL != pNewImageBuffer)
                {
                    CHX_LOG("[%s] ImageBuffer created = %p, bh = %p, w x h = %d x %d",
                            m_pBufferManagerName,
                            pNewImageBuffer,
                            pNewImageBuffer->GetBufferHandle(),
                            m_pBufferManagerData.width,
                            m_pBufferManagerData.height);

                    pNode = reinterpret_cast<LightweightDoublyLinkedListNode*>(
                        CHX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

                    CHX_ASSERT(NULL != pNode);

                    if (NULL != pNode)
                    {
                        pNode->pData = pNewImageBuffer;
                    }
                    else
                    {
                        CHX_LOG_ERROR("[%s] ImageBuffer couldn't be allocated", m_pBufferManagerName);

                        pNewImageBuffer->Destroy(&m_grallocInterface, m_pGralloc1Device);
                        pNewImageBuffer = NULL;
                    }
                }
                else
                {
                    CHX_LOG_ERROR("[%s] ImageBuffer allocated failed = %p",
                                  m_pBufferManagerName,
                                  pNewImageBuffer);

                    CHX_FREE(pNode);
                    pNode = NULL;
                }
            }
        }

        // If no free buffers were found and no more buffers can be allocated,
        // we wait until a busy buffer becomes free
        if (NULL == pNode)
        {
            CDKResult result = CDKResultSuccess;

            result = m_pWaitFreeBuffer->TimedWait(m_pLock->GetNativeHandle(), WAIT_BUFFER_TIMEOUT);

            CHX_ASSERT(CDKResultSuccess == result);

            if (CDKResultETimeout == result)
            {
                CHX_LOG_ERROR("[%s], *** wait for buffer timedout ***", m_pBufferManagerName);
            }
            else if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("[%s], *** failed to get a free buffer result:%d ***", m_pBufferManagerName, result);
            }
            else
            {
                // Try again to get buffer from free list
                if (NULL != m_pFreeBufferList->Head())
                {
                    pNode = m_pFreeBufferList->RemoveFromHead();
                }
            }
        }

        // Found a buffer. Increment it's reference count and add to the busy list.
        if (NULL != pNode)
        {
            pBuffer = reinterpret_cast<ImageBuffer*>(pNode->pData);
            pBuffer->AddReference();
            m_pBusyBufferList->InsertToTail(pNode);

            CHX_LOG("[%s] ImageBuffer = %p, Free buffers = %d, Busy buffers = %d",
                    m_pBufferManagerName, pBuffer, m_pFreeBufferList->NumNodes(), m_pBusyBufferList->NumNodes());
        }
        else
        {
            CHX_LOG_ERROR("[%s] GetImageBuffer failed! Free buffers = %d, Busy buffers = %d",
                          m_pBufferManagerName, m_pFreeBufferList->NumNodes(), m_pBusyBufferList->NumNodes());
        }

        m_pLock->Unlock();
    }
    else
    {
         CHX_LOG_ERROR("[%s] UnifiedBufferManager is enabled, this API should not be called!", m_pBufferManagerName);
    }

    return pBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::GetImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIBUFFERINFO CHIBufferManager::GetImageBufferInfo()
{
    CHIBUFFERINFO bufferInfo = {};

    if (TRUE == m_bIsUnifiedBufferManagerEnabled)
    {
        CHIBUFFERHANDLE hImageBuffer = g_chiBufferManagerOps.pGetImageBuffer(m_pUnifiedBufferManager);
        if (NULL != hImageBuffer)
        {
            bufferInfo.bufferType = ChiBufferType::ChiNative;
            bufferInfo.phBuffer   = hImageBuffer;
        }
        else
        {
            CHX_LOG_ERROR("[%s] GetImageBuffer failed!", m_pBufferManagerName);
        }
    }
    else
    {
        ImageBuffer* pImageBuffer = GetImageBuffer();
        if (NULL != pImageBuffer)
        {
            bufferInfo.bufferType = pImageBuffer->GetBufferHandleType();
            bufferInfo.phBuffer   = pImageBuffer->GetBufferHandle();
        }
        else
        {
            CHX_LOG_ERROR("[%s] GetImageBuffer failed!", m_pBufferManagerName);
        }
    }
    return bufferInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::AddReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIBufferManager::AddReference(const CHIBUFFERINFO* pBufferInfo)
{
    CDKResult                        result           = CDKResultSuccess;
    LightweightDoublyLinkedListNode* pImageBufferNode = NULL;
    ImageBuffer*                     pImageBuffer     = NULL;
    buffer_handle_t*                 pBufferHandle    = NULL;

    if ((NULL != pBufferInfo) && (NULL != pBufferInfo->phBuffer))
    {
        if (TRUE == m_bIsUnifiedBufferManagerEnabled)
        {
             result = g_chiBufferManagerOps.pAddReference(m_pUnifiedBufferManager, pBufferInfo->phBuffer);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("[%s] AddReference failed, bufferType=%d, phBuffer=%p",
                             m_pBufferManagerName, pBufferInfo->bufferType, pBufferInfo->phBuffer);
            }
        }
        else
        {
            m_pLock->Lock();

            pBufferHandle    = reinterpret_cast<buffer_handle_t*>(pBufferInfo->phBuffer);
            pImageBufferNode = LookupImageBuffer(pBufferHandle);

            if (NULL != pImageBufferNode)
            {
                pImageBuffer = reinterpret_cast<ImageBuffer*>(pImageBufferNode->pData);
                CHX_ASSERT(NULL != pImageBuffer);

                if (0 == pImageBuffer->GetReferenceCount())
                {
                    m_pFreeBufferList->RemoveNode(pImageBufferNode);
                    m_pBusyBufferList->InsertToTail(pImageBufferNode);
                }

                pImageBuffer->AddReference();
            }
            else
            {
                CHX_LOG_ERROR("[%s] AddReference failed, cannot find Image buffer for buffer handle %p",
                              m_pBufferManagerName, pBufferHandle);
                result = CDKResultEFailed;
            }

            m_pLock->Unlock();
        }
    }
    else
    {
        CHX_LOG_ERROR("[%s] Buffer handle is NULL", m_pBufferManagerName);
        result = CDKResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::ReleaseReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIBufferManager::ReleaseReference(const CHIBUFFERINFO* pBufferInfo)
{
    CDKResult                        result           = CDKResultSuccess;
    LightweightDoublyLinkedListNode* pImageBufferNode = NULL;
    ImageBuffer*                     pImageBuffer     = NULL;
    UINT                             refCount         = 0;
    buffer_handle_t*                 pBufferHandle    = NULL;

    if ((NULL != pBufferInfo) && (NULL != pBufferInfo->phBuffer))
    {

        if (TRUE == m_bIsUnifiedBufferManagerEnabled)
        {
            result = g_chiBufferManagerOps.pReleaseReference(m_pUnifiedBufferManager, pBufferInfo->phBuffer);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("[%s] ReleaseReference failed, bufferType=%d, phBuffer=%p",
                              m_pBufferManagerName, pBufferInfo->bufferType, pBufferInfo->phBuffer);
            }
        }
        else
        {
            m_pLock->Lock();

            pBufferHandle    = reinterpret_cast<buffer_handle_t*>(pBufferInfo->phBuffer);
            pImageBufferNode = LookupImageBuffer(pBufferHandle);

            if (NULL != pImageBufferNode)
            {
                pImageBuffer = reinterpret_cast<ImageBuffer*>(pImageBufferNode->pData);
                CHX_ASSERT(NULL != pImageBuffer);

                refCount = pImageBuffer->GetReferenceCount();

                if (0 != refCount)
                {
                    pImageBuffer->ReleaseReference();

                    refCount = pImageBuffer->GetReferenceCount();

                    // Move this unreferenced buffer to the free list
                    if (0 == refCount)
                    {
                        m_pBusyBufferList->RemoveNode(pImageBufferNode);
                        m_pFreeBufferList->InsertToTail(pImageBufferNode);
                        m_pWaitFreeBuffer->Signal();
                    }
                }
                else
                {
                    CHX_LOG_ERROR("[%s] ReleaseReference not necessary, reference count is 0 already for buffer handle %p",
                                  m_pBufferManagerName,
                                  pBufferHandle);
                }
            }
            else
            {
                CHX_LOG_ERROR("[%s] ReleaseReference failed, cannot find Image buffer for buffer handle %p",
                              m_pBufferManagerName,
                              pBufferHandle);

                result = CDKResultEFailed;
            }

            m_pLock->Unlock();
        }
    }
    else
    {
        CHX_LOG_ERROR("[%s] Buffer handle is NULL", m_pBufferManagerName);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::GetReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CHIBufferManager::GetReference(const CHIBUFFERINFO* pBufferInfo)
{
    LightweightDoublyLinkedListNode* pImageBufferNode = NULL;
    ImageBuffer*                     pImageBuffer     = NULL;
    UINT                             refCount         = 0;
    buffer_handle_t*                 pBufferHandle    = NULL;

    if ((NULL != pBufferInfo) && (NULL != pBufferInfo->phBuffer))
    {
        if (TRUE == m_bIsUnifiedBufferManagerEnabled)
        {
            refCount = g_chiBufferManagerOps.pGetReference(pBufferInfo->phBuffer);
        }
        else
        {
            m_pLock->Lock();

            pBufferHandle    = reinterpret_cast<buffer_handle_t*>(pBufferInfo->phBuffer);
            pImageBufferNode = LookupImageBuffer(pBufferHandle);

            if (NULL != pImageBufferNode)
            {
                pImageBuffer = reinterpret_cast<ImageBuffer*>(pImageBufferNode->pData);
                CHX_ASSERT(NULL != pImageBuffer);

                refCount = pImageBuffer->GetReferenceCount();
            }
            else
            {
                CHX_LOG_ERROR("[%s] GetReference failed, cannot find Image buffer for buffer handle %p",
                              m_pBufferManagerName, pBufferHandle);
            }

            m_pLock->Unlock();
        }
    }
    else
    {
        CHX_LOG_ERROR("[%s] Buffer handle is NULL", m_pBufferManagerName);
    }

    return refCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::BindBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIBufferManager::BindBuffer(
    const CHIBUFFERINFO* pBufferInfo)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL != pBufferInfo) && (NULL != pBufferInfo->phBuffer))
    {
        if (TRUE == m_bIsUnifiedBufferManagerEnabled)
        {
            result = g_chiBufferManagerOps.pBindBuffer(pBufferInfo->phBuffer);
        }
        else
        {
            CHX_LOG("[%s] UnifiedBufferManager is not enabled, Bind is dummy in this case", m_pBufferManagerName);
        }
    }
    else
    {
        CHX_LOG_ERROR("[%s] Buffer handle is NULL", m_pBufferManagerName);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::GetBufferSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CHIBufferManager::GetBufferSize(const CHIBUFFERINFO* pBufferInfo)
{
    UINT32 size                     = 0;
    buffer_handle_t* phBufferHandle = NULL;
    struct private_handle_t* ph     = NULL;

    if (NULL != pBufferInfo)
    {
        phBufferHandle = CHIBufferManager::GetGrallocHandle(pBufferInfo);
        if (NULL != phBufferHandle)
        {
            ph = reinterpret_cast<struct private_handle_t*>(const_cast<native_handle*>(*phBufferHandle));
            if (NULL != ph)
            {
                size = ph->size;
            }
        }
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::CopyBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIBufferManager::CopyBuffer(
    const CHIBUFFERINFO*    pSrcBufferInfo,
    CHIBUFFERINFO*          pDstBufferInfo)
{
    CDKResult                   result  = CDKResultSuccess;
    struct private_handle_t*    phSrc;
    struct private_handle_t*    phDst;

    if ((NULL == pSrcBufferInfo) || (NULL == pDstBufferInfo))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        buffer_handle_t* hSrc   = CHIBufferManager::GetGrallocHandle(pSrcBufferInfo);
        buffer_handle_t* hDst   = CHIBufferManager::GetGrallocHandle(pDstBufferInfo);

        if ((NULL == hSrc) || (NULL == hDst))
        {
            result = CDKResultEInvalidArg;
        }
        else
        {
            phSrc = reinterpret_cast<struct private_handle_t*>(
                const_cast<native_handle*>(*hSrc));

            phDst = reinterpret_cast<struct private_handle_t*>(
                const_cast<native_handle*>(*hDst));

            if ((NULL == phSrc) || (NULL == phDst))
            {
                result = CDKResultEInvalidArg;
            }
        }
    }

    if (CDKResultSuccess == result)
    {
        void*   pSrc    = ChxUtils::MemMap(phSrc->fd, phSrc->size, phSrc->offset);
        void*   pDst    = ChxUtils::MemMap(phDst->fd, phDst->size, phDst->offset);

        if ((NULL == pSrc) || (NULL == pDst))
        {
            CHX_LOG_ERROR("failed to map buffers, pSrc=%p, pDst=%p", pSrc, pDst);
            result = CDKResultENoMemory;
        }

        if (CDKResultSuccess == result)
        {
            int size = (phSrc->size < phDst->size) ? phSrc->size : phDst->size;
            ChxUtils::Memcpy(pDst, pSrc, size);
        }

        if (NULL != pSrc)
        {
            ChxUtils::MemUnmap(pSrc, phSrc->size);
        }

        if (NULL != pDst)
        {
            ChxUtils::MemUnmap(pDst, phDst->size);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::SetPerfMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHIBufferManager::SetPerfMode(
    CHIBUFFERINFO* pBufferInfo)
{
    UINT32 perfMode                 = 1;
    buffer_handle_t* phBufferHandle = NULL;
    struct private_handle_t* ph     = NULL;

    if (NULL != pBufferInfo)
    {
        phBufferHandle = CHIBufferManager::GetGrallocHandle(pBufferInfo);
        if (NULL != phBufferHandle)
        {
            ph = reinterpret_cast<struct private_handle_t*>(const_cast<native_handle*>(*phBufferHandle));
            if (NULL != ph)
            {
#if (!defined(LE_CAMERA))
                setMetaData(ph, SET_VIDEO_PERF_MODE, &perfMode);
#endif // LE_CAMERA
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::GetCPUAddress
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CHIBufferManager::GetCPUAddress(
    const CHIBUFFERINFO*    pBufferInfo,
    INT                     size)
{
    VOID* pVirtualAddress = NULL;

    if ((NULL != pBufferInfo) && (NULL != pBufferInfo->phBuffer))
    {
        if (ChiNative == pBufferInfo->bufferType)
        {
            // We are using unified Buffer Manager
            pVirtualAddress = const_cast<VOID*>(g_chiBufferManagerOps.pGetCPUAddress(pBufferInfo->phBuffer));
        }
        else
        {
            // phBuffer is a gralloc handle
            buffer_handle_t *native_handle = reinterpret_cast<buffer_handle_t*>(pBufferInfo->phBuffer);

            CHX_LOG("Type=%d, phBuffer=%p, fd=%d",
                    pBufferInfo->bufferType, pBufferInfo->phBuffer,
                    reinterpret_cast<const native_handle_t*>(*native_handle)->data[0]);

#if defined (_LINUX)
            pVirtualAddress = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_SHARED,
                                   reinterpret_cast<const native_handle_t*>(*native_handle)->data[0], 0);
#endif
        }

        if ((NULL == pVirtualAddress) || (reinterpret_cast<VOID*>(-1) == pVirtualAddress))
        {
            CHX_LOG_WARN("Failed in getting pVirtualAddress, pBufferInfo=%p, type=%d, phBuffer=%p, pVirtualAddress=%p",
                          pBufferInfo, pBufferInfo->bufferType, pBufferInfo->phBuffer, pVirtualAddress);

            // in case of mmap failures, it will return -1, not NULL. So, set to NULL so that callers just validate on NULL
            pVirtualAddress = NULL;
        }
    }
    else
    {
        CHX_LOG_ERROR("Buffer handle is NULL, pBufferInfo=%p", pBufferInfo);
    }

    return pVirtualAddress;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::PutCPUAddress
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHIBufferManager::PutCPUAddress(
    const CHIBUFFERINFO*    pBufferInfo,
    INT                     size,
    VOID*                   pVirtualAddress)
{
    if ((NULL != pBufferInfo) && (NULL != pBufferInfo->phBuffer) && (NULL != pVirtualAddress))
    {
        if (ChiNative == pBufferInfo->bufferType)
        {
            // We are using unified Buffer Manager
            // No need to do anything. Unified Buffer manager takes care of unmap while freeing the buffer
        }
        else
        {
#if defined (_LINUX)
            munmap(pVirtualAddress, size);
#endif
        }
    }
    else
    {
        CHX_LOG_ERROR("Buffer handle is NULL, pBufferInfo=%p", pBufferInfo);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::GetFileDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CHIBufferManager::GetFileDescriptor(
    const CHIBUFFERINFO* pBufferInfo)
{
    INT fd = -1;

    if ((NULL != pBufferInfo) && (NULL != pBufferInfo->phBuffer))
    {
        if (ChiNative == pBufferInfo->bufferType)
        {
            // We are using unified Buffer Manager
            fd = g_chiBufferManagerOps.pGetFileDescriptor(pBufferInfo->phBuffer);

            // If BufferManager is allocated through gralloc heap, CamX may not populate fd value always, unless required.
            // So, check and get Gralloc buffer handle to explicitly get fd value from it.
            if (-1 == fd)
            {
                buffer_handle_t* native_handle =
                    reinterpret_cast<buffer_handle_t*>(g_chiBufferManagerOps.pGetGrallocHandle(pBufferInfo->phBuffer));

                fd = reinterpret_cast<const native_handle_t*>(*native_handle)->data[0];
            }
        }
        else
        {
            // phBuffer is a gralloc handle
            buffer_handle_t *native_handle = reinterpret_cast<buffer_handle_t*>(pBufferInfo->phBuffer);

            fd = reinterpret_cast<const native_handle_t*>(*native_handle)->data[0];
        }

        if (-1 == fd)
        {
            CHX_LOG_ERROR("invalid fd, pBufferInfo=%p, type=%d, phBuffer=%p",
                          pBufferInfo, pBufferInfo->bufferType, pBufferInfo->phBuffer);
        }
    }
    else
    {
        CHX_LOG_ERROR("Buffer handle is NULL, pBufferInfo=%p", pBufferInfo);
    }

    return fd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::GetGrallocHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
buffer_handle_t* CHIBufferManager::GetGrallocHandle(
    const CHIBUFFERINFO* pBufferInfo)
{
    buffer_handle_t* handle = NULL;

    if ((NULL != pBufferInfo) && (NULL != pBufferInfo->phBuffer))
    {
        if (ChiNative == pBufferInfo->bufferType)
        {
            // We are using unified Buffer Manager
            handle = reinterpret_cast<buffer_handle_t*>(g_chiBufferManagerOps.pGetGrallocHandle(pBufferInfo->phBuffer));
        }
        else
        {
            // phBuffer is a gralloc handle
            handle = reinterpret_cast<buffer_handle_t*>(pBufferInfo->phBuffer);
        }

        if (NULL == handle)
        {
            CHX_LOG_ERROR("invalid handle, pBufferInfo=%p, type=%d, phBuffer=%p",
                          pBufferInfo, pBufferInfo->bufferType, pBufferInfo->phBuffer);
        }
    }
    else
    {
        CHX_LOG_ERROR("Buffer handle is NULL, pBufferInfo=%p", pBufferInfo);
    }

    return handle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::CacheOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHIBufferManager::CacheOps(
    const CHIBUFFERINFO* pBufferInfo,
    BOOL                 invalidate,
    BOOL                 clean)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL != pBufferInfo) && (NULL != pBufferInfo->phBuffer) &&
        ((TRUE == invalidate) || (TRUE == clean)))
    {
        if (TRUE == m_bIsUnifiedBufferManagerEnabled)
        {
            result = g_chiBufferManagerOps.pCacheOps(pBufferInfo->phBuffer, invalidate, clean);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("[%s] CacheOps failed! bufferType=%d, phBuffer=%p, invalidate=%d, clean=%d",
                              m_pBufferManagerName, pBufferInfo->bufferType, pBufferInfo->phBuffer, invalidate, clean);
            }
        }
        else
        {
            CHX_LOG("[%s] CacheOps on image buffers of type %d is not supported!",
                    m_pBufferManagerName, pBufferInfo->bufferType);
        }
    }
    else
    {
        CHX_LOG_ERROR("[%s] InvalidArgs! pBufferInfo=%p, invalidate=%d, clean=%d",
                      m_pBufferManagerName, pBufferInfo, invalidate, clean);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIBufferManager::LookupImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LightweightDoublyLinkedListNode* CHIBufferManager::LookupImageBuffer(
    buffer_handle_t* pBufferHandle)
{
    ImageBuffer*                        pBuffer     = NULL;
    UINT                                numNodes    = m_pBusyBufferList->NumNodes();
    LightweightDoublyLinkedListNode*    pNode       = m_pBusyBufferList->Head();
    BOOL                                found       = FALSE;

    // Go through busy list first
    for (UINT i = 0; i < numNodes; i++)
    {
        if (NULL == pNode)
        {
            break;
        }

        pBuffer = reinterpret_cast<ImageBuffer*>(pNode->pData);
        if (pBuffer->GetBufferHandle() == pBufferHandle)
        {
            found = TRUE;
            CHX_LOG("[%s] Found image buffer %p for handle %p in busy list",
                    m_pBufferManagerName, pBuffer, pBufferHandle);
            break;
        }
        pNode = m_pBusyBufferList->NextNode(pNode);
    }

    // If the image buffer is not found in busy list, go through free list then
    if (FALSE == found)
    {
        numNodes    = m_pFreeBufferList->NumNodes();
        pNode       = m_pFreeBufferList->Head();

        for (UINT i = 0; i < numNodes; i++)
        {
            if (NULL == pNode)
            {
                break;
            }

            pBuffer = reinterpret_cast<ImageBuffer*>(pNode->pData);
            if (pBuffer->GetBufferHandle() == pBufferHandle)
            {
                found = TRUE;
                CHX_LOG("[%s] Found image buffer %p for handle %p in free list",
                        m_pBufferManagerName, pBuffer, pBufferHandle);
                break;
            }
            pNode = m_pFreeBufferList->NextNode(pNode);
        }
    }

    if (FALSE == found)
    {
        pNode = NULL;
    }

    return pNode;
}
